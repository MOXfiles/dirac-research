/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$ $Name$
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

#include "libdirac_common/common.h"
#include <algorithm>
#ifndef _MOTION_H
#define _MOTION_H

////////////////////////////////////////////////////////////////
//classes and functions for motion estimation and compensation//
////////////////////////////////////////////////////////////////

//classes

//! Motion vector class - just a pair
template <class T>
class MotionVector{
public:

    //! Constructor 
	MotionVector<T>(T a, T b) : x(a), y(b) {};
	//! Default construct - sets components to 0
	MotionVector<T>() : x(0), y(0) {};
	//! Constructor 
	MotionVector<T>(T a) : x(a), y(a) {};

	//! Addition 
	inline MotionVector<T> operator+(MotionVector<T>& argument){
		MotionVector<T> temp;
		temp.x=x+argument.x;
		temp.y=y+argument.y;
		return temp;}
	//! Subtraction 
	inline MotionVector<T> operator-(MotionVector<T>& argument){
		MotionVector<T> temp;
		temp.x=x-argument.x;
		temp.y=y-argument.y;
		return temp;}
	//! Scalar multiplication
	inline MotionVector<T> operator*(float& argument){
		MotionVector<T> temp;
		temp.x=x*argument;
		temp.y=y*argument;
		return temp;}
	//! Scalar multiplication
	inline MotionVector<T> operator*(int& argument){
		MotionVector<T> temp;
		temp.x=x*argument;
		temp.y=y*argument;
		return temp;}
	//! x and y components 
	T x,y;
};

//! MVector class is a vector of ints 
typedef MotionVector<int> MVector;

//! ImageCoords class is a vector of ints 
typedef MotionVector<int> ImageCoords;

//! MvArray is a two-D array of MVectors
typedef TwoDArray<MVector> MvArray;

//! Class for recording costs derived in motion estimation
class MvCostData{
public:
	//! Constructor
	MvCostData():
	SAD(0.0),
	mvcost(0.0),
	total(0.0){}

	//! The Sum of Absolute Differences - easier to compute than Sum-Squared Differences
	float SAD;

    //! The (Lagrangian-weighted) motion vector cost - the difference of a motion vector from its neighbouring vectors
	float mvcost;

    //! Total=SAD+mvcost
	float total;
};

//! Data relating to a macroblock
class MBData{
public:

    //! Depth to which the MB is split 
	unsigned int split_mode;

    //! True is there is a single reference mode for the whole MB, false otherwise
	bool common_ref;
};

//! Class for all the motion vector data
/*!
    Motion vector data: the motion vectors themselves, the costs, DC values for blocks,
	the prediction modes chosen for each block and the MB data.
*/
class MvData{
	//class to encapsulate data used in motion estimation and compensation
public:
    //! Constructor
    /*!
		Constructor - data arrays are sized according to the number of blocks/MBs.        
     */
	MvData(int xnumMB, int ynumMB, int xnumBlock, int ynumBlock): 
	mb(xnumMB,ynumMB), 
	mv1(xnumBlock,ynumBlock),
	mv2(xnumBlock,ynumBlock),
	mode(xnumBlock,ynumBlock),
	dcY(xnumBlock,ynumBlock),
	dcU(xnumBlock,ynumBlock),
	dcV(xnumBlock,ynumBlock),
	MB_costs(xnumMB,ynumMB),
	block_costs1(xnumBlock,ynumBlock),
	block_costs2(xnumBlock,ynumBlock),
	block_intra_costs(xnumBlock,ynumBlock),
	block_bipred_costs(xnumBlock,ynumBlock){}

	//! Return the array of DC values for blocks for each component
	TwoDArray<ValueType>& dc(CompSort cs)
    {
		if (cs == U_COMP) return dcU;
		else if (cs == V_COMP) return dcV;
		else return dcY;}

	//! Return the array of DC values for blocks for each component
	const TwoDArray<ValueType>& dc(CompSort cs) const {
		if (cs == U_COMP) return dcU;
		else if (cs == V_COMP) return dcV;
		else return dcY;}

	    //! The MB data array
	TwoDArray<MBData> mb;

    //! The motion vectors for reference 1.
	MvArray mv1;

    //! The motion vectors for reference 2.
	MvArray mv2;

    //! The block prediction modes
	TwoDArray<PredMode> mode;

    //! The Y DC values
	TwoDArray<ValueType> dcY;

    //! The U DC values
	TwoDArray<ValueType> dcU;

    //! The V DC values
	TwoDArray<ValueType> dcV;

    //! The total cost for each MB
	TwoDArray<float> MB_costs;	

    //! The cost structures for each block for reference 1 
	TwoDArray<MvCostData> block_costs1;

    //! The cost structures for each block for reference 2
	TwoDArray<MvCostData> block_costs2;	

    //! The costs of coding each block Intra
	TwoDArray<float> block_intra_costs;

    //! The costs of coding each block with bi-directional prediction
	TwoDArray<MvCostData> block_bipred_costs;

};

//motion compensation stuff//
/////////////////////////////


//First have arithmetic classes to avoid code duplication

//! Abstract class to do arithmetic with weighted values, a weight being between 0 and 1023
class ArithObj{	
public:
	virtual ~ArithObj(){}
	virtual void DoArith(ValueType &lhs, const CalcValueType rhs, const CalcValueType &Weight) const = 0;

protected:
};

//! Class to do weighted sums of values
class ArithAddObj : public ArithObj{	
public:
	void DoArith(ValueType &lhs, const CalcValueType rhs, const CalcValueType &Weight) const;
};

//! Class to do weighted difference of values
class ArithSubtractObj : public ArithObj{	
public:
	void DoArith(ValueType &lhs, const CalcValueType rhs, const CalcValueType &Weight) const;
};

//! Class to add half a weighted value
class ArithHalfAddObj : public ArithObj{	
public:
	void DoArith(ValueType &lhs, const CalcValueType rhs, const CalcValueType &Weight) const;
};

//! Class to subtract half a weighted value
class ArithHalfSubtractObj : public ArithObj{	
public:
	void DoArith(ValueType &lhs, const CalcValueType rhs, const CalcValueType &Weight) const;
};


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
void CreateBlock(const OLBParams &bparams, bool FullX, bool FullY, TwoDArray<CalcValueType>& WeightArray);

//Flips the values in an array in the x direction
void FlipX(const TwoDArray<CalcValueType>& Original, const OLBParams &bparams, TwoDArray<CalcValueType>& Flipped);

//Flips the values in an array in the y direction.
void FlipY(const TwoDArray<CalcValueType>& Original, const OLBParams &bparams, TwoDArray<CalcValueType>& Flipped);

//motion estimation and coding stuff

//! Return the median of three motion vectors 
inline MVector MvMedian(const MVector& mv1,const MVector& mv2,const MVector& mv3) {
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

//! Return the median of a set of motion vectors 
inline MVector MvMedian(const std::vector<MVector>& vect_list){
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

//! Return the mean of two motion vectors
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

//! Return the squared length of a motion vector
inline int Norm2(MVector& mv){//L^2 norm of a motion vector
	return mv.x*mv.x+mv.y*mv.y;
}

//! Return the sum of the lengths of a motion vector's componets
inline int Norm1(MVector& mv){//L^1 norm of a motion vector
	return abs(mv.x)+abs(mv.y);
}

//! Return the mean of a set of integer values
inline int GetMean(std::vector<int>& values){
	int sum=0;
	for (unsigned int I=0;I<values.size();++I)
		sum+=values[I];
	sum/=int(values.size());
	return sum;
}

//! Return the mean of a set of unsigned integer values
inline unsigned int GetMean(std::vector<unsigned int>& values){
	int sum=0;
	for (unsigned int I=0;I<values.size();++I)
		sum+=values[I];
	sum+=(values.size()>>1);
	sum/=values.size();
	return sum;
}

#endif
