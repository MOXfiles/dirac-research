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
* Revision 1.4  2004-06-18 15:58:37  tjdwave
* Removed chroma format parameter cformat from CodecParams and derived
* classes to avoid duplication. Made consequential minor mods to
* seq_{de}compress and frame_{de}compress code.
* Revised motion compensation to use built-in arrays for weighting
* matrices and to enforce their const-ness.
* Removed unnecessary memory (de)allocations from Frame class copy constructor
* and assignment operator.
*
* Revision 1.3  2004/05/12 08:35:35  tjdwave
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

#include "libdirac_motionest/motion_estimate.h"
#include "libdirac_motionest/block_match.h"
#include "libdirac_common/motion.h"
#include "libdirac_common/frame_buffer.h"
#include "libdirac_motionest/downconvert.h"
#include "libdirac_motionest/me_mode_decn.h"
#include "libdirac_motionest/me_subpel.h"
#include <cmath>
#include <vector>

using std::vector;

MotionEstimator::MotionEstimator(const EncoderParams& params): encparams(params){
}

void MotionEstimator::DoME(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data){

	const FrameParams& fparams=my_buffer.GetFrame(frame_num).GetFparams();

	if (fparams.fsort!=I_frame)
	{
		if (encparams.VERBOSE)		
			std::cerr<<std::endl<<"Doing initial search ...";
		DoHierarchicalSearch(my_buffer,frame_num,mv_data);

		if (encparams.VERBOSE)
			std::cerr<<std::endl<<"Doing sub-pixel refinement ...";
		DoFinalSearch(my_buffer,frame_num,mv_data);				

		if (encparams.VERBOSE)
			std::cerr<<std::endl<<"Doing mode decision ...";
		ModeDecider my_mode_dec(encparams);
		my_mode_dec.DoModeDecn(my_buffer,frame_num,mv_data);

		if (fparams.cformat!=Yonly)
		{
			if (encparams.VERBOSE)
				std::cerr<<std::endl<<"Setting chroma DC values ... ";
			SetChromaDC(my_buffer,frame_num,mv_data);	
		}
	}
}

void MotionEstimator::DoHierarchicalSearch(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data){
 	//does an initial search using hierarchical matching to get guide vectors	
	int ref1,ref2;
	depth=4;
	int scale_factor;//factor by which pics have been downconverted
	const OLBParams& bparams=encparams.LumaBParams(2);

	DownConverter mydcon;
	OneDArray<PicArray*> ref1_down(Range(1,depth));//down-converted pictures	
	OneDArray<PicArray*> ref2_down(Range(1,depth));
	OneDArray<PicArray*> pic_down(Range(1,depth));
	OneDArray<MvData*> mv_data_set(Range(1,depth));

	const PicArray& pic_data=my_buffer.GetComponent(frame_num,Y);
	const vector<int>& refs=my_buffer.GetFrame(frame_num).GetFparams().refs;
	ref1=refs[0];
	if (refs.size()>1)
		ref2=refs[1];
	else	
		ref2=ref1;
	const PicArray& ref1_data=my_buffer.GetComponent(ref1,Y);
	const PicArray& ref2_data=my_buffer.GetComponent(ref2,Y);

	//allocate
	scale_factor=1;	
	int xnumblocks,ynumblocks;
	for (int I=1;I<=depth;++I){
   		//dimensions of pic_down[I] will be shrunk by a factor 2**I		
		scale_factor*=2;
		pic_down[I]=new PicArray(pic_data.length(0)/scale_factor,pic_data.length(1)/scale_factor);
		ref1_down[I]=new PicArray(ref1_data.length(0)/scale_factor,ref1_data.length(1)/scale_factor);
		if (ref1!=ref2)
			ref2_down[I]=new PicArray(ref2_data.length(0)/scale_factor,ref2_data.length(1)/scale_factor);
		xnumblocks=pic_down[I]->length(0)/bparams.XBSEP;
		ynumblocks=pic_down[I]->length(1)/bparams.YBSEP;
		if ((pic_down[I]->length(0))%bparams.XBSEP !=0)
			xnumblocks++;
		if ((pic_down[I]->length(1))%bparams.YBSEP !=0)
			ynumblocks++;
		mv_data_set[I]=new MvData(0,0,xnumblocks,ynumblocks);
	}
	//do all the downconversions
	if (depth>0){
		mydcon.DoDownConvert(pic_data,*(pic_down[1]));
		mydcon.DoDownConvert(ref1_data,*(ref1_down[1]));
		if (refs.size()>1 && ref1!=ref2)//there's a second reference, so repeat
			mydcon.DoDownConvert(ref2_data,*(ref2_down[1]));
		for (int I=1;I<depth;++I){
			mydcon.DoDownConvert(*(pic_down[I]),*(pic_down[I+1]));
			mydcon.DoDownConvert(*(ref1_down[I]),*(ref1_down[I+1]));				
			if (refs.size()>1 && ref1!=ref2)//there's a second reference, so repeat
				mydcon.DoDownConvert(*(ref2_down[I]),*(ref2_down[I+1]));
		}
	}
 	//start with motion estimating at the lowest level
	MatchPic(*(ref1_down[depth]),*(pic_down[depth]),*(mv_data_set[depth]),*(mv_data_set[depth]),1,depth);	
	if (ref1!=ref2)
		MatchPic(*(ref2_down[depth]),*(pic_down[depth]),*(mv_data_set[depth]),*(mv_data_set[depth]),2,depth);
 	//do the intervening levels
	for (int level=depth-1;level>=1;--level){
		MatchPic(*(ref1_down[level]),*(pic_down[level]),*(mv_data_set[level]),*(mv_data_set[level+1]),1,level);
		if (ref1!=ref2){
			MatchPic(*(ref2_down[level]),*(pic_down[level]),*(mv_data_set[level]),*(mv_data_set[level+1]),2,level);
		}
	}//level
	//finally, do the top level
	MatchPic(ref1_data,pic_data,mv_data,*(mv_data_set[1]),1,0);
	if (ref1!=ref2){
		MatchPic(ref2_data,pic_data,mv_data,*(mv_data_set[1]),2,0);
	}

	for (int I=1; I<=depth;++I){
		delete pic_down[I];
		delete ref1_down[I];
		delete mv_data_set[I];
		if (refs.size()>1 && ref1!=ref2)
			delete ref2_down[I];
	}
}

void MotionEstimator::DoFinalSearch(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data){
	SubpelRefine pelrefine(encparams);
	pelrefine.DoSubpel(my_buffer,frame_num,mv_data);
}

void MotionEstimator::MatchPic(const PicArray& ref_data,const PicArray& pic_data,MvData& mv_data,const MvData& guide_data,
int ref_id,int level){	

	if (level==depth){
		xr=4;yr=4;
	}
	else if (level==(depth-1)){
		xr=3;yr=3;
	}
	else{
		xr=2;yr=2;
	}

	BMParams matchparams;
	const OLBParams& bparams=encparams.LumaBParams(2);
	matchparams.pic_data=&pic_data;
	matchparams.ref_data=&ref_data;

 	//set lambda
	float loc_lambda;
	if (fsort==L1_frame)
		loc_lambda=encparams.L1_ME_lambda/pow(2.0,level);
	else//must have an L2 frame
		loc_lambda=encparams.L2_ME_lambda/pow(2.0,level);
	MVector tmp_mv;	
	MvArray* mv_array;
	const MvArray* guide_array;
	TwoDArray<MvCostData>* block_costs;
	vector<vector<MVector> > vect_list;
	matchparams.vect_list=&vect_list;	//point the matching parameters at the list of candidate vectors

	if (ref_id==1){
		mv_array=&mv_data.mv1;
		guide_array=&guide_data.mv1;
		block_costs=&mv_data.block_costs1;
	}
	else{
		mv_array=&mv_data.mv2;
		guide_array=&guide_data.mv2;
		block_costs=&mv_data.block_costs2;		
	}

	int xnum=mv_array->length(0);
	int ynum=mv_array->length(1);

	//make a zero-based list that is always used
	tmp_mv.x=0; tmp_mv.y=0;	
	AddNewVlistD(vect_list,tmp_mv,xr,yr);//always look in a region of 0

	//now loop over the blocks and find the best matches
	//loop is unrolled because predictions are different at picture edges

	//first do TL corner
	matchparams.mv_pred=tmp_mv;
	matchparams.me_lambda=0.0;
	if (level<depth){//use guide from lower down
		tmp_mv.x=(*guide_array)[0][0].x<<1;//scale up the
		tmp_mv.y=(*guide_array)[0][0].y<<1;//guide vector
		AddNewVlistD(vect_list,tmp_mv,xr,yr);
	}
	matchparams.Init(bparams,0,0);
	(*block_costs)[0][0]=FindBestMatch(matchparams);
	(*mv_array)[0][0]=matchparams.best_mv;
	vect_list.erase(vect_list.begin()+1,vect_list.end());

	//now do the rest of the first row
	matchparams.me_lambda=loc_lambda/float(encparams.Y_NUMBLOCKS);//use reduced lambda to start with
	for (int I=1;I<xnum;++I){
		if (level<depth){//use guide from lower down
			tmp_mv.x=(*guide_array)[0][I>>1].x<<1;//scale up the
			tmp_mv.y=(*guide_array)[0][I>>1].y<<1;//guide vector
			AddNewVlistD(vect_list,tmp_mv,xr,yr);
		}

		matchparams.mv_pred=(*mv_array)[0][I-1];
		AddNewVlistD(vect_list,matchparams.mv_pred,xr,yr);
		matchparams.Init(I,0);
		(*block_costs)[0][I]=FindBestMatch(matchparams);
		(*mv_array)[0][I]=matchparams.best_mv;
		vect_list.erase(vect_list.begin()+1,vect_list.end());
	}//I

	for (int J=1;J<ynum;++J){//now the main body

 		//I=0,J>0
		if (level<depth){//use guide from lower down
			tmp_mv.x=(*guide_array)[J>>1][0].x<<1;//scale up the
			tmp_mv.y=(*guide_array)[J>>1][0].y<<1;//guide vector
			AddNewVlistD(vect_list,tmp_mv,xr,yr);
		}

		matchparams.mv_pred=(*mv_array)[J-1][0];
		matchparams.me_lambda=loc_lambda/float(encparams.X_NUMBLOCKS);
		AddNewVlistD(vect_list,matchparams.mv_pred,xr,yr);
		matchparams.Init(0,J);
		(*block_costs)[J][0]=FindBestMatch(matchparams);
		(*mv_array)[J][0]=matchparams.best_mv;
		vect_list.erase(vect_list.begin()+1,vect_list.end());

 		//main loop
		for (int I=1;I<xnum-1;++I){//main loop
			matchparams.me_lambda=loc_lambda;			
			if (level<depth){//use guide from lower down
				tmp_mv.x=(*guide_array)[J>>1][I>>1].x<<1;//scale up the
				tmp_mv.y=(*guide_array)[J>>1][I>>1].y<<1;//guide vector
				AddNewVlistD(vect_list,tmp_mv,xr,yr);				
			}

			matchparams.mv_pred=MvMedian((*mv_array)[J][I-1],(*mv_array)[J-1][I],(*mv_array)[J-1][I+1]);
			AddNewVlistD(vect_list,matchparams.mv_pred,xr,yr);
			matchparams.Init(I,J);			
			(*block_costs)[J][I]=FindBestMatch(matchparams);
			(*mv_array)[J][I]=matchparams.best_mv;			
			vect_list.erase(vect_list.begin()+1,vect_list.end());
		}//I
 		//last element
		if (level<depth){//use guide from lower down
			tmp_mv.x=(*guide_array)[J>>1][(xnum-1)>>1].x<<1;//scale up the
			tmp_mv.y=(*guide_array)[J>>1][(xnum-1)>>1].y<<1;//guide vector
			AddNewVlistD(vect_list,tmp_mv,xr,yr);
		}		

		matchparams.mv_pred=MvMean((*mv_array)[J-1][xnum-1],(*mv_array)[J][xnum-2]);
		matchparams.me_lambda=loc_lambda/float(encparams.X_NUMBLOCKS);
		AddNewVlistD(vect_list,matchparams.mv_pred,xr,yr);
		matchparams.Init(xnum-1,J);
		(*block_costs)[J][xnum-1]=FindBestMatch(matchparams);
		(*mv_array)[J][xnum-1]=matchparams.best_mv;
		vect_list.erase(vect_list.begin()+1,vect_list.end());
	}//J
}

ValueType MotionEstimator::GetChromaBlockDC(const PicArray& pic_data, MvData& mv_data,int xblock,int yblock,int split){
	BMParams matchparams;
	matchparams.pic_data=&pic_data;
	matchparams.Init(encparams.ChromaBParams(split),xblock,yblock);
	BlockDiffParams dparams(matchparams);
	IntraBlockDiff intradiff(pic_data);
	intradiff.Diff(dparams,0,0.0f);
	return dparams.dc;
}

void MotionEstimator::SetChromaDC(const PicArray& pic_data, MvData& mv_data,CompSort csort){

	int xtl,ytl;//lower limit of block coords in MB
	int xbr,ybr;//upper limit of block coords in MB
	int xsubMBtl,ysubMBtl;//ditto, for subMBs	
	int xsubMBbr,ysubMBbr;

	TwoDArray<ValueType>* dcarray;
	if (csort==U)
		dcarray=&(mv_data.dcU);
	else//csort==V
		dcarray=&(mv_data.dcV);

	for (int ymb_loc=0;ymb_loc<mv_data.mb.length(1);++ymb_loc){
		for (int xmb_loc=0;xmb_loc<mv_data.mb.length(0);++xmb_loc){

			xtl=xmb_loc<<2;
			ytl=ymb_loc<<2;			
			xbr=xtl+4;
			ybr=ytl+4;

			xsubMBtl=xmb_loc<<1;
			ysubMBtl=ymb_loc<<1;			
			xsubMBbr=xsubMBtl+2;
			ysubMBbr=ysubMBtl+2;

			if (xmb_loc==mv_data.mb.last(0)){
				xbr=std::min(mv_data.mv1.length(0),xbr);
				xsubMBbr=std::min(mv_data.mv1.length(0)>>1,xsubMBbr);
			}
			if (ymb_loc==mv_data.mb.last(1)){
				ybr=std::min(mv_data.mv1.length(1),ybr);
				ysubMBbr=std::min(mv_data.mv1.length(1)>>1,ysubMBbr);
			}

			if (mv_data.mb[ymb_loc][xmb_loc].split_mode==2){
				for (int L=ytl;L<ybr;++L){
					for (int K=xtl;K<xbr;++K){
						if (mv_data.mode[L][K]==INTRA){//calculate the dc values for U and V
							(*dcarray)[L][K]=GetChromaBlockDC(pic_data,mv_data,K,L,2);
						}
					}//K
				}//L
			}
			else if (mv_data.mb[ymb_loc][xmb_loc].split_mode==1){
				for (int L=ysubMBtl,S=(ysubMBtl<<1);L<ysubMBbr;++L,S+=2){
					for (int K=xsubMBtl,R=(xsubMBtl<<1);K<xsubMBbr;++K,R+=2){
						if (mv_data.mode[S][R]==INTRA){//calculate the dc values for U and V
							(*dcarray)[S][R]=GetChromaBlockDC(pic_data,mv_data,K,L,1);
							//propagate values to all the blocks in the sub-MB
							for (int Q=S;Q<S+2;++Q){
								for (int P=R;P<R+2;++P){
									(*dcarray)[Q][P]=(*dcarray)[S][R];
								}//P
							}//Q
						}
					}//K	
				}//L
			}
			else{//split_mode=0
				if (mv_data.mode[ytl][xtl]==INTRA){//calculate the dc values for U and V
					(*dcarray)[ytl][xtl]=GetChromaBlockDC(pic_data,mv_data,xmb_loc,ymb_loc,0);
					//propagate values to all the blocks in the MB
					for (int Q=ytl;Q<ybr;++Q){
						for (int P=xtl;P<xbr;++P){
							(*dcarray)[Q][P]=(*dcarray)[ytl][xtl];
						}//P
					}//Q
				}
			}
		}//I
	}//J
}

void MotionEstimator::SetChromaDC(const FrameBuffer& my_buffer, int frame_num, MvData& mv_data){

	SetChromaDC(my_buffer.GetComponent(frame_num,U),mv_data,U);
	SetChromaDC(my_buffer.GetComponent(frame_num,V),mv_data,V);

}
