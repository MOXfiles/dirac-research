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

int MotionTypeDecider::DoMotionTypeDecn(MvData& in_data)
{

	int step,max; 
	int pstep,pmax; 
	int split_depth; 
	bool common_ref; 
	unsigned int UsingGlobal = 0;
	unsigned int NotUsingGlobal = 0;
	int GlobalMotionChoice;
	
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

			//now do all the block mvs in the mb 
			//("step" automatically handles block size, so might only have one block in the macro-block)           
			for (b_yp = mb_tlb_y; b_yp < mb_tlb_y+4; b_yp += step)
			{
				for (b_xp = mb_tlb_x; b_xp < mb_tlb_x+4; b_xp += step)
				{

					// For the current prediction unit, decide if we want global or local motion
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
		GlobalMotionChoice = 0; // NO Global Motion is used
	
	else if  (5*UsingGlobal > 95*NotUsingGlobal) // If more than 95% of Prediction Units use Global Motion then
		GlobalMotionChoice = 1; // ONLY Global Motion is used
	
	else
		GlobalMotionChoice = 2; // Allow each Pred Unit to choose global/block motion

	return GlobalMotionChoice; // indicate use_global_motion and use_global_motion_only flags
}


void MotionTypeDecider::DoUnitDecn(MvData& in_data)
{
	const int max_mv_diff = 2; // Motion Vector comparison threshold: Quarter pixel (horizontal and vertical)

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
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
	}
	else if (in_data.Mode()[b_yp][b_xp] == REF2_ONLY)
	{
		// If difference from Global Motion Vector is small, then rather use Global Motion
		if ((abs(mv2x-gmv2x) <= max_mv_diff) && (abs(mv2y-gmv2y) <= max_mv_diff))
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		else
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
	}
	else if (in_data.Mode()[b_yp][b_xp] == REF1AND2)
	{
		// If difference from BOTH Global Motion Vectors is small, then rather use Global Motion
		if ((abs(mv1x-gmv1x) <= max_mv_diff) && (abs(mv1y-gmv1y) <= max_mv_diff) && (abs(mv2x-gmv2x) <= max_mv_diff) && (abs(mv2y-gmv2y) <= max_mv_diff))
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		else
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
	}
}


