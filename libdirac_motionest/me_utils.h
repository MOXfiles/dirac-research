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
* Revision 1.4  2004-05-24 12:38:55  tjdwave
* Replaced spagetti code for linear interpolation in motion compensation
* and motion estimation routines with simple loops. Code is much clearer,
* although possibly slightly slower.
*
* Revision 1.3  2004/05/12 08:35:35  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/04/05 03:05:03  chaoticcoyote
* Updated Doxygen API documentation comments
* Test to see if Scott's CVS is now working correctly
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
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

//! A class for encapsulating the parameters for matching a block
struct BMParams{//block matching params	

	BMParams():
	xp(0),
	yp(0),
	xl(0),
	yl(0),
	bp(),
	me_lambda(0.0),
	mv_pred(),
	best_mv(),
	up_conv(false),
	pic_data(0),
	ref_data(0)
	{}//default constructor

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	//This means that ptrs are copied and deleted, not their objects.///
	////////////////////////////////////////////////////////////////////

	void Init(const OLBParams& bparams,int M,int N); 	//set up parameters for an overlapped block with given parameters
											  	//nominally located at (M*XBSEP-XOFFSET,N*YBSEP-YOFFSET)
												//overriding current block params bp
	void Init(int M,int N); //ditto, but using current block params bp

	//! The xcoord of the top-left coord of the block being matched
	int xp;
	//! The ycoord of the top-left coord of the block being matched
	int yp;
	//! The width of the block being matched
	int xl;
	//! The height of the block being matched
	int yl;

	//! The block parameter set being used for matching 
	OLBParams bp;
	//! The Lagrangian weighting parameter to be applied to the motion vector cost
	float me_lambda;
	//! The predictor for the motion vector for this block
	MVector mv_pred;
	//! The best motion vector found so far	
	MVector best_mv;
	//! The set of candidate vectors to choose from
	std::vector<std::vector<MVector> >* vect_list;
	//! Indicates whether the reference has been upconverted
	bool up_conv;
	//! A pointer to the picture data
	const PicArray* pic_data;
	//! A pointer to the reference picture data
	const PicArray* ref_data;
};


//! A class encapsulating parameters for calculating a block difference value (a single instance of matching)
struct BlockDiffParams{

	//! Constructor
	BlockDiffParams(){}

	//! Constructor
	BlockDiffParams(const BMParams& bmparams): 
	xp(bmparams.xp),
	yp(bmparams.yp),
	xl(bmparams.xl),
	yl(bmparams.yl),
	bailout(false),
	start_val(0.0f){}

	////////////////////////////////////////////////////////////////////
	//NB: Assume default copy constructor, assignment = and destructor//
	//This means that ptrs are copied and deleted, not their objects.///
	////////////////////////////////////////////////////////////////////

	//! The xcoord of the top-left coord of the block being matched
	int xp;
	//! The ycoord of the top-left coord of the block being matched
	int yp;
	//! The width of the block being matched
	int xl;
	//! The height of the block being matched
	int yl;
	//! The best motion vector so far
	MVector best_mv;		
	//! Whether we're allowed to bail out of the calculation early if we're not close to the best
	bool bailout;
	//! The average value of the current block
	ValueType dc;
	//! The cost data associated with the match
	MvCostData cost;
	//! The intra cost for the block
	float intra_cost;
	//! The value to start adding the SAD values to - will be the weighted motion vector cost
	float start_val;
};

//////////////////////////////////////////////////
//----Different difference classes, so that-----//
//bounds-checking need only be done as necessary//
//////////////////////////////////////////////////

//! An abstract class for doing block difference calculations
class BlockDiff{
public:
	//! Constructor
	BlockDiff(){}
	//! Constructor, initialising the reference and picture data
	BlockDiff(const PicArray& ref,const PicArray& pic):pic_data(&pic),ref_data(&ref){}
	//! Destructor	
	virtual ~BlockDiff(){}
	//! Do the actual difference
	/*!
		Do the actual difference
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv	the motion vector being used 
	*/
	virtual void Diff(BlockDiffParams& dparams,const MVector& mv)=0;//must be overidden
protected:
	const PicArray* pic_data;
	const PicArray* ref_data;
private:
	BlockDiff(const BlockDiff& cpy);			//private, bodyless copy-constructor: class and its 
												//derivatives should not be copied
	BlockDiff& operator=(const BlockDiff& rhs);	//private, bodyless assignment=: class and its 
												//derivatives should not be assigned
};

//! A class for doing block differences without bounds-checking, inherited from BlockDiff
class SimpleBlockDiff: public BlockDiff{//block difference with 1 reference and no bounds checking
public:
	//! Constructor
	SimpleBlockDiff(){}
	//! Constructor, initialising the reference and picture data
	SimpleBlockDiff(const PicArray& ref,const PicArray& pic):BlockDiff(ref,pic){}
	//! Do the actual difference without bounds checking
	/*!
		Do the actual difference without bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv	the motion vector being used 
	*/
	void Diff(BlockDiffParams& dparams, const MVector& mv);
private:
	SimpleBlockDiff(const SimpleBlockDiff& cpy);			//private, bodyless copy-constructor: class and its 
															//derivatives should not be copied
	SimpleBlockDiff& operator=(const SimpleBlockDiff& rhs);	//private, bodyless assignment=: class and its 
															//derivatives should not be assigned

};

//! A class for doing block differences with bounds-checking, inherited from BlockDiff
class BChkBlockDiff: public BlockDiff{//block difference with 1 reference and bounds checking
public:
	//! Constructor
	BChkBlockDiff(){}
	//! Constructor, initialising the reference and picture data
	BChkBlockDiff(const PicArray& ref,const PicArray& pic):BlockDiff(ref,pic){}	
	//! Do the actual difference with bounds checking
	/*!
		Do the actual difference with bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv	the motion vector being used 
	*/	
	void Diff(BlockDiffParams& dparams, const MVector& mv);
private:
	BChkBlockDiff(const BChkBlockDiff& cpy);			//private, bodyless copy-constructor: class and its 
															//derivatives should not be copied
	BChkBlockDiff& operator=(const BChkBlockDiff& rhs);	//private, bodyless assignment=: class and its 
															//derivatives should not be assigned
};

//! A class for calculating the difference between a block and its DC value (average)
class IntraBlockDiff: public BlockDiff{
public:
	//! Constructor
	IntraBlockDiff(){}
	//! Constructor, initialising the reference and picture data to be the same, as there's no temporal reference
	IntraBlockDiff(const PicArray& pic): BlockDiff(pic,pic){}
	//! Empty function to override virtual inherited function
	void Diff(BlockDiffParams& dparams, const MVector& mv){}
	//! Do the actual difference
	/*!
		Do the actual difference
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	dc_pred	a prediction for the DC value from neighbouring blocks
		\param	loc_lambda	a weighting parameter for the DC value coding cost
	*/		
	void Diff(BlockDiffParams& dparams,ValueType dc_pred,float loc_lambda);
private:
	IntraBlockDiff(const IntraBlockDiff& cpy);				//private, bodyless copy-constructor: class and its 
															//derivatives should not be copied
	IntraBlockDiff& operator=(const IntraBlockDiff& rhs);	//private, bodyless assignment=: class and its 
															//derivatives should not be assigned
};

//! A class for bi-directional differences with two references, and no bounds checking
class BiSimpleBlockDiff: public BlockDiff{
public:
	//! Constructor
	BiSimpleBlockDiff(){}
	//! Constructor, initialising the references and picture data
	BiSimpleBlockDiff(const PicArray& ref,const PicArray& ref2,const PicArray& pic):BlockDiff(ref,pic),ref_data2(&ref2){}
	//! Empty function, overriding pure virtual inherited function
	void Diff(BlockDiffParams& dparams, const MVector& mv){}
	//! Do the actual difference without bounds checking
	/*!
		Do the actual difference without bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv1	the motion vector being used for reference 1
		\param	mv2	the motion vector being used for reference 2
	*/		
	void Diff(BlockDiffParams& dparams, const MVector& mv1,const MVector& mv2);
private:
	const PicArray* ref_data2;

	BiSimpleBlockDiff(const BiSimpleBlockDiff& cpy);			//private, bodyless copy-constructor: class and its 
																//derivatives should not be copied
	BiSimpleBlockDiff& operator=(const BiSimpleBlockDiff& rhs);	//private, bodyless assignment=: class and its 
																//derivatives should not be assigned
};

//! A class for bi-directional differences with two references, with bounds checking
class BiBChkBlockDiff: public BlockDiff{
public:
	//! Constructor
	BiBChkBlockDiff(){}
	//! Constructor, initialising the references and picture data
	BiBChkBlockDiff(const PicArray& ref,const PicArray& ref2,const PicArray& pic):BlockDiff(ref,pic),ref_data2(&ref2){}
	//! Empty function, overriding pure virtual inherited function
	void Diff(BlockDiffParams& dparams, const MVector& mv){}
	//! Do the actual difference with bounds checking
	/*!
		Do the actual difference with bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv1	the motion vector being used for reference 1
		\param	mv2	the motion vector being used for reference 2
	*/		

	void Diff(BlockDiffParams& dparams, const MVector& mv1,const MVector& mv2);
private:
	const PicArray* ref_data2;

	BiBChkBlockDiff(const BiBChkBlockDiff& cpy);			//private, bodyless copy-constructor: class and its 
															//derivatives should not be copied
	BiBChkBlockDiff& operator=(const BiBChkBlockDiff& rhs);	//private, bodyless assignment=: class and its 
															//derivatives should not be assigned
};

//classes where the reference is upconverted

//! An abstract class for doing block differences with an upconverted reference
class BlockDiffUp: public BlockDiff{//overall class for upconversion
public:	
	//! Constructor
	BlockDiffUp(){}
	//! Constructor, initialising the reference and picture data
	BlockDiffUp(const PicArray& ref,const PicArray& pic): BlockDiff(ref,pic){}
	//! Destructor
	virtual ~BlockDiffUp(){}

protected:
	int InterpLookup[9][4];//A lookup table to simplify the 1/8 pixel accuracy code
private:
	BlockDiffUp(const BlockDiffUp& cpy);			//private, bodyless copy-constructor: class and its 
													//derivatives should not be copied
	BlockDiffUp& operator=(const BlockDiffUp& rhs);	//private, bodyless assignment=: class and its 
													//derivatives should not be assigned
};

//! A class for doing block differences without bounds-checking with upconverted references, inherited from BlockDiffUp
class SimpleBlockDiffUp: public BlockDiffUp{

public:
	//! Constructor
	SimpleBlockDiffUp(){}
	//! Constructor, initialising the reference and picture data
	SimpleBlockDiffUp(const PicArray& ref,const PicArray& pic):BlockDiffUp(ref,pic){}
	//! Destructor
	~SimpleBlockDiffUp(){}

	//! Do the actual difference without bounds checking
	/*!
		Do the actual difference without bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv	the motion vector being used 
	*/		
	void Diff(BlockDiffParams& dparams, const MVector& mv);

private:
	SimpleBlockDiffUp(const SimpleBlockDiffUp& cpy);//private, bodyless copy-constructor: class and its 
													//derivatives should not be copied
	SimpleBlockDiffUp& operator=(const SimpleBlockDiffUp& rhs);	//private, bodyless assignment=: class and its 
																//derivatives should not be assigned
};

//! A class for doing block differences with bounds-checking with upconverted references, inherited from BlockDiffUp
class BChkBlockDiffUp: public BlockDiffUp{

public:
	//! Constructor
	BChkBlockDiffUp() {}
	//! Constructor, initialising the reference and picture data
	BChkBlockDiffUp(const PicArray& ref,const PicArray& pic):BlockDiffUp(ref,pic){}
	//! Destructor
	~BChkBlockDiffUp(){}

	//! Do the actual difference with bounds checking
	/*!
		Do the actual difference with bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv	the motion vector being used 
	*/		
	void Diff(BlockDiffParams& dparams, const MVector& mv);
private:
	BChkBlockDiffUp(const BChkBlockDiffUp& cpy);//private, bodyless copy-constructor: class and its 
												//derivatives should not be copied
	BChkBlockDiffUp& operator=(const BChkBlockDiffUp& rhs);	//private, bodyless assignment=: class and its 
															//derivatives should not be assigned
};

//! A class for doing bi-directional block differences without bounds checking
class BiSimpleBlockDiffUp: public BlockDiffUp{
public:
	//! Constructor
	BiSimpleBlockDiffUp(){}
	//! Constructor, initialising the references and picture data
	BiSimpleBlockDiffUp(const PicArray& ref,const PicArray& ref2,const PicArray& pic):BlockDiffUp(ref,pic),ref_data2(&ref2){}

	//! Empty function, overriding pure virtual inherited function
	void Diff(BlockDiffParams& dparams, const MVector& mv){}

	//! Do the actual difference without bounds checking
	/*!
		Do the actual difference without bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv1	the motion vector being used for reference 1
		\param	mv2	the motion vector being used for reference 2
	*/		
	void Diff(BlockDiffParams& dparams, const MVector& mv1,const MVector& mv2);
private:
	const PicArray* ref_data2;

	BiSimpleBlockDiffUp(const BiSimpleBlockDiffUp& cpy);//private, bodyless copy-constructor: class and its 
														//derivatives should not be copied
	BiSimpleBlockDiffUp& operator=(const BiSimpleBlockDiffUp& rhs);	//private, bodyless assignment=: class and its 
																	//derivatives should not be assigned
};

//! A class for doing  bi-directional block differences with bounds checking
class BiBChkBlockDiffUp: public BlockDiffUp{
public:
	//Constructor
	BiBChkBlockDiffUp(){}
	//! Constructor, initialising the references and picture data
	BiBChkBlockDiffUp(const PicArray& ref,const PicArray& ref2,const PicArray& pic):BlockDiffUp(ref,pic),ref_data2(&ref2){}

	//! Empty function, overriding pure virtual inherited function
	void Diff(BlockDiffParams& dparams, const MVector& mv){}

	//! Do the actual difference with bounds checking
	/*!
		Do the actual difference with bounds checking
		\param	dparams	the parameters in which costs, block parameters etc are stored
		\param	mv1	the motion vector being used for reference 1
		\param	mv2	the motion vector being used for reference 2
	*/	
	void Diff(BlockDiffParams& dparams, const MVector& mv1,const MVector& mv2);
private:
	const PicArray* ref_data2;

	BiBChkBlockDiffUp(const BiBChkBlockDiffUp& cpy);//private, bodyless copy-constructor: class and its 
													//derivatives should not be copied
	BiBChkBlockDiffUp& operator=(const BiBChkBlockDiffUp& rhs);	//private, bodyless assignment=: class and its 
																//derivatives should not be assigned
};
#endif
