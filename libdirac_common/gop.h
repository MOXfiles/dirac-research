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

#ifndef _GOP_H_
#define _GOP_H_

#include "common.h"
#include "arrays.h"
#include "frame.h"
#include <vector>

class Gop {
	//class for group of pictures. Encapsulates both the basic data, the methods for accessing it
	//and the metadata associated with the coding GOP structure
public:

	//Constructors
	Gop(): gop_number(0){}
	Gop(const CodecParams& cp): cparams(cp),gop_number(0){Init();} 	
	//Destructor
	virtual ~Gop();

	//gets and sets
	int GopNumber(){return gop_number;}
	int GetLength(){return gop_len;}
	void SetGopStructure(int ref_sep);
	void SetGopStructure(OneDArray<std::vector<int>* >& rlist,OneDArray<int>& c2dmap);
	int CodedToDisplay(int c){return coded2display[c];}	
	std::vector<int>& GetRefs(int fnum){return *ref_list[fnum];}

	Frame& GetFrame(int frame_num) {return *(frame_buffer[frame_num]);}
	void SetFrame(int frame_num, Frame& frame_data);
	PicArray& GetComponent(int frame_num, CompSort c);
	PicArray& GetUpComponent(int frame_num, CompSort c);	
	void SetComponent(int frame_num, PicArray& comp_data,CompSort c);
	void IncrementGopNum();

private:
	CodecParams cparams;
	int gop_len;
	int gop_number;

	OneDArray<Frame*> frame_buffer;//all the frames	
	OneDArray<std::vector<int>* > ref_list;//ref_list(I) is a list of all the reference frames in the GOP for frame I
	OneDArray<int> coded2display;//maps position in coded order to position in display order

	void Init();
	void SetGopStructure();
	void SetFrameSorts();
};

#endif
