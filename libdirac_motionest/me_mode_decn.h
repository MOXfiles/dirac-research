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

#ifndef _ME_MODE_DECN_H_
#define _ME_MODE_DECN_H_

#include "libdirac_common/motion.h"
#include "libdirac_motionest/block_match.h"
#include "libdirac_common/gop.h"

class ModeDecider{
 					//does the mode decisions
public:
	ModeDecider(EncoderParams& params): encparams(params){
		split1_mv_data=new MvData(1,1,2,2);
		split0_mv_data=new MvData(1,1,1,1);}	
	~ModeDecider(){delete split1_mv_data; delete split0_mv_data;}

	void DoModeDecn(Gop& my_gop,int frame_num, MvData& mvd);

private:
 	//internal data
	FrameSort fsort;
	EncoderParams encparams;
	float loc_lambda;
	float factor1x1,factor2x2;

	MvData* mv_data;
	MvData* split1_mv_data;	//MV data associated with a level 1 splitting of the MB
	MvData* split0_mv_data;	//MV data associated with a non-split MB	
	PicArray* pic_data;
	PicArray* ref1_updata;
	PicArray* ref2_updata;
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
