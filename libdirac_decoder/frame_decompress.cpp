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
* Revision 1.4  2004-06-18 15:58:36  tjdwave
* Removed chroma format parameter cformat from CodecParams and derived
* classes to avoid duplication. Made consequential minor mods to
* seq_{de}compress and frame_{de}compress code.
* Revised motion compensation to use built-in arrays for weighting
* matrices and to enforce their const-ness.
* Removed unnecessary memory (de)allocations from Frame class copy constructor
* and assignment operator.
*
* Revision 1.3  2004/05/24 16:03:48  tjdwave
* Support for IO error handling. Decoder freezes on last frame if out of data.
*
* Revision 1.2  2004/05/12 08:35:34  tjdwave
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

//Decompression of frames
/////////////////////////

#include "libdirac_common/bit_manager.h"
#include "libdirac_decoder/frame_decompress.h"
#include "libdirac_decoder/comp_decompress.h"
#include "libdirac_common/mot_comp.h"
#include "libdirac_common/mv_codec.h"
#include "libdirac_common/golomb.h"
#include <iostream>

using std::vector;

bool FrameDecompressor::Decompress(FrameBuffer& my_buffer){

	FrameParams fparams( cformat , my_buffer.GetFParams().xl , my_buffer.GetFParams().yl );

		//Get the frame header (which includes the frame number)
	bool header_failure=ReadFrameHeader(fparams);

	if ( !(decparams.BIT_IN->End()) && !header_failure )
	{//if we've not finished the data, can proceed

		if ( !skipped )
		{//if we're not skipped then we can decode the rest of the frame

			if ( decparams.VERBOSE )
				std::cerr<<std::endl<<"Decoding frame "<<fparams.fnum<<" in display order";		

			//Add a frame into the buffer ready to receive the data		
			my_buffer.PushFrame(fparams);
			Frame& my_frame = my_buffer.GetFrame(fparams.fnum);//Reference to the frame being decoded
			FrameSort fsort = fparams.fsort;
			MvData* mv_data;
			unsigned int num_mv_bits;

			if ( fsort != I_frame )
			{//do all the MV stuff		
				mv_data = new MvData(decparams.X_NUM_MB,decparams.Y_NUM_MB , decparams.X_NUMBLOCKS , decparams.Y_NUMBLOCKS);

				//decode mv data
				if (decparams.VERBOSE)
					std::cerr<<std::endl<<"Decoding motion data ...";		
				vector<Context> ctxs(50);
				MvDataCodec my_mv_decoder( decparams.BIT_IN,ctxs , decparams , cformat );
				my_mv_decoder.InitContexts();//may not be necessary
				num_mv_bits = UnsignedGolombDecode( *(decparams.BIT_IN) );

				//Flush to the end of the header for the MV bits			
				decparams.BIT_IN->FlushInput();

				//Decompress the MV bits
				my_mv_decoder.Decompress( *mv_data , num_mv_bits );				
			}

	 	 	//decode components
			CompDecompress( my_buffer,fparams.fnum , Y );
			if ( fparams.cformat != Yonly )
			{
				CompDecompress( my_buffer , fparams.fnum , U );		
				CompDecompress( my_buffer , fparams.fnum , V );
			}

			if ( fsort != I_frame )
			{//motion compensate to add the data back in if we don't have an I frame
				MotionCompensator mycomp(decparams);
				mycomp.SetCompensationMode(ADD);
				mycomp.CompensateFrame(my_buffer , fparams.fnum , *mv_data);		
				delete mv_data;	
			}
			my_frame.Clip();

			if (decparams.VERBOSE)
				std::cerr<<std::endl;		

		}//?skipped,!End()
		else if (skipped){
		//TBD: decide what to return if we're skipped. Nearest frame in temporal order??	

		}

		//exit success
		return EXIT_SUCCESS;
	}
	//exit failure
	return EXIT_FAILURE;
}

void FrameDecompressor::CompDecompress(FrameBuffer& my_buffer, int fnum,CompSort cs)
{
	if ( decparams.VERBOSE )
		std::cerr<<std::endl<<"Decoding component data ...";
	CompDecompressor my_compdecoder( decparams , my_buffer.GetFrame(fnum).GetFparams() );	
	PicArray& comp_data=my_buffer.GetComponent( fnum , cs );
	my_compdecoder.Decompress( comp_data );
}

bool FrameDecompressor::ReadFrameHeader( FrameParams& fparams )
{

	if ( !(decparams.BIT_IN->End()) )
	{
		//read the frame number
		decparams.BIT_IN->InputBytes((char*) &(fparams.fnum),4);

    	//read whether the frame is skipped or not
		skipped=decparams.BIT_IN->InputBit();

		if (!skipped)
		{

            //read the expiry time relative to the frame number
			fparams.expiry_time=int( UnsignedGolombDecode(*decparams.BIT_IN) );

        	//read the frame sort
			fparams.fsort=FrameSort( UnsignedGolombDecode(*decparams.BIT_IN) );

			if ( fparams.fsort != I_frame ){

				//if not an I-frame, read how many references there are
				fparams.refs.clear();
				fparams.refs.resize( UnsignedGolombDecode(*decparams.BIT_IN) );

            	//for each reference, read the reference numbers
				for ( unsigned int I = 0 ; I < fparams.refs.size() ; ++I )
				{
					fparams.refs[I] = fparams.fnum + GolombDecode(*decparams.BIT_IN);
				}//I

				//determine whether or not there is global motion vector data
				use_global = decparams.BIT_IN->InputBit();

                //determine whether or not there is block motion vector data
				use_block_mv = decparams.BIT_IN->InputBit();

                //if there is global but no block motion vector data, determine the prediction mode to use
				//for the whole frame
				if ( use_global && !use_block_mv )
					global_pred_mode = PredMode(UnsignedGolombDecode(*decparams.BIT_IN));

			}//?is not an I frame
		}//?skipped

		//flush the header
		decparams.BIT_IN->FlushInput();

		//exit success
		return EXIT_SUCCESS;
	}//?decparams.BIT_IN->End()
	else
	{
		//exit failure	
		return EXIT_FAILURE;
	}
}
