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
* Revision 1.2  2004-04-11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

///////////////////////////////////////////
//---------------------------------------//
//Class to manage decompressing sequences//
//---------------------------------------//
///////////////////////////////////////////

#include "libdirac_decoder/seq_decompress.h"
#include "libdirac_common/common.h"
#include "libdirac_common/golomb.h"
#include "libdirac_common/gop.h"
#include "libdirac_decoder/frame_decompress.h"

//Add bitstream IO and initialise IO objects!!!!!

SequenceDecompressor::SequenceDecompressor(std::ifstream* ip,bool verbosity): all_done(false),infile(ip),
current_display_fnum(0),current_code_fnum(0),delay(1),last_frame_read(-1)
{
	decparams.BIT_IN=new BitInputManager(infile);
	decparams.VERBOSE=verbosity;
	ReadStreamHeader();
	my_gop=new Gop(decparams);	
}

SequenceDecompressor::~SequenceDecompressor(){
	infile->close();//should this be in this class ???
	delete my_gop;
	delete decparams.BIT_IN;
}

Frame& SequenceDecompressor::DecompressNextFrame(){
	//this function decodes the next frame in coding order and returns the next frame in display order
	//In general these will differ, and because of re-ordering there is a delay which needs to be imposed.
	//This creates problems at the start and at the end of the sequence which must be dealt with.
	//At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
	//the frames out. It's up to the calling function to do something with the decoded frames as they
	//come out - write them to screen or to file, as required.

	FrameDecompressor my_fdecoder(decparams);
	int fnum;
	int zpos;

	//set which frame to code
	fnum=my_gop->CodedToDisplay(current_code_fnum);
	zpos=my_gop->GopNumber()*my_gop->GetLength()+fnum;

	if (zpos>=decparams.sparams.zl)
		all_done=true;

	if (!all_done){//we haven't decoded everything

		if (decparams.VERBOSE){
			std::cerr<<std::endl<<"Decoding frame "<<my_gop->GopNumber()*my_gop->GetLength()+current_code_fnum;
			std::cerr<<" (gop number "<<my_gop->GopNumber()<<"), ";
			std::cerr<<zpos<<" in display order";
		}

		my_gop->GetFrame(fnum).Init();

		my_fdecoder.Decompress(*my_gop,fnum);

		//set which frame to display
		current_display_fnum=current_code_fnum-delay;
		if (current_display_fnum<0){		
			if (my_gop->GopNumber()>0)
				current_display_fnum+=my_gop->GetLength();
			else
				current_display_fnum=0;
		}

		current_code_fnum++;
		if (current_code_fnum>my_gop->GetLength()){
			current_code_fnum=1;
			my_gop->IncrementGopNum();
			last_frame_read=0;
		}
	}
	else{//we have decoded everything, but we may not have output everything
		current_display_fnum=DIRAC_MIN(current_code_fnum-delay,my_gop->GetLength());
		current_code_fnum++;
	}

	return my_gop->GetFrame(current_display_fnum);	
}

void SequenceDecompressor::ReadStreamHeader(){//called from constructor

	//read the stream header parameters
	//begin with the identifying string
	OLBParams bparams;
	char kwname[9];
	for (int I=0;I<8;++I){
		kwname[I]=(decparams.BIT_IN)->input_byte();	
	}	
	kwname[8]='\0';
	//TBC: test that kwname="KW-DIRAC"	

	//picture dimensions
	decparams.sparams.xl=GolombDecode(*(decparams.BIT_IN));
	decparams.sparams.yl=GolombDecode(*(decparams.BIT_IN));	
	decparams.sparams.zl=GolombDecode(*(decparams.BIT_IN));
	//picture rate
	decparams.sparams.framerate=GolombDecode(*(decparams.BIT_IN));
 	//block parameters
	bparams.XBLEN=GolombDecode(*(decparams.BIT_IN));	
	bparams.YBLEN=GolombDecode(*(decparams.BIT_IN));	
	bparams.XBSEP=GolombDecode(*(decparams.BIT_IN));
	bparams.YBSEP=GolombDecode(*(decparams.BIT_IN));		
	//chroma format
	decparams.sparams.cformat=ChromaFormat(GolombDecode(*(decparams.BIT_IN)));
	decparams.SetBlockSizes(bparams);
 	//GOP parameters
	decparams.NUM_L1=GolombDecode(*(decparams.BIT_IN));
	decparams.L1_SEP=GolombDecode(*(decparams.BIT_IN));
	if (decparams.NUM_L1>0 && decparams.L1_SEP>0)
		decparams.GOP_LEN=(decparams.NUM_L1+1)*decparams.L1_SEP;
	else{
		decparams.NUM_L1=0;
		decparams.L1_SEP=0;
		decparams.GOP_LEN=1;
	}

 	//interlace marker
	decparams.sparams.interlace=bool(GolombDecode(*(decparams.BIT_IN)));

	(decparams.BIT_IN)->FlushInput();	
}
