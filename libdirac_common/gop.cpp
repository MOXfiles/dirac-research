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

#include "gop.h"
#include <iostream>

using std::vector;

Gop::~Gop() {
	for (int I=0;I<=gop_len;++I){
		delete frame_buffer[I];
		delete ref_list[I];
	}		
}

void Gop::Init(){
	gop_len=(cparams.NUM_L1+1)*cparams.L1_SEP;
	frame_buffer.resize(gop_len+1);
	ref_list.resize(gop_len+1);
	coded2display.resize(gop_len+1);

	FrameParams fparams(cparams.sparams);

	for (int I=0;I<=gop_len;++I){
		frame_buffer[I]=new Frame(fparams);
		ref_list[I]=new vector<int>;
	}
	SetGopStructure();
}

void Gop::SetFrameSorts() {	
	for (int I=0;I<=gop_len;++I) {
		if (ref_list[I]->size()>0)
			frame_buffer[I]->SetFrameSort(L2_frame);
		else
			frame_buffer[I]->SetFrameSort(I_frame);
	}
	for (int I=0;I<=gop_len;++I) {
		for (vector<int>::iterator it=ref_list[I]->begin();it!=ref_list[I]->end();++it){
			if (ref_list[*it]->size()!=0){//if the reference itself has references, then our frame is an L1 frame
				frame_buffer[*it]->SetFrameSort(L1_frame);
			}
		}
	}
}

void Gop::SetGopStructure(){


	//Start by sorting out the frame reordering
	///////////////////////////////////////////

	//first calculate the number of L1 frames
	int ratio;
	if (cparams.L1_SEP>0){//we have a proper GOP
		ratio=gop_len/cparams.L1_SEP;
		if (ratio*cparams.L1_SEP==gop_len)
			cparams.NUM_L1=ratio-1;
		else
			cparams.NUM_L1=ratio;

		coded2display[0]=0;
		int display_pos;
		int coded_pos=1;
		for (int L=1;L<=cparams.NUM_L1;++L) {
		//do the L1 frames first
			display_pos=L*cparams.L1_SEP;
			coded2display[coded_pos]=display_pos;
			coded_pos++;
		//now code the prior L2 frames
			for (int J=1; J<cparams.L1_SEP;++J){
				display_pos=(L-1)*cparams.L1_SEP+J;
				coded2display[coded_pos]=display_pos;
				coded_pos++;
			}
		}
	//finally code the final I frame and the preceding L2 frames
		coded2display[coded_pos]=gop_len;
		coded_pos++;
		for (int J=1;J<gop_len-cparams.NUM_L1*cparams.L1_SEP;++J){
			display_pos=cparams.NUM_L1*cparams.L1_SEP+J;
			coded2display[coded_pos]=display_pos;
			coded_pos++;
		}

	//Next, sort out reference frames
	/////////////////////////////////

	//First, start with the L1 frames and their preceding L2 frames
		for (int L=1;L<=cparams.NUM_L1;++L) {
			int pos=L*cparams.L1_SEP;
			ref_list[pos]->push_back((L-1)*cparams.L1_SEP);
			if (L>1)
				ref_list[pos]->push_back((L-2)*cparams.L1_SEP);
			for (int J=1; J<cparams.L1_SEP;++J) {
				pos=(L-1)*cparams.L1_SEP+J;
				ref_list[pos]->push_back((L-1)*cparams.L1_SEP);
				ref_list[pos]->push_back(L*cparams.L1_SEP);
			}
		}


	//Now do the last frame and the preceding L2 frames
		for (int J=1; J<gop_len-cparams.NUM_L1*cparams.L1_SEP;++J) {
			int pos=cparams.NUM_L1*cparams.L1_SEP+J;
			ref_list[pos]->push_back(gop_len);
			ref_list[pos]->push_back(cparams.NUM_L1*cparams.L1_SEP);
		}
	}
	else{
		//we just have I frames
		for (int I=0;I<=gop_len;++I)
			coded2display[I]=I;
	}

	//Finally, sort out the identification of frame types
	/////////////////////////////////////////////////////

	SetFrameSorts();	
}

void Gop::SetGopStructure(int ref_sep) {
	//sets the GOP structure given a separation parameter
	cparams.L1_SEP=ref_sep;
	SetGopStructure();	
}

void Gop::SetGopStructure(OneDArray<vector<int>* >& rlist,OneDArray<int>& c2dmap){
	ref_list=rlist;
	coded2display=c2dmap;
	SetFrameSorts();
}


PicArray& Gop::GetComponent(int frame_num, CompSort c) {
	if (frame_num<0)
		frame_num=0;
	if (frame_num>gop_len)
		frame_num=gop_len;

	if (c==U) 
		return frame_buffer[frame_num]->Udata();
	else if (c==V) 
		return frame_buffer[frame_num]->Vdata();
	else 
		return frame_buffer[frame_num]->Ydata();

}

PicArray& Gop::GetUpComponent(int frame_num, CompSort c) {
	if (frame_num<0)
		frame_num=0;
	if (frame_num>gop_len)
		frame_num=gop_len;

	if (c==U) 
		return frame_buffer[frame_num]->UpUdata();
	else if (c==V) 
		return frame_buffer[frame_num]->UpVdata();
	else 
		return frame_buffer[frame_num]->UpYdata();

}


void Gop::SetFrame(int frame_num, Frame& frame_data) {
	if (frame_num>=0 && frame_num<=gop_len)	
		*(frame_buffer[frame_num])=frame_data;
}

void Gop::SetComponent(int frame_num, PicArray& comp_data,CompSort c) {
	if (frame_num>=0 && frame_num<=gop_len){
		if (c==Y)
			frame_buffer[frame_num]->SetY(comp_data);
		else if (c==U) 
			frame_buffer[frame_num]->SetU(comp_data);
		else if (c==V) 
			frame_buffer[frame_num]->SetV(comp_data);
	}
}

void Gop::IncrementGopNum(){
	gop_number++;
	//swap first and last frames, so the last I frame of the last GOP becomes the first frame of the new GOP
	Frame* temp_f_ptr=frame_buffer[gop_len];
	frame_buffer[gop_len]=frame_buffer[0];
	frame_buffer[0]=temp_f_ptr;
}
