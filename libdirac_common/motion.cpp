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

////////////////////////////////////////////////////////////////
//classes and functions for motion estimation and compensation//
////////////////////////////////////////////////////////////////

#include "motion.h"
#include <cmath>

//motion compensation stuff

//Overlapping blocks are acheived by applying a 2D raised cosine shape
//to them. This function facilitates the calculations
float RaisedCosine(float t, float B){
	if(std::abs(t)>(B+1.0)/2.0) return 0.0f;
	else if(std::abs(t)<(1.0-B)/2.0) return 1.0f;
	else return(0.5*(1.0+std::cos(3.141592654*(std::abs(t)-(1.0-B)/2.0)/B)));
}

//Calculates a weighting block.
//bparams defines the block parameters so the relevant weighting arrays can be created.
//FullX and FullY refer to whether the weight should be adjusted for the edge of an image.
//eg. 1D Weighting shapes in x direction

//  FullX true        FullX false
//     ***           ********
//   *     *                  *
//  *       *                  *
//*           *                  *

void CreateBlock(const OLBParams &bparams, bool FullX, bool FullY, CalcValueType** WeightArray){

	//Create temporary array.
	float** CalcArray = new float*[bparams.YBLEN];
	for(int i = 0; i < bparams.YBLEN; ++i)
		CalcArray[i] = new float[bparams.XBLEN];

	//Calculation variables
	float rolloffX = (float(bparams.XBLEN+1)/float(bparams.XBSEP)) - 1;
	float rolloffY = (float(bparams.YBLEN+1)/float(bparams.YBSEP)) - 1;
	float val;

	//Initialise the temporary array to one
	for(int y = 0; y < bparams.YBLEN; ++y){
		for(int x = 0; x < bparams.XBLEN; ++x){
			CalcArray[y][x] = 1;
		}
	}

	//Window temporary array in the x direction
	for(int y = 0; y < bparams.YBLEN; ++y){
		for(int x = 0; x < bparams.XBLEN; ++x){
			//Apply the window
			if(!FullX){
				if(x >= (bparams.XBLEN)>>1){
					val = (float(x) - (float(bparams.XBLEN-1)/2.0))/float(bparams.XBSEP);
					CalcArray[y][x] *= RaisedCosine(val,rolloffX);
				}
			}
			else{
				val = (float(x) - (float(bparams.XBLEN-1)/2.0))/float(bparams.XBSEP);
				CalcArray[y][x] *= RaisedCosine(val,rolloffX);
			}
		}
	}

	//Window the temporary array in the y direction
	for(int x = 0; x < bparams.XBLEN; ++x){
		for(int y = 0; y < bparams.YBLEN; ++y){
			//Apply the window			
			if(!FullY){
				if(y >= (bparams.YBLEN)>>1){
					val = (float(y) - (float(bparams.YBLEN-1)/2.0))/float(bparams.YBSEP);
					CalcArray[y][x] *= RaisedCosine(val,rolloffY);
				}
			}
			else{
				val = (float(y) - (float(bparams.YBLEN-1)/2.0))/float(bparams.YBSEP);
				CalcArray[y][x] *= RaisedCosine(val,rolloffY);
			}
		}
	}

	//Convert the temporary float array into our
	//weight array by multiplying the floating
	//point values by 1024. This can be removed
	//later using a right shift of ten.
	float g;
	for(int y = 0; y < bparams.YBLEN; ++y){
		for(int x = 0; x < bparams.XBLEN; ++x){
			g = floor((CalcArray[y][x]*1024)+0.5);
			WeightArray[y][x] = ValueType(g);
		}
	}

	//Delete the temporary array
	for(int i = 0; i < bparams.YBLEN; ++i)
		delete[] CalcArray[i];
	delete[] CalcArray;
}

//Flips the values in an array in the x direction.
void FlipX(CalcValueType** Original, const OLBParams &bparams, CalcValueType** Flipped){
	for(int x = 0; x < bparams.XBLEN; ++x){
		for(int y = 0; y < bparams.YBLEN; ++y){
			Flipped[y][x] = Original[y][(bparams.XBLEN-1) - x];
		}
	}
}

//Flips the values in an array in the y direction.
void FlipY(CalcValueType** Original, const OLBParams &bparams, CalcValueType** Flipped){
	for(int x = 0; x < bparams.XBLEN; ++x){
		for(int y = 0; y < bparams.YBLEN; ++y){
			Flipped[y][x] = Original[(bparams.YBLEN-1) - y][x];
		}
	}
}
