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

#include "libdirac_motionest/block_match.h"
#include <cmath>

using std::vector;

MvCostData FindBestMatch(BMParams& matchparams){

	BlockDiffParams dparams(matchparams);
	BlockDiff* mydiff;
	BlockDiff* simplediff;
	BlockDiff* checkdiff;

	const PicArray& pic_data=*(matchparams.pic_data);
	const PicArray& ref_data=*(matchparams.ref_data);
	if (matchparams.up_conv==true){
		simplediff=new SimpleBlockDiffUp(ref_data,pic_data);
		checkdiff=new BChkBlockDiffUp(ref_data,pic_data);				
	}
	else{
		simplediff=new SimpleBlockDiff(ref_data,pic_data);
		checkdiff=new BChkBlockDiff(ref_data,pic_data);
	}

	MVector temp_mv;
	vector<vector<MVector> >& vect_list=*(matchparams.vect_list);
	float lambda=matchparams.me_lambda;

   	//now test against the offsets in the MV list to get the lowest cost//
  	//////////////////////////////////////////////////////////////////////	

	vector<int> list_nums;//numbers of the lists to do more searching in
	OneDArray<float> list_costs(matchparams.vect_list->size());//costs of the initial vectors in each list
	float min_cost;	

   	//first test the first in each of the lists to choose which lists to pursue
	dparams.cost.total=100000000.0f;//initialise so that we choose a valid vector to start with!
	dparams.best_mv=vect_list[0][0];
	for (unsigned int L=0;L<vect_list.size();++L){
		temp_mv=vect_list[L][0];
		dparams.start_val=lambda*GetVar(matchparams.mv_pred,temp_mv);
		if (matchparams.up_conv){
			if (   ((dparams.xp<<1)+(temp_mv.x>>2))<0 
				|| (((dparams.xp+dparams.xl)<<1)+(temp_mv.x>>2))>=ref_data.length(0)
				|| ((dparams.yp<<1)+(temp_mv.y>>2))<0 
				|| (((dparams.yp+dparams.yl)<<1)+(temp_mv.y>>2))>=ref_data.length(1) )
				mydiff=checkdiff;
			else
				mydiff=simplediff;	
		}
		else{
			if ((dparams.xp+temp_mv.x)<0 || (dparams.xp+dparams.xl+temp_mv.x)>ref_data.length(0) ||
				(dparams.yp+temp_mv.y)<0 || (dparams.yp+dparams.yl+temp_mv.y)>ref_data.length(1) ){
				mydiff=checkdiff;
			}
			else{
				mydiff=simplediff;
			}			
		}
		mydiff->Diff(dparams,temp_mv);
		list_costs[L]=dparams.cost.total;
	}//L

	//select which lists we're going to use
	min_cost=list_costs[0];
	for (int L=1;L<list_costs.length();++L){
		if (list_costs[L]<min_cost)
			min_cost=list_costs[L];
	}//L
	for (int L=0;L<list_costs.length();++L){
		if (list_costs[L]<1.5*min_cost){//value of 1.5 tbd. Only do lists whose 1st element isn't too far off best
			list_nums.push_back(L);
		}
	}//L

   	//Ok, now we know which lists to pursue. Just go through all of them
	int lnum;

	for (unsigned int N=0;N<list_nums.size();++N){
		lnum=list_nums[N];
		for (unsigned int I=1;I<vect_list[lnum].size();++I){//start at 1 since did 0 above
			temp_mv=vect_list[lnum][I];
			dparams.start_val=lambda*GetVar(matchparams.mv_pred,temp_mv);
			if (matchparams.up_conv){
				if (   ((dparams.xp<<1)+(temp_mv.x>>2))<0 
					|| (((dparams.xp+dparams.xl)<<1)+(temp_mv.x>>2))>=ref_data.length(0)
					|| ((dparams.yp<<1)+(temp_mv.y>>2))<0 
					|| (((dparams.yp+dparams.yl)<<1)+(temp_mv.y>>2))>=ref_data.length(1) )
					mydiff=checkdiff;
				else
					mydiff=simplediff;
			}
			else{			
				if ((dparams.xp+temp_mv.x)<0 || (dparams.xp+dparams.xl+temp_mv.x)>ref_data.length(0) ||
					(dparams.yp+temp_mv.y)<0 || (dparams.yp+dparams.yl+temp_mv.y)>ref_data.length(1) )
					mydiff=checkdiff;
				else
					mydiff=simplediff;
			}
			mydiff->Diff(dparams,temp_mv);
		}//I
	}//N

	matchparams.best_mv=dparams.best_mv;

	delete checkdiff;
	delete simplediff;

	return dparams.cost;
}	

void FindBestMatchSubp(BMParams& matchparams,const MVector& pel_mv,MvCostData& block_cost){
	//as find_best_match, but does sub-pel refinement from a pixel-accurate motion vector
	//The reference is upconverted by a factor 2 in each dim, and vectors are computed to 1/8 pel
	//accuracy by extension with linear interpolation

	int list_idx=0;//index of the current list we're working on	
	BlockDiffParams dparams(matchparams);
	dparams.bailout=true;
	BlockDiff* mydiff;

	const PicArray& pic_data=*(matchparams.pic_data);
	const PicArray& ref_data=*(matchparams.ref_data);		
	SimpleBlockDiffUp* simplediff=new SimpleBlockDiffUp(ref_data,pic_data);
	BChkBlockDiffUp* checkdiff=new BChkBlockDiffUp(ref_data,pic_data);

	MVector temp_mv;
	vector<vector<MVector> >vect_list;
	float lambda=matchparams.me_lambda;

	//now test against the offsets in the MV list to get the lowest cost//
  	//////////////////////////////////////////////////////////////////////	

	dparams.best_mv.x=pel_mv.x<<3;
	dparams.best_mv.y=pel_mv.y<<3;
	AddNewVlist(vect_list,dparams.best_mv,0,0,1);	//creates a singleton list at position 0

 	//Recalculate the costs for pel-accurate vector given that we have the true predictor
	dparams.cost=block_cost;
	dparams.cost.mvcost=lambda*GetVar(matchparams.mv_pred,dparams.best_mv);//1/8th pel, remember! So divide by 8
	dparams.cost.total=dparams.cost.SAD+dparams.cost.mvcost;
	list_idx++;

	AddNewVlist(vect_list,dparams.best_mv,1,1,4);	//creates a list of all 1/2-pel vectors neighbouring
  													//pel_mv but not pel_mv itself. Will search this.	
 	//Next, go through list 1, which is 1/2-pel offsets
	for (unsigned int I=0;I<vect_list[list_idx].size();++I){
		temp_mv=vect_list[list_idx][I];
		dparams.start_val=lambda*GetVar(matchparams.mv_pred,temp_mv);
		if (   ((dparams.xp<<1)+(temp_mv.x>>2))<0 
			|| (((dparams.xp+dparams.xl)<<1)+(temp_mv.x>>2))>=ref_data.length(0)
			|| ((dparams.yp<<1)+(temp_mv.y>>2))<0 
			|| (((dparams.yp+dparams.yl)<<1)+(temp_mv.y>>2))>=ref_data.length(1) )
			mydiff=checkdiff;
		else
			mydiff=simplediff;
		mydiff->Diff(dparams,temp_mv);
	}//I

 	//next, the 1/4-pel offsets
	AddNewVlist(vect_list,dparams.best_mv,1,1,2);
	list_idx++;
	for (unsigned int I=0;I<vect_list[list_idx].size();++I){
		temp_mv=vect_list[list_idx][I];
		dparams.start_val=lambda*GetVar(matchparams.mv_pred,temp_mv);
		if (   ((dparams.xp<<1)+(temp_mv.x>>2))<0 
			|| (((dparams.xp+dparams.xl)<<1)+(temp_mv.x>>2))>=ref_data.length(0)
			|| ((dparams.yp<<1)+(temp_mv.y>>2))<0 
			|| (((dparams.yp+dparams.yl)<<1)+(temp_mv.y>>2))>=ref_data.length(1) )
			mydiff=checkdiff;
		else
			mydiff=simplediff;
		mydiff->Diff(dparams,temp_mv);
	}//I

 	//then, the 1/8-pel offsets
	AddNewVlist(vect_list,dparams.best_mv,1,1,1);

	list_idx++;
	for (unsigned int I=0;I<vect_list[list_idx].size();++I){
		temp_mv=vect_list[list_idx][I];
		dparams.start_val=lambda*GetVar(matchparams.mv_pred,temp_mv);
		if (   ((dparams.xp<<1)+(temp_mv.x>>2))<0 
			|| (((dparams.xp+dparams.xl)<<1)+(temp_mv.x>>2))>=ref_data.length(0)
			|| ((dparams.yp<<1)+(temp_mv.y>>2))<0 
			|| (((dparams.yp+dparams.yl)<<1)+(temp_mv.y>>2))>=ref_data.length(1) )
			mydiff=checkdiff;
		else
			mydiff=simplediff;
		mydiff->Diff(dparams,temp_mv);
	}//I

 	//Also use the guide vector as a starting point, also, for last searches	
	AddNewVlist(vect_list,matchparams.mv_pred,1,1,1);
	if (vect_list.size()>4){//we might not have successfully added anything
		list_idx++;
		for (unsigned int I=0;I<vect_list[list_idx].size();++I){
			temp_mv=vect_list[list_idx][I];
			dparams.start_val=lambda*GetVar(matchparams.mv_pred,temp_mv);
			if (   ((dparams.xp<<1)+(temp_mv.x>>2))<0 
				|| (((dparams.xp+dparams.xl)<<1)+(temp_mv.x>>2))>=ref_data.length(0)
				|| ((dparams.yp<<1)+(temp_mv.y>>2))<0 
				|| (((dparams.yp+dparams.yl)<<1)+(temp_mv.y>>2))>=ref_data.length(1) )
				mydiff=checkdiff;
			else
				mydiff=simplediff;
			mydiff->Diff(dparams,temp_mv);
		}//I
	}

	matchparams.best_mv=dparams.best_mv;

	delete checkdiff;
	delete simplediff;

	block_cost=dparams.cost;
}	

ValueType GetVar(const MVector& predmv,const MVector& mv){
	MVector diff;
	diff.x=mv.x-predmv.x;
	diff.y=mv.y-predmv.y;	
	return Norm1(diff);
}

ValueType GetVar(const std::vector<MVector>& pred_list,const MVector& mv){
	ValueType sum=0;
	MVector diff;
	for (unsigned int I=0;I<pred_list.size();++I){
		diff.x=mv.x-pred_list[I].x;
		diff.y=mv.y-pred_list[I].y;
		sum+=Norm1(diff);
	}
	return sum;
}


float GetModeVar(const MvData* mv_data,int xindex,int yindex, PredMode predmode,float loc_lambda){
	//computes the variation of the given mode, predmode, from its immediate neighbours
	//Currently, includes branches to cope with blocks on the edge of the picture.
	int I,J;
	float diff;
	float var=0.0;

	I=xindex-1;J=yindex;
	if (I>=0){
		diff=float((mv_data->mode)[J][I]-predmode);
		var=std::abs(diff);
	}

	I=xindex-1;J=yindex-1;
	if (I>=0 && J>=0){
		diff=float((mv_data->mode)[J][I]-predmode);
		var+=std::abs(diff);
	}

	I=xindex;J=yindex-1;
	if (J>=0){
		diff=float((mv_data->mode)[J][I]-predmode);
		var+=std::abs(diff);
	}

	return 2.0*var*loc_lambda;//multiple determined experimentally
}

void AddNewVlist(vector<vector<MVector> >& vect_list, const MVector& mv, int xr, int yr, int step){
  	//Creates a new motion vector list in a square region around mv

	vector<MVector> tmp_list;
	vect_list.push_back(tmp_list);
	int list_num=vect_list.size()-1;	

	MVector tmp_mv;

	tmp_mv=mv;
	AddVect(vect_list,tmp_mv,list_num);
	for (int I=1;I<=xr;++I){
		tmp_mv.x=mv.x+I*step;
		AddVect(vect_list,tmp_mv,list_num);
		tmp_mv.x=mv.x-I*step;		
		AddVect(vect_list,tmp_mv,list_num);	
	}
	for (int J=1;J<=yr;++J){
		for (int I=-xr;I<=xr;++I){
			tmp_mv.x=mv.x+I*step;
			tmp_mv.y=mv.y+J*step;
			AddVect(vect_list,tmp_mv,list_num);
			tmp_mv.y=mv.y-J*step;
			AddVect(vect_list,tmp_mv,list_num);			
		}		
	}
	if (vect_list[list_num].size()==0)				//if we've not managed to add any element to the list
		vect_list.erase(vect_list.begin()+list_num);//remove the list so we don't ever have to check its size
}

void AddNewVlist(vector<vector<MVector> >& vect_list, const MVector& mv, int xr, int yr){
  	//Creates a new motion vector list in a square region around mv
	//add_new_vlist(vect_list,mv,xr,yr,step);	
	vector<MVector> tmp_list;
	vect_list.push_back(tmp_list);
	int list_num=vect_list.size()-1;	

	MVector tmp_mv;

	tmp_mv=mv;
	AddVect(vect_list,tmp_mv,list_num);
	for (int I=1;I<=xr;++I){
		tmp_mv.x=mv.x+I;
		AddVect(vect_list,tmp_mv,list_num);
		tmp_mv.x=mv.x-I;		
		AddVect(vect_list,tmp_mv,list_num);	
	}
	for (int J=1;J<=yr;++J){
		for (int I=-xr;I<=xr;++I){
			tmp_mv.x=mv.x+I;
			tmp_mv.y=mv.y+J;
			AddVect(vect_list,tmp_mv,list_num);
			tmp_mv.y=mv.y-J;
			AddVect(vect_list,tmp_mv,list_num);			
		}		
	}
	if (vect_list[list_num].size()==0)				//if we've not managed to add any element to the list
		vect_list.erase(vect_list.begin()+list_num);//remove the list so we don't ever have to check its size
}

void AddNewVlistD(vector<vector<MVector> >& vect_list, const MVector& mv, int xr, int yr){
  	//As above, but using a diamond pattern

	vector<MVector> tmp_list;
	vect_list.push_back(tmp_list);
	int list_num=vect_list.size()-1;	

	MVector tmp_mv;
	int xlim;

	tmp_mv=mv;
	AddVect(vect_list,tmp_mv,list_num);
	for (int I=1;I<=xr;++I){
		tmp_mv.x=mv.x+I;
		AddVect(vect_list,tmp_mv,list_num);
		tmp_mv.x=mv.x-I;		
		AddVect(vect_list,tmp_mv,list_num);	
	}
	for (int J=1;J<=yr;++J){
		xlim=xr*(yr-std::abs(J))/yr;		
		for (int I=-xlim;I<=xlim;++I){
			tmp_mv.x=mv.x+I;
			tmp_mv.y=mv.y+J;
			AddVect(vect_list,tmp_mv,list_num);
			tmp_mv.y=mv.y-J;
			AddVect(vect_list,tmp_mv,list_num);			
		}		
	}
	if (vect_list[list_num].size()==0)				//if we've not managed to add any element to the list
		vect_list.erase(vect_list.begin()+list_num);//remove the list so we don't ever have to check its size
}

void AddVect(vector<vector<MVector> >& vect_list,const MVector& mv,int list_num){

	bool is_in_list=false;	
	unsigned int L=0;unsigned int I;	
	while(!is_in_list && L<vect_list.size()){
		I=0;		
		while(!is_in_list && I<vect_list[L].size()){		
			if (vect_list[L][I].x==mv.x && vect_list[L][I].y==mv.y)	
				is_in_list=true;		
			++I;	
		}
		++L;
	}

	if (!is_in_list){
		vect_list[list_num].push_back(mv);
	}
}
