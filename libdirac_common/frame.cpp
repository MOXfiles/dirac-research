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

//Implementation of frame classes in frame.h

#include "frame.h"
#include "upconvert.h"
#include <iostream>

///////////////
//---Frame---//
///////////////

//Destructor

Frame::~Frame(){
	ClearData();	
}

//Other functions

void Frame::Init() {
	SeqParams& sparams=fparams.seq_params;
	int xl=sparams.xl;
	int yl=sparams.yl;

	//first delete data if we need to
	ClearData();

	ChromaFormat cformat=sparams.cformat;
	Y_data=new PicArray(xl,yl);
	Y_data->csort=Y;
	if(cformat==format422) {
		U_data=new PicArray(xl/2,yl); U_data->csort=U;
		V_data=new PicArray(xl/2,yl); V_data->csort=V;
	}
	else if (cformat==format420){
		U_data=new PicArray(xl/2,yl/2); U_data->csort=U;
		V_data=new PicArray(xl/2,yl/2); V_data->csort=V;
	}
	else if (cformat==format411){
		U_data=new PicArray(xl/4,yl); U_data->csort=U;
		V_data=new PicArray(xl/4,yl); V_data->csort=V;
	}
	else if (cformat==format444){
		U_data=new PicArray(xl,yl); U_data->csort=U;
		V_data=new PicArray(xl,yl); V_data->csort=V;
	}
	//(other formats all assumed to be Yonly
}

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

void Frame::SetY(PicArray& in_array){
	*Y_data=in_array;
}

void Frame::SetU(PicArray& in_array){
	*U_data=in_array;
}

void Frame::SetV(PicArray& in_array){
	*V_data=in_array;
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
	SeqParams& sparams=fparams.seq_params;
	ClipComponent(*Y_data);
	if (sparams.cformat!=Yonly){
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
