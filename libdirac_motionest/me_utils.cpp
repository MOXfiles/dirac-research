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
* Revision 1.2  2004-04-11 22:50:46  chaoticcoyote
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

///////////////////////////////////
//-------------------------------//
//utilities for motion estimation//
//-------------------------------//
///////////////////////////////////

#include "libdirac_motionest/me_utils.h"
#include "libdirac_common/common.h"
#include <algorithm>

void BMParams::Init(OLBParams& bparams,int M, int N){

	bp=bparams;
	Init(M,N);
}
void BMParams::Init(int M, int N){

	int xpos=M*bp.XBSEP-bp.XOFFSET;
	int ypos=N*bp.YBSEP-bp.YOFFSET;

	xp=DIRAC_MAX(xpos,0);//TL corner of 
	yp=DIRAC_MAX(ypos,0);//block to be matched
	xl=bp.XBLEN-xp+xpos;
	yl=bp.YBLEN-yp+ypos;

 	//constrain block lengths to fall within the picture
	xl=((xp+xl-1)>(pic_data->ubound(0)))?(pic_data->ubound(0)+1-xp):xl;
	yl=((yp+yl-1)>(pic_data->ubound(1)))?(pic_data->ubound(1)+1-yp):yl;
	me_lambda/=(bp.XBLEN*bp.YBLEN);
	me_lambda*=(xl*yl);//if I've shrunk the block I need to shrink the weight I apply to the entropy measure	
}

void SimpleBlockDiff::Diff(BlockDiffParams& dparams, MVector& mv){

	TwoDArray<ValueType> diff(dparams.xl,dparams.yl);	
	float sum=dparams.start_val;
	for (int J=dparams.yp,L=0;J!=dparams.yp+dparams.yl;++J,++L){
		for(int I=dparams.xp,K=0;I!=dparams.xp+dparams.xl;++I,++K){
			diff[L][K]=(*pic_data)[J][I]-(*ref_data)[J+mv.y][I+mv.x];
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

void BChkBlockDiff::Diff(BlockDiffParams& dparams, MVector& mv){

	int xmax=ref_data->length(0);
	int ymax=ref_data->length(1);
	TwoDArray<ValueType> diff(dparams.xl,dparams.yl);
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
};

void IntraBlockDiff::Diff(BlockDiffParams& dparams,ValueType dc_pred,float loc_lambda){

 	//computes the cost if block is predicted by its dc component
	int J, I;
	int dc=0;

	for (J=dparams.yp;J!=dparams.yp+dparams.yl;++J)
		for(I=dparams.xp;I!=dparams.xp+dparams.xl;++I)
			dc+=int((*pic_data)[J][I]);

	dparams.dc=ValueType(dc/(dparams.xl*dparams.yl));	
	dparams.dc=(dparams.dc+2)>>2;	//just give dc to 8-bit accuracy

	dparams.intra_cost=float(abs(dparams.dc-dc_pred))*loc_lambda;
	for (J=dparams.yp;J!=dparams.yp+dparams.yl;++J)
		for(I=dparams.xp;I!=dparams.xp+dparams.xl;++I)
			dparams.intra_cost+=float(abs((*pic_data)[J][I]-(dparams.dc<<2)));
};

void BiSimpleBlockDiff::Diff(BlockDiffParams& dparams, MVector& mv1,MVector& mv2){

	int I, J, K, L;
	TwoDArray<ValueType> diff(dparams.xl,dparams.yl);

	dparams.cost.mvcost=dparams.start_val;
	dparams.cost.SAD=0.0;

	for (J=dparams.yp,L=0;J!=dparams.yp+dparams.yl;++J,++L){
		for(I=dparams.xp,K=0;I!=dparams.xp+dparams.xl;++I,++K){
			diff[L][K]=(*pic_data)[J][I]-(((*ref_data)[J+mv1.y][I+mv1.x]+1)>>1);
			diff[L][K]-=(((*ref_data2)[J+mv2.y][I+mv2.x]+1)>>1);
		}//I
	}//J

	for (J=0;J!=dparams.yl;++J)
		for(I=0;I!=dparams.xl;++I)
			dparams.cost.SAD+=float(abs(diff[J][I]));

	dparams.cost.total=dparams.cost.mvcost+dparams.cost.SAD;
}

void BiBChkBlockDiff::Diff(BlockDiffParams& dparams, MVector& mv1,MVector& mv2){

	int I, J, K, L;
	TwoDArray<ValueType> diff(dparams.xl,dparams.yl);
	int xmax1=ref_data->length(0); int ymax1=ref_data->length(1);
	int xmax2=ref_data2->length(0);	int ymax2=ref_data2->length(1);

	dparams.cost.mvcost=dparams.start_val;
	dparams.cost.SAD=0.0;

	for (J=dparams.yp,L=0;J!=dparams.yp+dparams.yl;++J,++L){
		for(I=dparams.xp,K=0;I!=dparams.xp+dparams.xl;++I,++K){
			diff[L][K]=(*pic_data)[J][I]-(((*ref_data)[BChk(J+mv1.y,ymax1)][BChk(I+mv1.x,xmax1)]+1)>>1);
			diff[L][K]-=(((*ref_data2)[BChk(J+mv2.y,ymax2)][BChk(I+mv2.x,xmax2)]+1)>>1);
		}//I
	}//J


	for (J=0;J!=dparams.yl;++J){
		for(I=0;I!=dparams.xl;++I){
			dparams.cost.SAD+=float(abs(diff[J][I]));
		}//I
	}//J
	dparams.cost.total=dparams.cost.mvcost+dparams.cost.SAD;
}

void BlockDiffUp::Init(){
	InterpLookup[0][0] = 9; InterpLookup[0][1] = 3; InterpLookup[0][2] = 3; InterpLookup[0][3] = 1;
	InterpLookup[1][0] = 6; InterpLookup[1][1] = 6; InterpLookup[1][2] = 2; InterpLookup[1][3] = 2;
	InterpLookup[2][0] = 3; InterpLookup[2][1] = 9; InterpLookup[2][2] = 1; InterpLookup[2][3] = 3;
	InterpLookup[3][0] = 6; InterpLookup[3][1] = 2; InterpLookup[3][2] = 6; InterpLookup[3][3] = 2;
	InterpLookup[4][0] = 4; InterpLookup[4][1] = 4; InterpLookup[4][2] = 4; InterpLookup[4][3] = 4;
	InterpLookup[5][0] = 2; InterpLookup[5][1] = 6; InterpLookup[5][2] = 2; InterpLookup[5][3] = 6;
	InterpLookup[6][0] = 3; InterpLookup[6][1] = 1; InterpLookup[6][2] = 9; InterpLookup[6][3] = 3;
	InterpLookup[7][0] = 2; InterpLookup[7][1] = 2; InterpLookup[7][2] = 6; InterpLookup[7][3] = 6;
	InterpLookup[8][0] = 1; InterpLookup[8][1] = 3; InterpLookup[8][2] = 3; InterpLookup[8][3] = 9;	
}

void SimpleBlockDiffUp::Diff(BlockDiffParams& dparams, MVector& mv){

	ImageCoords StartPos;//Coordinates in the image being written to
	StartPos.x=dparams.xp;
	StartPos.y=dparams.yp;
	ImageCoords EndPos;	
	EndPos.x=StartPos.x+dparams.xl;
	EndPos.y=StartPos.y+dparams.yl;

	ImageCoords RefStart;
	ValueType temp;//Temporary Variable.
 	//Set up the start point in the reference image.
	MVector roundvec,rmdr;
	roundvec.x=mv.x>>2;//bit shift NB rounds negative
	roundvec.y=mv.y>>2;//numbers DOWN, as required
	rmdr.x=mv.x-(roundvec.x<<2);
	rmdr.y=mv.y-(roundvec.y<<2);	
	RefStart.x = (StartPos.x<<1) + roundvec.x;
	RefStart.y = (StartPos.y<<1) + roundvec.y;	

	float sum=dparams.start_val;

	if((rmdr.x%4 == 0)&&(rmdr.y%4 == 0)){
 		//Quick process where we can just copy from the double size image.
		for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
			for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
				sum+=abs((*pic_data)[c][l]-(*ref_data)[uY][uX]);
			}
		}
	}
	else if((rmdr.x%4 == 0)||(rmdr.y%4 == 0)){
 		//Slower process where pixels are calculated from two other pixels
		if(rmdr.x%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr.y%2 == 0){
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = ((*ref_data)[uY][uX]+(*ref_data)[uY+1][uX] + 1)>>1;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
			else{

 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the y range we are in
				if(rmdr.y%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the y direction.
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = (F1*(*ref_data)[uY][uX]+F2*(*ref_data)[uY+1][uX] + 2)>>2;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
		}
		else if(rmdr.y%4 == 0){
 			//Can copy in y direction but need to interpolate in x
			if(rmdr.x%2 == 0){
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x,uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = ((*ref_data)[uY][uX]+(*ref_data)[uY][uX+1] + 1)>>1;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the x range we are in
				if(rmdr.x%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the x direction.
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = (F1*(*ref_data)[uY][uX]+F2*(*ref_data)[uY][uX+1] + 2)>>2;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
		}
	}
	else{
		int PixLookup;//which of the nine centre points we're in
		if(rmdr.x%2 == 0){
			if(rmdr.y%2 == 0) PixLookup = 4;
			if(rmdr.y%4 == 3) PixLookup = 7;
			else PixLookup = 1;
		}
		else if (rmdr.x%4==1){
			if(rmdr.y%2==0) PixLookup = 3;
			if(rmdr.y%4 == 3) PixLookup = 6;
			else PixLookup = 0;
		}
		else {//rmdr.x%4==3
			if(rmdr.y%2==0) PixLookup = 5;
			if(rmdr.y%4 == 3) PixLookup = 8;
			else PixLookup = 2;
		}
		for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
			for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){

				temp = (InterpLookup[PixLookup][0]*(*ref_data)[uY][uX] +
						InterpLookup[PixLookup][1]*(*ref_data)[uY][uX+1] +
						InterpLookup[PixLookup][2]*(*ref_data)[uY+1][uX] +
						InterpLookup[PixLookup][3]*(*ref_data)[uY+1][uX+1] +
						8)>>4;
				sum+=abs((*pic_data)[c][l]-temp);
			}
		}
	}

	if (sum<dparams.cost.total){
		dparams.cost.total=sum;
		dparams.cost.mvcost=dparams.start_val;			
		dparams.cost.SAD=sum-dparams.cost.mvcost;
		dparams.best_mv=mv;			
	}	
}

void BChkBlockDiffUp::Diff(BlockDiffParams& dparams, MVector& mv){
 	//Same as SimpleBlockDiffUp, but with bounds-checking and edge extension.
	ImageCoords StartPos;//Coordinates in the image being written to
	StartPos.x=dparams.xp;
	StartPos.y=dparams.yp;
	ImageCoords EndPos;	
	EndPos.x=StartPos.x+dparams.xl;
	EndPos.y=StartPos.y+dparams.yl;
	ImageCoords RefStart;
	ValueType temp;//Temporary Variable

 	//Set up the start point in the reference image.
	MVector roundvec,rmdr;
	roundvec.x=mv.x>>2;//bit shift NB rounds negative
	roundvec.y=mv.y>>2;//numbers DOWN, as required
	rmdr.x=mv.x-(roundvec.x<<2);
	rmdr.y=mv.y-(roundvec.y<<2);	
	RefStart.x=(StartPos.x<<1)+roundvec.x;
	RefStart.y=(StartPos.y<<1)+roundvec.y;	

	int DoubleXdim=ref_data->length(0);
	int DoubleYdim=ref_data->length(1);
	float sum=dparams.start_val;

	if((rmdr.x%4 == 0)&&(rmdr.y%4 == 0)){
 		//Quick process where we can just copy from the double size image.
		for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
			for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
				sum+=abs((*pic_data)[c][l]-(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]);
			}
		}
	}
	else if((rmdr.x%4 == 0)||(rmdr.y%4 == 0)){
 		//Slower process where pixels are calculated from two other pixels
		if(rmdr.x%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr.y%2 == 0){
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = ((*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+
								(*ref_data)[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)]+ 1)>>1;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the y range we are in
				if(rmdr.y%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the y direction.
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = (F1*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+
								F2*(*ref_data)[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)] + 2)>>2;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
		}
		else if(rmdr.y%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr.x%2 == 0){
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x,uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = ((*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+
								(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] + 1)>>1;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the x range we are in
				if(rmdr.x%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the x direction.
				for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
					for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){
						temp = (F1*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+
								F2*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] + 2)>>2;
						sum+=abs((*pic_data)[c][l]-temp);
					}
				}
			}
		}
	}
	else{
 		//This is the slowest process of all - each pixel is calculated from
 		//four others :(
 		// A . . . B
 		// . 0 1 2 .
 		// . 3 4 5 .
 		// . 6 7 8 .
 		// C . . . D
 		//Assuming that we are already in the correct quadrant....
 		//Can easily figure out which number we are dealing with.
		int PixLookup;
		if(rmdr.x%2 == 0){
			if(rmdr.y%2 == 0) PixLookup = 4;
			if(rmdr.y%4 == 3) PixLookup = 7;
			else PixLookup = 1;
		}
		else if (rmdr.x%4==1){
			if(rmdr.y%2==0) PixLookup = 3;
			if(rmdr.y%4 == 3) PixLookup = 6;
			else PixLookup = 0;
		}
		else {//rmdr.x%4==3
			if(rmdr.y%2==0) PixLookup = 5;
			if(rmdr.y%4 == 3) PixLookup = 8;
			else PixLookup = 2;
		}
		for(int c = StartPos.y, uY = RefStart.y; c < EndPos.y; ++c, uY += 2){
			for(int l = StartPos.x, uX = RefStart.x; l < EndPos.x; ++l, uX += 2){

				temp = (InterpLookup[PixLookup][0]*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)] +
						InterpLookup[PixLookup][1]*(*ref_data)[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] +
						InterpLookup[PixLookup][2]*(*ref_data)[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)] +
						InterpLookup[PixLookup][3]*(*ref_data)[BChk(uY+1,DoubleYdim)][BChk(uX+1,DoubleXdim)] +
						8)>>4;
				sum+=abs((*pic_data)[c][l]-temp);
			}
		}
	}

	if (sum<dparams.cost.total){
		dparams.cost.total=sum;
		dparams.cost.mvcost=dparams.start_val;			
		dparams.cost.SAD=sum-dparams.cost.mvcost;
		dparams.best_mv=mv;			
	}	
}

void BiSimpleBlockDiffUp::Diff(BlockDiffParams& dparams, MVector& mv1, MVector& mv2){

	ImageCoords StartPos;//Coordinates in the current image
	StartPos.x=dparams.xp;
	StartPos.y=dparams.yp;
	ImageCoords EndPos;	
	EndPos.x=StartPos.x+dparams.xl;
	EndPos.y=StartPos.y+dparams.yl;
	ImageCoords RefStart1;
	ImageCoords RefStart2;
	ValueType temp;//Temporary Variable.

 	//Set up the start point in the reference image.
	MVector roundvec1,rmdr1;
	MVector roundvec2,rmdr2;
	roundvec1.x=mv1.x>>2;
	roundvec1.y=mv1.y>>2;
	roundvec2.x=mv2.x>>2;
	roundvec2.y=mv2.y>>2;
	rmdr1.x=mv1.x-(roundvec1.x<<2);
	rmdr1.y=mv1.y-(roundvec1.y<<2);
	rmdr2.x=mv2.x-(roundvec2.x<<2);
	rmdr2.y=mv2.y-(roundvec2.y<<2);
	RefStart1.x = (StartPos.x<<1) + roundvec1.x;
	RefStart1.y = (StartPos.y<<1) + roundvec1.y;
	RefStart2.x = (StartPos.x<<1) + roundvec2.x;
	RefStart2.y = (StartPos.y<<1) + roundvec2.y;

	TwoDArray<ValueType> tempdiff(dparams.xl,dparams.yl);

	//first subtract half-values from reference 1 and put into the temp array//
	///////////////////////////////////////////////////////////////////////////

	if((rmdr1.x%4 == 0)&&(rmdr1.y%4 == 0)){
 		//Quick process where we can just copy from the double size image.
		for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
			for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
				temp=(*ref_data)[uY][uX];
				temp++;
				temp>>=1;
				tempdiff[y][x]=(*pic_data)[c][l]-temp;
			}
		}
	}
	else if((rmdr1.x%4 == 0)||(rmdr1.y%4 == 0)){
 		//Slower process where pixels are calculated from two other pixels
		if(rmdr1.x%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr1.y%2 == 0){
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++y){
						temp = ((*ref_data)[uY][uX]+(*ref_data)[uY+1][uX] + 1)>>1;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the y range we are in
				if(rmdr1.y%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the y direction.
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = (F1*(*ref_data)[uY][uX]+F2*(*ref_data)[uY+1][uX] + 2)>>2;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
		}
		else if(rmdr1.y%4 == 0){
 			//Can copy in y direction but need to interpolate in x
			if(rmdr1.x%2 == 0){
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x,uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = ((*ref_data)[uY][uX]+(*ref_data)[uY][uX+1] + 1)>>1;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the x range we are in
				if(rmdr1.x%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the x direction.
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = (F1*(*ref_data)[uY][uX]+F2*(*ref_data)[uY][uX+1] + 2)>>2;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
		}
	}
	else{
		int PixLookup;//which of the nine centre points we're in
		if(rmdr1.x%2 == 0){
			if(rmdr1.y%2 == 0) PixLookup = 4;
			if(rmdr1.y%4 == 3) PixLookup = 7;
			else PixLookup = 1;
		}
		else if (rmdr1.x%4==1){
			if(rmdr1.y%2==0) PixLookup = 3;
			if(rmdr1.y%4 == 3) PixLookup = 6;
			else PixLookup = 0;
		}
		else {//rmdr.x%4==3
			if(rmdr1.y%2==0) PixLookup = 5;
			if(rmdr1.y%4 == 3) PixLookup = 8;
			else PixLookup = 2;
		}
		for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
			for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){

				temp = (InterpLookup[PixLookup][0]*(*ref_data)[uY][uX] +
						InterpLookup[PixLookup][1]*(*ref_data)[uY][uX+1] +
						InterpLookup[PixLookup][2]*(*ref_data)[uY+1][uX] +
						InterpLookup[PixLookup][3]*(*ref_data)[uY+1][uX+1] +
						8)>>4;
				temp++;
				temp>>=1;
				tempdiff[y][x]=(*pic_data)[c][l]-temp;
			}
		}
	}

	//second, subtract half-values from reference 2 and do the sum//
	////////////////////////////////////////////////////////////////

	dparams.cost.mvcost=dparams.start_val;

	if((rmdr2.x%4 == 0)&&(rmdr2.y%4 == 0)){
 		//Quick process where we can just copy from the double size image.
		for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
			for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
				temp=(*ref_data2)[uY][uX];
				temp++;
				temp>>=1;
				dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
			}
		}
	}
	else if((rmdr2.x%4 == 0)||(rmdr2.y%4 == 0)){
 		//Slower process where pixels are calculated from two other pixels
		if(rmdr2.x%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr2.y%2 == 0){
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++y){
						temp = ((*ref_data2)[uY][uX]+(*ref_data2)[uY+1][uX] + 1)>>1;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the y range we are in
				if(rmdr2.y%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the y direction.
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = (F1*(*ref_data2)[uY][uX]+F2*(*ref_data2)[uY+1][uX] + 2)>>2;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
		}
		else if(rmdr2.y%4 == 0){
 			//Can copy in y direction but need to interpolate in x
			if(rmdr2.x%2 == 0){
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = ((*ref_data2)[uY][uX]+(*ref_data2)[uY][uX+1] + 1)>>1;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the x range we are in
				if(rmdr2.x%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the x direction.
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = (F1*(*ref_data2)[uY][uX]+F2*(*ref_data2)[uY][uX+1] + 2)>>2;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
		}
	}
	else{
		int PixLookup;//which of the nine centre points we're in
		if(rmdr2.x%2 == 0){
			if(rmdr2.y%2 == 0) PixLookup = 4;
			if(rmdr2.y%4 == 3) PixLookup = 7;
			else PixLookup = 1;
		}
		else if (rmdr2.x%4==1){
			if(rmdr2.y%2==0) PixLookup = 3;
			if(rmdr2.y%4 == 3) PixLookup = 6;
			else PixLookup = 0;
		}
		else {//rmdr.x%4==3
			if(rmdr2.y%2==0) PixLookup = 5;
			if(rmdr2.y%4 == 3) PixLookup = 8;
			else PixLookup = 2;
		}
		for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
			for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){

				temp = (InterpLookup[PixLookup][0]*(*ref_data2)[uY][uX] +
						InterpLookup[PixLookup][1]*(*ref_data2)[uY][uX+1] +
						InterpLookup[PixLookup][2]*(*ref_data2)[uY+1][uX] +
						InterpLookup[PixLookup][3]*(*ref_data2)[uY+1][uX+1] +
						8)>>4;
				temp++;
				temp>>=1;
				dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
			}
		}
	}	

	dparams.cost.total=dparams.cost.SAD+dparams.cost.mvcost;		
}

void BiBChkBlockDiffUp::Diff(BlockDiffParams& dparams, MVector& mv1, MVector& mv2){

	//as above, but with bounds checking
	int xmax1=ref_data->length(0); int ymax1=ref_data->length(1);
	int xmax2=ref_data2->length(0);	int ymax2=ref_data2->length(1);	

	ImageCoords StartPos;//Coordinates in the current image
	StartPos.x=dparams.xp;
	StartPos.y=dparams.yp;
	ImageCoords EndPos;	
	EndPos.x=StartPos.x+dparams.xl;
	EndPos.y=StartPos.y+dparams.yl;
	ImageCoords RefStart1;
	ImageCoords RefStart2;
	ValueType temp;//Temporary Variable.

 	//Set up the start point in the reference image.
	MVector roundvec1,rmdr1;
	MVector roundvec2,rmdr2;
	roundvec1.x=mv1.x>>2;
	roundvec1.y=mv1.y>>2;
	roundvec2.x=mv2.x>>2;
	roundvec2.y=mv2.y>>2;
	rmdr1.x=mv1.x-(roundvec1.x<<2);
	rmdr1.y=mv1.y-(roundvec1.y<<2);
	rmdr2.x=mv2.x-(roundvec2.x<<2);
	rmdr2.y=mv2.y-(roundvec2.y<<2);
	RefStart1.x = (StartPos.x<<1) + roundvec1.x;
	RefStart1.y = (StartPos.y<<1) + roundvec1.y;
	RefStart2.x = (StartPos.x<<1) + roundvec2.x;
	RefStart2.y = (StartPos.y<<1) + roundvec2.y;

	TwoDArray<ValueType> tempdiff(dparams.xl,dparams.yl);

	//first subtract half-values from reference 1 and put into the temp array//
	///////////////////////////////////////////////////////////////////////////

	if((rmdr1.x%4 == 0)&&(rmdr1.y%4 == 0)){
 		//Quick process where we can just copy from the double size image.
		for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
			for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
				temp=(*ref_data)[BChk(uY,ymax1)][BChk(uX,xmax1)];
				temp++;
				temp>>=1;
				tempdiff[y][x]=(*pic_data)[c][l]-temp;
			}
		}
	}
	else if((rmdr1.x%4 == 0)||(rmdr1.y%4 == 0)){
 		//Slower process where pixels are calculated from two other pixels
		if(rmdr1.x%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr1.y%2 == 0){
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = ((*ref_data)[BChk(uY,ymax1)][BChk(uX,xmax1)]+
								(*ref_data)[BChk(uY+1,ymax1)][BChk(uX,xmax1)] + 1)>>1;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the y range we are in
				if(rmdr1.y%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the y direction.
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = (F1*(*ref_data)[BChk(uY,ymax1)][BChk(uX,xmax1)]+
								F2*(*ref_data)[BChk(uY+1,ymax1)][BChk(uX,xmax1)] + 2)>>2;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
		}
		else if(rmdr1.y%4 == 0){
 			//Can copy in y direction but need to interpolate in x
			if(rmdr1.x%2 == 0){
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x,uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = ((*ref_data)[BChk(uY,ymax1)][BChk(uX,xmax1)]+
								(*ref_data)[BChk(uY,ymax1)][BChk(uX+1,xmax1)] + 1)>>1;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the x range we are in
				if(rmdr1.x%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the x direction.
				for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
					for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){
						temp = (F1*(*ref_data)[BChk(uY,ymax1)][BChk(uX,xmax1)]+
								F2*(*ref_data)[BChk(uY,ymax1)][BChk(uX+1,xmax1)] + 2)>>2;
						temp++;
						temp>>=1;
						tempdiff[y][x]=(*pic_data)[c][l]-temp;
					}
				}
			}
		}
	}
	else{
		int PixLookup;//which of the nine centre points we're in
		if(rmdr1.x%2 == 0){
			if(rmdr1.y%2 == 0) PixLookup = 4;
			if(rmdr1.y%4 == 3) PixLookup = 7;
			else PixLookup = 1;
		}
		else if (rmdr1.x%4==1){
			if(rmdr1.y%2==0) PixLookup = 3;
			if(rmdr1.y%4 == 3) PixLookup = 6;
			else PixLookup = 0;
		}
		else {//rmdr.x%4==3
			if(rmdr1.y%2==0) PixLookup = 5;
			if(rmdr1.y%4 == 3) PixLookup = 8;
			else PixLookup = 2;
		}
		for(int c = StartPos.y, uY = RefStart1.y,y=0; c < EndPos.y; ++c, uY += 2,++y){
			for(int l = StartPos.x, uX = RefStart1.x,x=0; l < EndPos.x; ++l, uX += 2,++x){

				temp = (InterpLookup[PixLookup][0]*(*ref_data)[BChk(uY,ymax1)][BChk(uX,xmax1)] +
						InterpLookup[PixLookup][1]*(*ref_data)[BChk(uY,ymax1)][BChk(uX+1,xmax1)] +
						InterpLookup[PixLookup][2]*(*ref_data)[BChk(uY+1,ymax1)][BChk(uX,xmax1)] +
						InterpLookup[PixLookup][3]*(*ref_data)[BChk(uY+1,ymax1)][BChk(uX+1,xmax1)] +
						8)>>4;
				temp++;
				temp>>=1;
				tempdiff[y][x]=(*pic_data)[c][l]-temp;
			}
		}
	}
	//second, subtract half-values from reference 2 and do the sum//
	////////////////////////////////////////////////////////////////

	dparams.cost.mvcost=dparams.start_val;

	if((rmdr2.x%4 == 0)&&(rmdr2.y%4 == 0)){
 		//Quick process where we can just copy from the double size image.
		for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
			for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
				temp=(*ref_data2)[BChk(uY,ymax2)][BChk(uX,xmax2)];
				temp++;
				temp>>=1;
				dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
			}
		}
	}
	else if((rmdr2.x%4 == 0)||(rmdr2.y%4 == 0)){
 		//Slower process where pixels are calculated from two other pixels
		if(rmdr2.x%4 == 0){
 			//Can copy in x direction but need to interpolate in y
			if(rmdr2.y%2 == 0){
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = ((*ref_data2)[BChk(uY,ymax2)][BChk(uX,xmax2)]
								+(*ref_data2)[BChk(uY+1,ymax2)][BChk(uX,xmax2)] + 1)>>1;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the y range we are in
				if(rmdr2.y%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the y direction.
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = (F1*(*ref_data2)[BChk(uY,ymax2)][BChk(uX,xmax2)]+
								F2*(*ref_data2)[BChk(uY+1,ymax2)][BChk(uX,xmax2)] + 2)>>2;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
		}
		else if(rmdr2.y%4 == 0){
 			//Can copy in y direction but need to interpolate in x
			if(rmdr2.x%2 == 0){
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = ((*ref_data2)[BChk(uY,ymax2)][BChk(uX,xmax2)]+
								(*ref_data2)[BChk(uY,ymax2)][BChk(uX+1,xmax2)] + 1)>>1;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
			else{
 				//Need to do 1/4, 3/4 weighting.
				int F1, F2;
 				//Decide which quarter of the x range we are in
				if(rmdr2.x%4 > 1){
					F2 = 3; F1 = 1;
				}
				else{
					F1 = 3; F2 = 1;
				}
 				//Interpolate in-between the pixels in the x direction.
				for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
					for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){
						temp = (F1*(*ref_data2)[BChk(uY,ymax2)][BChk(uX,xmax2)]+
								F2*(*ref_data2)[BChk(uY,ymax2)][BChk(uX+1,xmax2)] + 2)>>2;
						temp++;
						temp>>=1;
						dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
					}
				}
			}
		}
	}
	else{
		int PixLookup;//which of the nine centre points we're in
		if(rmdr2.x%2 == 0){
			if(rmdr2.y%2 == 0) PixLookup = 4;
			if(rmdr2.y%4 == 3) PixLookup = 7;
			else PixLookup = 1;
		}
		else if (rmdr2.x%4==1){
			if(rmdr2.y%2==0) PixLookup = 3;
			if(rmdr2.y%4 == 3) PixLookup = 6;
			else PixLookup = 0;
		}
		else {//rmdr.x%4==3
			if(rmdr2.y%2==0) PixLookup = 5;
			if(rmdr2.y%4 == 3) PixLookup = 8;
			else PixLookup = 2;
		}
		for(int uY = RefStart2.y, y=0; y < dparams.yl; uY += 2,++y){
			for(int uX = RefStart2.x, x=0; x < dparams.xl; uX += 2,++x){

				temp = (InterpLookup[PixLookup][0]*(*ref_data2)[BChk(uY,ymax2)][BChk(uX,xmax2)] +
						InterpLookup[PixLookup][1]*(*ref_data2)[BChk(uY,ymax2)][BChk(uX+1,xmax2)] +
						InterpLookup[PixLookup][2]*(*ref_data2)[BChk(uY+1,ymax2)][BChk(uX,xmax2)] +
						InterpLookup[PixLookup][3]*(*ref_data2)[BChk(uY+1,ymax2)][BChk(uX+1,xmax2)] +
						8)>>4;
				temp++;
				temp>>=1;
				dparams.cost.SAD+=abs(tempdiff[y][x]-temp);
			}
		}
	}	
	dparams.cost.total=dparams.cost.SAD+dparams.cost.mvcost;		
}
