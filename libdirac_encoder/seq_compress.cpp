/* ***** BEGIN LICENSE BLOCK *****
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
* Contributor(s):
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


/*
*
* $Author$
* $Revision$
* $Log$
* Revision 1.6  2004-05-18 07:46:15  tjdwave
* Added support for I-frame only coding by setting num_L1 equal 0; num_L1 negative gives a single initial I-frame ('infinitely' many L1 frames). Revised quantiser selection to cope with rounding error noise.
*
* Revision 1.5  2004/05/12 16:06:20  tjdwave
*
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.4  2004/05/11 14:17:59  tjdwave
* Removed dependency on XParam CLI library for both encoder and decoder.
*
* Revision 1.3  2004/04/11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
*
* Revision 1.2  2004/03/22 01:04:28  chaoticcoyote
* Added API documentation to encoder library
* Moved large constructors so they are no longer inlined
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "libdirac_encoder/seq_compress.h"
#include "libdirac_encoder/frame_compress.h"
#include "libdirac_common/golomb.h"

SequenceCompressor::SequenceCompressor(PicInput* pin,std::ofstream* outfile, const EncoderParams& encp)
:all_done(false),
just_finished(true),
encparams(encp),
picIn(pin),
current_code_fnum(0),
show_fnum(-1),last_frame_read(-1),
delay(1)
{
	encparams.EntCorrect=new EntropyCorrector(4);
	encparams.BIT_OUT=new BitOutputManager(outfile);
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

	const SeqParams& sparams=picIn->GetSeqParams();
	if (sparams.cformat==format411){
		x_chroma_fac=4; y_chroma_fac=1;
	}
	else if (sparams.cformat==format420){
		x_chroma_fac=2; y_chroma_fac=2;
	}
	else if (sparams.cformat==format422){
		x_chroma_fac=2; y_chroma_fac=1;
	}
	else{
		x_chroma_fac=1; y_chroma_fac=1;
	}

	int xl_chroma=sparams.xl/x_chroma_fac;
	int yl_chroma=sparams.yl/y_chroma_fac;

	//make sure we have enough macroblocks to cover the pictures
	encparams.X_NUM_MB=xl_chroma/encparams.ChromaBParams(0).XBSEP;
	encparams.Y_NUM_MB=yl_chroma/encparams.ChromaBParams(0).YBSEP;
	if (encparams.X_NUM_MB*encparams.ChromaBParams(0).XBSEP<xl_chroma){
		encparams.X_NUM_MB++;
		xpad_chroma=encparams.X_NUM_MB*encparams.ChromaBParams(0).XBSEP-xl_chroma;
	}
	else
		xpad_chroma=0;

	if (encparams.Y_NUM_MB*encparams.ChromaBParams(0).YBSEP<yl_chroma){
		encparams.Y_NUM_MB++;
		ypad_chroma=encparams.Y_NUM_MB*encparams.ChromaBParams(0).YBSEP-yl_chroma;
	}
	else
		ypad_chroma=0;

	//Now we have an integral number of macroblocks in a picture and we set the number of blocks
	encparams.X_NUMBLOCKS=4*encparams.X_NUM_MB;
	encparams.Y_NUMBLOCKS=4*encparams.Y_NUM_MB;

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
	picIn->SetPadding(xpad_luma,ypad_luma);

	//Set up the frame buffer with the PADDED picture sizes
	my_buffer=new FrameBuffer(encparams.cformat,encparams.NUM_L1,encparams.L1_SEP,sparams.xl+xpad_luma,sparams.yl+ypad_luma);
}

SequenceCompressor::~SequenceCompressor(){
	delete encparams.BIT_OUT;
	delete encparams.EntCorrect;
	delete my_buffer;
}

Frame& SequenceCompressor::CompressNextFrame(){

	//this function codes the next frame in coding order and returns the next frame in display order
	//In general these will differ, and because of re-ordering there is a delay which needs to be imposed.
	//This creates problems at the start and at the end of the sequence which must be dealt with.
	//At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
	//the frames out. It's up to the calling function to do something with the decoded frames as they
	//come out - write them to screen or to file, or whatever. TJD 13Feb04.

	//current_fnum is the number of the current frame being coded in display order
	//current_code_fnum is the number of the current frame in coding order. This function increments
	//current_code_fnum by 1 each time and works out what the number is in display order.
	//show_fnum is the index of the frame number that can be shown when current_fnum has been coded.
	//Var delay is the delay caused by reordering (as distinct from buffering)

	int old_total_bits,total_bits;
	current_display_fnum=CodedToDisplay(current_code_fnum);

	if (current_code_fnum!=0)//if we're not at the beginning, clean the buffer
		my_buffer->Clean(show_fnum);

	show_fnum=std::max(current_code_fnum-delay,0);

	//read in the data if necessary and if we can

	for (int I=last_frame_read+1;I<=int(current_display_fnum);++I){
		//read from the last frame read to date to the current frame to be coded
		//(which may NOT be the next frame in display order)
		my_buffer->PushFrame(picIn,I);
		if (picIn->End()){//if we've read past the end, then should stop
			all_done=true;
		}
		last_frame_read=I;
	}//I

	if (!all_done){//we haven't coded everything, so compress the next frame
		old_total_bits=encparams.BIT_OUT->GetTotalBytes()*8;
		//set up the frame compression
		FrameCompressor my_fcoder(encparams);
		if (encparams.VERBOSE){
			std::cerr<<std::endl<<std::endl<<"Compressing frame "<<current_code_fnum<<", ";
			std::cerr<<current_display_fnum<<" in display order";
		}

		//compress the frame
		my_fcoder.Compress(*my_buffer,current_display_fnum);

		total_bits=encparams.BIT_OUT->GetTotalBytes()*8;
		if (encparams.VERBOSE)
			std::cerr<<std::endl<<std::endl<<"Total bits for frame="<<(total_bits-old_total_bits)<<std::endl;
	}
	else{
		int total_bits=encparams.BIT_OUT->GetTotalBytes()*8;
		if (encparams.VERBOSE && just_finished){
			std::cerr<<std::endl<<std::endl<<"Finished encoding.";
			std::cerr<<"Total bits for sequence="<<total_bits;
			std::cerr<<", of which "<<encparams.BIT_OUT->GetTotalHeadBytes()*8<<" were header.";
			std::cerr<<std::endl<<"Resulting bit-rate at "<<picIn->GetSeqParams().framerate<<"Hz is ";
			std::cerr<<total_bits*(picIn->GetSeqParams().framerate)/picIn->GetSeqParams().zl<<" bits/sec.";
		}
		just_finished=false;
	}
	current_code_fnum++;
	return my_buffer->GetFrame(show_fnum);
}

void SequenceCompressor::WriteStreamHeader(){
	//write out all the header data

   	//begin with the ID of the codec
	((encparams.BIT_OUT)->header).OutputBytes("KW-DIRAC");
   	//picture dimensions
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) picIn->GetSeqParams().xl);
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) picIn->GetSeqParams().yl);
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) picIn->GetSeqParams().zl);
	//picture rate
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) picIn->GetSeqParams().framerate);
    //block parameters
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.LumaBParams(2).XBLEN);
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.LumaBParams(2).YBLEN);
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.LumaBParams(2).XBSEP);
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.LumaBParams(2).YBSEP);
	//also send the number of blocks horizontally and vertically
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.X_NUMBLOCKS);
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.Y_NUMBLOCKS);
    //chroma format
	UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int) encparams.cformat);
    //interlace marker
	((encparams.BIT_OUT)->header).OutputBit(encparams.interlace);

	encparams.BIT_OUT->WriteToFile();
}

int SequenceCompressor::CodedToDisplay(int fnum){
	int div;
	if (encparams.L1_SEP>0){//we have L1 and L2 frames
		if (fnum==0)
			return 0;
		else if ((fnum-1)% encparams.L1_SEP==0){//we have L1 or subsequent I frames
			div=(fnum-1)/encparams.L1_SEP;
			return fnum+encparams.L1_SEP-1;
		}
		else//we have L2 frames
			return fnum-1;
	}
	else{//we just have I-frames, so no re-ordering
		return fnum;
	}
}
