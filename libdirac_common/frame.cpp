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

//Implementation of frame classes in frame.h

#include "libdirac_common/frame.h"
#include "libdirac_common/upconvert.h"
#include <iostream>

///////////////
//---Frame---//
///////////////

Frame::Frame(const FrameParams& fp)
: 
fparams(fp),
Y_data(0),
U_data(0),
V_data(0),
upY_data(0),
upU_data(0),
upV_data(0)
{
	Init();
}

Frame::Frame(const Frame& cpy)
: 
fparams(cpy.fparams),
Y_data(0),
U_data(0),
V_data(0),
upY_data(0),
upU_data(0),
upV_data(0){
	ChromaFormat cformat=fparams.cformat;

	Init();
	*Y_data=*(cpy.Y_data);
	if (cpy.upY_data!=0){
		upY_data=new PicArray(2*Y_data->length(0),2*Y_data->length(1));
		*upY_data=*(cpy.upY_data);
	}
	if (cformat!=Yonly){
		*U_data=*(cpy.U_data);
		*V_data=*(cpy.V_data);
		if (cpy.U_data!=0){
			upU_data=new PicArray(2*U_data->length(0),2*U_data->length(1));
			*upU_data=*(cpy.upU_data);
		}
		if (cpy.V_data!=0){
			upV_data=new PicArray(2*V_data->length(0),2*V_data->length(1));
			*upV_data=*(cpy.upV_data);
		}
	}
}

Frame::~Frame(){
	ClearData();	
}

Frame& Frame::operator=(const Frame& rhs){
	if (&rhs!=this){
		fparams=rhs.fparams;
		ChromaFormat cformat=fparams.cformat;

		if (Y_data!=0)
			delete Y_data;
		Y_data=new PicArray((rhs.Y_data)->length(0),(rhs.Y_data)->length(1));
		*Y_data=*(rhs.Y_data);

		if (upY_data!=0)
			delete upY_data;
		upY_data=new PicArray((rhs.upY_data)->length(0),(rhs.upY_data)->length(1));
		*upY_data=*(rhs.upY_data);

		if (cformat!=Yonly){

			if (U_data!=0)
				delete U_data;
			U_data=new PicArray((rhs.U_data)->length(0),(rhs.U_data)->length(1));
			*U_data=*(rhs.U_data);

			if (upU_data!=0)
				delete upU_data;			
			upU_data=new PicArray((rhs.upU_data)->length(0),(rhs.upU_data)->length(1));
			*upU_data=*(rhs.upU_data);

			if (V_data!=0)
				delete V_data;
			V_data=new PicArray((rhs.V_data)->length(0),(rhs.V_data)->length(1));
			*V_data=*(rhs.V_data);

			if (upV_data!=0)
				delete upV_data;			
			upV_data=new PicArray((rhs.upV_data)->length(0),(rhs.upV_data)->length(1));
			*upV_data=*(rhs.upV_data);
		}
	}
	return *this;
}

//Other functions

void Frame::Init() {
	ChromaFormat cformat=fparams.cformat;

	//first delete data if we need to
	ClearData();

	Y_data=new PicArray(fparams.xl,fparams.yl);
	Y_data->csort=Y;
	if(cformat==format422) {
		U_data=new PicArray(fparams.xl/2,fparams.yl); U_data->csort=U;
		V_data=new PicArray(fparams.xl/2,fparams.yl); V_data->csort=V;
	}
	else if (cformat==format420){
		U_data=new PicArray(fparams.xl/2,fparams.yl/2); U_data->csort=U;
		V_data=new PicArray(fparams.xl/2,fparams.yl/2); V_data->csort=V;
	}
	else if (cformat==format411){
		U_data=new PicArray(fparams.xl/4,fparams.yl); U_data->csort=U;
		V_data=new PicArray(fparams.xl/4,fparams.yl); V_data->csort=V;
	}
	else if (cformat==format444){
		U_data=new PicArray(fparams.xl,fparams.yl); U_data->csort=U;
		V_data=new PicArray(fparams.xl,fparams.yl); V_data->csort=V;
	}
	//(other formats all assumed to be Yonly
}

PicArray& Frame::Data(CompSort cs){//another way to access the data
	if (cs==U) return *U_data; 
	else if (cs==V) return *V_data; 
	else return *Y_data;}	

const PicArray& Frame::Data(CompSort cs) const {//another way to access the data
	if (cs==U) return *U_data; 
	else if (cs==V) return *V_data; 
	else return *Y_data;}

PicArray& Frame::UpYdata(){
	if (upY_data!=0)
		return *upY_data;
	else{//we have to do the upconversion
		upY_data=new PicArray(2*Y_data->length(0),2*Y_data->length(1));
		UpConverter myupconv;
		myupconv.DoUpConverter(*Y_data,*upY_data);
		return *upY_data;
	}
}
PicArray& Frame::UpUdata(){
	if (upU_data!=0)
		return *upU_data;
	else{//we have to do the upconversion
		upU_data=new PicArray(2*U_data->length(0),2*U_data->length(1));
		UpConverter myupconv;
		myupconv.DoUpConverter(*U_data,*upU_data);
		return *upU_data;
	}
}
PicArray& Frame::UpVdata(){
	if (upV_data!=0)
		return *upV_data;
	else{//we have to do the upconversion
		upV_data=new PicArray(2*V_data->length(0),2*V_data->length(1));
		UpConverter myupconv;
		myupconv.DoUpConverter(*V_data,*upV_data);
		return *upV_data;
	}
}

PicArray& Frame::UpData(CompSort cs){
	if (cs==U) return UpUdata(); 
	else if (cs==V) return UpVdata(); 
	else return UpYdata();
}	

const PicArray& Frame::UpYdata() const{
	if (upY_data!=0)
		return *upY_data;
	else{//We have to do the upconversion
		//Although we're changing a value - the pointer to the array - it doesn't affect the state of
		//the object as viewable from outside. So the pointers to the upconveted data have been 
		//declared mutable.
		upY_data=new PicArray(2*Y_data->length(0),2*Y_data->length(1));
		UpConverter myupconv;
		myupconv.DoUpConverter(*Y_data,*upY_data);
		return *upY_data;
	}
}

const PicArray& Frame::UpUdata() const{
	if (upU_data!=0)
		return *upU_data;
	else{//We have to do the upconversion
		//Although we're changing a value - the pointer to the array - it doesn't affect the state of
		//the object as viewable from outside. So the pointers to the upconveted data have been 
		//declared mutable.
		upU_data=new PicArray(2*U_data->length(0),2*U_data->length(1));
		UpConverter myupconv;
		myupconv.DoUpConverter(*U_data,*upU_data);
		return *upU_data;
	}
}

const PicArray& Frame::UpVdata() const{
	if (upV_data!=0)
		return *upV_data;
	else{//We have to do the upconversion
		//Although we're changing a value - the pointer to the array - it doesn't affect the state of
		//the object as viewable from outside. So the pointers to the upconveted data have been 
		//declared mutable.
		upV_data=new PicArray(2*V_data->length(0),2*V_data->length(1));
		UpConverter myupconv;
		myupconv.DoUpConverter(*V_data,*upV_data);
		return *upV_data;
	}
}

const PicArray& Frame::UpData(CompSort cs) const {
	if (cs==U) return UpUdata(); 
	else if (cs==V) return UpVdata(); 
	else return UpYdata();
}	

void Frame::ClipComponent(PicArray& pic_data){
	for (int J=pic_data.first(1);J<=pic_data.last(1);++J){
		for (int I=pic_data.first(0);I<=pic_data.last(0);++I){
			pic_data[J][I]=BChk(pic_data[J][I],1021);
		}//I		
	}//J
}

void Frame::Clip(){
	//just clips the straight picture data, not the upconverted data

	ClipComponent(*Y_data);
	if (fparams.cformat!=Yonly){
		ClipComponent(*U_data);
		ClipComponent(*V_data);	
	}	
}

void Frame::ClearData(){
	if (Y_data!=0){
		delete Y_data;
		Y_data=0;
	}
	if (U_data!=0){
		delete U_data;
		U_data=0;
	}
	if (V_data!=0){
		delete V_data;
		V_data=0;
	}
	if (upY_data!=0){
		delete upY_data;
		upY_data=0;
	}
	if (upU_data!=0){
		delete upU_data;
		upU_data=0;
	}
	if (upV_data!=0){
		delete upV_data;
		upV_data=0;
	}
}
