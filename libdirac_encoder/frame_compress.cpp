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
* Revision 1.2  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

//Compression of frames within a gop
////////////////////////////////////

#include "libdirac_encoder/frame_compress.h"
#include "libdirac_encoder/comp_compress.h"
#include "libdirac_common/mot_comp.h"
#include "libdirac_motionest/motion_estimate.h"
#include "libdirac_common/mv_codec.h"
#include "libdirac_common/golomb.h"
#include "libdirac_common/bit_manager.h"
#include <iostream>

FrameCompressor::FrameCompressor(const EncoderParams& encp) :
encparams(encp),
skipped(false),
use_global(false),
use_block_mv(true),
global_pred_mode(REF1_ONLY){ }

void FrameCompressor::Compress(FrameBuffer& my_buffer, int fnum){

	ChromaFormat cf=encparams.cformat;	
	Frame& my_frame=my_buffer.GetFrame(fnum);
	FrameSort fsort=my_frame.GetFparams().fsort;
	unsigned int num_mv_bits;//number of bits written, without byte alignment

	CompCompressor my_compcoder(encparams,my_frame.GetFparams());

	if (fsort!=I_frame){
		mv_data=new MvData(encparams.X_NUM_MB,encparams.Y_NUM_MB,encparams.X_NUMBLOCKS,encparams.Y_NUMBLOCKS);
		//motion estimate first
		if (encparams.VERBOSE) std::cerr<<std::endl<<"Motion estimating ...";
		MotionEstimator my_motEst(encparams);
		my_motEst.DoME(my_buffer,fnum,*mv_data);
	}

	//Write the frame header. We wait until after motion estimation, since this allows us to do cut-detection
	//and (possibly) to decide whether or not to skip a frame before actually encoding anything. However we 
	//can do this at any point prior to actually writing any frame data.
	WriteFrameHeader(my_frame.GetFparams());

	if (!skipped){//if not skipped we continue with the coding

		if (fsort!=I_frame){

 			//code the MV data
			if (encparams.VERBOSE) std::cerr<<std::endl<<"Coding motion data ...";
			if (use_global){//code the global motion parameters

			}
			std::vector<Context> ctxs(50);
			MvDataCodec my_mv_coder(&((encparams.BIT_OUT)->data),ctxs,encparams,cf);
			my_mv_coder.InitContexts();//may not be necessary
			num_mv_bits=my_mv_coder.Compress(*mv_data);			
			UnsignedGolombCode((encparams.BIT_OUT)->header,num_mv_bits);
			encparams.BIT_OUT->WriteToFile();

			unsigned int mv_bits=encparams.BIT_OUT->GetUnitBytes()*8;			//actual number of mv bits written including
			unsigned int mv_head_bits=encparams.BIT_OUT->GetUnitHeadBytes()*8;	// alignment and header
			if (encparams.VERBOSE) {
				std::cerr<<std::endl<<"Number of MV bits is: "<<mv_bits;
				std::cerr<<", of which "<<mv_head_bits<<" were header";
			}

 			//then motion compensate
			if (encparams.VERBOSE)
				std::cerr<<std::endl<<"Motion compensating ...";

			MotionCompensator mycomp(encparams);
			mycomp.SetCompensationMode(SUBTRACT);
			mycomp.CompensateFrame(my_buffer,fnum,*mv_data);

		}//?fsort

 		//code components
		my_compcoder.Compress(my_buffer.GetComponent(fnum,Y));
		if (cf!=Yonly){
			my_compcoder.Compress(my_buffer.GetComponent(fnum,U));
			my_compcoder.Compress(my_buffer.GetComponent(fnum,V));
		}
 		//motion compensate again
		if (fsort!=I_frame){
			MotionCompensator mycomp(encparams);
			mycomp.SetCompensationMode(ADD);
			mycomp.CompensateFrame(my_buffer,fnum,*mv_data);
			delete mv_data;	
		}//?fsort
 		//finally clip the data to keep it in range
		my_buffer.GetFrame(fnum).Clip();

	}//?skipped
}

void FrameCompressor::WriteFrameHeader(const FrameParams& fparams){

	//write the frame number
	((encparams.BIT_OUT)->header).OutputBytes((char*) &(fparams.fnum),4);
	//write whether the frame is skipped or not
	((encparams.BIT_OUT)->header).OutputBit(skipped);
	if (!skipped){//if we're not skipped, then we write the rest of the metadata
		//write the expiry time relative to the frame number
		UnsignedGolombCode((encparams.BIT_OUT)->header,fparams.expiry_time);
		//write the frame sort
		UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int)fparams.fsort);		
		if (fparams.fsort!=I_frame){		
			//if not an I-frame, write how many references there are		
			UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int)fparams.refs.size());
			//for each reference, write the reference number relative to the frame number
			for (unsigned int I=0;I<fparams.refs.size();++I){
				GolombCode((encparams.BIT_OUT)->header,fparams.refs[I]-fparams.fnum);
			}//I

			//indicate whether or not there is global motion vector data
			((encparams.BIT_OUT)->header).OutputBit(use_global);
			//indicate whether or not there is block motion vector data
			((encparams.BIT_OUT)->header).OutputBit(use_block_mv);
			//if there is global but no block motion vector data, indicate the prediction mode to use
			//for the whole frame
			if (use_global && !use_block_mv){
				UnsignedGolombCode((encparams.BIT_OUT)->header,(unsigned int)global_pred_mode);
			}
		}
	}//?skipped

	//write the header out to file
	encparams.BIT_OUT->WriteToFile();
}
