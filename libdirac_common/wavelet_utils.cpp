/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$ $Name$
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License");  you may not use this file except in compliance
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
* Contributor(s): Thomas Davies (Original Author), Scott R Ladd
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


#include "libdirac_common/wavelet_utils.h"
#include "libdirac_common/common.h"
#include <cstdlib>

// Default constructor
Subband::Subband()
{
    // this space intentionally left blank
}

// Constructor
Subband::Subband(int xpos,int ypos, int xlen, int ylen)
  : xps(xpos),
    yps(ypos),
    xln(xlen),
    yln(ylen),
    wgt(1),
    qfac(8)
{
    // this space intentionally left blank
}

// Constructor
Subband::Subband(int xpos,int ypos, int xlen, int ylen, int d)
  : xps(xpos),
    yps(ypos),
    xln(xlen), 
    yln(ylen),
    wgt(1),
    dpth(d),
    qfac(8)
{
    // this space intentionally left blank
}

//! Destructor
Subband::~Subband()
{
    // this space intentionally left blank
}

//subband list methods

void SubbandList::Init(const int depth,const int xlen,const int ylen)
{
	int xl=xlen; 
	int yl=ylen; 
    
	Clear(); 
	Subband* tmp;
     
	for (int l = 1; l <= depth; ++l)
    {
		xl/=2; 
		yl/=2; 
        
		tmp=new Subband(xl,0,xl,yl,l); 
		AddBand(*tmp); 
		delete tmp; 
        
		tmp=new Subband(0,yl,xl,yl,l); 
		AddBand(*tmp); 
		delete tmp; 
        
		tmp=new Subband(xl,yl,xl,yl,l); 
		AddBand(*tmp); 
		delete tmp; 
        
		if (l == depth)
        {
			tmp=new Subband(0,0,xl,yl,l); 
			AddBand(*tmp); 
			delete tmp; 
		}		
	}
	//now set the parent-child relationships
	int len = bands.size(); 
	(*this)(len).SetParent(0); 		
	(*this)(len).AddChild(len-3); 
	(*this)(len-3).SetParent(len); 
	(*this)(len).AddChild(len-2); 
	(*this)(len-2).SetParent(len); 
	(*this)(len).AddChild(len-1); 
	(*this)(len-1).SetParent(len); 

	for (int l = 1; l < depth; ++l)
    {
 		//do parent-child relationship for other bands
		(*this)(3*l+1).AddChild(3*(l-1)+1); 
		(*this)(3*(l-1)+1).SetParent(3*l+1); 
		(*this)(3*l+2).AddChild(3*(l-1)+2); 
		(*this)(3*(l-1)+2).SetParent(3*l+2); 
		(*this)(3*l+3).AddChild(3*(l-1)+3); 
		(*this)(3*(l-1)+3).SetParent(3*l+3); 
	}
}

//wavelet transform methods
///////////////////////////

//public methods

WaveletTransform::WaveletTransform(int d, WltFilter f)
  : depth(d),
    filt_sort(f)
{
    // this space intentionally left blank
}

//! Destructor
WaveletTransform::~WaveletTransform()
{
    // this space intentionally left blank
}

void WaveletTransform::Transform(const Direction d, PicArray& pic_data)
{
	int xl,yl; 

	if (d == FORWARD)
    {
		//do work
		xl=pic_data.length(0); 
		yl=pic_data.length(1); 
        
		for (int l = 1; l <= depth; ++l)
        {
			VHSplit(0,0,xl,yl,pic_data); 
			xl /= 2; 
			yl /= 2; 
		}

		band_list.Init(depth,pic_data.length(0),pic_data.length(1)); 
	}
	else
    {
		//do work
		xl=pic_data.length(0)/(1<<(depth-1)); 
		yl=pic_data.length(1)/(1<<(depth-1)); 
        
		for (int l = 1; l <= depth; ++l)
        {
			VHSynth(0,0,xl,yl,pic_data); 
			xl *= 2; 
			yl *= 2; 
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

	OneDArray<ValueType *> tmp_data(yl); 
	const int xl2 = xl/2; 
	const int yl2 = yl/2; 
	register ValueType * line_data; 
	register ValueType * col_data=new ValueType[yl]; 	

	int i,j,k,l; 

 	//first do horizontal 

	int xend=xl2; 	
	int yend=yl; 


	for (j = 0;  j < yend; ++j)
    {
		line_data=new ValueType[xl]; 

		for (i = 0, k = xl2, l = 0;  i < xend; ++i, ++k, ++l)
        {
			line_data[i]=pic_data[j+yp][l+xp]; 
			line_data[k]=pic_data[j+yp][++l+xp]; 			
		}//i

    	//first lifting stage
		line_data[xl2] -= ValueType((6497*int(line_data[1]+line_data[0]))>>12); 
		line_data[0]   -= ValueType((217*int(line_data[xl2]+line_data[xl2]))>>12); 
        
        //main body
		for (i = 1, k = xl2+1; i < xend-1; ++i, ++k)
        {
			line_data[k] -= ValueType((6497*int(line_data[i+1]+line_data[i]))>>12); 
			line_data[i] -= ValueType((217*int(line_data[k-1]+line_data[k]))>>12); 
		}//i
        
		line_data[xl-1] -= ValueType((6497*int(line_data[xend-1]+line_data[xend-1]))>>12); 
		line_data[xend-1] -= ValueType((217*int(line_data[xl-2]+line_data[xl-1]))>>12); 

     	//second lifting stage
		line_data[xl2] += ValueType((3616*int(line_data[1]+line_data[0]))>>12); 
		line_data[0] += ValueType((1817*int(line_data[xl2]+line_data[xl2]))>>12); 
        
        //main body
		for (i = 1, k = xl2+1;  i < xend-1; ++i,++k)
        {
			line_data[k] += ValueType((3616*int(line_data[i+1]+line_data[i]))>>12); 
			line_data[i] += ValueType((1817*int(line_data[k-1]+line_data[k]))>>12); 
		}
        
		line_data[xl-1] += ValueType((3616*int(line_data[xend-1]+line_data[xend-1]))>>12); 
		line_data[xend-1] += ValueType((1817*int(line_data[xl-2]+line_data[xl-1]))>>12); 

		tmp_data[j] = line_data; 
	}

 	//next do vertical
	xend = xl; 	
	yend = yl2; 

	for (i = 0; i < xend; ++i)
    {

		for (j = 0, k = yl2, l=0;  j < yend; ++j, ++k, ++l)
        {
			col_data[j] = tmp_data[l][i]; 
			col_data[k] = tmp_data[++l][i]; 			
		}//j

   		//first lifting stage
		col_data[yl2] -= ValueType((6497*int(col_data[1]+col_data[0]))>>12); 
		col_data[0] -= ValueType((217*int(col_data[yl2]+col_data[yl2]))>>12); 
        
        //main body
		for (j = 1, k = yl2+1;  j < yend-1; ++j, ++k)
        {
			col_data[k] -= ValueType((6497*int(col_data[j+1]+col_data[j]))>>12); 
			col_data[j] -= ValueType((217*int(col_data[k-1]+col_data[k]))>>12); 
		}//j
        
		col_data[yl-1] -= ValueType((6497*int(col_data[yend-1]+col_data[yend-1]))>>12); 
		col_data[yend-1] -= ValueType((217*int(col_data[yl-2]+col_data[yl-1]))>>12); 		

    	//second lifting stage
		col_data[yl2] += ValueType((3616*int(col_data[1]+col_data[0]))>>12); 
		col_data[0] += ValueType((1817*int(col_data[yl2]+col_data[yl2]))>>12); 
        
		for (j = 1,k = yl2+1;  j < yend-1; ++j, ++k)
        {
			col_data[k]+=ValueType((3616*int(col_data[j+1]+col_data[j]))>>12); 	
			col_data[j]+=ValueType((1817*int(col_data[k-1]+col_data[k]))>>12); 
		}//j
        
		col_data[yl-1]+=ValueType((3616*int(col_data[yend-1]+col_data[yend-1]))>>12); 
		col_data[yend-1]+=ValueType((1817*int(col_data[yl-2]+col_data[yl-1]))>>12); 

		for (j=0, k=yl2;  j < yend; ++j, ++k)
        {
			pic_data[j+yp][i+xp]=col_data[j]; 
			pic_data[k+yp][i+xp]=col_data[k]; 
		}//i

	}
    
	delete [] col_data;
    
	for (int j=0; j<yl; ++j)
		delete[] tmp_data[j]; 

}


void WaveletTransform::VHSynth(const int xp, const int yp, const int xl, const int yl, PicArray& pic_data){	

	register ValueType* line_data=new ValueType[xl]; 
	register ValueType* col_data; 	
	OneDArray<ValueType*> tmp_data(xl); 

	const int xl2=xl/2; 
	const int yl2=yl/2; 
	int i,j,k,l,m; //positional variables

	//first do vertical synth//
	///////////////////////////

	int xend=xl; 	
	int yend=yl2; 

	for (i=0;  i<xend; ++i){		
		col_data=new ValueType[yl]; 
		for (j=0,k=yl2; j<yl2; ++j,++k){
			col_data[j]=pic_data[j+yp][i+xp]; 	
			col_data[k]=pic_data[k+yp][i+xp]; 			
		}

		//first lifting stage
		col_data[yend-1] -= ValueType((1817*int(col_data[yl-2]+col_data[yl-1]))>>12); 
		col_data[yl-1] -= ValueType((3616*int(col_data[yend-1]+col_data[yend-1]))>>12); 
		for (j=yend-2,k=yl-2;  j>=1; --j,--k){//main body
			col_data[j] -= ValueType((1817*int(col_data[k-1]+col_data[k]))>>12); 
			col_data[k] -= ValueType((3616*int(col_data[j+1]+col_data[j]))>>12); 
		}//j
		col_data[0] -= ValueType((1817*int(col_data[yl2]+col_data[yl2]))>>12); 
		col_data[yl2] -= ValueType((3616*int(col_data[1]+col_data[0]))>>12); 

  		//second lifting stage
		col_data[yend-1]+=ValueType((217*int(col_data[yl-2]+col_data[yl-1]))>>12); 
		col_data[yl-1]+=ValueType((6497*int(col_data[yend-1]+col_data[yend-1]))>>12); 			
		for (j=yend-2,k=yl-2;  j>=1; --j,--k){//main body
			col_data[j]+=ValueType((217*int(col_data[k-1]+col_data[k]))>>12); 
			col_data[k]+=ValueType((6497*int(col_data[j+1]+col_data[j]))>>12); 
		}//j
		col_data[0]+=ValueType((217*int(col_data[yl2]+col_data[yl2]))>>12); 	
		col_data[yl2]+=ValueType((6497*int(col_data[1]+col_data[0]))>>12); 

		tmp_data[i]=col_data; 	

	}

 	//next do horizontal//	
 	//////////////////////

	xend=xl2; 	
	yend=yl; 

	for (j=0;  j<yend; ++j){

		for (i=0,k=xl2; i<xl2; ++i,++k){
			line_data[i]=tmp_data[i][j]; 
			line_data[k]=tmp_data[k][j]; 
		}		

		//first lifting stage
		line_data[xend-1] -= ValueType((1817*int(line_data[xl-2]+line_data[xl-1]))>>12); 
		line_data[xl-1] -= ValueType((3616*int(line_data[xend-1]+line_data[xend-1]))>>12); 						
		for (i=xend-2,k=xl-2;  i>=1; --i,--k){//main body
			line_data[i] -= ValueType((1817*int(line_data[k-1]+line_data[k]))>>12); 
			line_data[k] -= ValueType((3616*int(line_data[i+1]+line_data[i]))>>12); 
		}
		line_data[0] -= ValueType((1817*int(line_data[xl2]+line_data[xl2]))>>12); 
		line_data[xl2] -= ValueType((3616*int(line_data[1]+line_data[0]))>>12); 

   		//second lifting stage
		line_data[xend-1] += ValueType((217*int(line_data[xl-2]+line_data[xl-1]))>>12); 	
		line_data[xl-1] += ValueType((6497*int(line_data[xend-1]+line_data[xend-1]))>>12); 
		for (i=xend-2,k=xl-2;  i>=1; --i,--k){//main body
			line_data[i] += ValueType((217*int(line_data[k-1]+line_data[k]))>>12); 
			line_data[k] += ValueType((6497*int(line_data[i+1]+line_data[i]))>>12); 
		}//i
		line_data[0] += ValueType((217*int(line_data[xl2]+line_data[xl2]))>>12); 
		line_data[xl2] += ValueType((6497*int(line_data[1]+line_data[0]))>>12); 	

		if (j<yl2){
			l=2*j; 
			for (i=0,k=xl2,m=0; i<xl2; ++i,++k,m=m+2){
				pic_data[l][m] = line_data[i]; 
				pic_data[l][m+1] = line_data[k]; 
			}
		}
		else{
			l=2*j-yl+1; 
			for (i=0,k=xl2,m=0; i<xl2; ++i,++k, m=m+2){
				pic_data[l+yp][m+xp]=line_data[i]; 
				pic_data[l+yp][m+1+xp]=line_data[k]; 
			}

		}
	}
	delete[] line_data; 
	for (int i=0; i<xl; ++i)
		delete[] tmp_data[i]; 

}

//perceptual weighting stuff
////////////////////////////

inline float WaveletTransform::Twodto1d (float f,float g)
{
	return (sqrt(2.0*(f*f+g*g)) -0.736*std::abs(f-g)); 
}

float WaveletTransform::Threshold(float xf,float yf,CompSort cs)
{
	float freq,a,k,f0,g0; 
    
	if(cs == Y)
    {
		a=0.495; 
		k=0.466; 
		f0=0.401; 
		g0=1.501; 
	}
	else if(cs == U)
    {
		a=2.032; 
		k=0.437; 
		f0=0.239; 
		g0=1.613; 
	}
	else
    {
		a=0.873; 
		k=0.53; 
		f0=0.366; 
		g0=1.942; 
	} 

	freq=Twodto1d(xf,yf); 

	return pow((double)10.0,(double)(log10(a)+k*(log10(freq)-log10((g0*f0))*(log10(freq)-log10(g0*f0))))); 
}

void WaveletTransform::SetBandWeights (const float cpd, const FrameParams& fparams, const CompSort csort)
{
	int xlen,ylen,xl,yl,xp,yp,depth; 
	float xfreq,yfreq; 
	const FrameSort& fsort = fparams.FSort(); 
    const ChromaFormat& cformat = fparams.CFormat(); 
	float temp; 

	//factor used to compensate for the absence of scaling in the wavelet transform	
	const double alpha(1.149658203); 

	xlen=2*band_list(1).Xl(); 
	ylen=2*band_list(1).Yl(); 

	if (cpd != 0.0){
		for(int i = 1; i <= band_list.Length(); i++){
			xp=band_list(i).Xp(); 
			yp=band_list(i).Yp(); 
			xl=band_list(i).Xl(); 
			yl=band_list(i).Yl(); 

			if(fsort == I_frame){ 
				xfreq=cpd * (float(xp)+float(xl)/2.0)/float(xlen); 
				yfreq=cpd * (float(yp)+float(yl)/2.0)/float(ylen); 
			}
			else{
				xfreq=(cpd * (float(xp)+float(xl)/2.0)/float(xlen))/4.0; 
				yfreq=(cpd * (float(yp)+float(yl)/2.0)/float(ylen))/4.0; 
			}

			if( csort!=Y){
				if( cformat == format422){
					xfreq/=2.0; 
				}
				else if( cformat == format411 ){
					xfreq/=4.0; 
				}
				else if( cformat == format420 ){
					xfreq/=2.0; 
					yfreq/=2.0; 
				}
			}
			temp=2.0*Threshold(xfreq,yfreq,csort); 
			band_list(i).SetWt(temp); 

		}
		//make sure dc is always the lowest weight
		float min_weight=band_list(band_list.Length()).Wt(); 
		for(int i = 1; i <= band_list.Length()-1; i++ ){
			min_weight=((min_weight>band_list(i).Wt()) ? band_list(i).Wt() : min_weight); 
		}		
		band_list(band_list.Length()).SetWt(min_weight); 

		//normalize weights wrt dc subband
		for(int i = 1; i <= band_list.Length(); i++ ){
			band_list(i).SetWt(band_list(i).Wt()/band_list(band_list.Length()).Wt()); 		
		}
	}
	else{//CPD=0 so set all weights to 1
		for(int i = 1; i <= band_list.Length(); i++ ){
			band_list(i).SetWt(1.0); 		
		}	
	}

	//Finally, compensate for the absence of scaling in the transform
	for (int i = 1; i<band_list.Length(); ++i){

		depth=band_list(i).Depth(); 
		if (band_list(i).Xp() == 0 && band_list(i).Yp() == 0){
			temp=std::pow(alpha,2*depth); 
		} 
		else if (band_list(i).Xp()!=0 && band_list(i).Yp()!=0){
			temp=std::pow(alpha,2*(depth-2)); 
		}
		else {
			temp=std::pow(alpha,2*(depth-1)); 
		}

		band_list(i).SetWt(band_list(i).Wt()/temp); 

	}//i		

}	
