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

///////////////////////////////////
//-------------------------------//
//utilities for motion estimation//
//-------------------------------//
///////////////////////////////////

#include <libdirac_motionest/me_utils.h>
#include <libdirac_common/common.h>
#include <algorithm>

void BMParams::Init(const OLBParams& bparams,int M, int N)
{
	bp=bparams;
	Init(M,N);
}
void BMParams::Init(int M, int N)
{

	const int xpos=M*bp.Xbsep()-bp.Xoffset();
	const int ypos=N*bp.Ybsep()-bp.Yoffset();

	xp=std::max(xpos,0);//TL corner of 
	yp=std::max(ypos,0);//block to be matched
	xl=bp.Xblen()-xp+xpos;
	yl=bp.Yblen()-yp+ypos;

 	//constrain block lengths to fall within the picture
	xl=( (xp+xl-1) > (pic_data->LastX()) ) ? (pic_data->LastX()+1-xp) : xl;
	yl=( (yp+yl-1) > (pic_data->LastY()) ) ? (pic_data->LastY()+1-yp) : yl;
	me_lambda/=(bp.Xblen()*bp.Yblen());
	me_lambda*=(xl*yl);//if I've shrunk the block I need to shrink the weight I apply to the entropy measure	
}

void SimpleBlockDiff::Diff(BlockDiffParams& dparams, const MVector& mv){

	TwoDArray<ValueType> diff(dparams.yl , dparams.xl);	
	float sum=dparams.start_val;
	for (int J=dparams.yp , L=0 ; J != dparams.yp+dparams.yl ; ++J , ++L )
    {
		for(int I=dparams.xp , K=0 ; I!= dparams.xp+dparams.xl ; ++I , ++K )
        {
			diff[L][K] = (*pic_data)[J][I]-(*ref_data)[J+mv.y][I+mv.x];
		}// I, K
	}// J, L

	if (dparams.bailout){//I'm allowed to bail out early if I can
		for (int J=0;J!=dparams.yl;++J){
			for(int I=0;I!=dparams.xl;++I){
				sum+=float(abs(diff[J][I]));
				if (dparams.cost.total<sum)
					return;					
			}//I
		}//J
		//if I've got here then I must have done better than before, so set the MV etc
		dparams.cost.total=sum;
		dparams.cost.mvcost=dparams.start_val;
		dparams.cost.SAD=sum-dparams.cost.mvcost;
		dparams.best_mv=mv;

	}
	else{//I'm not allowed to bail out
		for (int J=0;J!=dparams.yl;++J)
			for(int I=0;I!=dparams.xl;++I)
				sum+=float(abs(diff[J][I]));

   		//now check whether I've done better ...
		if (sum<dparams.cost.total){
			dparams.cost.total=sum;
			dparams.cost.mvcost=dparams.start_val;			
			dparams.cost.SAD=sum-dparams.cost.mvcost;
			dparams.best_mv=mv;
		}
	}
}

void BChkBlockDiff::Diff(BlockDiffParams& dparams, const MVector& mv){

	const int xmax=ref_data->LengthX();
	const int ymax=ref_data->LengthY();
	TwoDArray<ValueType> diff(dparams.yl , dparams.xl);
	float sum=dparams.start_val;
	for (int J=dparams.yp,L=0;J!=dparams.yp+dparams.yl;++J,++L){
		for(int I=dparams.xp,K=0;I!=dparams.xp+dparams.xl;++I,++K){
			diff[L][K]=(*pic_data)[J][I]-(*ref_data)[BChk(J+mv.y,ymax)][BChk(I+mv.x,xmax)];
		}
	}

	if (dparams.bailout){//I'm allowed to bail out early if I can
		for (int J=0;J!=dparams.yl;++J){
			for(int I=0;I!=dparams.xl;++I){
				sum+=float(abs(diff[J][I]));
				if (dparams.cost.total<sum)
					return;					
			}//I
		}//J
		//if I've got here then I must have done better than before, so set the MV etc
		dparams.cost.total=sum;
		dparams.cost.mvcost=dparams.start_val;
		dparams.cost.SAD=sum-dparams.cost.mvcost;
		dparams.best_mv=mv;

	}
	else{//I'm not allowed to bail out		
		for (int J=0;J!=dparams.yl;++J)
			for(int I=0;I!=dparams.xl;++I){
				sum+=float(abs(diff[J][I]));

			}
  		//now check whether I've done better ...
		if (sum<dparams.cost.total){
			dparams.cost.total=sum;
			dparams.cost.mvcost=dparams.start_val;			
			dparams.cost.SAD=sum-dparams.cost.mvcost;
			dparams.best_mv=mv;
		}
	}
}

void IntraBlockDiff::Diff(BlockDiffParams& dparams,ValueType dc_pred,float loc_lambda){

 	//computes the cost if block is predicted by its dc component

	int dc=0;

	for (int J=dparams.yp;J!=dparams.yp+dparams.yl;++J)
		for(int I=dparams.xp;I!=dparams.xp+dparams.xl;++I)
			dc+=int((*pic_data)[J][I]);

	dparams.dc=ValueType(dc/(dparams.xl*dparams.yl));	
	dparams.dc=(dparams.dc+2)>>2;	//just give dc to 8-bit accuracy

	dparams.intra_cost=float(abs(dparams.dc-dc_pred))*loc_lambda;
	for (int J=dparams.yp;J!=dparams.yp+dparams.yl;++J)
		for(int I=dparams.xp;I!=dparams.xp+dparams.xl;++I)
			dparams.intra_cost+=float(abs((*pic_data)[J][I]-(dparams.dc<<2)));
}

void BiSimpleBlockDiff::Diff(BlockDiffParams& dparams, const MVector& mv1,const MVector& mv2){

	TwoDArray<ValueType> diff(dparams.yl , dparams.xl);

	dparams.cost.mvcost=dparams.start_val;
	dparams.cost.SAD=0.0;

	for (int J=dparams.yp,L=0;J!=dparams.yp+dparams.yl;++J,++L){
		for(int I=dparams.xp,K=0;I!=dparams.xp+dparams.xl;++I,++K){
			diff[L][K]=(*pic_data)[J][I]-(((*ref_data)[J+mv1.y][I+mv1.x]+1)>>1);
			diff[L][K]-=(((*ref_data2)[J+mv2.y][I+mv2.x]+1)>>1);
		}//I
	}//J

	for (int J=0;J!=dparams.yl;++J)
		for(int I=0;I!=dparams.xl;++I)
			dparams.cost.SAD+=float(abs(diff[J][I]));

	dparams.cost.total=dparams.cost.mvcost+dparams.cost.SAD;
}

void BiBChkBlockDiff::Diff(BlockDiffParams& dparams, const MVector& mv1,const MVector& mv2){

	TwoDArray<ValueType> diff(dparams.yl , dparams.xl);
	const int xmax1=ref_data->LengthX();
    int ymax1=ref_data->LengthY();

	const int xmax2=ref_data2->LengthX();
	int ymax2=ref_data2->LengthY();

	dparams.cost.mvcost=dparams.start_val;
	dparams.cost.SAD=0.0;

	for (int J=dparams.yp,L=0;J!=dparams.yp+dparams.yl;++J,++L){
		for(int I=dparams.xp,K=0;I!=dparams.xp+dparams.xl;++I,++K){
			diff[L][K]=(*pic_data)[J][I]-(((*ref_data)[BChk(J+mv1.y,ymax1)][BChk(I+mv1.x,xmax1)]+1)>>1);
			diff[L][K]-=(((*ref_data2)[BChk(J+mv2.y,ymax2)][BChk(I+mv2.x,xmax2)]+1)>>1);
		}//I
	}//J


	for (int J=0;J!=dparams.yl;++J){
		for(int I=0;I!=dparams.xl;++I){
			dparams.cost.SAD+=float(abs(diff[J][I]));
		}//I
	}//J
	dparams.cost.total=dparams.cost.mvcost+dparams.cost.SAD;
}

void SimpleBlockDiffUp::Diff(BlockDiffParams& dparams, const MVector& mv){

	//Coordinates in the image being written to
	const ImageCoords StartPos(dparams.xp,dparams.yp);
	const ImageCoords EndPos(StartPos.x+dparams.xl,StartPos.y+dparams.yl);

	//the rounded motion vectors, accurate to 1/2 pel
	//NB: bitshift rounds negative numbers DOWN, as required	
	const MVector roundvec(mv.x>>2,mv.y>>2);

	//remainder, giving 1/8 pel accuracy, needed for linear interp
	const MVector rmdr(mv.x-(roundvec.x<<2),mv.y-(roundvec.y<<2));

	//Set up the start point in the reference image.	
	const ImageCoords RefStart((StartPos.x<<1) + roundvec.x,(StartPos.y<<1) + roundvec.y);


	//weights for doing linear interpolation, calculated from the remainder values

	const ValueType	TLweight((4-rmdr.x)*(4-rmdr.y));
	const ValueType	TRweight(rmdr.x*(4-rmdr.y));
	const ValueType	BLweight((4-rmdr.x)*rmdr.y);
	const ValueType	BRweight(rmdr.x*rmdr.y);	

	float sum=dparams.start_val;

	ValueType temp;//Temporary Variable.	

	for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
		for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
			temp = (TLweight*(*ref_data)[uY][uX] +
					TRweight*(*ref_data)[uY][uX+1] +
					BLweight*(*ref_data)[uY+1][uX] +
					BRweight*(*ref_data)[uY+1][uX+1] +
					8)>>4;

			sum+=abs((*pic_data)[c][l]-temp);
		}//l
	}//c

	if (sum<dparams.cost.total){
		dparams.cost.total=sum;
		dparams.cost.mvcost=dparams.start_val;			
		dparams.cost.SAD=sum-dparams.cost.mvcost;
		dparams.best_mv=mv;			
	}	
}

void BChkBlockDiffUp::Diff(BlockDiffParams& dparams, const MVector& mv){

	//the picture sizes
	const int DoubleXdim=ref_data->LengthX();
	const int DoubleYdim=ref_data->LengthY();

	//Coordinates in the image being written to
	const ImageCoords StartPos(dparams.xp,dparams.yp);
	const ImageCoords EndPos(StartPos.x+dparams.xl,StartPos.y+dparams.yl);

	//the rounded motion vectors, accurate to 1/2 pel
	//NB: bitshift rounds negative numbers DOWN, as required	
	const MVector roundvec(mv.x>>2,mv.y>>2);

	//remainder, giving 1/8 pel accuracy, needed for linear interp
	const MVector rmdr(mv.x-(roundvec.x<<2),mv.y-(roundvec.y<<2));

	//Set up the start point in the reference image.	
	const ImageCoords RefStart((StartPos.x<<1) + roundvec.x,(StartPos.y<<1) + roundvec.y);


	//weights for doing linear interpolation, calculated from the remainder values

	const ValueType	TLweight((4-rmdr.x)*(4-rmdr.y));
	const ValueType	TRweight(rmdr.x*(4-rmdr.y));
	const ValueType	BLweight((4-rmdr.x)*rmdr.y);
	const ValueType	BRweight(rmdr.x*rmdr.y);	

	float sum=dparams.start_val;

	ValueType temp;//Temporary Variable.

	for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
		for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
			temp = (TLweight*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)] +
					TRweight*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] +
					BLweight*(*ref_data)[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)] +
					BRweight*(*ref_data)[BChk(uY+1,DoubleYdim)][BChk(uX+1,DoubleXdim)] +
					8)>>4;

			sum+=abs((*pic_data)[c][l]-temp);
		}//l
	}//c	
	if (sum<dparams.cost.total){
		dparams.cost.total=sum;
		dparams.cost.mvcost=dparams.start_val;			
		dparams.cost.SAD=sum-dparams.cost.mvcost;
		dparams.best_mv=mv;			
	}	
}

void BiSimpleBlockDiffUp::Diff(BlockDiffParams& dparams, const MVector& mv1, const MVector& mv2){

	//the start and end points in the current frame
	const ImageCoords StartPos(dparams.xp,dparams.yp);//Coordinates in the current image
	const ImageCoords EndPos(StartPos.x+dparams.xl,StartPos.y+dparams.yl);	

	//the motion vectors rounded to 1/2 pel accuracy
	const MVector roundvec1(mv1.x>>2,mv1.y>>2);
	const MVector roundvec2(mv2.x>>2,mv2.y>>2);

	//the remainders giving 1/8 pel accuracy	
	const MVector rmdr1(mv1.x-(roundvec1.x<<2),mv1.y-(roundvec1.y<<2));
	const MVector rmdr2(mv2.x-(roundvec2.x<<2),mv2.y-(roundvec2.y<<2));

	//the starting points of the reference blocks in the reference images, to 1/2 pel accuracy
	const ImageCoords RefStart1((StartPos.x<<1) + roundvec1.x,(StartPos.y<<1) + roundvec1.y);
	const ImageCoords RefStart2((StartPos.x<<1) + roundvec2.x,(StartPos.y<<1) + roundvec2.y);

	//weights for doing linear interpolation, calculated from the remainder values
	const ValueType	TLweight1((4-rmdr1.x)*(4-rmdr1.y));
	const ValueType	TRweight1(rmdr1.x*(4-rmdr1.y));
	const ValueType	BLweight1((4-rmdr1.x)*rmdr1.y);
	const ValueType	BRweight1(rmdr1.x*rmdr1.y);		

	const ValueType	TLweight2((4-rmdr2.x)*(4-rmdr2.y));
	const ValueType	TRweight2(rmdr2.x*(4-rmdr2.y));
	const ValueType	BLweight2((4-rmdr2.x)*rmdr2.y);
	const ValueType	BRweight2(rmdr2.x*rmdr2.y);		

	CalcValueType temp;

	dparams.cost.mvcost=dparams.start_val;
	dparams.cost.SAD=0;

	for(int c = StartPos.y, uY1 = RefStart1.y,uY2=RefStart2.y; c < EndPos.y; ++c, uY1 += 2,uY2 += 2){
		for(int l = StartPos.x, uX1 = RefStart1.x,uX2=RefStart2.x; l < EndPos.x; ++l, uX1 += 2, uX2 += 2){
			temp=(TLweight1*(*ref_data)[uY1][uX1] +
					TRweight1*(*ref_data)[uY1][uX1+1] +
					BLweight1*(*ref_data)[uY1+1][uX1] +
					BRweight1*(*ref_data)[uY1+1][uX1+1]+16)>>5;

			temp+=(TLweight2*(*ref_data2)[uY2][uX2] +
					TRweight2*(*ref_data2)[uY2][uX2+1] +
					BLweight2*(*ref_data2)[uY2+1][uX2] +
					BRweight2*(*ref_data2)[uY2+1][uX2+1]+16)>>5;

			dparams.cost.SAD+=abs((*pic_data)[c][l]-temp);
		}//l
	}//c	

	dparams.cost.total=dparams.cost.SAD+dparams.cost.mvcost;		
}

void BiBChkBlockDiffUp::Diff(BlockDiffParams& dparams, const MVector& mv1, const MVector& mv2){

	//as above, but with bounds checking
	const int xmax1=ref_data->LengthX(); 
	const int ymax1=ref_data->LengthY();
	const int xmax2=ref_data2->LengthX(); 
	const int ymax2=ref_data2->LengthY();	

    //the start and end points in the current frame
	const ImageCoords StartPos(dparams.xp,dparams.yp);//Coordinates in the current image
	const ImageCoords EndPos(StartPos.x+dparams.xl,StartPos.y+dparams.yl);	

	//the motion vectors rounded to 1/2 pel accuracy
	const MVector roundvec1(mv1.x>>2,mv1.y>>2);
	const MVector roundvec2(mv2.x>>2,mv2.y>>2);

	//the remainders giving 1/8 pel accuracy	
	const MVector rmdr1(mv1.x-(roundvec1.x<<2),mv1.y-(roundvec1.y<<2));
	const MVector rmdr2(mv2.x-(roundvec2.x<<2),mv2.y-(roundvec2.y<<2));

	//the starting points of the reference blocks in the reference images, to 1/2 pel accuracy
	const ImageCoords RefStart1((StartPos.x<<1) + roundvec1.x,(StartPos.y<<1) + roundvec1.y);
	const ImageCoords RefStart2((StartPos.x<<1) + roundvec2.x,(StartPos.y<<1) + roundvec2.y);

	//weights for doing linear interpolation, calculated from the remainder values
	const ValueType	TLweight1((4-rmdr1.x)*(4-rmdr1.y));
	const ValueType	TRweight1(rmdr1.x*(4-rmdr1.y));
	const ValueType	BLweight1((4-rmdr1.x)*rmdr1.y);
	const ValueType	BRweight1(rmdr1.x*rmdr1.y);		

	const ValueType	TLweight2((4-rmdr2.x)*(4-rmdr2.y));
	const ValueType	TRweight2(rmdr2.x*(4-rmdr2.y));
	const ValueType	BLweight2((4-rmdr2.x)*rmdr2.y);
	const ValueType	BRweight2(rmdr2.x*rmdr2.y);		

	CalcValueType temp;

	dparams.cost.mvcost=dparams.start_val;
	dparams.cost.SAD=0;

	for(int c = StartPos.y, uY1 = RefStart1.y,uY2=RefStart2.y; c < EndPos.y; ++c, uY1 += 2,uY2 += 2){
		for(int l = StartPos.x, uX1 = RefStart1.x,uX2=RefStart2.x; l < EndPos.x; ++l, uX1 += 2, uX2 += 2){
			temp=(TLweight1*(*ref_data)[BChk(uY1,ymax1)][BChk(uX1,xmax1)] +
					TRweight1*(*ref_data)[BChk(uY1,ymax1)][BChk(uX1+1,xmax1)] +
					BLweight1*(*ref_data)[BChk(uY1+1,ymax1)][BChk(uX1,xmax1)] +
					BRweight1*(*ref_data)[BChk(uY1+1,ymax1)][BChk(uX1+1,xmax1)]+16)>>5;

			temp+=(TLweight2*(*ref_data2)[BChk(uY2,ymax2)][BChk(uX2,xmax2)] +
					TRweight2*(*ref_data2)[BChk(uY2,ymax2)][BChk(uX2+1,xmax2)] +
					BLweight2*(*ref_data2)[BChk(uY2+1,ymax2)][BChk(uX2,xmax2)] +
					BRweight2*(*ref_data2)[BChk(uY2+1,ymax2)][BChk(uX2+1,xmax2)]+16)>>5;

			dparams.cost.SAD+=abs((*pic_data)[c][l]-temp);
		}//l
	}//c	

	dparams.cost.total=dparams.cost.SAD+dparams.cost.mvcost;
}
