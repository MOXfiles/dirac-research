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

#include "libdirac_common/frame_buffer.h"
#include <algorithm>

//Simple constructor for general operation
FrameBuffer::FrameBuffer(ChromaFormat cf,int xlen,int ylen): 
fparams(cf,xlen,ylen),
num_L1(0),
L1_sep(1),
gop_len(0){}	

//Constructor for coding with an initial I-frame only	
FrameBuffer::FrameBuffer(ChromaFormat cf,int L1sep, int xlen, int ylen):
fparams(cf,xlen,ylen),
num_L1(0),
L1_sep(L1sep),
gop_len(0){}

//Constructor setting GOP parameters for use with a standard GOP
FrameBuffer::FrameBuffer(ChromaFormat cf,int numL1,int L1sep,int xlen,int ylen): 
fparams(cf,xlen,ylen),
num_L1(numL1),
L1_sep(L1sep)
{	
	if (num_L1>0){//conventional GOP coding
		gop_len=(num_L1+1)*L1_sep;
	}
	else if (num_L1==0){//I-frame only coding
		gop_len=1;
		L1_sep=0;
	}
	else{ //don't have a proper GOP, only an initial I-frame
		gop_len=0;
	}	
}	

//Copy constructor. Why anyone would need this I don't know.
FrameBuffer::FrameBuffer(const FrameBuffer& cpy){
	//first delete all frames in the current buffer
	for (unsigned int I=0;I<frame_data.size();++I){
		delete frame_data[I];
	}//I

	//next create new arrays, copying from the initialising buffer
	frame_data.resize(cpy.frame_data.size());
	for (unsigned int I=0;I<frame_data.size();++I){
		frame_data[I]=new Frame(*(cpy.frame_data[I]));
	}//I

	//now copy the map
	fnum_map=cpy.fnum_map;

	//and the internal frame parameters
	fparams=cpy.fparams;
}

//Assignment=. Not sure why this would be used either.
FrameBuffer& FrameBuffer::operator=(const FrameBuffer& rhs){
	if (&rhs!=this){
		//delete all the frames in the lhs buffer
		for (unsigned int I=0;I<frame_data.size();++I){
			delete frame_data[I];
		}//I

		//next create new arrays, copying from the rhs
		frame_data.resize(rhs.frame_data.size());
		for (unsigned int I=0;I<frame_data.size();++I){
			frame_data[I]=new Frame(*(rhs.frame_data[I]));
		}//I

		//now copy the map
		fnum_map=rhs.fnum_map;

		//and the internal frame parameters
		fparams=rhs.fparams;
	}
	return *this;
}

//Destructor
FrameBuffer::~FrameBuffer(){
	for (unsigned int I=0;I<frame_data.size();++I){
		delete frame_data[I];
	}
}

Frame& FrameBuffer::GetFrame(unsigned int fnum){	//get frame with a given frame number, NOT with a given position in the buffer.
										//If the frame number does not occur, the first frame in the buffer is returned.

	std::map<unsigned int,unsigned int>::iterator it=fnum_map.find(fnum);

	unsigned int pos=0;	
	if (it!=fnum_map.end()) 
		pos=it->second;	

	return *(frame_data[pos]);
}

const Frame& FrameBuffer::GetFrame(unsigned int fnum) const{	//as above, but const version

	std::map<unsigned int,unsigned int>::const_iterator it=fnum_map.find(fnum);

	unsigned int pos=0;
	if (it!=fnum_map.end()) 
		pos=it->second;

	return *(frame_data[pos]);
}

PicArray& FrameBuffer::GetComponent(unsigned int fnum, CompSort c){//as GetFrame, but returns corresponding component
	std::map<unsigned int,unsigned int>::iterator it=fnum_map.find(fnum);

	unsigned int pos=0;
	if (it!=fnum_map.end()) 
		pos=it->second;

	if (c==U) 
		return frame_data[pos]->Udata();
	else if (c==V) 
		return frame_data[pos]->Vdata();
	else 
		return frame_data[pos]->Ydata();
}

const PicArray& FrameBuffer::GetComponent(unsigned int fnum, CompSort c) const {//as above, but const version
	std::map<unsigned int,unsigned int>::const_iterator it=fnum_map.find(fnum);

	unsigned int pos;
	if (it!=fnum_map.end()) 
		pos=it->second;

	if (c==U) 
		return frame_data[pos]->Udata();
	else if (c==V) 
		return frame_data[pos]->Vdata();
	else 
		return frame_data[pos]->Ydata();
}

PicArray& FrameBuffer::GetUpComponent(unsigned int fnum, CompSort c){//as GetFrame, but returns corresponding upconverted component
	std::map<unsigned int,unsigned int>::iterator it=fnum_map.find(fnum);

	unsigned int pos=0;
	if (it!=fnum_map.end())
		pos=it->second;

	if (c==U) 
		return frame_data[pos]->UpUdata();
	else if (c==V) 
		return frame_data[pos]->UpVdata();
	else 
		return frame_data[pos]->UpYdata();

}

const PicArray& FrameBuffer::GetUpComponent(unsigned int fnum, CompSort c) const {//as above, but const version
	std::map<unsigned int,unsigned int>::const_iterator it=fnum_map.find(fnum);

	unsigned int pos=0;
	if (it!=fnum_map.end())
		pos=it->second;

	if (c==U) 
		return frame_data[pos]->UpUdata();
	else if (c==V) 
		return frame_data[pos]->UpVdata();
	else 
		return frame_data[pos]->UpYdata();

}

void FrameBuffer::PushFrame(unsigned int frame_num){//Put a new frame onto the top of the stack using built-in frame parameters
													//with frame number frame_num
	fparams.fnum=frame_num;
	Frame* fptr=new Frame(fparams);
	frame_data.push_back(fptr);
	std::pair<unsigned int,unsigned int> temp_pair(fparams.fnum,frame_data.size()-1);
	fnum_map.insert(temp_pair);
}

void FrameBuffer::PushFrame(const FrameParams& fp){	//Put a new frame onto the top of the stack

	Frame* fptr=new Frame(fp);
	frame_data.push_back(fptr);
	std::pair<unsigned int,unsigned int> temp_pair(fp.fnum,frame_data.size()-1);
	fnum_map.insert(temp_pair);
}


void FrameBuffer::PushFrame(PicInput* picin,const FrameParams& fp){	//Read a frame onto the top of the stack

	PushFrame(fp);
	picin->ReadNextFrame(*(frame_data[frame_data.size()-1]));
}

void FrameBuffer::PushFrame(PicInput* picin, unsigned int fnum){	//Read a frame onto the top of the stack	
	SetFrameParams(fnum);
	PushFrame(picin,fparams);
}

void FrameBuffer::Remove(unsigned int pos){//remove frame fnum from the buffer, shifting everything above down

	std::pair<unsigned int,unsigned int>* tmp_pair;

	if (pos<frame_data.size()){

		delete frame_data[pos];

		frame_data.erase(frame_data.begin()+pos,frame_data.begin()+pos+1);

 		//make a new map
		fnum_map.clear();
		for (unsigned int I=0;I<frame_data.size();++I){
			tmp_pair=new std::pair<unsigned int,unsigned int>(frame_data[I]->GetFparams().fnum,I);
			fnum_map.insert(*tmp_pair);		
			delete tmp_pair;
		}//I

	}
}

void FrameBuffer::Clean(int fnum){//clean out all frames that have expired
	for (unsigned int I=0;I<frame_data.size();++I){
		if ((frame_data[I]->GetFparams().fnum+frame_data[I]->GetFparams().expiry_time)<=fnum)
			Remove(I);
	}//I
}

void FrameBuffer::SetFrameParams(unsigned int fnum){	//set the frame parameters, given the GOP set-up and the frame number in display order
														//This function can be ignored by setting the frame parameters directly if required

	fparams.fnum=fnum;
	fparams.refs.clear();	

	if (gop_len>0){

		if (fnum%gop_len==0){
			fparams.fsort=I_frame;
			fparams.expiry_time=gop_len;//expires after we've coded the next I frame
		}
		else if (fnum % L1_sep==0){
			fparams.fsort=L1_frame;
			fparams.refs.push_back((fnum/gop_len)*gop_len);//ref the last I frame
			if ((fnum-L1_sep) % gop_len>0)//we don't have the first L1 frame	
				fparams.refs.push_back(fnum-L1_sep);//other ref is the prior L1 frame
			fparams.expiry_time=L1_sep;//expires after the next L1 or I frame
		}
		else{
			fparams.fsort=L2_frame;
			fparams.refs.push_back((fnum/L1_sep)*L1_sep);
			fparams.refs.push_back(((fnum/L1_sep)+1)*L1_sep);
			fparams.expiry_time=1;	//L2 frames could expire directly after being coded, but putting in a delay of 1
										//allows for frame-skipping to be done, since the frame will still be around to
										//be used if the next frame is skipped.
		}
	}	
	else{		
		if (fnum==0){
			fparams.fsort=I_frame;
			fparams.expiry_time=1<<30;//ie never
		}
		else if (fnum % L1_sep==0){
			fparams.fsort=L1_frame;
			fparams.refs.push_back(0);//frame 0 is the I frame
			if (fnum!=L1_sep)//we don't have the first L1 frame	
				fparams.refs.push_back(fnum-L1_sep);//other ref is the prior L1 frame
			fparams.expiry_time=L1_sep;//expires after the next L1 or I frame			
		}
		else{
			fparams.fsort=L2_frame;
			fparams.refs.push_back((fnum/L1_sep)*L1_sep);
			fparams.refs.push_back(((fnum/L1_sep)+1)*L1_sep);
			fparams.expiry_time=1;	//L2 frames could expire directly after being coded, but putting in a delay of 1
										//allows for frame-skipping to be done, since the frame will still be around to
										//be used if the next frame is skipped.
		}
	}
}
