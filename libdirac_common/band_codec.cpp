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
* Revision 1.3  2004-05-20 10:50:24  stuart_hc
* Corrected CRLF line-endings to LF for consistency.
*
* Revision 1.2  2004/05/12 08:35:33  tjdwave
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

#include "libdirac_common/band_codec.h"

//encoding function
void BandCodec::DoWorkCode(PicArray& InData){

	//main coding function, using binarisation
	if (node.Parent()!=0){
		pxp=pnode.Xp();pyp=pnode.Yp();
		pxl=pnode.Xl();pyl=pnode.Yl();
	}
	else{
		pxp=0;pyp=0;
		pxl=0;pyl=0;
	}
	register ValueType val;
	qf=node.Qf(0);
	qfinv=(1<<17)/qf;
	offset=(3*qf+4)>>3;	
	cut_off_point*=qf;

	coeff_count=0;

	for (ypos=yp,pypos=pyp;ypos<yp+yl;++ypos,pypos=((ypos-yp)>>1)+pyp){
		for (xpos=xp,pxpos=pxp;xpos<xp+xl;++xpos,pxpos=((xpos-xp)>>1)+pxp){
			if (xpos==xp) nhood_sum=(ypos!=yp)? abs(InData[ypos-1][xpos]): 0;
			else
				nhood_sum=(ypos!=yp)? (abs(InData[ypos-1][xpos])+abs(InData[ypos][xpos-1])):abs(InData[ypos][xpos-1]);	
			parent_zero=bool(InData[pypos][pxpos]);
			val=InData[ypos][xpos];
			InData[ypos][xpos]=0;
			CodeVal(InData,val);

		}//xpos
	}//ypos	

//Show the symbol counts
// 		cerr<<endl<<"Context counts";
// 		for (int C=0;C<16;++C){
// 			cerr<<endl<<C<<": Zero-"<<ContextList[C].get_count0()<<", One-"<<ContextList[C].get_count1();	
// 		}
}

void BandCodec::CodeVal(PicArray& InData,ValueType& val){
	register int abs_val;

	abs_val=abs(val);
	abs_val*=qfinv;
	abs_val>>=17;

	for (int bin=1;bin<=abs_val;++bin){
		EncodeSymbol(0,ChooseContext(InData,bin));
	}
	EncodeSymbol(1,ChooseContext(InData,abs_val+1));

	if (abs_val){
		abs_val*=qf;
		InData[ypos][xpos]=ValueType(abs_val);				
		if (val>0){
			EncodeSymbol(1,ChooseSignContext(InData));
			InData[ypos][xpos]+=offset;
		}
		else{
			EncodeSymbol(0,ChooseSignContext(InData));
			InData[ypos][xpos]=-InData[ypos][xpos];
			InData[ypos][xpos]-=offset;
		}
	}
	coeff_count++;
	if (coeff_count>reset_coeff_num){
		coeff_count=0;
		Reset_all();
	}
}


void BandCodec::DoWorkDecode(PicArray& OutData, int num_bits){

	if (node.Parent()!=0){
		pxp=pnode.Xp();pyp=pnode.Yp();
		pxl=pnode.Xl();pyl=pnode.Yl();
	}
	else{
		pxp=0;pyp=0;
		pxl=0;pyl=0;
	}	

	qf=node.Qf(0);
	offset=(3*qf+4)>>3;
	cut_off_point*=qf;

	//Work
	coeff_count=0;
	for (ypos=yp,pypos=pyp;ypos<yp+yl;++ypos,pypos=((ypos-yp)>>1)+pyp){		
		for (xpos=xp,pxpos=pxp;xpos<xp+xl;++xpos,pxpos=((xpos-xp)>>1)+pxp){

			if (xpos==xp) nhood_sum=(ypos!=yp)? abs(OutData[ypos-1][xpos]): 0;
			else
				nhood_sum=(ypos!=yp)? (abs(OutData[ypos-1][xpos])+abs(OutData[ypos][xpos-1])):abs(OutData[ypos][xpos-1]);
			parent_zero=bool(OutData[pypos][pxpos]);			
			DecodeVal(OutData);			

		}//xpos
	}//ypos
}

void BandCodec::DecodeVal(PicArray& OutData){

	ValueType val=0;
	bool bit;
	int bin=1;
	do{
		DecodeSymbol(bit,ChooseContext(OutData,bin));
		if (!bit){
			val++;
		}
		bin++;
	}while (!bit);			

	OutData[ypos][xpos]=val;
	if (OutData[ypos][xpos]){
		OutData[ypos][xpos]*=qf;
		OutData[ypos][xpos]+=offset;
		DecodeSymbol(bit,ChooseSignContext(OutData));
	}
	if (!bit)
		OutData[ypos][xpos]=-OutData[ypos][xpos];

	coeff_count++;
	if (coeff_count>reset_coeff_num){
		Reset_all();
		coeff_count=0;
	}
}

int BandCodec::ChooseContext(const PicArray& Data, const int BinNumber) const{
	//condition on neighbouring values and parent values

	if (!parent_zero && (pxp!=0 || pyp!=0)){
		if (BinNumber==1){
			if(nhood_sum==0)
				return Z_BIN1z_CTX;
			else
				return Z_BIN1nz_CTX;
		}
		else if(BinNumber==2)
			return Z_BIN2_CTX;
		else if(BinNumber==3)
			return Z_BIN3_CTX;
		else if(BinNumber==4)
			return Z_BIN4_CTX;
		else
			return Z_BIN5plus_CTX;
	}
	else{
		if (BinNumber==1){
			if(nhood_sum==0)
				return NZ_BIN1z_CTX;
			else if (nhood_sum>cut_off_point)
				return NZ_BIN1b_CTX;
			else
				return NZ_BIN1a_CTX;
		}
		else if(BinNumber==2)
			return NZ_BIN2_CTX;
		else if(BinNumber==3)
			return NZ_BIN3_CTX;
		else if(BinNumber==4)
			return NZ_BIN4_CTX;
		else
			return NZ_BIN5plus_CTX;
	}
}

int BandCodec::ChooseSignContext(const PicArray& Data) const{	
	if (yp==0 && xp!=0){
		//we're in a vertically oriented subband
		if (ypos==0)
			return SIGN0_CTX;
		else{
			if (Data[ypos-1][xpos]>0)
				return SIGN_POS_CTX;		
			else if (Data[ypos-1][xpos]<0)
				return SIGN_NEG_CTX;
			else
				return SIGN0_CTX;
		}		
	}
	else if (xp==0 && yp!=0){
		//we're in a horizontally oriented subband
		if (xpos==0)
			return SIGN0_CTX;
		else{
			if (Data[ypos][xpos-1]>0)
				return SIGN_POS_CTX;				
			else if (Data[ypos][xpos-1]<0)
				return SIGN_NEG_CTX;
			else
				return SIGN0_CTX;
		}
	}
	else
		return SIGN0_CTX;

}

//////////////////////////////////////////////////////////////////////////////////
//Now for special class for LF bands (since we don't want/can't refer to parent)//
//////////////////////////////////////////////////////////////////////////////////

void LFBandCodec::DoWorkCode(PicArray& InData){
	//main coding function, using binarisation
	pxp=0; pyp=0;
	parent_zero=false;//set parent to always be zero
	register ValueType val;

	qf=node.Qf(0);
	qfinv=(1<<17)/qf;
	offset=(3*qf+4)>>3;
	cut_off_point*=qf;

	coeff_count=0;
	for (ypos=yp;ypos<yp+yl;++ypos){		
		for (xpos=xp;xpos<xp+xl;++xpos){
			if (xpos==xp) nhood_sum=(ypos!=yp)? abs(InData[ypos-1][xpos]): 0;
			else
				nhood_sum=(ypos!=yp)? (abs(InData[ypos-1][xpos])+abs(InData[ypos][xpos-1])):abs(InData[ypos][xpos-1]);	
			val=InData[ypos][xpos];
			InData[ypos][xpos]=0;
			CodeVal(InData,val);			
		}//xpos
	}//ypos	

}

void LFBandCodec::DoWorkDecode(PicArray& OutData, int num_bits){

	pxp=0; pyp=0;
	parent_zero=false;//set parent to always be zero	
	qf=node.Qf(0);
	offset=(3*qf+4)>>3;
	cut_off_point*=qf;

	//Work
	coeff_count=0;
	for (ypos=yp;ypos<yp+yl;++ypos){
		for (xpos=0;xpos<xp+xl;++xpos){
			if (xpos==xp) nhood_sum=(ypos!=yp)? abs(OutData[ypos-1][xpos]): 0;
			else
				nhood_sum=(ypos!=yp)? (abs(OutData[ypos-1][xpos])+abs(OutData[ypos][xpos-1])):abs(OutData[ypos][xpos-1]);
			DecodeVal(OutData);			
		}//xpos
	}//ypos
}

//////////////////////////////////////////////////////////////////////////////////
//Finally,special class incorporating prediction for the DC band of intra frames//
//////////////////////////////////////////////////////////////////////////////////

void IntraDCBandCodec::DoWorkCode(PicArray& InData){

	//main coding function, using binarisation
	pxp=0; pyp=0;
	parent_zero=false;//set parent to always be zero
	register ValueType val;
	PicArray PredRes(xl,yl);//residues after prediction, quantisation and inverse quant
	ValueType prediction;

	qf=node.Qf(0);
	qfinv=(1<<17)/qf;
	offset=(3*qf+4)>>3;
	cut_off_point*=qf;

	coeff_count=0;
	for (ypos=yp;ypos<yp+yl;++ypos){
		for (xpos=xp;xpos<xp+xl;++xpos){
			if (xpos==xp) nhood_sum=(ypos!=yp)? abs(PredRes[ypos-1][xpos]): 0;
			else
				nhood_sum=(ypos!=yp)? (abs(PredRes[ypos-1][xpos])+abs(PredRes[ypos][xpos-1])):abs(PredRes[ypos][xpos-1]);	
			prediction=GetPrediction(InData);			
			val=InData[ypos][xpos]-prediction;
			InData[ypos][xpos]=0;
			CodeVal(InData,val);			
			PredRes[ypos][xpos]=InData[ypos][xpos];
			InData[ypos][xpos]+=prediction;
		}//xpos

		// 		std::cerr<<std::endl<<"Val at "<<ypos<<" "<<xl-1<<" : "<<InData[ypos][xl-1];
// 		std::cerr<<std::endl<<"Contexts at "<<ypos<<" "<<xl-1<<" : ";
// 		for (int C=0;C<18;++C){
// 			std::cerr<<std::endl<<C<<": Zero "<<ContextList[C].get_count0()<<", One "<<ContextList[C].get_count1();	
// 		}

	}//ypos	

}

void IntraDCBandCodec::DoWorkDecode(PicArray& OutData, int num_bits){

	pxp=0; pyp=0;
	parent_zero=false;//set parent to always be zero
	PicArray PredRes(xl,yl);//residues after prediction, quantisation and inverse quant

	qf=node.Qf(0);
	offset=(3*qf+4)>>3;
	cut_off_point*=qf;

	//Work
	coeff_count=0;
	for (ypos=yp;ypos<yp+yl;++ypos){
		for (xpos=0;xpos<xp+xl;++xpos){
			if (xpos==xp) nhood_sum=(ypos!=yp)? abs(PredRes[ypos-1][xpos]): 0;
			else
				nhood_sum=(ypos!=yp)? (abs(PredRes[ypos-1][xpos])+abs(PredRes[ypos][xpos-1])):abs(PredRes[ypos][xpos-1]);
			DecodeVal(OutData);
			PredRes[ypos][xpos]=OutData[ypos][xpos];
			OutData[ypos][xpos]+=GetPrediction(OutData);
		}//xpos
	}//ypos
}

ValueType IntraDCBandCodec::GetPrediction(PicArray& Data){

	if (ypos!=0){
		if (xpos!=0) return (Data[ypos][xpos-1]+Data[ypos-1][xpos-1]+Data[ypos-1][xpos])/3;
		else return Data[ypos-1][0];
	}
	else{
		if(xpos!=0) return Data[0][xpos-1];
		else return 8187;
	}
}
