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
* Revision 1.6  2004-06-18 15:58:36  tjdwave
* Removed chroma format parameter cformat from CodecParams and derived
* classes to avoid duplication. Made consequential minor mods to
* seq_{de}compress and frame_{de}compress code.
* Revised motion compensation to use built-in arrays for weighting
* matrices and to enforce their const-ness.
* Removed unnecessary memory (de)allocations from Frame class copy constructor
* and assignment operator.
*
* Revision 1.5  2004/05/26 15:18:28  tjdwave
* Corrected behaviour at end of stream, so that decoder freezes on the last
* frame.
*
* Revision 1.4  2004/05/24 16:03:48  tjdwave
* Support for IO error handling. Decoder freezes on last frame if out of data.
*
* Revision 1.3  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/04/11 22:50:46  chaoticcoyote
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
#include "libdirac_common/frame_buffer.h"
#include "libdirac_decoder/frame_decompress.h"

SequenceDecompressor::SequenceDecompressor(std::ifstream* ip,bool verbosity): 
all_done(false),
infile(ip),
current_code_fnum(0),
delay(1),
last_frame_read(-1),
show_fnum(-1)
{

	decparams.BIT_IN = new BitInputManager(infile);
	decparams.VERBOSE = verbosity;
	ReadStreamHeader();

	//Amount of horizontal padding for Y,U and V components
	int xpad_luma,xpad_chroma;

	//Amount of vertical padding for Y,U and V components
	int ypad_luma,ypad_chroma;

	//scaling factors for chroma based on chroma format
	int x_chroma_fac,y_chroma_fac;

	//First, we need to have sufficient padding to take account of the blocksizes.
	//It's sufficient to check for chroma

	if ( sparams.cformat == format411 )
	{
		x_chroma_fac = 4; 
		y_chroma_fac = 1;
	}
	else if ( sparams.cformat == format420 )
	{
		x_chroma_fac = 2; 
		y_chroma_fac = 2;
	}
	else if ( sparams.cformat == format422 ){
		x_chroma_fac = 2; 
		y_chroma_fac = 1;
	}
	else{
		x_chroma_fac = 1; 
		y_chroma_fac = 1;
	}

	int xl_chroma=sparams.xl / x_chroma_fac;
	int yl_chroma=sparams.yl / y_chroma_fac;

	//make sure we have enough macroblocks to cover the pictures 
	decparams.X_NUM_MB = sparams.xl / decparams.LumaBParams(0).XBSEP;
	decparams.Y_NUM_MB = sparams.yl / decparams.LumaBParams(0).YBSEP;
	if ( decparams.X_NUM_MB * decparams.ChromaBParams(0).XBSEP < xl_chroma ){
		decparams.X_NUM_MB++;
		xpad_chroma = decparams.X_NUM_MB * decparams.ChromaBParams(0).XBSEP - xl_chroma;
	}
	else
		xpad_chroma=0;

	if (decparams.Y_NUM_MB*decparams.ChromaBParams(0).YBSEP<yl_chroma){
		decparams.Y_NUM_MB++;
		ypad_chroma=decparams.Y_NUM_MB*decparams.ChromaBParams(0).YBSEP-yl_chroma;
	}
	else
		ypad_chroma=0;	

	//Now we have an integral number of macroblocks in a picture and we set the number of blocks
	decparams.X_NUMBLOCKS = 4*decparams.X_NUM_MB;
	decparams.Y_NUMBLOCKS = 4*decparams.Y_NUM_MB;

	//Next we work out the additional padding due to the wavelet transform
	//For the moment, we'll fix the transform depth to be 4, so we need divisibility by 16.
	//In the future we'll want arbitrary transform depths. It's sufficient to check for
	//chroma only

	int xpad_len = xl_chroma+xpad_chroma;
	int ypad_len = yl_chroma+ypad_chroma;
	if ( xpad_len%16 != 0 )
		xpad_chroma=( ( xpad_len/16 ) + 1 )*16 - xl_chroma;
	if ( ypad_len%16 != 0)
		ypad_chroma = ( ( ypad_len/16 ) + 1 )*16 - yl_chroma;	

	xpad_luma = xpad_chroma*x_chroma_fac;
	ypad_luma = ypad_chroma*y_chroma_fac;

	//set up padded picture sizes, based on original picture sizes, the block parameters and the wavelet transform depth
	my_buffer = new FrameBuffer( sparams.cformat , sparams.xl + xpad_luma , sparams.yl + ypad_luma );
}

SequenceDecompressor::~SequenceDecompressor(){
	infile->close();//should this be in this class ???
	delete my_buffer;
	delete decparams.BIT_IN;
}

Frame& SequenceDecompressor::DecompressNextFrame(){
	//this function decodes the next frame in coding order and returns the next frame in display order
	//In general these will differ, and because of re-ordering there is a delay which needs to be imposed.
	//This creates problems at the start and at the end of the sequence which must be dealt with.
	//At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
	//the frames out. It's up to the calling function to do something with the decoded frames as they
	//come out - write them to screen or to file, as required.

	FrameDecompressor my_fdecoder(decparams,sparams.cformat);

	if (current_code_fnum!=0){
		//if we're not at the beginning, clean the buffer of frames that can be discarded
		my_buffer->Clean(show_fnum);
	}

	int end_of_data=my_fdecoder.Decompress(*my_buffer);

	//if we've exited with success, there's a new frame to display, so increment
	//the counters. Otherwise, freeze on the last frame shown
	show_fnum=std::min(std::max(current_code_fnum-delay,0),sparams.zl-1);
	if (!end_of_data){
		current_code_fnum++;
	}
	return my_buffer->GetFrame(show_fnum);
}

void SequenceDecompressor::ReadStreamHeader(){//called from constructor

	//read the stream header parameters
	//begin with the identifying string
	OLBParams bparams;
	char kwname[9];
	for (int I=0;I<8;++I){
		kwname[I]=(decparams.BIT_IN)->InputByte();	
	}	
	kwname[8]='\0';
	//TBC: test that kwname="KW-DIRAC"	

	//picture dimensions
	sparams.xl=int(UnsignedGolombDecode(*(decparams.BIT_IN)));
	sparams.yl=int(UnsignedGolombDecode(*(decparams.BIT_IN)));	
	sparams.zl=int(UnsignedGolombDecode(*(decparams.BIT_IN)));	

	//picture rate
	sparams.framerate=int(UnsignedGolombDecode(*(decparams.BIT_IN)));

 	//block parameters
	bparams.XBLEN=int(UnsignedGolombDecode(*(decparams.BIT_IN)));
	bparams.YBLEN=int(UnsignedGolombDecode(*(decparams.BIT_IN)));	
	bparams.XBSEP=int(UnsignedGolombDecode(*(decparams.BIT_IN)));
	bparams.YBSEP=int(UnsignedGolombDecode(*(decparams.BIT_IN)));

	//dimensions of block arrays (remember there may need to be padding for some block and picture sizes)
	decparams.X_NUMBLOCKS=int(UnsignedGolombDecode(*(decparams.BIT_IN)));
	decparams.Y_NUMBLOCKS=int(UnsignedGolombDecode(*(decparams.BIT_IN)));
	decparams.X_NUM_MB=decparams.X_NUMBLOCKS/4;
	decparams.Y_NUM_MB=decparams.Y_NUMBLOCKS/4;

	//chroma format
	sparams.cformat=ChromaFormat(UnsignedGolombDecode(*(decparams.BIT_IN)));		
	decparams.SetBlockSizes(bparams,sparams.cformat);

 	//interlace marker
	decparams.interlace=decparams.BIT_IN->InputBit();
	sparams.interlace=decparams.interlace;

	//Flush the input to the end of the header
	(decparams.BIT_IN)->FlushInput();	
}
