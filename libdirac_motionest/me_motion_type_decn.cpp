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
* Contributor(s): Marc Servais (Original Author)
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

#include <libdirac_motionest/me_motion_type_decn.h>
#include <libdirac_common/frame_buffer.h>

using namespace dirac;

#include <algorithm>

using std::vector;


MotionTypeDecider::MotionTypeDecider()
{
}


MotionTypeDecider::~MotionTypeDecider()
{
}


void MotionTypeDecider::DoMotionTypeDecn(MvData& in_data)
{

	int step, split_depth; 

	unsigned int MBPUsUsingGlobal;
	unsigned int MBPUsNotUsingGlobal;
	unsigned int BlocksUsingGlobal = 0;
	unsigned int BlocksNotUsingGlobal = 0;
	unsigned int MBsUsingGlobal = 0;
	unsigned int MBsNotUsingGlobal = 0;

	in_data.QuantiseGlobalMotionParameters();
	in_data.DequantiseGlobalMotionParameters();
	in_data.GenerateGlobalMotionVectors();


	for (mb_yp = 0, mb_tlb_y = 0;  mb_yp < in_data.MBSplit().LengthY();  ++mb_yp, mb_tlb_y += 4)
	{
		for (mb_xp = 0,mb_tlb_x = 0; mb_xp < in_data.MBSplit().LengthX(); ++mb_xp,mb_tlb_x += 4)
		{

			split_depth = in_data.MBSplit()[mb_yp][mb_xp]; 
			step = 4  >>  (split_depth); 

			MBPUsUsingGlobal = 0;
			MBPUsNotUsingGlobal = 0;

			//now do all the Prediction Unit MVs in the MB 
			//("step" automatically handles block size, so might only have one PU in the macro-block)           
			for (b_yp = mb_tlb_y; b_yp < mb_tlb_y+4; b_yp += step)
			{
				for (b_xp = mb_tlb_x; b_xp < mb_tlb_x+4; b_xp += step)
				{
					if(in_data.Mode()[b_yp][b_xp] != INTRA)
					{
						// For the current prediction unit, decide if we want global or block motion
						DoPredUnitDecn(in_data, step);
						if(in_data.BlockUseGlobal()[b_yp][b_xp])
							MBPUsUsingGlobal++;
						else
							MBPUsNotUsingGlobal++;
					}
				}//b_xp
			}//b_yp

			// Update Counters
			BlocksUsingGlobal += MBPUsUsingGlobal * step * step; 
			BlocksNotUsingGlobal += MBPUsNotUsingGlobal * step * step; 

			// Determine GM choice for the current MB (based on its constituent PUs):
			if ((MBPUsNotUsingGlobal + MBPUsUsingGlobal) > 0) // true if an inter-macroblock 
			{
				if (MBPUsNotUsingGlobal == 0) // ALL PU's in MB using GM
				{
					MBsUsingGlobal++;
					in_data.MacroBlockUseGlobal()[mb_yp][mb_xp] = true;
				}
				else
				{
					MBsNotUsingGlobal++;
					in_data.MacroBlockUseGlobal()[mb_yp][mb_xp] = false;
				}
			}

		}//mb_xp
	}//mb_yp

	// Decide whether frame should use: "Some Global Motion", "Only Global Motion" or "No Global Motion"
	DoFrameDecn(in_data, MBsUsingGlobal, MBsNotUsingGlobal, BlocksUsingGlobal, BlocksNotUsingGlobal);		

	UpdateGlobalMotionFlags(in_data); // overide block motion type choices (if necessary)

}

void MotionTypeDecider::DoFrameDecn(MvData& in_data, int MBsUsingGlobal, int MBsNotUsingGlobal, int BlocksUsingGlobal, int BlocksNotUsingGlobal)
{

	std::cerr<<std::endl<<"Macro-Blocks using global / not using global: "<<MBsUsingGlobal<<" / "<<MBsNotUsingGlobal;
	std::cerr<<std::endl<<"Blocks using global / not using global      : "<<BlocksUsingGlobal<<" / "<<BlocksNotUsingGlobal;

	if 	(FLAG_GLOBAL_MOTION_BY_MACRO_BLOCK) // frame decision based on macro-blocks 
	{
		if (1*MBsUsingGlobal < 1*MBsNotUsingGlobal) // If less than 50% of Macro-blocks use Global Motion then
			in_data.SetGlobalMotionFlags(0,0); //(0,0)// NO Global Motion is used

		else if  (1*MBsUsingGlobal > 9*MBsNotUsingGlobal) // If more than 90% of MBs use Global Motion then
			in_data.SetGlobalMotionFlags(1,1); //(1,1)// ONLY Global Motion is used

		else
			in_data.SetGlobalMotionFlags(1,0); // Global Motion is used, but not exclusively
	}
	else	// frame decision based on blocks (frame area), but flagged per Prediction Unit
	{
		if (1*BlocksUsingGlobal < 1*BlocksNotUsingGlobal) // If less than 50% of Blocks use Global Motion then
			in_data.SetGlobalMotionFlags(0,0); //(0,0)// NO Global Motion is used

		else if  (1*BlocksUsingGlobal > 9*BlocksNotUsingGlobal) // If more than 90% of MBs use Global Motion then
			in_data.SetGlobalMotionFlags(1,1); //(1,1)// ONLY Global Motion is used

		else
			in_data.SetGlobalMotionFlags(1,0); // Global Motion is used, but not exclusively
	}

}



void MotionTypeDecider::DoPredUnitDecn(MvData& in_data, int step)
{
	const int max_mv_SqDiff = 16;	// Squared Motion Vector comparison threshold: (16 -> Half Pixel)

	bool UseMeanGMV = true;			// true : GMV based on all blocks in PU 
									// false: GMV based on top left block of PU only

	int gmv1x, gmv1y, gmv2x, gmv2y;
	if (UseMeanGMV)
	{
		gmv1x = 0;
		gmv1y = 0;
		gmv2x = 0;
		gmv2y = 0;

		// Global Motion Vector for the PU is the mean of the GMVs of its constituent blocks  
		for (int b_yp2 = b_yp; b_yp2 < b_yp+step; b_yp2++)
		{
			for (int b_xp2 = b_xp; b_xp2 < b_xp+step; b_xp2++)
			{
				gmv1x += in_data.GlobalMotionVectors(1)[b_yp2][b_xp2].x; 
				gmv1y += in_data.GlobalMotionVectors(1)[b_yp2][b_xp2].y; 
				gmv2x += in_data.GlobalMotionVectors(2)[b_yp2][b_xp2].x; 
				gmv2y += in_data.GlobalMotionVectors(2)[b_yp2][b_xp2].y; 
			}
		}
		gmv1x = (int)floor((float)gmv1x / (step*step) + 0.5);
		gmv1y = (int)floor((float)gmv1y / (step*step) + 0.5);
		gmv2x = (int)floor((float)gmv2x / (step*step) + 0.5);
		gmv2y = (int)floor((float)gmv2y / (step*step) + 0.5);
	}
	else // (UseMeanGMV==false)
	{
		gmv1x = in_data.GlobalMotionVectors(1)[b_yp][b_xp].x; 
		gmv1y = in_data.GlobalMotionVectors(1)[b_yp][b_xp].y; 
		gmv2x = in_data.GlobalMotionVectors(2)[b_yp][b_xp].x; 
		gmv2y = in_data.GlobalMotionVectors(2)[b_yp][b_xp].y; 
	}

	// Motion Vector for Current Prediction Unit:
	const int mv1x = in_data.Vectors(1)[b_yp][b_xp].x; 
	const int mv1y = in_data.Vectors(1)[b_yp][b_xp].y; 
	const int mv2x = in_data.Vectors(2)[b_yp][b_xp].x; 
	const int mv2y = in_data.Vectors(2)[b_yp][b_xp].y; 

	if (in_data.Mode()[b_yp][b_xp] == REF1_ONLY)
	{
		// Calculate squared difference between Global and PU motion vectors:
		int mv1SqDiff = (mv1x-gmv1x)*(mv1x-gmv1x) + (mv1y-gmv1y)*(mv1y-gmv1y);

		//std::cerr<<std::endl<<"MV1 = ("<<mv1x<<", "<<mv1y<<"); GMV1 = ("<<gmv1x<<", "<<gmv1y<<")"; 
		//std::cerr<<"  :  Squared Difference = " << mv1SqDiff; 

		// If difference from Global Motion Vector is small, then rather use Global Motion:
		if (mv1SqDiff <= max_mv_SqDiff)
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		else 
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;

	}
	else if (in_data.Mode()[b_yp][b_xp] == REF2_ONLY)
	{
		// Calculate squared difference between Global and PU motion vectors:
		int mv2SqDiff = (mv2x-gmv2x)*(mv2x-gmv2x) + (mv2y-gmv2y)*(mv2y-gmv2y);

		// If difference from Global Motion Vector is small, then rather use Global Motion:
		if (mv2SqDiff <= max_mv_SqDiff)
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		else
		{
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
		}
	}
	else if (in_data.Mode()[b_yp][b_xp] == REF1AND2)
	{
		// Calculate squared difference between Global and PU motion vectors:
		int mv1SqDiff = (mv1x-gmv1x)*(mv1x-gmv1x) + (mv1y-gmv1y)*(mv1y-gmv1y);
		int mv2SqDiff = (mv2x-gmv2x)*(mv2x-gmv2x) + (mv2y-gmv2y)*(mv2y-gmv2y);

		// If difference from BOTH Global Motion Vectors is small, then rather use Global Motion
		if ((mv1SqDiff <= max_mv_SqDiff) && (mv2SqDiff <= max_mv_SqDiff))
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		else
		{
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
		}
	}
}


void MotionTypeDecider::UpdateGlobalMotionFlags(MvData& in_data)
{
	// Overide individual MB/PU Global Motion Flags if we use the same Global Motion option for the entire frame: 
	bool UseGM, overide;

	if (in_data.m_use_global && in_data.m_use_global_only) // Only Global Motion
	{
		UseGM = true;
		overide = true;
	}
	else if (!in_data.m_use_global) // No Global Motion
	{
		UseGM = false;
		overide = true;
	}
	else
		overide = false;

	if (overide) {
		
		int x, y;

		if (FLAG_GLOBAL_MOTION_BY_MACRO_BLOCK) 
		{
			for (y = 0;  y < in_data.MacroBlockUseGlobal().LengthY();  y++) 
				for (x = 0;  x < in_data.MacroBlockUseGlobal().LengthX();  x++) 
					in_data.MacroBlockUseGlobal()[y][x] = UseGM;
		}
		for (y = 0;  y < in_data.BlockUseGlobal().LengthY();  y++) 
			for (x = 0;  x < in_data.BlockUseGlobal().LengthX();  x++) 
				in_data.BlockUseGlobal()[y][x] = UseGM;
	}
}

/*


void MotionTypeDecider::UpdateGlobalMotionFlags(MvData& in_data)
{

	int step,max; 
	int pstep,pmax; 
	int split_depth; 
	bool common_ref; 
	unsigned int UsingGlobal = 0;
	unsigned int NotUsingGlobal = 0;

	for (mb_yp = 0, mb_tlb_y = 0;  mb_yp < in_data.MBSplit().LengthY();  ++mb_yp, mb_tlb_y += 4)
	{
		for (mb_xp = 0,mb_tlb_x = 0; mb_xp < in_data.MBSplit().LengthX(); ++mb_xp,mb_tlb_x += 4)
		{
			split_depth = in_data.MBSplit()[mb_yp][mb_xp]; 
			step = 4 >> (split_depth);             

			//now do all the block mvs in the mb 
			//("step" automatically handles block size, so might only have one block in the macro-block)           
			for (b_yp = mb_tlb_y; b_yp < mb_tlb_y+4; b_yp += step)
			{
				for (b_xp = mb_tlb_x; b_xp < mb_tlb_x+4; b_xp += step)
				{

					// For the current prediction unit, specify if we want global or local motion
					if(in_data.Mode()[b_yp][b_xp] != INTRA)
					{

						if (in_data.m_use_global && in_data.m_use_global_only) // Only Global Motion
							in_data.BlockUseGlobal()[b_yp][b_xp] = true;

						else if (!in_data.m_use_global) // No Global Motion
							in_data.BlockUseGlobal()[b_yp][b_xp] = false;

					}
				}//b_xp
			}//b_yp    
		}//mb_xp
	}//mb_yp
}




void MotionTypeDecider::DoMotionTypeDecn(MvData& in_data)
{

int step,max; 
int pstep,pmax; 
int split_depth; 
bool common_ref; 
unsigned int UsingGlobal = 0;
unsigned int NotUsingGlobal = 0;
int GlobalMotionChoice;

in_data.QuantiseGlobalMotionParameters();
in_data.DequantiseGlobalMotionParameters();
in_data.GenerateGlobalMotionVectors();


for (mb_yp = 0, mb_tlb_y = 0;  mb_yp < in_data.MBSplit().LengthY();  ++mb_yp, mb_tlb_y += 4)
{
for (mb_xp = 0,mb_tlb_x = 0; mb_xp < in_data.MBSplit().LengthX(); ++mb_xp,mb_tlb_x += 4)
{
//start with split mode
split_depth = in_data.MBSplit()[mb_yp][mb_xp]; 

step = 4  >>  (split_depth); 
max = (1 << split_depth); 

//next do common_ref
if(split_depth != 0)
{
pstep = step; 
pmax = max; 
}
else
{
pstep = 4; 
pmax = 1; 
}
common_ref = in_data.MBCommonMode()[mb_yp][mb_xp]; 

step = 4 >> (split_depth);             

//now do all the Prediction Unit MVs in the MB 
//("step" automatically handles block size, so might only have one PU in the macro-block)           
for (b_yp = mb_tlb_y; b_yp < mb_tlb_y+4; b_yp += step)
{
for (b_xp = mb_tlb_x; b_xp < mb_tlb_x+4; b_xp += step)
{

// For the current prediction unit, decide if we want global or block motion
if(in_data.Mode()[b_yp][b_xp] != INTRA)
{
DoUnitDecn(in_data);
if(in_data.BlockUseGlobal()[b_yp][b_xp])
UsingGlobal++;
else
NotUsingGlobal++;
}
}//b_xp
}//b_yp    
}//mb_xp
}//mb_yp

std::cerr<<std::endl<<"Prediction Units using Global Motion    : "<< UsingGlobal;
std::cerr<<std::endl<<"Prediction Units not using Global Motion: "<< NotUsingGlobal;

if (75*UsingGlobal < 25*NotUsingGlobal) // If less than 25% of Prediction Units use Global Motion then
in_data.SetGlobalMotionFlags(0,0); //(0,0)// NO Global Motion is used

else if  (5*UsingGlobal > 95*NotUsingGlobal) // If more than 95% of Prediction Units use Global Motion then
in_data.SetGlobalMotionFlags(1,1); //(1,1)// ONLY Global Motion is used

else
in_data.SetGlobalMotionFlags(1,0); //(1,0)// ONLY Global Motion is used


UpdateGlobalMotionFlags(in_data);

}


void MotionTypeDecider::DoUnitDecn(MvData& in_data)
{
const int max_mv_diff = 1; // Motion Vector comparison threshold: Quarter pixel (horizontal and vertical)

const int mv1x = in_data.Vectors(1)[b_yp][b_xp].x; 
const int mv1y = in_data.Vectors(1)[b_yp][b_xp].y; 
const int gmv1x = in_data.GlobalMotionVectors(1)[b_yp][b_xp].x; 
const int gmv1y = in_data.GlobalMotionVectors(1)[b_yp][b_xp].y; 
const int mv2x = in_data.Vectors(2)[b_yp][b_xp].x; 
const int mv2y = in_data.Vectors(2)[b_yp][b_xp].y; 
const int gmv2x = in_data.GlobalMotionVectors(2)[b_yp][b_xp].x; 
const int gmv2y = in_data.GlobalMotionVectors(2)[b_yp][b_xp].y; 


if (in_data.Mode()[b_yp][b_xp] == REF1_ONLY)
{
// If difference from Global Motion Vector is small, then rather use Global Motion
if ((abs(mv1x-gmv1x) <= max_mv_diff) && (abs(mv1y-gmv1y) <= max_mv_diff))
in_data.BlockUseGlobal()[b_yp][b_xp] = true;
else 
{
//std::cerr<<std::endl<<"A*** "<<mv1x<<","<<gmv1x<<";"<<abs(mv1x-gmv1x)<< " "<<mv1y<<","<<gmv1y<<";"<<abs(mv1y-gmv1y);
in_data.BlockUseGlobal()[b_yp][b_xp] = false;
}
}
else if (in_data.Mode()[b_yp][b_xp] == REF2_ONLY)
{
// If difference from Global Motion Vector is small, then rather use Global Motion
if ((abs(mv2x-gmv2x) <= max_mv_diff) && (abs(mv2y-gmv2y) <= max_mv_diff))
in_data.BlockUseGlobal()[b_yp][b_xp] = true;
else
{
in_data.BlockUseGlobal()[b_yp][b_xp] = false;
//std::cerr<<std::endl<<"B***";
}
}
else if (in_data.Mode()[b_yp][b_xp] == REF1AND2)
{
// If difference from BOTH Global Motion Vectors is small, then rather use Global Motion
if ((abs(mv1x-gmv1x) <= max_mv_diff) && (abs(mv1y-gmv1y) <= max_mv_diff) && (abs(mv2x-gmv2x) <= max_mv_diff) && (abs(mv2y-gmv2y) <= max_mv_diff))
in_data.BlockUseGlobal()[b_yp][b_xp] = true;
else
{
in_data.BlockUseGlobal()[b_yp][b_xp] = false;
//std::cerr<<std::endl<<"C***";
}
}
}

*/
