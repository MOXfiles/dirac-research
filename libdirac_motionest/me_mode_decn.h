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
* Revision 1.2  2004/04/05 03:05:02  chaoticcoyote
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

#ifndef _ME_MODE_DECN_H_
#define _ME_MODE_DECN_H_

#include "libdirac_common/motion.h"
#include "libdirac_motionest/block_match.h"

class FrameBuffer;

//! Decides between macroblock and block prediction modes, doing additional motion estimation as necessary
/*!
    Loops over all the macroblocks and decides on the best modes. A macroblock is a square of 16 blocks.
	 There are three possible splitting levels: level 0 means
	the macroblock is considered as a single block; level 1 means the macroblock is considered as 4 larger blocks, termed
	sub-macroblocks; level 0 means the macroblock is split right down to blocks. In addition there is a common_ref
	mode which if true means the prediction mode of all units within the MB are the same (e.g. all sub-MBs are predicted
	only from reference 1). In deciding which modes to adopt, the ModeDecider object calculates costs for all
	permutations, doing motion estimation for the level 1 and level 0 modes as these have not been calculated before.
 */
class ModeDecider{

public:
	//! Constructor
    /*!
		The constructor creates arrays for handling the motion vector data at splitting levels 0 and 1, as motion
		estimation must be performed for these levels.
     */
	ModeDecider(EncoderParams& params):
	encparams(params){
		split1_mv_data=new MvData(1,1,2,2);
		split0_mv_data=new MvData(1,1,1,1);}	

    //! Destructor
    /*!
		The destructor destroys the classes created in the constructor
     */	
	~ModeDecider(){delete split1_mv_data; delete split0_mv_data;}

	//! Does the actual mode decision
	/*!
		Does the mode decision
		/param	my_buffer	the buffer of all the relevant frames
		/param	frame_num	the frame number for which motion estimation is being done
		/param	mvd	the motion vector data into which decisions will be written
     */
	void DoModeDecn(const FrameBuffer& my_buffer,int frame_num, MvData& mvd);

private:
	ModeDecider(const ModeDecider& cpy);//private, body-less copy constructor: this class should not be copied
	ModeDecider& operator=(const ModeDecider& rhs);//private, body-less assignment=: this class should not be assigned

 	//internal data
	FrameSort fsort;
	EncoderParams encparams;
	float loc_lambda;
	float factor1x1,factor2x2;

	const MvData* mv_data;
	MvData* split1_mv_data;	//MV data associated with a level 1 splitting of the MB
	MvData* split0_mv_data;	//MV data associated with a non-split MB	
	const PicArray* pic_data;
	const PicArray* ref1_updata;
	const PicArray* ref2_updata;
	int num_refs;

	BlockDiff* mydiff;
	IntraBlockDiff* intradiff;
	BChkBlockDiffUp* checkdiff1;
	SimpleBlockDiffUp* simplediff1;	
	BChkBlockDiffUp* checkdiff2;	
	SimpleBlockDiffUp* simplediff2;
	BiBChkBlockDiffUp* bicheckdiff;

	//position variables, used in all the mode decisions
	int xmb_loc,ymb_loc;	//coords of the current MB
	int xtl,ytl;			//coords of top-left block in MB
	int xbr,ybr;			//coords of TL block in next MB to the bottom-right
	int xsubMBtl,ysubMBtl;	//coords of top-left sub-MB in MB
	int xsubMBbr,ysubMBbr;	//coords of TL sub-MB in next MB to the bottom-right


 	//functions
	void DoMBDecn();	//called by do_mode_decn for each MB
	void Do4x4Decn();
	void Do2x2Decn();
	void Do1x1Decn();
	float DoBlockDecn4x4(int xblock,int yblock);
	float DoCommonMode4x4(PredMode& predmode);
	void Do2x2ME();
	float DoBlockDecn2x2(int xsubMB,int ysubMB);
	float DoCommonMode2x2(PredMode& predmode);
	void Do1x1ME();
	float DoCommonMode1x1(PredMode& predmode);
	inline float ModeCost(int xindex,int yindex,PredMode predmode){
		return 2.0*GetModeVar(mv_data,xindex,yindex,predmode,loc_lambda);}
};

#endif
