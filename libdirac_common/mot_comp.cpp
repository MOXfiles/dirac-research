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
* Revision 1.5  2004-05-24 12:38:55  tjdwave
* Replaced spagetti code for linear interpolation in motion compensation
* and motion estimation routines with simple loops. Code is much clearer,
* although possibly slightly slower.
*
* Revision 1.3  2004/05/19 09:16:48  tjdwave
* Replaced zero-padding with edge-padding to eliminate colour-fringeing at low bitrates. Mod to set padded values to 0 when compensating frames.
*
* Revision 1.2  2004/05/12 08:35:34  tjdwave
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
MotionCompensator::MotionCompensator(const CodecParams &cp): 
cparams(cp),
luma_or_chroma(true),
add_or_sub(SUBTRACT)
{

	//Arithmetic objects to avoid code duplication
	add = new ArithAddObj;
	subtract = new ArithSubtractObj;
	addhalf = new ArithHalfAddObj;
	subtracthalf = new ArithHalfSubtractObj;

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

	//Coords of the top-left corner of a block
	ImageCoords Pos;

	//Set-up our blocksize and store the size of the image
	ImageWidth = pic_data.length(0);
	ImageHeight = pic_data.length(1);

	//Loop for each block in the output image.
	//The CompensateBlock function will use the image pointed to by ref1up
	//and add the compensated pixels to the image pointed to by pic_data.
	CalcValueType** WeightIndex;

	//Loop over all the block rows

	Pos.y=-bparams.YOFFSET;
	for(int yBlock = 0; yBlock < cparams.Y_NUMBLOCKS; ++yBlock){
		Pos.x=-bparams.XOFFSET;
		//loop over all the blocks in a row
		for(int xBlock = 0 ; xBlock < cparams.X_NUMBLOCKS; ++xBlock){

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
			dc=dcarray[yBlock][xBlock]<<2;//DC is only given 8 bits, so need to shift to get 10-bit data
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

			//Increment the block horizontal position
			Pos.x+=bparams.XBSEP;
		}//xBlock
		//Okay, we've done all the actual blocks. Now if the picture is further padded
		//we need to set the padded values to zero beyond the last block in the row,
		//for all the picture lines in the block row.
		if (add_or_sub==SUBTRACT){//only need to do this when we're subtracting
			for (int y=yBlock*bparams.YBSEP;y<(yBlock+1)*bparams.YBSEP;++y){
				for (int x=(cparams.X_NUMBLOCKS*bparams.XBSEP);x<pic_data.length(0);++x){
					pic_data[y][x]=0;
				}//x
			}//y
		}//?add_or_sum

		//Increment the block vertical position
		Pos.y+=bparams.YBSEP;
	}//yBlock

	//Finally, now we've done all the blocks, we must set all padded lines below the last row equal to 0
	if (add_or_sub==SUBTRACT){//only need to do this when we're subtracting
		for (int y=cparams.Y_NUMBLOCKS*bparams.YBSEP;y<pic_data.length(1);++y){
			for (int x=0;x<pic_data.length(0);++x){
				pic_data[y][x]=0;
			}//x
		}//y
	}//?add_or_sum

}

void MotionCompensator::CompensateBlock(PicArray &pic_data, const PicArray &refup_data, const MVector &Vec, 
const ImageCoords Pos, CalcValueType** Weights){

	//Coordinates in the image being written to.
	const ImageCoords StartPos(std::max(Pos.x,0),std::max(Pos.y,0));
	const ImageCoords EndPos(std::min(Pos.x + xBlockSize,ImageWidth),std::min(Pos.y + yBlockSize,ImageHeight));

	//The difference between the desired start point
	//Pos and the actual start point StartPos.
	const ImageCoords Difference(StartPos.x - Pos.x,StartPos.y - Pos.y);

	//Set up the start point in the reference image by rounding the motion vector
	//NB: bit shift rounds negative values DOWN, as required
	const MVector roundvec(Vec.x>>2,Vec.y>>2);

	//Get the remainder after rounding. NB rmdr values always 0,1,2 or 3
	const MVector rmdr(Vec.x-(roundvec.x<<2),Vec.y-(roundvec.y<<2));

	//Where to start in the upconverted image
	const ImageCoords RefStart((StartPos.x<<1) + roundvec.x,(StartPos.y<<1) + roundvec.y);

	//weights for doing linear interpolation, calculated from the remainder values
	const ValueType TLweight((4-rmdr.x)*(4-rmdr.y));
	const ValueType TRweight(rmdr.x*(4-rmdr.y));
	const ValueType BLweight((4-rmdr.x)*rmdr.y);
	const ValueType BRweight(rmdr.x*rmdr.y);

	//An additional stage to make sure the block to be copied does not fall outside
	//the reference image.
	const int DoubleXdim = ImageWidth * 2;
	const int DoubleYdim = ImageHeight * 2;
	bool DoBoundsChecking = false;

	//Check if there are going to be any problems copying the block from
	//the upvconverted reference image.
	if(RefStart.x < 0) DoBoundsChecking = true;
	else if(RefStart.x + ((EndPos.x - StartPos.x)<<1) >= DoubleXdim) DoBoundsChecking = true;
	if(RefStart.y < 0) DoBoundsChecking = true;
	else if(RefStart.y + ((EndPos.y - StartPos.y)<<1) >= DoubleYdim) DoBoundsChecking = true;

	//Temporary Variable.
	CalcValueType temp;


	if(!DoBoundsChecking){
		for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y; c < EndPos.y; ++c, ++wY, uY += 2){
			for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x; l < EndPos.x; ++l, ++wX, uX += 2){

				temp = (TLweight*refup_data[uY][uX] +
						TRweight*refup_data[uY][uX+1] +
						BLweight*refup_data[uY+1][uX] +
						BRweight*refup_data[uY+1][uX+1] +
						8)>>4;

				arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
			}//l
		}//c
	}
	else{
     	//We're doing bounds checking because we'll fall off the edge of the reference otherwise.

		for(int c = StartPos.y, wY = Difference.y, uY = RefStart.y,BuY=BChk(uY,DoubleYdim),BuY1=BChk(uY+1,DoubleYdim);
			c < EndPos.y; ++c, ++wY, uY += 2,BuY=BChk(uY,DoubleYdim),BuY1=BChk(uY+1,DoubleYdim)){
			for(int l = StartPos.x, wX = Difference.x, uX = RefStart.x,BuX=BChk(uX,DoubleXdim),BuX1=BChk(uX+1,DoubleXdim);
				l < EndPos.x; ++l, ++wX, uX += 2,BuX=BChk(uX,DoubleXdim),BuX1=BChk(uX+1,DoubleXdim)){

				temp = (TLweight*refup_data[BuY][BuX] +
						TRweight*refup_data[BuY][BuX1]  +
						BLweight*refup_data[BuY1][BuX]+
						BRweight*refup_data[BuY1][BuX1] +
						8)>>4;
				arith->DoArith(pic_data[c][l],CalcValueType(temp),Weights[wY][wX]);
			}//l
		}//c

	}

}

void MotionCompensator::DCBlock(PicArray &pic_data,const ValueType dc, const ImageCoords Pos, CalcValueType** Weights){

	//Coordinates in the image being written to.
	const ImageCoords StartPos(std::max(0,Pos.x),std::max(0,Pos.y));
	const ImageCoords EndPos(std::min(Pos.x + xBlockSize,ImageWidth),std::min(Pos.y + yBlockSize,ImageHeight));

	//The difference between the desired start point
	//Pos and the actual start point StartPos.
	const ImageCoords Difference(StartPos.x - Pos.x,StartPos.y - Pos.y);

	//Quick process where we can just copy from the double size image.
	for(int c = StartPos.y, wY = Difference.y; c < EndPos.y; ++c, ++wY){
		for(int l = StartPos.x, wX = Difference.x; l < EndPos.x; ++l, ++wX){
			arith->DoArith(pic_data[c][l],CalcValueType(dc),Weights[wY][wX]);
		}//l
	}//c
}
