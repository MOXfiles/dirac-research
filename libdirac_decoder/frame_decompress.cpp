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
* Revision 1.1  2004-03-11 17:45:43  timborer
* Initial revision
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

//Decompression of frames
/////////////////////////

#include "libdirac_decoder/frame_decompress.h"
#include "libdirac_decoder/comp_decompress.h"
#include "libdirac_common/mot_comp.h"
#include "libdirac_common/mv_codec.h"
#include "libdirac_common/golomb.h"
#include <iostream>

using std::vector;

void FrameDecompressor::Decompress(Gop& my_gop, int fnum){
	ChromaFormat cf=decparams.sparams.cformat;	
	Frame& my_frame=my_gop.GetFrame(fnum);
	FrameSort fsort=my_frame.GetFparams().fsort;
	MvData* mv_data;
	int num_mv_bits;

	if (fsort!=I_frame){
		//do all the MV stuff		
		mv_data=new MvData(decparams.X_NUM_MB,decparams.Y_NUM_MB,decparams.X_NUMBLOCKS,decparams.Y_NUMBLOCKS);
        //decode mv data
		if (decparams.VERBOSE)
			std::cerr<<std::endl<<"Decoding motion data ...";		
		vector<Context> ctxs(50);
		MvDataCodec my_mv_decoder(decparams.BIT_IN,ctxs,decparams,cf);
		my_mv_decoder.InitContexts();//may not be necessary
		num_mv_bits=GolombDecode(*(decparams.BIT_IN));
		decparams.BIT_IN->FlushInput();		
		if (decparams.VERBOSE)
			std::cerr<<std::endl<<"Number of MV bits is: "<<num_mv_bits;
		my_mv_decoder.Decompress(*mv_data,num_mv_bits);		

	}

	//decode components
	CompDecompress(my_gop,fnum,Y);
	if (cf!=Yonly){
		CompDecompress(my_gop,fnum,U);		
		CompDecompress(my_gop,fnum,V);
	}

	if (fsort!=I_frame){
		//motion compensate
		MotionCompensator mycomp(decparams);
		mycomp.SetCompensationMode(ADD);
		mycomp.CompensateFrame(my_gop,fnum,*mv_data);		
		delete mv_data;	
	}
	my_gop.GetFrame(fnum).Clip();
}

void FrameDecompressor::CompDecompress(Gop& my_gop, int fnum,CompSort cs){
	if (decparams.VERBOSE)
		std::cerr<<std::endl<<"Decoding component data ...";
	CompDecompressor my_compdecoder(decparams,my_gop.GetFrame(fnum).GetFparams());	
	PicArray& comp_data=my_gop.GetComponent(fnum,cs);
	my_compdecoder.Decompress(comp_data);
}
