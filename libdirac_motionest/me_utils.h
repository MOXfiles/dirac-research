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

#ifndef _ME_UTILS_H_
#define _ME_UTILS_H_

#include <algorithm>
#include "libdirac_common/motion.h"
#include "libdirac_common/common.h"

///////////////////////////////////
//Utilities for motion estimation//
//-------------------------------//
///////////////////////////////////

class BMParams{//block matching params	
public:
	BMParams():xp(0),yp(0),xl(0),yl(0),bp(),me_lambda(0.0),mv_pred(),best_mv(),up_conv(false),
	pic_data(0),ref_data(0)
	{}//default constructor

	void Init(OLBParams& bparams,int M,int N); 	//set up parameters for an overlapped block with given parameters
											  	//nominally located at (M*XBSEP-XOFFSET,N*YBSEP-YOFFSET)
												//overriding current block params bp
	void Init(int M,int N); //ditto, but using current block params bp

	int xp,yp,xl,yl;
	OLBParams bp;//block params
	float me_lambda;//lagrangian parameter used
	MVector mv_pred;//predictor computed from neighbouring blocks	
	MVector best_mv;
	std::vector<std::vector<MVector> >* vect_list;
	bool up_conv;//indicates whether the reference has been upconverted
	PicArray* pic_data;
	PicArray* ref_data;
};

class BlockDiffParams{//parameters for doing block differences	
public:
	BlockDiffParams(BMParams& bmparams): xp(bmparams.xp),yp(bmparams.yp),xl(bmparams.xl),yl(bmparams.yl),
	bailout(false),start_val(0.0f){}
	BlockDiffParams(){}

	int xp,yp,xl,yl;		//coords and dimensions of current block
	MVector best_mv;		//best motion vector so far
	bool bailout;			//allow bail-out
	ValueType dc;
	MvCostData cost;
	float intra_cost;
	float start_val;		//value, normally representing motion vector cost, which is to be added to the raw block difference
};

//////////////////////////////////////////////////
//----Different difference classes, so that-----//
//bounds-checking need only be done as necessary//
//////////////////////////////////////////////////
class BlockDiff{
public:
	BlockDiff(){}
	BlockDiff(PicArray& ref,PicArray& pic):pic_data(&pic),ref_data(&ref){}
	virtual ~BlockDiff(){}
	virtual void Diff(BlockDiffParams& dparams,MVector& mv)=0;//must be overidden
protected:
	PicArray* pic_data;
	PicArray* ref_data;
};

class SimpleBlockDiff: public BlockDiff{//block difference with 1 reference and no bounds checking
public:
	SimpleBlockDiff(){}
	SimpleBlockDiff(PicArray& ref,PicArray& pic):BlockDiff(ref,pic){}
	void Diff(BlockDiffParams& dparams, MVector& mv);

};

class BChkBlockDiff: public BlockDiff{//block difference with 1 reference and bounds checking
public:
	BChkBlockDiff(){}
	BChkBlockDiff(PicArray& ref,PicArray& pic):BlockDiff(ref,pic){}	
	void Diff(BlockDiffParams& dparams, MVector& mv);

};

class IntraBlockDiff: public BlockDiff{//block difference with pred by dc value
public:
	IntraBlockDiff(){}
	IntraBlockDiff(PicArray& pic): BlockDiff(pic,pic){}
	void Diff(BlockDiffParams& dparams, MVector& mv){}
	void Diff(BlockDiffParams& dparams,ValueType dc_pred,float loc_lambda);
};

class BiSimpleBlockDiff: public BlockDiff{//block difference with 2 references and no bounds checking
public:
	BiSimpleBlockDiff(){}
	BiSimpleBlockDiff(PicArray& ref,PicArray& ref2,PicArray& pic):BlockDiff(ref,pic),ref_data2(&ref2){}
	void Diff(BlockDiffParams& dparams, MVector& mv){}
	void Diff(BlockDiffParams& dparams, MVector& mv1,MVector& mv2);
private:
	PicArray* ref_data2;
};

class BiBChkBlockDiff: public BlockDiff{//block difference with 2 references with bounds checking
public:
	BiBChkBlockDiff(){}
	BiBChkBlockDiff(PicArray& ref,PicArray& ref2,PicArray& pic):BlockDiff(ref,pic),ref_data2(&ref2){}
	void Diff(BlockDiffParams& dparams, MVector& mv){}
	void Diff(BlockDiffParams& dparams, MVector& mv1,MVector& mv2);
private:
	PicArray* ref_data2;
};

//classes where the reference is upconverted

class BlockDiffUp: public BlockDiff{//overall class for upconversion
public:	
	BlockDiffUp(){}
	BlockDiffUp(PicArray& ref,PicArray& pic): BlockDiff(ref,pic){Init();}
	virtual ~BlockDiffUp(){}

protected:
	int InterpLookup[9][4];//A lookup table to simplify the 1/8 pixel accuracy code
	void Init();
};

class SimpleBlockDiffUp: public BlockDiffUp{//no bounds checking
	//does block difference without bounds checking, assuming an upconverted reference and 1/8 pel vectors
public:
	SimpleBlockDiffUp(){}
	~SimpleBlockDiffUp(){}
	SimpleBlockDiffUp(PicArray& ref,PicArray& pic):BlockDiffUp(ref,pic){}
	void Diff(BlockDiffParams& dparams, MVector& mv);
};


class BChkBlockDiffUp: public BlockDiffUp{//bounds checking
 	//does block difference with bounds checking
public:
	BChkBlockDiffUp() {}
	~BChkBlockDiffUp(){}
	BChkBlockDiffUp(PicArray& ref,PicArray& pic):BlockDiffUp(ref,pic){}
	void Diff(BlockDiffParams& dparams, MVector& mv);
};

class BiSimpleBlockDiffUp: public BlockDiffUp{//no bounds checking
public:
	BiSimpleBlockDiffUp(){}
	BiSimpleBlockDiffUp(PicArray& ref,PicArray& ref2,PicArray& pic):BlockDiffUp(ref,pic),ref_data2(&ref2){}
	void Diff(BlockDiffParams& dparams, MVector& mv){}
	void Diff(BlockDiffParams& dparams, MVector& mv1,MVector& mv2);
private:
	PicArray* ref_data2;
};

class BiBChkBlockDiffUp: public BlockDiffUp{//no bounds checking
public:
	BiBChkBlockDiffUp(){}
	BiBChkBlockDiffUp(PicArray& ref,PicArray& ref2,PicArray& pic):BlockDiffUp(ref,pic),ref_data2(&ref2){}
	void Diff(BlockDiffParams& dparams, MVector& mv){}
	void Diff(BlockDiffParams& dparams, MVector& mv1,MVector& mv2);
private:
	PicArray* ref_data2;
};
#endif
