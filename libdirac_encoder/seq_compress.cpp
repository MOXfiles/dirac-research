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
* Revision 1.2  2004-03-22 01:04:28  chaoticcoyote
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

/////////////////////////////////////////
//-------------------------------------//
//Class to manage compressing sequences//
//-------------------------------------//
/////////////////////////////////////////

#include "libdirac_encoder/seq_compress.h"
#include "libdirac_encoder/frame_compress.h"
#include "libdirac_common/golomb.h"
#include <iostream>

SequenceCompressor::SequenceCompressor(PicInput* pin, EncoderParams& encp)
  : all_done(false),
    picIn(pin),
    sparams(picIn->GetSeqParams()),
	encparams(encp),
    my_gop(encparams),
    current_display_fnum(0),
    current_code_fnum(0),
    delay(1),
	last_frame_read(-1)
{
    WriteStreamHeader();
}

Frame& SequenceCompressor::CompressNextFrame(){

	//this function codes the next frame in coding order and returns the next frame in display order
	//In general these will differ, and because of re-ordering there is a delay which needs to be imposed.
	//This creates problems at the start and at the end of the sequence which must be dealt with.
	//At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
	//the frames out. It's up to the calling function to do something with the decoded frames as they
	//come out - write them to screen or to file, or whatever. TJD 13Feb04.

	FrameCompressor my_fcoder(encparams);
	int fnum;
	int zpos;
	int old_total_bits;
	int total_bits;

	//set which frame to code
	fnum=my_gop.CodedToDisplay(current_code_fnum);
	zpos=my_gop.GopNumber()*my_gop.GetLength()+fnum;

	//read in the data if necessary
	if (zpos<sparams.zl){	
		for (int I=last_frame_read+1;I<=fnum;++I){
			//read from the last frame read to date to the current frame to be coded
			//(which may NOT be the next frame in display order)
			my_gop.GetFrame(fnum).Init();			
			picIn->ReadNextFrame(my_gop.GetFrame(I));
			last_frame_read=I;
		}//I		
	}
	else
		all_done=true;

	if (!all_done){//we haven't coded everything

		if (encparams.VERBOSE){
			std::cerr<<std::endl<<std::endl;
			std::cerr<<"Coding frame "<<my_gop.GopNumber()*my_gop.GetLength()+current_code_fnum;
			std::cerr<<" (gop number "<<my_gop.GopNumber()<<"), ";
			std::cerr<<zpos<<" in display order";
		}

		old_total_bits=encparams.BIT_OUT->GetTotalBytes()*8;
		my_fcoder.Compress(my_gop,fnum);
		total_bits=encparams.BIT_OUT->GetTotalBytes()*8;
		if (encparams.VERBOSE)
			std::cerr<<std::endl<<std::endl<<"Total bits for frame="<<(total_bits-old_total_bits)<<std::endl;

		//set which frame to display
		current_display_fnum=current_code_fnum-delay;
		if (current_display_fnum<0){		
			if (my_gop.GopNumber()>0)
				current_display_fnum+=my_gop.GetLength();
			else
				current_display_fnum=0;
		}

		current_code_fnum++;
		if (current_code_fnum>my_gop.GetLength()){
			current_code_fnum=1;
			my_gop.IncrementGopNum();
			last_frame_read=0;
		}
	}
	else{//we have coded everything, but we may not have displayed everything
		current_display_fnum=std::min(current_code_fnum-delay,my_gop.GetLength());
		current_code_fnum++;
	}

	return my_gop.GetFrame(current_display_fnum);
}

void SequenceCompressor::WriteStreamHeader(){
	//write out all the header data

   	//begin with the ID of the codec
	((encparams.BIT_OUT)->header).OutputBytes("KW-DIRAC");

   	//picture dimensions
	GolombCode((encparams.BIT_OUT)->header,encparams.sparams.xl);
	GolombCode((encparams.BIT_OUT)->header,encparams.sparams.yl);
	GolombCode((encparams.BIT_OUT)->header,encparams.sparams.zl);
	//picture rate
	GolombCode((encparams.BIT_OUT)->header,encparams.sparams.framerate);

    //block parameters
	GolombCode((encparams.BIT_OUT)->header,encparams.LumaBParams(2).XBLEN);	
	GolombCode((encparams.BIT_OUT)->header,encparams.LumaBParams(2).YBLEN);
	GolombCode((encparams.BIT_OUT)->header,encparams.LumaBParams(2).XBSEP);
	GolombCode((encparams.BIT_OUT)->header,encparams.LumaBParams(2).YBSEP);

    //chroma format
	GolombCode((encparams.BIT_OUT)->header,int(encparams.sparams.cformat));

    //GOP parameters
	GolombCode((encparams.BIT_OUT)->header,encparams.NUM_L1);
	GolombCode((encparams.BIT_OUT)->header,encparams.L1_SEP);

    //interlace marker
	GolombCode((encparams.BIT_OUT)->header,int(encparams.sparams.interlace));

	encparams.BIT_OUT->WriteToFile();
}
