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

//Compression of frames//
/////////////////////////

#include <libdirac_encoder/frame_compress.h>
#include <libdirac_encoder/comp_compress.h>
#include <libdirac_common/mot_comp.h>
#include <libdirac_motionest/motion_estimate.h>
#include <libdirac_common/mv_codec.h>
#include <libdirac_common/golomb.h>
#include <libdirac_common/bit_manager.h>
#include <iostream>

FrameCompressor::FrameCompressor( EncoderParams& encp ) :
m_encparams(encp),
m_skipped(false),
m_use_global(false),
m_use_block_mv(true),
m_global_pred_mode(REF1_ONLY)
{}

void FrameCompressor::Compress(FrameBuffer& my_buffer, int fnum)
{

	Frame& my_frame=my_buffer.GetFrame(fnum);
	const FrameSort& fsort=my_frame.GetFparams().FSort();
	const ChromaFormat cformat=my_frame.GetFparams().CFormat();
	MvData* mv_data;

	unsigned int num_mv_bits;//number of bits written, without byte alignment

	CompCompressor my_compcoder(m_encparams , my_frame.GetFparams());

	if (fsort!=I_frame){
		mv_data=new MvData(m_encparams.XNumMB(),m_encparams.YNumMB(),m_encparams.XNumBlocks(),m_encparams.YNumBlocks());
		//motion estimate first
		if (m_encparams.Verbose()) std::cerr<<std::endl<<"Motion estimating ...";
		MotionEstimator my_motEst(m_encparams);
		my_motEst.DoME(my_buffer,fnum,*mv_data);
	}

	//Write the frame header. We wait until after motion estimation, since this allows us to do cut-detection
	//and (possibly) to decide whether or not to skip a frame before actually encoding anything. However we 
	//can do this at any point prior to actually writing any frame data.
	WriteFrameHeader(my_frame.GetFparams());

	if ( !m_skipped )
	{	//if not skipped we continue with the coding

		if ( fsort != I_frame){

 			//code the MV data
			if (m_encparams.Verbose()) std::cerr<<std::endl<<"Coding motion data ...";

			//if we're using global motion parameters, code them
			if (m_use_global)
			{
				/*
					Code the global motion parameters
					TBD ....
				*/
			}

			//if we're using block motion vectors, code them
			if (m_use_block_mv)
			{
				MvDataCodec my_mv_coder( &(m_encparams.BitsOut().Data() ) , 50 , cformat);
				my_mv_coder.InitContexts();//may not be necessary
				num_mv_bits=my_mv_coder.Compress(*mv_data);			
				UnsignedGolombCode(m_encparams.BitsOut().Header() , num_mv_bits);
				m_encparams.BitsOut().WriteToFile();
			}

			//actual number of mv bits written including alignment and header
			unsigned int mv_bits=m_encparams.BitsOut().GetUnitBytes()*8;			
			unsigned int mv_head_bits=m_encparams.BitsOut().GetUnitHeadBytes()*8;

			if (m_encparams.Verbose())
			{
				std::cerr<<std::endl<<"Number of MV bits is: "<<mv_bits;
				std::cerr<<", of which "<<mv_head_bits<<" were header";
			}

 			//then motion compensate
			if (m_encparams.Verbose())
				std::cerr<<std::endl<<"Motion compensating ...";

			MotionCompensator mycomp(m_encparams);
			mycomp.SetCompensationMode(SUBTRACT);
			mycomp.CompensateFrame(my_buffer,fnum,*mv_data);

		}//?fsort

 		//code component data
		my_compcoder.Compress(my_buffer.GetComponent( fnum , Y_COMP));
 		if (cformat != Yonly)
         {
 			my_compcoder.Compress(my_buffer.GetComponent( fnum , U_COMP));
 			my_compcoder.Compress(my_buffer.GetComponent( fnum , V_COMP));
 		}

 		//motion compensate again if necessary
		if ( fsort != I_frame )
		{
			MotionCompensator mycomp(m_encparams);
			mycomp.SetCompensationMode(ADD);
			mycomp.CompensateFrame(my_buffer,fnum,*mv_data);
			delete mv_data;	
		}//?fsort
 		//finally clip the data to keep it in range
		my_buffer.GetFrame(fnum).Clip();

	}//?m_skipped
}

void FrameCompressor::WriteFrameHeader(const FrameParams& fparams){

	//write the frame number
	int temp_int=fparams.FrameNum();
	m_encparams.BitsOut().Header().OutputBytes((char*) &temp_int,4);

	//write whether the frame is m_skipped or not
	m_encparams.BitsOut().Header().OutputBit(m_skipped);

	if (!m_skipped)
	{//if we're not m_skipped, then we write the rest of the metadata

		//write the expiry time relative to the frame number
		UnsignedGolombCode(m_encparams.BitsOut().Header() , fparams.ExpiryTime());

		//write the frame sort
		UnsignedGolombCode(m_encparams.BitsOut().Header() , (unsigned int)fparams.FSort());		

		if (fparams.FSort() != I_frame)
		{		
			//if not an I-frame, write how many references there are		
			UnsignedGolombCode(m_encparams.BitsOut().Header() , (unsigned int)fparams.Refs().size());

			//for each reference, write the reference number relative to the frame number
			for (unsigned int I=0 ; I<fparams.Refs().size() ; ++I){
				GolombCode(m_encparams.BitsOut().Header() , fparams.Refs()[I]-fparams.FrameNum() );
			}//I

			//indicate whether or not there is global motion vector data
			m_encparams.BitsOut().Header().OutputBit(m_use_global);

			//indicate whether or not there is block motion vector data
			m_encparams.BitsOut().Header().OutputBit(m_use_block_mv);

			//if there is global but no block motion vector data, indicate the prediction mode to use
			//for the whole frame
			if (m_use_global && !m_use_block_mv)
			{
				UnsignedGolombCode(m_encparams.BitsOut().Header() , (unsigned int)m_global_pred_mode);
			}
		}

	}//?m_skipped

	//finish by writing the header out to file
	m_encparams.BitsOut().WriteToFile();
}
