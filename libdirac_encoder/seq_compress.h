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
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _SEQ_COMPRESS_H_
#define _SEQ_COMPRESS_H_

/////////////////////////////////////////
//-------------------------------------//
//Class to manage compressing sequences//
//-------------------------------------//
/////////////////////////////////////////

#include "libdirac_common/common.h"
#include "libdirac_common/gop.h"
#include "libdirac_common/pic_io.h"

class SequenceCompressor{
public:

	SequenceCompressor(PicInput* pin, EncoderParams& encp): all_done(false),picIn(pin),sparams(picIn->GetSeqParams()),
	encparams(encp),my_gop(encparams),current_display_fnum(0),current_code_fnum(0),delay(1),
	last_frame_read(-1){WriteStreamHeader();}

	Frame& CompressNextFrame();	//compress the next frame in the sequence if not all done, else return last frame. 
								//Returns reference to the next locally decoded frame available for display
								//so that if coding is fast enough the compressed version could be watched real-time
								//(with suitable buffering in the calling function to account for encode-time variations).

	bool Finished(){return all_done;}

private:
	void WriteStreamHeader();	//writes the sequence data to the bitstream. This contains all the block and GOP information. 
								//In the long term we want to be able to reset all this stuff on the fly by sending a
								//parameter sets. We also want to move away from simple GOPs, to a picture buffer model.

	bool all_done;
	PicInput* picIn;
	SeqParams sparams;
	EncoderParams encparams;
	Gop my_gop;

	//state variables for CompressNextFrame
	int current_display_fnum;	//the number of the frame in display order which is to be displayed
	int current_code_fnum;		//the number of the frame in coded order which is to be coded
	int delay;					//the value so that when converted to display order, current_code_fnum is <= current_display_fnum
	int last_frame_read;		//the index, in display order, of the last frame read	
};

#endif
