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
* Revision 1.3  2004-05-12 08:35:34  tjdwave
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

#include "libdirac_common/wavelet_utils.h"
#include "libdirac_common/common.h"
#include <iostream>
#include <cstdlib>

//subband list methods

void SubbandList::Init(const int depth,const int xlen,const int ylen){
	int xl=xlen;
	int yl=ylen;
	Clear();
	Subband* tmp;
	for (int L=1;L<=depth;++L){
		xl/=2;
		yl/=2;
		tmp=new Subband(xl,0,xl,yl,L);
		AddBand(*tmp);
		delete tmp;
		tmp=new Subband(0,yl,xl,yl,L);
		AddBand(*tmp);
		delete tmp;
		tmp=new Subband(xl,yl,xl,yl,L);
		AddBand(*tmp);
		delete tmp;
		if (L==depth){
			tmp=new Subband(0,0,xl,yl,L);
			AddBand(*tmp);
			delete tmp;
		}		
	}
	//now set the parent-child relationships
	int len=bands.size();
	(*this)(len).SetParent(0);		
	(*this)(len).AddChild(len-3);
	(*this)(len-3).SetParent(len);
	(*this)(len).AddChild(len-2);
	(*this)(len-2).SetParent(len);
	(*this)(len).AddChild(len-1);
	(*this)(len-1).SetParent(len);

	for (int L=1;L<depth;++L){
 		//do parent-child relationship for other bands
		(*this)(3*L+1).AddChild(3*(L-1)+1);
		(*this)(3*(L-1)+1).SetParent(3*L+1);
		(*this)(3*L+2).AddChild(3*(L-1)+2);
		(*this)(3*(L-1)+2).SetParent(3*L+2);
		(*this)(3*L+3).AddChild(3*(L-1)+3);
		(*this)(3*(L-1)+3).SetParent(3*L+3);
	}
}

//wavelet transform methods
///////////////////////////

//public methods

void WaveletTransform::Transform(Direction d, PicArray& pic_data){
	int xl,yl;

	if (d==FORWARD){
		//do work
		xl=pic_data.length(0);
		yl=pic_data.length(1);
		for (int L=1;L<=params.depth;++L){
			VHSplit(0,0,xl,yl,pic_data);
			xl=xl/2;
			yl=yl/2;
		}

		band_list.Init(params.depth,pic_data.length(0),pic_data.length(1));
	}
	else{
		//do work
		xl=pic_data.length(0)/(1<<(params.depth-1));
		yl=pic_data.length(1)/(1<<(params.depth-1));
		for (int L=1;L<=params.depth;++L){
			VHSynth(0,0,xl,yl,pic_data);
			xl*=2;
			yl*=2;
		}
		//band list now inaccurate, so clear		
		band_list.Clear();	
	}
}

//private functions
///////////////////

void WaveletTransform::VHSplit(const int xp, const int yp, const int xl, const int yl, PicArray& pic_data){

	//version based on integer-like types
	//using edge-extension rather than reflection

	OneDArray<ValueType*> tmp_data(yl);
	const int xl2=xl/2;
	const int yl2=yl/2;
	register ValueType* line_data;
	register ValueType* col_data=new ValueType[yl];	

	int I,J,K,L;

 	//first do horizontal 

	int xend=xl2;	
	int yend=yl;


	for (J=0; J<yend;++J){
		line_data=new ValueType[xl];

		for (I=0,K=xl2,L=0; I<xend;++I,++K,++L){
			line_data[I]=pic_data[J+yp][L+xp];
			line_data[K]=pic_data[J+yp][++L+xp];			
		}//I

    		//first lifting stage
		line_data[xl2]-=ValueType((6497*int(line_data[1]+line_data[0]))>>12);
		line_data[0]-=ValueType((217*int(line_data[xl2]+line_data[xl2]))>>12);
		for (I=1,K=xl2+1; I<xend-1;++I,++K){//main body
			line_data[K]-=ValueType((6497*int(line_data[I+1]+line_data[I]))>>12);
			line_data[I]-=ValueType((217*int(line_data[K-1]+line_data[K]))>>12);
		}//I
		line_data[xl-1]-=ValueType((6497*int(line_data[xend-1]+line_data[xend-1]))>>12);
		line_data[xend-1]-=ValueType((217*int(line_data[xl-2]+line_data[xl-1]))>>12);

     		//second lifting stage
		line_data[xl2]+=ValueType((3616*int(line_data[1]+line_data[0]))>>12);
		line_data[0]+=ValueType((1817*int(line_data[xl2]+line_data[xl2]))>>12);
		for (I=1,K=xl2+1; I<xend-1;++I,++K){//main body
			line_data[K]+=ValueType((3616*int(line_data[I+1]+line_data[I]))>>12);
			line_data[I]+=ValueType((1817*int(line_data[K-1]+line_data[K]))>>12);
		}
		line_data[xl-1]+=ValueType((3616*int(line_data[xend-1]+line_data[xend-1]))>>12);
		line_data[xend-1]+=ValueType((1817*int(line_data[xl-2]+line_data[xl-1]))>>12);

		for (I=0,K=xl2; I<xl2;++I,++K){
//			TBC: scale factors can be removed and incorporated into noise
//			weighting factors for quantisation

			line_data[I]=ValueType((4709*int(line_data[I]))>>12);
			line_data[K]=ValueType((3563*int(line_data[K]))>>12);
		}
		tmp_data[J]=line_data;
	}

 	 	//next do vertical

	xend=xl;	
	yend=yl2;

	for (I=0; I<xend;++I){

		for (J=0,K=yl2,L=0; J<yend;++J,++K,++L){
			col_data[J]=tmp_data[L][I];
			col_data[K]=tmp_data[++L][I];			
		}//J

   		//first lifting stage
		col_data[yl2]-=ValueType((6497*int(col_data[1]+col_data[0]))>>12);
		col_data[0]-=ValueType((217*int(col_data[yl2]+col_data[yl2]))>>12);
		for (J=1,K=yl2+1; J<yend-1;++J,++K){//main body
			col_data[K]-=ValueType((6497*int(col_data[J+1]+col_data[J]))>>12);
			col_data[J]-=ValueType((217*int(col_data[K-1]+col_data[K]))>>12);
		}//J
		col_data[yl-1]-=ValueType((6497*int(col_data[yend-1]+col_data[yend-1]))>>12);
		col_data[yend-1]-=ValueType((217*int(col_data[yl-2]+col_data[yl-1]))>>12);		

    		//second lifting stage
		col_data[yl2]+=ValueType((3616*int(col_data[1]+col_data[0]))>>12);
		col_data[0]+=ValueType((1817*int(col_data[yl2]+col_data[yl2]))>>12);
		for (J=1,K=yl2+1; J<yend-1;++J,++K){
			col_data[K]+=ValueType((3616*int(col_data[J+1]+col_data[J]))>>12);	
			col_data[J]+=ValueType((1817*int(col_data[K-1]+col_data[K]))>>12);
		}//J
		col_data[yl-1]+=ValueType((3616*int(col_data[yend-1]+col_data[yend-1]))>>12);
		col_data[yend-1]+=ValueType((1817*int(col_data[yl-2]+col_data[yl-1]))>>12);

		for (J=0,K=yl2; J<yend;++J,++K){
// 			pic_data[J+yp][I+xp]=col_data[J];
// 			pic_data[K+yp][I+xp]=col_data[K];
//			TBC: scale factors can be removed and incorporated into noise
//			weighting factors for quantisation

			pic_data[J+yp][I+xp]=ValueType((4709*int(col_data[J]))>>12);
			pic_data[K+yp][I+xp]=ValueType((3563*int(col_data[K]))>>12);

		}//I

	}
	delete[] col_data;	
	for (int J=0;J<yl;++J)
		delete[] tmp_data[J];

}


void WaveletTransform::VHSynth(const int xp, const int yp, const int xl, const int yl, PicArray& pic_data){	

	register ValueType* line_data=new ValueType[xl];
	register ValueType* col_data;	
	OneDArray<ValueType*> tmp_data(xl);

	const int xl2=xl/2;
	const int yl2=yl/2;
	int I,J,K,L,M;//positional variables

	//first do vertical synth//
	///////////////////////////

	int xend=xl;	
	int yend=yl2;

	for (I=0; I<xend;++I){		
		col_data=new ValueType[yl];
		for (J=0,K=yl2;J<yl2;++J,++K){
// 			col_data[J]=pic_data[J+yp][I+xp];	
// 			col_data[K]=pic_data[K+yp][I+xp];			
//			TBC: scale factors can be removed and incorporated into noise
//			weighting factors for quantisation

			col_data[J]=ValueType((3563*int(pic_data[J+yp][I+xp]))>>12);	
			col_data[K]=ValueType((4709*int(pic_data[K+yp][I+xp]))>>12);			
		}

		//first lifting stage
		col_data[yend-1]-=ValueType((1817*int(col_data[yl-2]+col_data[yl-1]))>>12);
		col_data[yl-1]-=ValueType((3616*int(col_data[yend-1]+col_data[yend-1]))>>12);
		for (J=yend-2,K=yl-2; J>=1;--J,--K){//main body
			col_data[J]-=ValueType((1817*int(col_data[K-1]+col_data[K]))>>12);
			col_data[K]-=ValueType((3616*int(col_data[J+1]+col_data[J]))>>12);
		}//J
		col_data[0]-=ValueType((1817*int(col_data[yl2]+col_data[yl2]))>>12);
		col_data[yl2]-=ValueType((3616*int(col_data[1]+col_data[0]))>>12);

  		//second lifting stage
		col_data[yend-1]+=ValueType((217*int(col_data[yl-2]+col_data[yl-1]))>>12);
		col_data[yl-1]+=ValueType((6497*int(col_data[yend-1]+col_data[yend-1]))>>12);			
		for (J=yend-2,K=yl-2; J>=1;--J,--K){//main body
			col_data[J]+=ValueType((217*int(col_data[K-1]+col_data[K]))>>12);
			col_data[K]+=ValueType((6497*int(col_data[J+1]+col_data[J]))>>12);
		}//J
		col_data[0]+=ValueType((217*int(col_data[yl2]+col_data[yl2]))>>12);	
		col_data[yl2]+=ValueType((6497*int(col_data[1]+col_data[0]))>>12);

		tmp_data[I]=col_data;	

	}

 	//next do horizontal//	
 	//////////////////////

	xend=xl2;	
	yend=yl;

	for (J=0; J<yend;++J){

		for (I=0,K=xl2;I<xl2;++I,++K){
// 			line_data[I]=tmp_data[I][J];
// 			line_data[K]=tmp_data[K][J];
//			TBC: scale factors can be removed and incorporated into noise
//			weighting factors for quantisation
			line_data[I]=ValueType((3563*int(tmp_data[I][J]))>>12);
			line_data[K]=ValueType((4709*int(tmp_data[K][J]))>>12);
		}		

		//first lifting stage
		line_data[xend-1]-=ValueType((1817*int(line_data[xl-2]+line_data[xl-1]))>>12);
		line_data[xl-1]-=ValueType((3616*int(line_data[xend-1]+line_data[xend-1]))>>12);						
		for (I=xend-2,K=xl-2; I>=1;--I,--K){//main body
			line_data[I]-=ValueType((1817*int(line_data[K-1]+line_data[K]))>>12);
			line_data[K]-=ValueType((3616*int(line_data[I+1]+line_data[I]))>>12);
		}
		line_data[0]-=ValueType((1817*int(line_data[xl2]+line_data[xl2]))>>12);
		line_data[xl2]-=ValueType((3616*int(line_data[1]+line_data[0]))>>12);

   		//second lifting stage
		line_data[xend-1]+=ValueType((217*int(line_data[xl-2]+line_data[xl-1]))>>12);	
		line_data[xl-1]+=ValueType((6497*int(line_data[xend-1]+line_data[xend-1]))>>12);
		for (I=xend-2,K=xl-2; I>=1;--I,--K){//main body
			line_data[I]+=ValueType((217*int(line_data[K-1]+line_data[K]))>>12);
			line_data[K]+=ValueType((6497*int(line_data[I+1]+line_data[I]))>>12);
		}//I
		line_data[0]+=ValueType((217*int(line_data[xl2]+line_data[xl2]))>>12);
		line_data[xl2]+=ValueType((6497*int(line_data[1]+line_data[0]))>>12);	

		if (J<yl2){
			L=2*J;
			for (I=0,K=xl2,M=0;I<xl2;++I,++K,M=M+2){
				pic_data[L][M]=line_data[I];
				pic_data[L][M+1]=line_data[K];
			}
		}
		else{
			L=2*J-yl+1;
			for (I=0,K=xl2,M=0;I<xl2;++I,++K,M=M+2){
				pic_data[L+yp][M+xp]=line_data[I];
				pic_data[L+yp][M+1+xp]=line_data[K];
			}

		}
	}
	delete[] line_data;
	for (int I=0;I<xl;++I)
		delete[] tmp_data[I];

}

//perceptual weighting stuff
////////////////////////////

float WaveletTransform::Twodto1d (float f,float g){
	return (sqrt(2.0*(f*f+g*g)) -0.736*std::abs(f-g));
}

float WaveletTransform::Threshold(float xf,float yf,CompSort cs){
	float freq,a,k,f0,g0;
	if(cs==Y){
		a=0.495;
		k=0.466;
		f0=0.401;
		g0=1.501;
	}
	else if(cs==U){
		a=2.032;
		k=0.437;
		f0=0.239;
		g0=1.613;
	}
	else{
		a=0.873;
		k=0.53;
		f0=0.366;
		g0=1.942;
	} 

	freq=Twodto1d(xf,yf);

	return pow((double)10.0,(double)(log10(a)+k*(log10(freq)-log10((g0*f0))*(log10(freq)-log10(g0*f0)))));
}

void WaveletTransform::SetBandWeights (const EncoderParams& encparams,const FrameParams& fparams,const CompSort csort){
	int xlen,ylen,xl,yl,xp,yp;
	float xfreq,yfreq;
	const FrameSort& fsort=fparams.fsort;	

	xlen=2*band_list(1).Xl();
	ylen=2*band_list(1).Yl();

	if (encparams.CPD!=0.0){
		for(int I=1;I<=band_list.Length();I++){
			xp=band_list(I).Xp();
			yp=band_list(I).Yp();
			xl=band_list(I).Xl();
			yl=band_list(I).Yl();

			if(fsort==I_frame){ 
				xfreq=encparams.CPD*(float(xp)+float(xl)/2.0)/float(xlen);
				yfreq=encparams.CPD*(float(yp)+float(yl)/2.0)/float(ylen);
			}
			else{
				xfreq=(encparams.CPD*(float(xp)+float(xl)/2.0)/float(xlen))/4.0;
				yfreq=(encparams.CPD*(float(yp)+float(yl)/2.0)/float(ylen))/4.0;
			}
			if(encparams.interlace){ 
				yfreq/=2.0;
			}

			if( csort!=Y){
				if( encparams.cformat==format422){
					xfreq/=2.0;
				}
				else if(encparams.cformat==format411 ){
					xfreq/=4.0;
				}
				else if(encparams.cformat==format420 ){
					xfreq/=2.0;
					yfreq/=2.0;
				}
			}
			band_list(I).SetWt(2.0*Threshold(xfreq,yfreq,csort));

		}
		//make sure dc is always the lowest weight
		float min_weight=band_list(band_list.Length()).Wt();
		for(int I=1;I<=band_list.Length()-1;I++ ){
			min_weight=((min_weight>band_list(I).Wt()) ? band_list(I).Wt() : min_weight);
		}

		band_list(band_list.Length()).SetWt(min_weight);
 		//normalize weights wrt dc subband
		for(int I=1;I<=band_list.Length();I++ ){
			band_list(I).SetWt(band_list(I).Wt()/band_list(band_list.Length()).Wt());		
		}
	}
	else{//CPD=0 so set all weights to 1
		for(int I=1;I<=band_list.Length();I++ ){
			band_list(I).SetWt(1.0);		
		}	
	}
}	
