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

#include "common.h"
#include <algorithm>
#ifndef _MOTION_H
#define _MOTION_H

////////////////////////////////////////////////////////////////
//classes and functions for motion estimation and compensation//
////////////////////////////////////////////////////////////////

//classes

template <class T>
class MotionVector{
public:
	T x,y;
	//constructors
	MotionVector<T>(T a, T b) : x(a), y(b) {};
	MotionVector<T>() : x(0), y(0) {};
	MotionVector<T>(T a) : x(a), y(a) {};

	inline MotionVector<T> operator+(MotionVector<T>& argument){
		MotionVector<T> temp;
		temp.x=x+argument.x;
		temp.y=y+argument.y;
		return temp;}
	inline MotionVector<T> operator-(MotionVector<T>& argument){
		MotionVector<T> temp;
		temp.x=x-argument.x;
		temp.y=y-argument.y;
		return temp;}
	inline MotionVector<T> operator*(float& argument){
		MotionVector<T> temp;
		temp.x=x*argument;
		temp.y=y*argument;
		return temp;}
	inline MotionVector<T> operator*(int& argument){
		MotionVector<T> temp;
		temp.x=x*argument;
		temp.y=y*argument;
		return temp;}
};


typedef MotionVector<int> MVector;
typedef MotionVector<int> ImageCoords;//Re-use the motionvector class without confusing the reader

typedef TwoDArray<MVector> MvArray;

class MvCostData{
public:
	MvCostData():SAD(0.0),mvcost(0.0),total(0.0){}
	float SAD;
	float mvcost;
	float total;
};

class MBData{
public:
	unsigned int split_mode;	//depth to which MB is split
	bool common_ref;//true if there is a single reference picture for the whole MB, false otherwise
};

class MvData{
	//class to encapsulate data used in motion estimation and compensation
public:
	//constructor
	MvData(int xnumMB, int ynumMB, int xnumBlock, int ynumBlock): mb(xnumMB,ynumMB), 
	mv1(xnumBlock,ynumBlock),mv2(xnumBlock,ynumBlock),mode(xnumBlock,ynumBlock),
	dcY(xnumBlock,ynumBlock),dcU(xnumBlock,ynumBlock),dcV(xnumBlock,ynumBlock),
	MB_costs(xnumMB,ynumMB),block_costs1(xnumBlock,ynumBlock),block_costs2(xnumBlock,ynumBlock),
	block_intra_costs(xnumBlock,ynumBlock),block_bipred_costs(xnumBlock,ynumBlock){}

	TwoDArray<ValueType>& dc(CompSort cs){
		if (cs==U) return dcU;
		else if (cs==V) return dcV;
		else return dcY;}

	TwoDArray<MBData> mb;
	MvArray mv1;
	MvArray mv2;
	TwoDArray<PredMode> mode;
	TwoDArray<ValueType> dcY;
	TwoDArray<ValueType> dcU;
	TwoDArray<ValueType> dcV;
	TwoDArray<float> MB_costs;	
	TwoDArray<MvCostData> block_costs1;
	TwoDArray<MvCostData> block_costs2;	
	TwoDArray<float> block_intra_costs;
	TwoDArray<MvCostData> block_bipred_costs;	
private:

};

//motion compensation stuff

//Overlapping blocks are acheived by applying a 2D raised cosine shape
//to them. This function facilitates the calculations
float RaisedCosine(float t, float B);

//Calculates a weighting block.
//Params defines the block parameters so the relevant weighting arrays can be created.
//FullX and FullY refer to whether the weight should be adjusted for the edge of an image.
//eg. 1D Weighting shapes in x direction
//  FullX true        FullX false
//     ***           ********
//   *     *                  *
//  *       *                  *
//*           *                  *
void CreateBlock(const OLBParams &bparams, bool FullX, bool FullY, CalcValueType** WeightArray);
void FlipX(CalcValueType** Original, const OLBParams &bparams, CalcValueType** Flipped);//Flips the values in an array in the x direction.
void FlipY(CalcValueType** Original, const OLBParams &bparams, CalcValueType** Flipped);//Flips the values in an array in the y direction.

//motion estimation and coding stuff

inline MVector MvMedian(MVector& mv1,MVector& mv2,MVector& mv3) {
	//takes median of each vector component	
	MVector tmp_mv;

	tmp_mv.x=mv1.x;
	tmp_mv.x+=mv2.x;
	tmp_mv.x+=mv3.x;

	tmp_mv.x-=std::max(std::max(mv1.x,mv2.x),mv3.x);
	tmp_mv.x-=std::min(std::min(mv1.x,mv2.x),mv3.x);

	tmp_mv.y=mv1.y;
	tmp_mv.y+=mv2.y;
	tmp_mv.y+=mv3.y;

	tmp_mv.y-=std::max(std::max(mv1.y,mv2.y),mv3.y);
	tmp_mv.y-=std::min(std::min(mv1.y,mv2.y),mv3.y);

	return tmp_mv;
}

inline MVector MvMedian(std::vector<MVector>& vect_list){
	//more general median. Takes the median of each vector component	

	MVector median;
	int num_vals=int(vect_list.size());
	if (num_vals>0)	{
		int pos=0;
		std::vector<int> ordered_vals(vect_list.size());
		//do x first
		ordered_vals[0]=vect_list[0].x;		
		for (int I=1;I<num_vals;++I){
			for (int K=0;K<I;++K){
				if (vect_list[I].x<ordered_vals[K]){
					pos=K;
					break;
				}
				else
					pos=K+1;
			}//K
			if (pos==I)
				ordered_vals[I]=vect_list[I].x;
			else{
				for (int K=pos;K>=I-1;--K){
					ordered_vals[K+1]=ordered_vals[K];
				}
				ordered_vals[pos]=vect_list[I].x;
			}
		}//I
		if (vect_list.size()%2!=0)
			median.x=ordered_vals[(num_vals-1)/2];
		else
			median.x=(ordered_vals[(num_vals/2)-1]+ordered_vals[num_vals/2])/2;

		//now do y
		ordered_vals[0]=vect_list[0].y;		
		for (int I=1;I<num_vals;++I){
			for (int K=0;K<I;++K){
				if (vect_list[I].y<ordered_vals[K]){
					pos=K;
					break;
				}
				else
					pos=K+1;
			}//K
			if (pos==I)
				ordered_vals[I]=vect_list[I].y;
			else{
				for (int K=pos;K>=I-1;--K){
					ordered_vals[K+1]=ordered_vals[K];
				}
				ordered_vals[pos]=vect_list[I].y;
			}
		}//I
		if (num_vals%2!=0)
			median.y=ordered_vals[(num_vals-1)/2];
		else
			median.y=(ordered_vals[(num_vals/2)-1]+ordered_vals[num_vals/2])/2;		

	}
	else{
		median.x=0;
		median.y=0;
	}
	return median;
}

inline MVector MvMean(MVector& mv1,MVector& mv2) {
	//takes median of each vector component	
	MVector tmp_mv;

	tmp_mv.x=mv1.x;
	tmp_mv.x+=mv2.x;
	tmp_mv.x/=2;

	tmp_mv.y=mv1.y;
	tmp_mv.y+=mv2.y;
	tmp_mv.y/=2;

	return tmp_mv;
}

inline int Norm2(MVector& mv){//L^2 norm of a motion vector
	return mv.x*mv.x+mv.y*mv.y;
}


inline int Norm1(MVector& mv){//L^1 norm of a motion vector
	return abs(mv.x)+abs(mv.y);
}

inline int GetMean(std::vector<int>& values){
	int sum=0;
	for (unsigned int I=0;I<values.size();++I)
		sum+=values[I];
	sum/=int(values.size());
	return sum;
}

inline unsigned int GetMean(std::vector<unsigned int>& values){
	int sum=0;
	for (unsigned int I=0;I<values.size();++I)
		sum+=values[I];
	sum+=(values.size()>>1);
	sum/=values.size();
	return sum;
}

#endif
