/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$ $Name$
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License"); you may not use this file except in compliance
* with the License. You may obtain a copy of the License at
* http://www.mozilla.org/MPL/
*
* Software distributed under the License is distributed on an "AS IS" basis,
* WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License for
* the specific language governing rights and limitations under the License.
*
* The Original Code is BBC Research and Development code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Thomas Davies (Original Author), Scott R Ladd
*
* Alternatively, the contents of this file may be used under the terms of
* the GNU General Public License Version 2 (the "GPL"), or the GNU Lesser
* Public License Version 2.1 (the "LGPL"), in which case the provisions of
* the GPL or the LGPL are applicable instead of those above. If you wish to
* allow use of your version of this file only under the terms of the either
* the GPL or LGPL and not to allow others to use your version of this file
* under the MPL, indicate your decision by deleting the provisions above
* and replace them with the notice and other provisions required by the GPL
* or LGPL. If you do not delete the provisions above, a recipient may use
* your version of this file under the terms of any one of the MPL, the GPL
* or the LGPL.
* ***** END LICENSE BLOCK ***** */


#include <libdirac_encoder/seq_compress.h>
#include <libdirac_encoder/frame_compress.h>
#include <libdirac_common/golomb.h>

SequenceCompressor::SequenceCompressor(PicInput* pin,std::ofstream* outfile, EncoderParams& encp)
: m_all_done(false),
  m_just_finished(true),
  m_encparams(encp),
  m_pic_in(pin),
  m_current_code_fnum(0),
  m_show_fnum(-1),m_last_frame_read(-1),
  m_delay(1),
  m_qmonitor(m_encparams, m_pic_in->GetSeqParams() )
{
	//TBD: put into the constructor for EncoderParams
	m_encparams.SetEntropyFactors(new EntropyCorrector(4));
	m_encparams.SetBitsOut(new BitOutputManager(outfile));
	WriteStreamHeader();

	//We have to set up the block parameters and file padding. This needs to take into
	//account both blocks for motion compensation and also wavelet transforms

	//Amount of horizontal padding for Y,U and V components
	int xpad_luma,xpad_chroma;

	//Amount of vertical padding for Y,U and V components
	int ypad_luma,ypad_chroma;

	//scaling factors for chroma based on chroma format
	int x_chroma_fac,y_chroma_fac;	

	//First, we need to have sufficient padding to take account of the blocksizes.
	//It's sufficient to check for chroma

	const SeqParams& sparams=m_pic_in->GetSeqParams();
	if (sparams.CFormat() == format411)
	{
		x_chroma_fac=4; 
		y_chroma_fac=1;
	}
	else if (sparams.CFormat()==format420)
	{
		x_chroma_fac=2;
		y_chroma_fac=2;
	}
	else if (sparams.CFormat() == format422)
	{
		x_chroma_fac=2;
		y_chroma_fac=1;
	}
	else
	{
		x_chroma_fac=1;
		y_chroma_fac=1;
	}

	int xl_chroma=sparams.Xl()/x_chroma_fac;
	int yl_chroma=sparams.Yl()/y_chroma_fac;

	//make sure we have enough macroblocks to cover the pictures
	m_encparams.SetXNumMB( xl_chroma/m_encparams.ChromaBParams(0).Xbsep() );
	m_encparams.SetYNumMB( yl_chroma/m_encparams.ChromaBParams(0).Ybsep() );
	if ( m_encparams.XNumMB() * m_encparams.ChromaBParams(0).Xbsep() < xl_chroma )
	{
		m_encparams.SetXNumMB( m_encparams.XNumMB() + 1 );
		xpad_chroma=m_encparams.XNumMB()*m_encparams.ChromaBParams(0).Xbsep()-xl_chroma;
	}
	else
		xpad_chroma=0;

	if (m_encparams.YNumMB()*m_encparams.ChromaBParams(0).Ybsep()<yl_chroma){
		m_encparams.SetYNumMB( m_encparams.YNumMB() + 1 );
		ypad_chroma=m_encparams.YNumMB()*m_encparams.ChromaBParams(0).Ybsep()-yl_chroma;
	}
	else
		ypad_chroma=0;

	//Now we have an integral number of macroblocks in a picture and we set the number of blocks
	m_encparams.SetXNumBlocks( 4*m_encparams.XNumMB() );
	m_encparams.SetYNumBlocks( 4*m_encparams.YNumMB() );

	//Next we work out the additional padding due to the wavelet transform
	//For the moment, we'll fix the transform depth to be 4, so we need divisibility by 16.
	//In the future we'll want arbitrary transform depths. It's sufficient to check for
	//chroma only

	int xpad_len=xl_chroma+xpad_chroma;
	int ypad_len=yl_chroma+ypad_chroma;
	if (xpad_len%16!=0)
		xpad_chroma=((xpad_len/16)+1)*16-xl_chroma;
	if (ypad_len%16!=0)
		ypad_chroma=((ypad_len/16)+1)*16-yl_chroma;

	xpad_luma=xpad_chroma*x_chroma_fac;
	ypad_luma=ypad_chroma*y_chroma_fac;


	//Set the resulting padding values
	m_pic_in->SetPadding(xpad_luma,ypad_luma);

	//Set up the frame buffer with the PADDED picture sizes
	m_fbuffer=new FrameBuffer( sparams.CFormat() , m_encparams.NumL1() , m_encparams.L1Sep() , 
			sparams.Xl() + xpad_luma , sparams.Yl() + ypad_luma );
}

SequenceCompressor::~SequenceCompressor(){
	//TBD: put into the destructor for EncoderParams	
	delete &m_encparams.BitsOut();
	delete &m_encparams.EntropyFactors();
	delete m_fbuffer;
}

Frame& SequenceCompressor::CompressNextFrame(){

	//this function codes the next frame in coding order and returns the next frame in display order
	//In general these will differ, and because of re-ordering there is a m_delay which needs to be imposed.
	//This creates problems at the start and at the end of the sequence which must be dealt with.
	//At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
	//the frames out. It's up to the calling function to do something with the decoded frames as they
	//come out - write them to screen or to file, or whatever. TJD 13Feb04.

	//current_fnum is the number of the current frame being coded in display order
	//m_current_code_fnum is the number of the current frame in coding order. This function increments
	//m_current_code_fnum by 1 each time and works out what the number is in display order.
	//m_show_fnum is the index of the frame number that can be shown when current_fnum has been coded.
	//Var m_delay is the m_delay caused by reordering (as distinct from buffering)

    //Keep a copy of the original frame for measuring quality
    Frame* orig_frame;

	int old_total_bits,total_bits;
	m_current_display_fnum=CodedToDisplay(m_current_code_fnum);

	if (m_current_code_fnum!=0)//if we're not at the beginning, clean the buffer
		m_fbuffer->Clean(m_show_fnum);

	m_show_fnum=std::max(m_current_code_fnum-m_delay,0);

	//read in the data if necessary and if we can

    for (int I=m_last_frame_read+1;I<=int(m_current_display_fnum);++I)
    {
        //read from the last frame read to date to the current frame to be coded
        //(which may NOT be the next frame in display order)
        m_fbuffer->PushFrame(m_pic_in,I);

        if (m_pic_in->End())
        {//if we've read past the end, then should stop
	        m_all_done=true;
        }
     m_last_frame_read=I;

	}//I

	if (!m_all_done)
	{   //we haven't coded everything, so compress the next frame

		old_total_bits=m_encparams.BitsOut().GetTotalBytes()*8;

		//set up the frame compression
		FrameCompressor my_fcoder( m_encparams );

		if (m_encparams.Verbose())
		{
			std::cerr<<std::endl<<std::endl<<"Compressing frame "<<m_current_code_fnum<<", ";
			std::cerr<<m_current_display_fnum<<" in display order";
		}

        // Make a copy of the uncompressed frame in order to measure the quality
        orig_frame = new Frame( m_fbuffer->GetFrame(m_current_display_fnum) );

		// Compress the frame//
        ///////////////////////
		
        my_fcoder.Compress(*m_fbuffer,m_current_display_fnum);

		total_bits=m_encparams.BitsOut().GetTotalBytes()*8;
		if (m_encparams.Verbose())
			std::cerr<<std::endl<<std::endl<<"Total bits for frame="<<(total_bits-old_total_bits)<<std::endl;

        // Adjust the Lagrangian parameters
        m_qmonitor.UpdateModel(m_fbuffer->GetFrame(m_current_display_fnum), *orig_frame, m_encparams.CPD() );

        delete orig_frame;

	}
	else
	{
		int total_bits=m_encparams.BitsOut().GetTotalBytes()*8;
		if (m_encparams.Verbose() && m_just_finished){
			std::cerr<<std::endl<<std::endl<<"Finished encoding.";
			std::cerr<<"Total bits for sequence="<<total_bits;
			std::cerr<<", of which "<<m_encparams.BitsOut().GetTotalHeadBytes()*8<<" were header.";
			std::cerr<<std::endl<<"Resulting bit-rate at "<<m_pic_in->GetSeqParams().FrameRate()<<"Hz is ";
			std::cerr<<total_bits*(m_pic_in->GetSeqParams().FrameRate())/m_pic_in->GetSeqParams().Zl()<<" bits/sec.";
		}
		m_just_finished=false;
	}
	m_current_code_fnum++;
	return m_fbuffer->GetFrame(m_show_fnum);
}

void SequenceCompressor::WriteStreamHeader(){
	//write out all the header data

   	//begin with the ID of the codec
	m_encparams.BitsOut().Header().OutputBytes("KW-DIRAC");

   	//picture dimensions
	UnsignedGolombCode( m_encparams.BitsOut().Header(),(unsigned int) m_pic_in->GetSeqParams().Xl());
	UnsignedGolombCode( m_encparams.BitsOut().Header(),(unsigned int) m_pic_in->GetSeqParams().Yl());
	UnsignedGolombCode( m_encparams.BitsOut().Header(),(unsigned int) m_pic_in->GetSeqParams().Zl());

	//picture rate
	UnsignedGolombCode( m_encparams.BitsOut().Header() , (unsigned int) m_pic_in->GetSeqParams().FrameRate());

    //block parameters
	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_encparams.LumaBParams(2).Xblen());
	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_encparams.LumaBParams(2).Yblen());
	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_encparams.LumaBParams(2).Xbsep());
	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_encparams.LumaBParams(2).Ybsep());

	//also send the number of blocks horizontally and vertically
	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_encparams.XNumBlocks());
	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_encparams.YNumBlocks());
    //chroma format

	UnsignedGolombCode( m_encparams.BitsOut().Header() ,(unsigned int) m_pic_in->GetSeqParams().CFormat());

    //interlace marker
	m_encparams.BitsOut().Header().OutputBit(m_pic_in->GetSeqParams().Interlace());

	m_encparams.BitsOut().WriteToFile();
}

int SequenceCompressor::CodedToDisplay(int fnum){
	int div;
	if (m_encparams.L1Sep()>0)
	{//we have L1 and L2 frames
		if (fnum==0)
			return 0;
		else if ((fnum-1)% m_encparams.L1Sep()==0)
		{//we have L1 or subsequent I frames
			div=(fnum-1)/m_encparams.L1Sep();
			return fnum+m_encparams.L1Sep()-1;
		}
		else//we have L2 frames
			return fnum-1;
	}
	else
	{//we just have I-frames, so no re-ordering
		return fnum;
	}
}
