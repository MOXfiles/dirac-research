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

#include "libdirac_motionest/me_mode_decn.h"
#include "libdirac_common/frame_buffer.h"
#include <algorithm>

using std::vector;

 //mode decision stuff
void ModeDecider::DoModeDecn(const FrameBuffer& my_buffer, int frame_num, MvData& mvdata){

 	//we've got 'raw' block motion vectors for up to two reference frames. Now we want
 	//to make a decision as to mode. In this initial implementation,
 	//this is bottom-up i.e. find mvs for MBs and sub-MBs and see whether it's worthwhile merging.	
	int ref1,ref2;

  	//Following factors normalise costs for sub-MBs and MBs to those of blocks, so that the overlap is
  	//take into account (e.g. a sub-MB has length XBLEN+XBSEP and YBLEN+YBSEP). 
	//The MB costs for a 1x1 decomposition are not directly comprable to those for other
	//decompositions because of the block overlaps. These factors remove these effects, so that
	//all SAD costs are normalised to the area corresponding to non-overlapping 16 blocks of size XBLEN*YBLEN.	
	factor1x1=float(16*encparams.LumaBParams(2).XBLEN*encparams.LumaBParams(2).YBLEN)/
		float(encparams.LumaBParams(0).XBLEN*encparams.LumaBParams(0).YBLEN);
	factor2x2=float(4*encparams.LumaBParams(2).XBLEN*encparams.LumaBParams(2).YBLEN)/
		float(encparams.LumaBParams(1).XBLEN*encparams.LumaBParams(1).YBLEN);

	fsort=my_buffer.GetFrame(frame_num).GetFparams().fsort;
	if (fsort!=I_frame){
		mv_data=&mvdata;
		pic_data=&(my_buffer.GetComponent(frame_num,Y));
		const vector<int>& refs=my_buffer.GetFrame(frame_num).GetFparams().refs;
		num_refs=refs.size();
		ref1=refs[0];
		intradiff=new IntraBlockDiff(*pic_data);
		ref1_updata=&(my_buffer.GetUpComponent(ref1,Y));
		checkdiff1=new BChkBlockDiffUp(*ref1_updata,*pic_data);
		simplediff1=new SimpleBlockDiffUp(*ref1_updata,*pic_data);
		if (num_refs>1){
			ref2=refs[1];
			ref2_updata=&(my_buffer.GetUpComponent(ref2,Y));			
			checkdiff2=new BChkBlockDiffUp(*ref2_updata,*pic_data);
			simplediff2=new SimpleBlockDiffUp(*ref2_updata,*pic_data);
			bicheckdiff=new BiBChkBlockDiffUp(*ref1_updata,*ref2_updata,*pic_data);
		}
		else{	
			ref2=ref1;
		}
		for (ymb_loc=0;ymb_loc<encparams.Y_NUM_MB;++ymb_loc){
			for (xmb_loc=0;xmb_loc<encparams.X_NUM_MB;++xmb_loc){
				xtl=xmb_loc<<2;//top-left block
				ytl=ymb_loc<<2;//coords	
				xsubMBtl=xmb_loc<<1;//top-left subMB
				ysubMBtl=ymb_loc<<1;//coords	

				if (xmb_loc<encparams.X_NUM_MB-1 && ymb_loc<encparams.Y_NUM_MB-1){
					xbr=xtl+4;
					ybr=ytl+4;
					xsubMBbr=xsubMBtl+2;
					ysubMBbr=ysubMBtl+2;
				}
				else{
					xbr=std::min(xtl+4,encparams.X_NUMBLOCKS);
					ybr=std::min(ytl+4,encparams.Y_NUMBLOCKS);
					xsubMBbr=std::min(xsubMBtl+2,encparams.X_NUMBLOCKS>>1);
					ysubMBbr=std::min(ysubMBtl+2,encparams.Y_NUMBLOCKS>>1);
				}
				DoMBDecn();				
			}//xmb_loc		
		}//ymb_loc

		delete intradiff;
		delete checkdiff1;
		delete simplediff1;
		if (num_refs>1){
			delete checkdiff2;
			delete simplediff2;
			delete bicheckdiff;
		}		
	}
}

void ModeDecider::DoMBDecn(){
  	//does the mode decision for the given MB

	Do4x4Decn();//start with 4x4 modes
	float old_best_MB_cost=(mv_data->MB_costs)[ymb_loc][xmb_loc];
	Do2x2Decn();//next do 2x2 modes
	if ((mv_data->MB_costs)[ymb_loc][xmb_loc]<=old_best_MB_cost){
		old_best_MB_cost=(mv_data->MB_costs)[ymb_loc][xmb_loc];		
		Do1x1Decn();//finally do 1x1 mode if merging worked before
	}	
}

void ModeDecider::Do4x4Decn(){
  	//computes the best costs if we were to
 	//stick to a 4x4 decomposition
  	//Assumes that we already have block-level costs defined
 	//for each reference, but not intra or bi-pred costs	

	float MB_cost=0.0;
	if (fsort==L1_frame)
		loc_lambda=encparams.L1_ME_lambda;
	else
		loc_lambda=encparams.L2_ME_lambda;
	if (xmb_loc==0 || ymb_loc==0 || xmb_loc==encparams.X_NUM_MB-1 || ymb_loc==encparams.Y_NUM_MB-1)
		loc_lambda/=5.0;//have reduced lambda at the picture edges

	(mv_data->mb)[ymb_loc][xmb_loc].split_mode=2;//split down to second level

 	// 	Case 1: prediction modes are all different
	(mv_data->mb)[ymb_loc][xmb_loc].common_ref=false;//modes all different	
	for (int J=ytl;J<ybr;++J){
		for (int I=xtl; I<xbr;++I){
			MB_cost+=DoBlockDecn4x4(I,J);
		}//I
	}//J
	(mv_data->MB_costs)[ymb_loc][xmb_loc]=MB_cost;

  	// 	Case 2: prediction modes are all the same
	PredMode predmode;
	MB_cost=DoCommonMode4x4(predmode);
	if (MB_cost<=(mv_data->MB_costs)[ymb_loc][xmb_loc]){
		MB_cost=(mv_data->MB_costs)[ymb_loc][xmb_loc];
   		//propagate data - must set all modes to the common mode
		(mv_data->mb)[ymb_loc][xmb_loc].common_ref=true;
		for (int J=ytl;J<ybr;++J){
			for (int I=xtl;I<xbr;++I){
				(mv_data->mode)[J][I]=predmode;
			}//I
		}//J
	}
}

void ModeDecider::Do2x2Decn(){
   	//computes the best costs if we were to
  	//stick to a 2x2 decomposition
	if (fsort==L1_frame)
		loc_lambda=encparams.L1_ME_lambda/factor2x2;
	else
		loc_lambda=encparams.L2_ME_lambda/factor2x2;
	if (xmb_loc==0 || ymb_loc==0 || xmb_loc==encparams.X_NUM_MB-1 || ymb_loc==encparams.Y_NUM_MB-1)
		loc_lambda/=5.0;//have reduced lambda at the picture edges

	Do2x2ME();//first do the motion estimation for the subMBs

	// 	Case 1: prediction modes are all different

	float MB_cost=0.0;	
	for (int J=0;J<2;++J){
		for (int I=0; I<2;++I){
			MB_cost+=DoBlockDecn2x2(I,J);
		}//I
	}//J

	if (MB_cost<=(mv_data->MB_costs)[ymb_loc][xmb_loc]){
		(mv_data->MB_costs)[ymb_loc][xmb_loc]=MB_cost;
		(mv_data->mb)[ymb_loc][xmb_loc].split_mode=1;//split down to 1st
		(mv_data->mb)[ymb_loc][xmb_loc].common_ref=false;
		for (int J=ytl,Q=0;J<ybr;J+=2,++Q){
			for (int I=xtl,P=0; I<xbr;I+=2,++P){
  				//copy data across				
				for (int L=J;L<std::min(ybr,J+2);++L){
					for (int K=I;K<std::min(xbr,I+2);++K){
						(mv_data->mv1)[L][K]=(split1_mv_data->mv1)[Q][P];
						(mv_data->mv2)[L][K]=(split1_mv_data->mv2)[Q][P];
						(mv_data->mode)[L][K]=(split1_mv_data->mode)[Q][P];
						(mv_data->dcY)[L][K]=(split1_mv_data->dcY)[Q][P];
					}//K
				}//L
			}//I
		}//J
	}

	// 	Case 2: prediction modes are all the same

	PredMode predmode;
	MB_cost=DoCommonMode2x2(predmode); 	
	if (MB_cost<=(mv_data->MB_costs)[ymb_loc][xmb_loc]){
		(mv_data->MB_costs)[ymb_loc][xmb_loc]=MB_cost;
		(mv_data->mb)[ymb_loc][xmb_loc].split_mode=1;//split down to 1st
		(mv_data->mb)[ymb_loc][xmb_loc].common_ref=true;
		for (int J=ytl,Q=0;J<ybr;J+=2,++Q){
			for (int I=xtl,P=0; I<xbr;I+=2,++P){
           				//copy data across				
				for (int L=J;L<std::min(ybr,J+2);++L){
					for (int K=I;K<std::min(xbr,I+2);++K){
						(mv_data->mv1)[L][K]=(split1_mv_data->mv1)[Q][P];
						(mv_data->mv2)[L][K]=(split1_mv_data->mv2)[Q][P];
						(mv_data->dcY)[L][K]=(split1_mv_data->dcY)[Q][P];
						(mv_data->mode)[L][K]=predmode;
					}//K
				}//L
			}//I
		}//J
	}
}


void ModeDecider::Do1x1Decn(){
	float MB_cost;	

	if (fsort==L1_frame)
		loc_lambda=encparams.L1_ME_lambda/factor1x1;
	else
		loc_lambda=encparams.L2_ME_lambda/factor1x1;
	if (xmb_loc==0 || ymb_loc==0 || xmb_loc==encparams.X_NUM_MB-1 || ymb_loc==encparams.Y_NUM_MB-1)
		loc_lambda/=5.0;//have reduced lambda at the picture edges

	Do1x1ME();	//begin by finding motion vectors for the whole MB, using the 
				//subMB vectors as a guide

	PredMode predmode;
	MB_cost=DoCommonMode1x1(predmode);

	if (MB_cost<=(mv_data->MB_costs)[ymb_loc][xmb_loc]){
		(mv_data->MB_costs)[ymb_loc][xmb_loc]=MB_cost;
		(mv_data->mb)[ymb_loc][xmb_loc].split_mode=0;//split down to 1st
		(mv_data->mb)[ymb_loc][xmb_loc].common_ref=true;
		for (int J=ytl;J<ybr;++J){
			for (int I=xtl; I<xbr;++I){
   				//copy data across		
				(mv_data->mv1)[J][I]=(split0_mv_data->mv1)[0][0];
				(mv_data->mv2)[J][I]=(split0_mv_data->mv2)[0][0];
				(mv_data->dcY)[J][I]=(split0_mv_data->dcY)[0][0];
				(mv_data->mode)[J][I]=predmode;
			}//I
		}//J
	}
}

void ModeDecider::Do1x1ME(){
	//motion estimation for the whole MB, using the subMB vectors as a guide
	vector<vector<MVector> > vect_list;
	BMParams matchparams;
	matchparams.up_conv=true;
	matchparams.pic_data=pic_data;
	matchparams.ref_data=ref1_updata;
	matchparams.vect_list=&vect_list;
	matchparams.Init(encparams.LumaBParams(0),xmb_loc,ymb_loc);

	matchparams.me_lambda=loc_lambda;

	vect_list.clear();
	for (int J=0;J<2;++J){
		for (int I=0;I<2;++I){
			AddNewVlist(vect_list,(split1_mv_data->mv1)[J][I],1,1);
		}//I
	}//J
	if (xtl>0 && ytl>0)
		matchparams.mv_pred=MvMedian((mv_data->mv1)[ytl][xtl-1],(mv_data->mv1)[ytl-1][xtl-1],
				(mv_data->mv1)[ytl-1][xtl]);
	else if (xtl==0 && ytl>0)
		matchparams.mv_pred=MvMean((mv_data->mv1)[ytl-1][xtl],(mv_data->mv1)[ytl-1][xtl+1]);
	else if (xtl>0 && ytl==0)
		matchparams.mv_pred=MvMean((mv_data->mv1)[ytl][xtl-1],(mv_data->mv1)[ytl+1][xtl-1]);
	else{
		matchparams.mv_pred.x=0;
		matchparams.mv_pred.y=0;
	}
	matchparams.ref_data=ref1_updata;
	(split0_mv_data->block_costs1)[0][0]=FindBestMatch(matchparams);
	(split0_mv_data->mv1)[0][0]=matchparams.best_mv;
	if (num_refs>1){//do the same for the other reference
		vect_list.clear();				
		for (int J=0;J<2;++J){
			for (int I=0;I<2;++I){
				AddNewVlist(vect_list,(split1_mv_data->mv2)[J][I],0,0);
			}//I
		}//J
		if (xtl>0 && ytl>0)
			matchparams.mv_pred=MvMedian((mv_data->mv2)[ytl][xtl-1],(mv_data->mv2)[ytl-1][xtl-1],
					(mv_data->mv2)[ytl-1][xtl]);
		else if (xtl==0 && ytl>0)
			matchparams.mv_pred=MvMean((mv_data->mv2)[ytl-1][xtl],(mv_data->mv2)[ytl-1][xtl+1]);
		else if (xtl>0 && ytl==0)
			matchparams.mv_pred=MvMean((mv_data->mv2)[ytl][xtl-1],(mv_data->mv2)[ytl+1][xtl-1]);
		else{
			matchparams.mv_pred.x=0;
			matchparams.mv_pred.y=0;
		}
		matchparams.ref_data=ref2_updata;
 		//no need to init matchparams, as already done above				
		(split0_mv_data->block_costs2)[0][0]=FindBestMatch(matchparams);
		(split0_mv_data->mv2)[0][0]=matchparams.best_mv;
	}
	(split0_mv_data->block_costs1)[0][0].total*=factor1x1;
	(split0_mv_data->block_costs2)[0][0].total*=factor1x1;
}


float ModeDecider::DoCommonMode1x1(PredMode& predmode){
   	//computes the best costs if we were to stick to a 1x1 decomposition

	float best_1x1_cost;
	float MB_cost;
	float md_cost;
	BMParams matchparams;
	matchparams.pic_data=pic_data;
	matchparams.Init(encparams.LumaBParams(0),xmb_loc,ymb_loc);
	BlockDiffParams dparams(matchparams);

 	//intra first
	ValueType dc_pred=128;
	if (xtl>0 && ytl>0)
		dc_pred=((mv_data->dcY)[ytl][xtl-1]+(mv_data->dcY)[ytl-1][xtl]+(mv_data->dcY)[ytl-1][xtl-1])/3;
	else if (xtl==0 && ytl>0)
		dc_pred=(mv_data->dcY)[ytl-1][xtl];
	else if (xtl>0 && ytl==0)
		dc_pred=(mv_data->dcY)[ytl][xtl-1];

	intradiff->Diff(dparams,dc_pred,loc_lambda);
	(split0_mv_data->block_intra_costs)[0][0]=dparams.intra_cost;
	(split0_mv_data->block_intra_costs)[0][0]*=factor1x1;
	(split0_mv_data->dcY)[0][0]=dparams.dc;

	md_cost=ModeCost(xtl,ytl,INTRA)*0.13;//multiple determined experimentally----TBD-------------------------------
	best_1x1_cost=(split0_mv_data->block_intra_costs)[0][0]+md_cost;	
	predmode=INTRA;

 	//next do ref1
	md_cost=ModeCost(xtl,ytl,REF1_ONLY)*0.13;//multiple determined experimentally----TBD-------------------------------
	MB_cost=(split0_mv_data->block_costs1)[0][0].total+md_cost;
	if (MB_cost<best_1x1_cost){
		predmode=REF1_ONLY;
		best_1x1_cost=MB_cost;
	}	
	if (num_refs>1){

		md_cost=ModeCost(xtl,ytl,REF2_ONLY)*0.13;//multiple determined experimentally----TBD-------------------------------
		MB_cost=(split0_mv_data->block_costs2)[0][0].total+md_cost;
		if (MB_cost<best_1x1_cost){
			predmode=REF2_ONLY;
			best_1x1_cost=MB_cost;
		}

		(split0_mv_data->block_bipred_costs)[0][0].mvcost=(split0_mv_data->block_costs1)[0][0].mvcost+
			(split0_mv_data->block_costs2)[0][0].mvcost;
		dparams.start_val=(split0_mv_data->block_bipred_costs)[0][0].mvcost;
		bicheckdiff->Diff(dparams,(split0_mv_data->mv1)[0][0],(split0_mv_data->mv2)[0][0]);
		(split0_mv_data->block_bipred_costs)[0][0]=dparams.cost;
		(split0_mv_data->block_bipred_costs)[0][0].total*=factor1x1;
		md_cost=ModeCost(xtl,ytl,REF1AND2)*0.13;//multiple determined experimentally----TBD-------------------------------
		MB_cost=(split0_mv_data->block_bipred_costs)[0][0].total+md_cost;

		if (MB_cost<best_1x1_cost){
			predmode=REF1AND2;
			best_1x1_cost=MB_cost;			
		}
	}
	return best_1x1_cost;
}

float ModeDecider::DoBlockDecn4x4(int xblock, int yblock){

	float block_cost;
	float md_cost;
	float min_block_cost;
	BMParams matchparams;
	matchparams.pic_data=pic_data;
	matchparams.Init(encparams.LumaBParams(2),xblock,yblock);
	BlockDiffParams dparams(matchparams);

 	//first check REF1 and REF2 costs
	md_cost=ModeCost(xblock,yblock,REF1_ONLY);
	(mv_data->mode)[yblock][xblock]=REF1_ONLY;
	min_block_cost=(mv_data->block_costs1)[yblock][xblock].total+md_cost;

	if (num_refs>1){
		md_cost=ModeCost(xblock,yblock,REF2_ONLY);
		block_cost=(mv_data->block_costs2)[yblock][xblock].total+md_cost;
		if (block_cost<min_block_cost){
			(mv_data->mode)[yblock][xblock]=REF2_ONLY;
			min_block_cost=block_cost;
		}
	}

	//next, calculate the cost if we were to code the block as intra
	md_cost=ModeCost(xblock,yblock,INTRA);
	ValueType dc_pred=128;

	if (xblock>0 && yblock>0)
		dc_pred=((mv_data->dcY)[yblock][xblock-1]+(mv_data->dcY)[yblock-1][xblock]+(mv_data->dcY)[yblock-1][xblock-1])/3;
	else if (xblock==0 && yblock>0)
		dc_pred=(mv_data->dcY)[yblock-1][xblock];
	else if (xblock>0 && yblock==0)
		dc_pred=(mv_data->dcY)[yblock][xblock-1];

	intradiff->Diff(dparams,dc_pred,loc_lambda);	
	(mv_data->block_intra_costs)[yblock][xblock]=dparams.intra_cost+md_cost;//need some multiple of md_cost TBD.................

	(mv_data->dcY)[yblock][xblock]=dparams.dc;	
	if (dparams.intra_cost<min_block_cost){
		(mv_data->mode)[yblock][xblock]=INTRA;
		min_block_cost=dparams.intra_cost;
	}

   	//finally, calculate the cost if we were to use bi-predictions
	if (num_refs>1){
		md_cost=ModeCost(xblock,yblock,REF1AND2);
		(mv_data->block_bipred_costs)[yblock][xblock].mvcost=(mv_data->block_costs1)[yblock][xblock].mvcost+
			(mv_data->block_costs2)[yblock][xblock].mvcost;
		dparams.start_val=(mv_data->block_bipred_costs)[yblock][xblock].mvcost;
		bicheckdiff->Diff(dparams,(mv_data->mv1)[yblock][xblock],(mv_data->mv1)[yblock][xblock]);
		(mv_data->block_bipred_costs)[yblock][xblock]=dparams.cost;		
		dparams.cost.total+=md_cost;
		if (dparams.cost.total<min_block_cost){
			(mv_data->mode)[yblock][xblock]=REF1AND2;
			min_block_cost=dparams.cost.total;			
		}		
	}
	return min_block_cost;
}

float ModeDecider::DoCommonMode4x4(PredMode& predmode){

	float MB_cost;
	float best_4x4_cost;
	//start with the intra cost
	MB_cost=0.0;	
	for (int J=ytl;J<ybr;++J){
		for (int I=xtl;I<xbr;++I){
			MB_cost+=(mv_data->block_intra_costs)[J][I];
		}//I
	}//J
	MB_cost+=ModeCost(xtl,ytl,INTRA)*0.13;//multiple determined experimentally -- TBD -------------------------------
	predmode=INTRA;
	best_4x4_cost=MB_cost;

	//next do ref1	
	MB_cost=0.0;	
	for (int J=ytl;J<ybr;++J){
		for (int I=xtl;I<xbr;++I){
			MB_cost+=(mv_data->block_costs1)[J][I].total;
		}//I
	}//J
	MB_cost+=ModeCost(xtl,ytl,REF1_ONLY)*0.13;//multiple determined experimentally -- TBD -------------------------------

	if (MB_cost<best_4x4_cost){
		predmode=REF1_ONLY;
		best_4x4_cost=MB_cost;
	}

	if (num_refs>1){
  		//next do ref2	
		MB_cost=0.0;	
		for (int J=ytl;J<ybr;++J){
			for (int I=xtl;I<xbr;++I){
				MB_cost+=(mv_data->block_costs2)[J][I].total;
			}//I
		}//J
		MB_cost+=ModeCost(xtl,ytl,REF2_ONLY)*0.13;//multiple determined experimentally -- TBD -------------------------------
		if (MB_cost<best_4x4_cost){
			predmode=REF2_ONLY;
			best_4x4_cost=MB_cost;
		}
  		//finally do ref 1 and 2	
		MB_cost=0.0;	
		for (int J=ytl;J<ybr;++J){
			for (int I=xtl;I<xbr;++I){
				MB_cost+=(mv_data->block_bipred_costs)[J][I].total;
			}//I
		}//J
		MB_cost+=ModeCost(xtl,ytl,REF1AND2)*0.13;//multiple determined experimentally -- TBD -------------------------------		
		if (MB_cost<best_4x4_cost){
			predmode=REF1AND2;
			best_4x4_cost=MB_cost;
		}
	}

	return best_4x4_cost;
}

void ModeDecider::Do2x2ME(){
	//does the motion estimation for the sub-Macroblocks (subMBs), using the block vectors as guides

	vector<vector<MVector> > vect_list;
	BMParams matchparams;
	matchparams.up_conv=true;
	matchparams.pic_data=pic_data;
	matchparams.ref_data=ref1_updata;
	matchparams.vect_list=&vect_list;
	matchparams.me_lambda=loc_lambda/factor2x2;//take into account the block overlaps in the ME lagrangian param

	for (int J=ytl,Q=0;J<ybr;J+=2,++Q){
		for (int I=xtl,P=0; I<xbr;I+=2,++P){
			vect_list.clear();
			//use the 4 corresponding block vectors as a guide
			for (int L=J,V=0;L<ybr && V<2;++L,++V){
				for (int K=I,U=0;K<xbr && U<2;++K,++U){
					AddNewVlist(vect_list,(mv_data->mv1)[L][K],1,1);
				}//K
			}//L
			if (I>0 && J>0)
				matchparams.mv_pred=MvMedian((mv_data->mv1)[J][I-1],(mv_data->mv1)[J-1][I-1],
						(mv_data->mv1)[J-1][I]);
			else if (I==0 && J>0)
				matchparams.mv_pred=MvMean((mv_data->mv1)[J-1][I],(mv_data->mv1)[J-1][I+1]);
			else if (I>0 && J==0)
				matchparams.mv_pred=MvMean((mv_data->mv1)[J][I-1],(mv_data->mv1)[J+1][I-1]);
			else{
				matchparams.mv_pred.x=0;
				matchparams.mv_pred.y=0;
			}			
			matchparams.ref_data=ref1_updata;
			matchparams.Init(encparams.LumaBParams(1),I>>1,J>>1);//divided by 2 as block separations are twice as big
			(split1_mv_data->block_costs1)[Q][P]=FindBestMatch(matchparams);			
			(split1_mv_data->mv1)[Q][P]=matchparams.best_mv;
			if (num_refs>1){//do the same for the other reference
				vect_list.clear();				
				for (int V=0,L=J;L<ybr && V<2;++L,++V){
					for (int K=I,U=0;K<xbr && U<2;++K,++U){
						AddNewVlist(vect_list,(mv_data->mv2)[L][K],1,1);
					}//K
				}//L
				if (I>0 && J>0)
					matchparams.mv_pred=MvMedian((mv_data->mv2)[J][I-1],(mv_data->mv2)[J-1][I-1],
							(mv_data->mv2)[J-1][I]);
				else if (I==0 && J>0)
					matchparams.mv_pred=MvMean((mv_data->mv2)[J-1][I],(mv_data->mv2)[J-1][I+1]);
				else if (I>0 && J==0)
					matchparams.mv_pred=MvMean((mv_data->mv2)[J][I-1],(mv_data->mv2)[J+1][I-1]);
				else{
					matchparams.mv_pred.x=0;
					matchparams.mv_pred.y=0;
				}
				matchparams.ref_data=ref2_updata;
  				//no need to init the matchparams, as already done above				
				(split1_mv_data->block_costs2)[Q][P]=FindBestMatch(matchparams);
				(split1_mv_data->mv2)[Q][P]=matchparams.best_mv;
			}
			(split1_mv_data->block_costs1)[Q][P].total*=factor2x2;
			(split1_mv_data->block_costs2)[Q][P].total*=factor2x2;
		}//I
	}//J	

}

float ModeDecider::DoBlockDecn2x2(int xsubMB, int ysubMB){
 	// decides on the best mode for each sub-MB 

	const int xblock=xtl+xsubMB*2;//block position of
	const int yblock=ytl+ysubMB*2;//TL 

	float subMB_cost;
	float min_subMB_cost;
	float md_cost;
	BMParams matchparams;
	matchparams.pic_data=pic_data;
	matchparams.Init(encparams.LumaBParams(1),xsubMBtl+xsubMB,ysubMBtl+ysubMB);
	BlockDiffParams dparams(matchparams);

   	//first check REF1
	(split1_mv_data->mode)[ysubMB][xsubMB]=REF1_ONLY;
	md_cost=ModeCost(xblock,yblock,REF1_ONLY)*0.36;//multiple determined experimentally----TBD-------------------------------
	min_subMB_cost=(split1_mv_data->block_costs1)[ysubMB][xsubMB].total+md_cost;	

	//next, calculate the cost if we were to code the block as intra
	ValueType dc_pred=128;
	if (xblock>0 && yblock>0)
		dc_pred=((mv_data->dcY)[yblock][xblock-1]+(mv_data->dcY)[yblock-1][xblock]+(mv_data->dcY)[yblock-1][xblock-1])/3;
	else if (xblock==0 && yblock>0)
		dc_pred=(mv_data->dcY)[yblock-1][xblock];
	else if (xblock>0 && yblock==0)
		dc_pred=(mv_data->dcY)[yblock][xblock-1];	

	intradiff->Diff(dparams,dc_pred,loc_lambda/factor2x2);
	(split1_mv_data->block_intra_costs)[ysubMB][xsubMB]=dparams.intra_cost*factor2x2;
	(split1_mv_data->dcY)[ysubMB][xsubMB]=dparams.dc;
	md_cost=ModeCost(xblock,yblock,INTRA)*0.36;//multiple determined experimentally----TBD-------------------------------
	subMB_cost=(split1_mv_data->block_intra_costs)[ysubMB][xsubMB]+md_cost;

	if (subMB_cost<min_subMB_cost){
		(split1_mv_data->mode)[ysubMB][xsubMB]=INTRA;
		min_subMB_cost=subMB_cost;
	}

//finally, calculate the costs if we have another reference
	if (num_refs>1){
		md_cost=ModeCost(xblock,yblock,REF2_ONLY)*0.36;//multiple determined experimentally----TBD-------------------------------
		subMB_cost=(split1_mv_data->block_costs2)[ysubMB][xsubMB].total+md_cost;

		if (subMB_cost<min_subMB_cost){
			(split1_mv_data->mode)[ysubMB][xsubMB]=REF2_ONLY;
			min_subMB_cost=subMB_cost;
		}

		(split1_mv_data->block_bipred_costs)[ysubMB][xsubMB].mvcost=(split1_mv_data->block_costs1)[ysubMB][xsubMB].mvcost+
			(split1_mv_data->block_costs2)[ysubMB][xsubMB].mvcost;
		dparams.start_val=(split1_mv_data->block_bipred_costs)[ysubMB][xsubMB].mvcost;
		bicheckdiff->Diff(dparams,(split1_mv_data->mv1)[ysubMB][xsubMB],(split1_mv_data->mv2)[ysubMB][xsubMB]);
		(split1_mv_data->block_bipred_costs)[ysubMB][xsubMB]=dparams.cost;
		(split1_mv_data->block_bipred_costs)[ysubMB][xsubMB].total*=factor2x2;
		md_cost=ModeCost(xblock,yblock,REF1AND2)*0.36;//multiple determined experimentally----TBD-------------------------------		
		subMB_cost=(split1_mv_data->block_bipred_costs)[ysubMB][xsubMB].total+md_cost;		

		if (subMB_cost<min_subMB_cost){
			(split1_mv_data->mode)[ysubMB][xsubMB]=REF1AND2;
			min_subMB_cost=subMB_cost;			
		}		

	}
	return min_subMB_cost;
}

float ModeDecider::DoCommonMode2x2(PredMode& predmode){
	float MB_cost;
	float best_2x2_cost;

  	//start with the intra cost
	MB_cost=ModeCost(xtl,ytl,INTRA)*0.13;//multiple determined experimentally----TBD-------------------------------	
	for (int J=0;J<2;++J){
		for (int I=0;I<2;++I){
			MB_cost+=(split1_mv_data->block_intra_costs)[J][I];			
		}//I
	}//J

	predmode=INTRA;
	best_2x2_cost=MB_cost;

  	//next do ref1	
	MB_cost=ModeCost(xtl,ytl,REF1_ONLY)*0.13;//multiple determined experimentally----TBD-------------------------------	
	for (int J=0;J<2;++J){
		for (int I=0;I<2;++I){
			MB_cost+=(split1_mv_data->block_costs1)[J][I].total;
		}//I
	}//J

	if (MB_cost<best_2x2_cost){
		predmode=REF1_ONLY;
		best_2x2_cost=MB_cost;
	}


	if (num_refs>1){
  		//next do ref2	
		MB_cost=ModeCost(xtl,ytl,REF2_ONLY)*0.13;//multiple determined experimentally----TBD-------------------------------	
		for (int J=0;J<2;++J){
			for (int I=0;I<2;++I){
				MB_cost+=(split1_mv_data->block_costs2)[J][I].total;
			}//I
		}//J


		if (MB_cost<best_2x2_cost){
			predmode=REF2_ONLY;
			best_2x2_cost=MB_cost;
		}
  		//finally do ref 1 and 2	
		MB_cost=ModeCost(xtl,ytl,REF1AND2)*0.13;//multiple determined experimentally----TBD-------------------------------	
		for (int J=0;J<2;++J){
			for (int I=0;I<2;++I){
				MB_cost+=(split1_mv_data->block_bipred_costs)[J][I].total;
			}//I
		}//J

		if (MB_cost<best_2x2_cost){
			predmode=REF1AND2;
			best_2x2_cost=MB_cost;
		}
	}

	return best_2x2_cost;
}
