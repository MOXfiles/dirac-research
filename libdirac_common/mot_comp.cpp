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
* Revision 1.2  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
* Motion compensation by Richard Felton, with mods by Thomas Davies.
*/

//Implements the motion compensation functions outlined in mot_comp.h
#include "libdirac_common/mot_comp.h"
#include "libdirac_common/motion.h"
#include "libdirac_common/frame_buffer.h"

using std::vector;

//--public member functions--//
///////////////////////////////


//Constructor
//Initialises the lookup table that is needed for 1/8th pixel accuracy
//motion estimation. Creates the necessary arithmetic objects and
//calls ReConfig to create weighting blocks to fit the values within
//cparams.
MotionCompensator::MotionCompensator(const CodecParams &cp): cparams(cp),luma_or_chroma(true), add_or_sub(SUBTRACT){

	//Arithmetic objects to avoid code duplication
	add = new ArithAddObj;
	subtract = new ArithSubtractObj;
	addhalf = new ArithHalfAddObj;
	subtracthalf = new ArithHalfSubtractObj;

	//Lazy way of initialising the lookup table - must be a more consise way?
	InterpLookup[0][0] = 9; InterpLookup[0][1] = 3; InterpLookup[0][2] = 3; InterpLookup[0][3] = 1;
	InterpLookup[1][0] = 6; InterpLookup[1][1] = 6; InterpLookup[1][2] = 2; InterpLookup[1][3] = 2;
	InterpLookup[2][0] = 3; InterpLookup[2][1] = 9; InterpLookup[2][2] = 1; InterpLookup[2][3] = 3;
	InterpLookup[3][0] = 6; InterpLookup[3][1] = 2; InterpLookup[3][2] = 6; InterpLookup[3][3] = 2;
	InterpLookup[4][0] = 4; InterpLookup[4][1] = 4; InterpLookup[4][2] = 4; InterpLookup[4][3] = 4;
	InterpLookup[5][0] = 2; InterpLookup[5][1] = 6; InterpLookup[5][2] = 2; InterpLookup[5][3] = 6;
	InterpLookup[6][0] = 3; InterpLookup[6][1] = 1; InterpLookup[6][2] = 9; InterpLookup[6][3] = 3;
	InterpLookup[7][0] = 2; InterpLookup[7][1] = 2; InterpLookup[7][2] = 6; InterpLookup[7][3] = 6;
	InterpLookup[8][0] = 1; InterpLookup[8][1] = 3; InterpLookup[8][2] = 3; InterpLookup[8][3] = 9;

	//Configure weighting blocks for the first time
	BlockWeights = NULL;
	ReConfig();
}

//Destructor
MotionCompensator::~MotionCompensator(){

	//Tidy up the pointers.
	arith= NULL;
	delete add;
	delete subtract;
	delete addhalf;
	delete subtracthalf;
	for(int i = 0; i < 9; i++){
		for(int a = 0; a < yBlockSize; ++a)
			delete[] BlockWeights[i][a];
		delete[] BlockWeights[i];
	}
	delete[] BlockWeights;
}

//Called to perform motion compensated addition/subtraction on an entire frame.
void MotionCompensator::CompensateFrame(FrameBuffer& my_buffer,int fnum,const MvData& mv_data){

	int ref1_idx,ref2_idx;	
	Frame& my_frame=my_buffer.GetFrame(fnum);
	ChromaFormat cformat=cparams.cformat;
	const FrameSort& fsort=my_frame.GetFparams().fsort;

	if (fsort!=I_frame){//we can motion compensate

		const vector<int>& refs=my_frame.GetFparams().refs;
		if (refs.size()>0){
			ref1_idx=refs[0];
			if (refs.size()>1)
				ref2_idx=refs[1];
			else
				ref2_idx=refs[0];
			Frame& ref1frame=my_buffer.GetFrame(ref1_idx);
			Frame& ref2frame=my_buffer.GetFrame(ref2_idx);

			CompensateComponent(my_frame,ref1frame,ref2frame,mv_data,Y);
			if (cformat!=Yonly){
				luma_or_chroma=false;				
				CompensateComponent(my_frame,ref1frame,ref2frame,mv_data,U);
				CompensateComponent(my_frame,ref1frame,ref2frame,mv_data,V);
			}
		}
	}
}

//--private member functions--//
////////////////////////////////

//Needs to be called if the blocksize changes (and
//on startup). This method creates an array of weighting
//blocks that are used to acheive correctly overlapping
//blocks.
void MotionCompensator::ReConfig(){
	if (luma_or_chroma)
		bparams=cparams.LumaBParams(2);
	else
		bparams=cparams.ChromaBParams(2);

	if(BlockWeights != NULL){
		//Need to delete the old weights
		for(int i = 0; i < 9; i++){
			for(int a = 0; a < bparams.YBLEN; ++a)
				delete[] BlockWeights[i][a];
			delete[] BlockWeights[i];
		}
		delete[] BlockWeights;
	}

	//Create new weights array.
	BlockWeights = new CalcValueType**[9];
	for(int i = 0; i < 9; i++){
		BlockWeights[i] = new CalcValueType*[bparams.YBLEN];
		for(int a = 0; a < bparams.YBLEN; ++a)
			BlockWeights[i][a] = new CalcValueType[bparams.XBLEN];
	}

	//We can create all nine weighting blocks by calculating values
	//for four blocks and mirroring them to generate the others.
	CreateBlock(bparams, false, false, BlockWeights[0]);
	CreateBlock(bparams, false, true, BlockWeights[3]);
	CreateBlock(bparams, true, false, BlockWeights[1]);
	CreateBlock(bparams, true, true, BlockWeights[4]);
	//Note order of flipping is important.	
	FlipX(BlockWeights[3], bparams, BlockWeights[5]);
	FlipX(BlockWeights[0], bparams, BlockWeights[2]);
	FlipY(BlockWeights[0], bparams, BlockWeights[6]);
	FlipX(BlockWeights[6], bparams, BlockWeights[8]);
	FlipY(BlockWeights[1], bparams, BlockWeights[7]);

	//Record the blocksize locally.
	xBlockSize = bparams.XBLEN;
	yBlockSize = bparams.YBLEN;

}

void MotionCompensator::CompensateComponent
(Frame& picframe, const Frame &ref1frame, const Frame& ref2frame, const MvData& mv_data,const CompSort cs){

	const MvArray& mv1array=mv_data.mv1;
	const MvArray& mv2array=mv_data.mv2;
	PicArray& pic_data=picframe.Data(cs);
	const PicArray& ref1up=ref1frame.UpData(cs);
	const PicArray& ref2up=ref2frame.UpData(cs);
	const TwoDArray<ValueType>& dcarray=mv_data.dc(cs);

	const TwoDArray<PredMode>& mode_array=mv_data.mode;

	ReConfig();//set all the weighting blocks up	

	//Blocks are listed left to right, line by line.
	MVector mv1,mv2;
	PredMode block_mode;
	ValueType dc;

	ImageCoords Pos;

	//Set-up our blocksize and store the size of the image
	ImageWidth = pic_data.length(0);
	ImageHeight = pic_data.length(1);

	//Loop for each block in the output image.
	//The CompensateBlock function will use the image pointed to by ref1up
	//and add the compensated pixels to the image pointed to by pic_data.
	CalcValueType** WeightIndex;

	for(int yBlock = 0; yBlock < cparams.Y_NUMBLOCKS; ++yBlock){
		for(int xBlock = 0; xBlock < cparams.X_NUMBLOCKS; ++xBlock){
			//Calculate which part of the image we are writing to.
			Pos.x = (xBlock * bparams.XBSEP) - bparams.XOFFSET;
			Pos.y = (yBlock * bparams.YBSEP) - bparams.YOFFSET;

			//Decide which weights to use.
			if((xBlock != 0)&&(xBlock < cparams.X_NUMBLOCKS - 1)){
				if((yBlock != 0)&&(yBlock < cparams.Y_NUMBLOCKS - 1))	
					WeightIndex = BlockWeights[4];
				else if(yBlock == 0) WeightIndex = BlockWeights[1];
				else WeightIndex = BlockWeights[7];
			}
			else if(xBlock == 0){
				if((yBlock != 0)&&(yBlock < cparams.Y_NUMBLOCKS - 1))	
					WeightIndex = BlockWeights[3];
				else if(yBlock == 0) WeightIndex = BlockWeights[0];
				else WeightIndex = BlockWeights[6];
			}
			else{
				if((yBlock != 0)&&(yBlock < cparams.Y_NUMBLOCKS - 1))	
					WeightIndex = BlockWeights[5];
				else if(yBlock == 0) WeightIndex = BlockWeights[2];
				else WeightIndex = BlockWeights[8];
			}

			block_mode=mode_array[yBlock][xBlock];
			mv1=mv1array[yBlock][xBlock];
			mv2=mv2array[yBlock][xBlock];
			dc=dcarray[yBlock][xBlock]<<2;//DC is only given 8 bits
			if(block_mode == REF1_ONLY){
				if(add_or_sub==ADD) arith = add;
				else arith = subtract;
				CompensateBlock(pic_data, ref1up, mv1, Pos, WeightIndex);
			}
			else if (block_mode == REF2_ONLY){				
				if(add_or_sub==ADD) arith = add;
				else arith = subtract;
				CompensateBlock(pic_data, ref2up, mv2, Pos, WeightIndex);
			}
			else if(block_mode == REF1AND2){
				if(add_or_sub==ADD) arith = addhalf;
				else arith = subtracthalf;
				CompensateBlock(pic_data, ref1up, mv1, Pos, WeightIndex);
				CompensateBlock(pic_data, ref2up, mv2, Pos, WeightIndex);
			}
			else{//we have a DC block.
				if(add_or_sub==ADD) arith = add;
				else arith = subtract;
				DCBlock(pic_data, dc,Pos, WeightIndex);
			}
		}
	}
}

void MotionCompensator::CompensateBlock(PicArray &pic_data, const PicArray &refup_data, const MVector &Vec, 
const ImageCoords Pos, CalcValueType** Weights){

	//Coordinates in the image being written to.
	ImageCoords StartPos;
	ImageCoords EndPos;
	//The difference between the desired start point
	//Pos and the actual start point StartPos.
	ImageCoords Difference;
	//Where to start in the upconverter image
	ImageCoords RefStart;
	//Temporary Variable.
	ValueType temp;

	//Firstly, check to see if Pos is inside the image.
	//Lower bounds
	if(Pos.x < 0) StartPos.x = 0;
	else StartPos.x = Pos.x;
	Difference.x = StartPos.x - Pos.x;

	if(Pos.y < 0) StartPos.y = 0;
	else StartPos.y = Pos.y;
	Difference.y = StartPos.y - Pos.y;

	//Higher bounds - just need to set EndPos
	EndPos.x = Pos.x + xBlockSize;
	if(EndPos.x > ImageWidth) EndPos.x = ImageWidth;
	EndPos.y = Pos.y + yBlockSize;
	if(EndPos.y > ImageHeight) EndPos.y = ImageHeight;

	//Set up the start point in the reference image.
	MVector roundvec,rmdr;

	roundvec.x=Vec.x>>2;//bit shift NB rounds negative
	roundvec.y=Vec.y>>2;//numbers DOWN, as required
	rmdr.x=Vec.x-(roundvec.x<<2);
	rmdr.y=Vec.y-(roundvec.y<<2);	
	RefStart.x = (StartPos.x<<1) + roundvec.x;
	RefStart.y = (StartPos.y<<1) + roundvec.y;

	//An additional stage to make sure the block to be copied does not fall outside
	//the reference image.
	int DoubleXdim = ImageWidth * 2;
	int DoubleYdim = ImageHeight * 2;
	bool DoBoundsChecking = false;

	//Check if there are going to be any problems copying the block from
	//the upvconverted reference image.
	if(RefStart.x < 0) DoBoundsChecking = true;
	else if(RefStart.x + ((EndPos.x - StartPos.x)<<1) >= DoubleXdim) DoBoundsChecking = true;
	if(RefStart.y < 0) DoBoundsChecking = true;
	else if(RefStart.y + ((EndPos.y - StartPos.y)<<1) >= DoubleYdim) DoBoundsChecking = true;

	//If we need to do bounds checking we can use an alternative version
	//of the following routines and leave these optimised 'unsafe' ones.

	if(!DoBoundsChecking){
		if((rmdr.x%4 == 0)&&(rmdr.y%4 == 0)){
		//Quick process where we can just copy from the double size image.
			for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
				for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
					arith->DoArith(pic_data[c][l],CalcValueType(refup_data[uY][uX]),Weights[wY][wX]);
				}
			}
		}

		else if((rmdr.x%4 == 0)||(rmdr.y%4 == 0)){

			//Slower process where pixels are calculated from two other pixels
			if(rmdr.x%4 == 0){
			//Can copy in x direction but need to interpolate in y
				if(rmdr.y%2 == 0){
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (refup_data[uY][uX]+refup_data[uY+1][uX] + 1)>>1;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
						}
					}
				}
				else{
				//Need to do 1/4, 3/4 weighting.
					int F1, F2;
				//Decide which quarter of the y range we are in
					if(rmdr.y%4 > 1){
						F2 = 3;
						F1 = 1;
					}
					else{
						F1 = 3;
						F2 = 1;
					}
				//Interpolate in-between the pixels in the y direction.
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (F1*refup_data[uY][uX]+F2*refup_data[uY+1][uX] + 2)>>2;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
						}
					}
				}
			}
			else if(rmdr.y%4 == 0){
			//Can copy in x direction but need to interpolate in y
				if(rmdr.x%2 == 0){
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (refup_data[uY][uX]+refup_data[uY][uX+1] + 1)>>1;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
						}
					}
				}
				else{
				//Need to do 1/4, 3/4 weighting.
					int F1, F2;
				//Decide which quarter of the y range we are in
					if(rmdr.x%4 > 1){
						F2 = 3;
						F1 = 1;
					}
					else{
						F1 = 3;
						F2 = 1;
					}
				//Interpolate in-between the pixels in the y direction.
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (F1*refup_data[uY][uX]+F2*refup_data[uY][uX+1] + 2)>>2;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
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
				if(rmdr.y%4 > 1) PixLookup = 7;
				else PixLookup = 1;
			}
			else if (rmdr.y%2 == 0){
				if(rmdr.x%4 > 1) PixLookup = 5;
				else PixLookup = 3;
			}
			else if(rmdr.x%4 > 1){
				if(rmdr.y%4 > 1) PixLookup = 8;
				else PixLookup = 2;
			}
			else{
				if(rmdr.y%4 > 1) PixLookup = 6;
				else PixLookup = 0;
			}
			for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
				for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){

					temp = (InterpLookup[PixLookup][0]*refup_data[uY][uX] +
							InterpLookup[PixLookup][1]*refup_data[uY][uX+1] +
							InterpLookup[PixLookup][2]*refup_data[uY+1][uX] +
							InterpLookup[PixLookup][3]*refup_data[uY+1][uX+1] +
							8)>>4;
					arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
				}
			}
		}
	}

	//These routines are similar to those above but perform bounds checking
	//and edge extension.
	else{
		if((rmdr.x%4 == 0)&&(rmdr.y%4 == 0)){
		//Quick process where we can just copy from the double size image.
			for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
				for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
					arith->DoArith(pic_data[c][l],CalcValueType(refup_data[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]),Weights[wY][wX]);
				}
			}
		}

		else if((rmdr.x%4 == 0)||(rmdr.y%4 == 0)){

		//Slower process where pixels are calculated from two other pixels
			if(rmdr.x%4 == 0){
			//Can copy in x direction but need to interpolate in y
				if(rmdr.y%2 == 0){
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (refup_data[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+refup_data[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)] + 1)>>1;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
						}
					}
				}
				else{
				//Need to do 1/4, 3/4 weighting.
					int F1, F2;
				//Decide which quarter of the y range we are in
					if(rmdr.y%4 > 1){
						F2 = 3;
						F1 = 1;
					}
					else{
						F1 = 3;
						F2 = 1;
					}
				//Interpolate in-between the pixels in the y direction.
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (F1*refup_data[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+F2*refup_data[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)] + 2)>>2;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
						}
					}
				}
			}
			else if(rmdr.y%4 == 0){
			//Can copy in y direction but need to interpolate in x
				if(rmdr.x%2 == 0){
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (refup_data[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+refup_data[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] + 1)>>1;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
						}
					}
				}
				else{
				//Need to do 1/4, 3/4 weighting.
					int F1, F2;
				//Decide which quarter of the y range we are in
					if(rmdr.x%4 > 1){
						F2 = 3;
						F1 = 1;
					}
					else{
						F1 = 3;
						F2 = 1;
					}
				//Interpolate in-between the pixels in the x direction.
					for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
						for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
							temp = (F1*refup_data[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)]+F2*refup_data[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] + 2)>>2;
							arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
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
				if(rmdr.y%4 > 1) PixLookup = 7;
				else PixLookup = 1;
			}
			else if (rmdr.y%2 == 0){
				if(rmdr.x%4 > 1) PixLookup = 5;
				else PixLookup = 3;
			}
			else if(rmdr.x%4 > 1){
				if(rmdr.y%4 > 1) PixLookup = 8;
				else PixLookup = 2;
			}
			else{
				if(rmdr.y%4 > 1) PixLookup = 6;
				else PixLookup = 0;
			}
			for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
				for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){
					temp = (InterpLookup[PixLookup][0]*refup_data[BChk(uY,DoubleYdim)][BChk(uX,DoubleXdim)] +
							InterpLookup[PixLookup][1]*refup_data[BChk(uY,DoubleYdim)][BChk(uX+1,DoubleXdim)] +
							InterpLookup[PixLookup][2]*refup_data[BChk(uY+1,DoubleYdim)][BChk(uX,DoubleXdim)] +
							InterpLookup[PixLookup][3]*refup_data[BChk(uY+1,DoubleYdim)][BChk(uX+1,DoubleXdim)] +
							8)>>4;
					arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
				}
			}
		}
	}
}


void MotionCompensator::DCBlock(PicArray &pic_data,ValueType dc, ImageCoords Pos, CalcValueType** Weights){

	//Coordinates in the image being written to.
	ImageCoords StartPos;
	ImageCoords EndPos;
	//The difference between the desired start point
	//Pos and the actual start point StartPos.
	ImageCoords Difference;

	//Firstly, check to see if Pos is inside the image.
	//Lower bounds
	if(Pos.x < 0) StartPos.x = 0;
	else StartPos.x = Pos.x;
	Difference.x = StartPos.x - Pos.x;

	if(Pos.y < 0) StartPos.y = 0;
	else StartPos.y = Pos.y;
	Difference.y = StartPos.y - Pos.y;

	//Higher bounds - just need to set EndPos
	EndPos.x = Pos.x + xBlockSize;
	if(EndPos.x > ImageWidth) EndPos.x = ImageWidth;
	EndPos.y = Pos.y + yBlockSize;
	if(EndPos.y > ImageHeight) EndPos.y = ImageHeight;


		//Quick process where we can just copy from the double size image.
	for(int c = StartPos.y, wY = Difference.y; c < EndPos.y; ++c, ++wY){
		for(int l = StartPos.x, wX = Difference.x; l < EndPos.x; ++l, ++wX){
			arith->DoArith(pic_data[c][l],CalcValueType(dc),Weights[wY][wX]);
		}//l
	}//c
}
