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

#ifndef _WAVELET_UTILS_H_
#define _WAVELET_UTILS_H_

#include "arrays.h"
#include "common.h"
#include <vector>
#include <cmath>
#include <iostream>

//utilities for subband and wavelet transforms
//Includes fast transform using lifting

class PicArray;

class Subband{
public:
	//type for a wavelet (or LOT or bestbasis or ...) subband
	//constructors
	Subband(){}
	Subband(int xpos,int ypos, int xlen, int ylen): xps(xpos), yps(ypos), xln(xlen), yln(ylen), wgt(1), qfac(8){}
	Subband(int xpos,int ypos, int xlen, int ylen, int d): xps(xpos), yps(ypos), xln(xlen), yln(ylen), wgt(1),
	dpth(d),qfac(8){}

	//gets ...
	int xl() const {return xln;}	
	int xp() const {return xps;}
	int yl() const {return yln;}
	int yp() const {return yps;}
	int max() const {return max_bit;}
	double wt() const {return wgt;}
	int depth() const {return dpth;}
	int scale() const {return (1<<dpth);}
	int qf(int n) {return qfac[n];}
	int parent() const {return prt;}
	std::vector<int> children() const {return childvec;}
	int child(int n) const {return childvec[n];}

	// ... and sets
	void set_qf(int n, int q){if (n>=qfac.lbound(0) && n<=qfac.ubound(0)) qfac[n]=q;}
	//void set_costs(int n, CostType c){if (n>=cstarray.lbound(0) && n<=cstarray.ubound(0)) cstarray[n]=c;}
	void set_wt(float w){wgt=w;}
	void set_parent(int p){prt=p;}
	void set_depth(int d){dpth=d;}
	void set_max(int m){max_bit=m;};
	void set_children(std::vector<int>& clist){childvec=clist;}
	void add_child(int c){childvec.push_back(c);}

private:
	int xps,yps,xln,yln;		//subband bounds
	double wgt;					//perceptual weight for quantisation
	int dpth;					//depth in the transform
	OneDArray<int> qfac;		//quantisers
	int prt;					//position of parent in a subband list
	std::vector<int> childvec;	//positions of children in the subband list
	int max_bit;				//position of the MSB of the largest absolute value
};

class SubbandList {
public:
	void init(const int depth,const int xlen,const int ylen);
		//initialise for wavelet transforming an array
		//of given dimension to given depth
	int length() const {return bands.size();}
	Subband& operator()(int n){return bands[n-1];}
	Subband operator()(int n) const {return bands[n-1];}	
	void add_band(Subband& b){bands.push_back(b);}
	void clear(){bands.clear();}
private:	
	std::vector<Subband> bands;
};


class WaveletTransformParams{
	//class for encapsulating the data that is needed to configure the wavelet transform	
public:
	WaveletTransformParams():depth(4),filt_sort(DAUB){}	
	WaveletTransformParams(int d):depth(d),filt_sort(DAUB){}		
	int depth;
	WltFilter filt_sort;	
};

class WaveletTransform {
public:
	//constructor
	WaveletTransform(WaveletTransformParams p): params(p){}

	//destructor
	virtual ~WaveletTransform(){}
	//functions
	void Transform(Direction d, PicArray& pic_data);
	SubbandList& BandList(){return band_list;}

	void SetBandWeights (EncoderParams& encparams,FrameParams& fparams,CompSort csort);

private:
	//other private variables	
	WaveletTransformParams params;
	SubbandList band_list;

	//functions
	void vhsplit(int xp, int yp, int xl, int yl, PicArray&pic_data);
	void vhsynth(int xp, int yp, int xl, int yl, PicArray& pic_data);

	float Twodto1d (float f,float g);//used for perceptual weighting
	float Threshold(float xf,float yf,CompSort cs);//ditto

};

void  set_band_weights (CodecParams& cparams,FrameParams& fparams,SubbandList& bands,CompSort csort);

#endif
