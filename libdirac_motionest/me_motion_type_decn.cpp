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
* Contributor(s): Thomas Davies (Original Author)
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
/*    if (fsort != I_frame)
    {
        delete m_me_data_set[0];
        delete m_me_data_set[1];
    }
*/
}

int MotionTypeDecider::DoMotionTypeDecn(MvData& in_data)
{

	int step,max; 
	int pstep,pmax; 
	int split_depth; 
	bool common_ref; 
	int UsingGlobal = 0;
	int NotUsingGlobal = 0;
	int GlobalMotionChoice;
	
	//MB_count = 0; 

	for (mb_yp = 0, mb_tlb_y = 0;  mb_yp < in_data.MBSplit().LengthY();  ++mb_yp, mb_tlb_y += 4)
	{
		for (mb_xp = 0,mb_tlb_x = 0; mb_xp < in_data.MBSplit().LengthX(); ++mb_xp,mb_tlb_x += 4)
		{
			//start with split mode
			//CodeMBSplit(in_data); 
			split_depth = in_data.MBSplit()[mb_yp][mb_xp]; 

			step = 4  >>  (split_depth); 
			max = (1 << split_depth); 

			//next do common_ref
			if(split_depth != 0)
			{
				//CodeMBCom(in_data); 
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
	if (UsingGlobal==0)
		GlobalMotionChoice = 0; // No Global Motion
	else if  (NotUsingGlobal==0)
		GlobalMotionChoice = 1; // Only Global Motion
	else
		GlobalMotionChoice = 2; // Allow each Pred Unit to choose global/block motion



	return GlobalMotionChoice; // indicate use_global_motion and use_global_motion_only flags
}




void MotionTypeDecider::DoUnitDecn(MvData& in_data)
{
	const int max_mv_diff = 4; // Motion Vector comparison threshold

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
		if ((abs(mv1x-gmv1x) < max_mv_diff) && (abs(mv1y-gmv1y) < max_mv_diff))
		{
			in_data.Vectors(1)[b_yp][b_xp].x = gmv1x;
			in_data.Vectors(1)[b_yp][b_xp].y = gmv1y;
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		}
		else
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
	}
	else if (in_data.Mode()[b_yp][b_xp] == REF2_ONLY)
	{
		// If difference from Global Motion Vector is small, then rather use Global Motion
		if ((abs(mv2x-gmv2x) < max_mv_diff) && (abs(mv2y-gmv2y) < max_mv_diff))
		{
			in_data.Vectors(2)[b_yp][b_xp].x = gmv2x;
			in_data.Vectors(2)[b_yp][b_xp].y = gmv2y;
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		}
		else
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
	}
	else if (in_data.Mode()[b_yp][b_xp] == REF1AND2)
	{
		// If difference from BOTH Global Motion Vectors is small, then rather use Global Motion
		if ((abs(mv1x-gmv1x) < max_mv_diff) && (abs(mv1y-gmv1y) < max_mv_diff) && (abs(mv2x-gmv2x) < max_mv_diff) && (abs(mv2y-gmv2y) < max_mv_diff))
		{
			in_data.Vectors(1)[b_yp][b_xp].x = gmv1x;
			in_data.Vectors(1)[b_yp][b_xp].y = gmv1y;
			in_data.Vectors(2)[b_yp][b_xp].x = gmv2x;
			in_data.Vectors(2)[b_yp][b_xp].y = gmv2y;
			in_data.BlockUseGlobal()[b_yp][b_xp] = true;
		}
		else
			in_data.BlockUseGlobal()[b_yp][b_xp] = false;
	}
}



/*
void ModeDecider::DoMBDecn()
{
      // Does the mode decision for the given MB, in three stages

    // Start with 4x4 modes
    DoLevelDecn(2);
    float old_best_MB_cost = m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc];

    // Next do 2x2 modes
    DoLevelDecn(1);

    // Do 1x1 mode if merging worked before
    if ( m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] <= old_best_MB_cost)
    {
        old_best_MB_cost = m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc];        
        DoLevelDecn(0);
    }

}

void ModeDecider::DoLevelDecn( int level )
{
    // Computes the best costs if we were to
    // stick to a decomposition at this level

    // Looks at two cases: the prediction mode is
    // constant across the MB; and the pred mode
    // for each constituent is different.

    // The limits of the prediction units
    const int xstart = m_xmb_loc <<level;
    const int ystart = m_ymb_loc <<level;

    const int xend = xstart + (1<<level);
    const int yend = ystart + (1<<level);

    //    Case 1: prediction modes are all different

    float MB_cost = 0.0;    
    for ( int j=ystart ; j<yend ; ++j)
    {
        for (int i=xstart ; i<xend ; ++i)
       {
           if ( level<2 )
               DoME( i , j , level);
            MB_cost += DoUnitDecn( i , j ,level );

        }// i
    }// j

    // if we've improved on the best cost, we should propagate data in 
    // the base level motion vector set
    if (level == 2)
    {
        m_me_data_set[2]->MBSplit()[m_ymb_loc][m_xmb_loc] = 2;
        m_me_data_set[2]->MBCommonMode()[m_ymb_loc][m_xmb_loc] = false;
        m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] = MB_cost;
    }

    if ( level<2 && MB_cost <= m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] )
    {
        m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] = MB_cost;
        m_me_data_set[2]->MBSplit()[m_ymb_loc][m_xmb_loc] = level;
        m_me_data_set[2]->MBCommonMode()[m_ymb_loc][m_xmb_loc] = false;

        // Parameters of the base-level blocks corresponding to each
        // prediction unit
        int xblock_start;
        int yblock_start;
        int xblock_end;
        int yblock_end;

        for ( int j=ystart ; j<yend ; ++j )
        {
            yblock_start = j<<(2-level);
            yblock_end = (j+1)<<(2-level);
            for ( int i=xstart ; i<xend ; ++i )
            {
                xblock_start = i<<(2-level);
                xblock_end = (i+1)<<(2-level);

                for ( int v=yblock_start ; v<yblock_end ; ++v )
                {
                    for ( int u=xblock_start ; u<xblock_end ; ++u )
                    {
                        m_me_data_set[2]->Mode()[v][u] = m_me_data_set[level]->Mode()[j][i];
                        m_me_data_set[2]->DC( Y_COMP )[v][u] = m_me_data_set[level]->DC( Y_COMP )[j][i];
                        m_me_data_set[2]->Vectors(1)[v][u] = m_me_data_set[level]->Vectors(1)[j][i];
                        if ( num_refs>1 )
                            m_me_data_set[2]->Vectors(2)[v][u] = m_me_data_set[level]->Vectors(2)[j][i];

                    }// u
                }// v

            }// i
        }// j

    }

    //     Case 2: prediction modes are all the same

    PredMode predmode;

    MB_cost = DoCommonMode( predmode , level );

    if ( MB_cost <= m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] )
    {
        m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] = MB_cost;
        m_me_data_set[2]->MBSplit()[m_ymb_loc][m_xmb_loc] = level;
        m_me_data_set[2]->MBCommonMode()[m_ymb_loc][m_xmb_loc] = true;
        // Parameters of the base-level blocks corresponding to each
        // prediction unit
        int xblock_start;
        int yblock_start;
        int xblock_end;
        int yblock_end;

        for ( int j=ystart ; j<yend ; ++j )
        {
            yblock_start = j<<(2-level);
            yblock_end = (j+1)<<(2-level);
            for ( int i=xstart ; i<xend ; ++i )
            {
                xblock_start = i<<(2-level);
                xblock_end = (i+1)<<(2-level);
                for ( int v=yblock_start ; v<yblock_end ; ++v )
                {
                    for ( int u=xblock_start ; u<xblock_end ; ++u )
                    {
                        m_me_data_set[2]->Vectors(1)[v][u] = m_me_data_set[level]->Vectors(1)[j][i];
                        m_me_data_set[2]->Mode()[v][u] = predmode;
                        m_me_data_set[2]->DC( Y_COMP )[v][u] = m_me_data_set[level]->DC( Y_COMP )[j][i];
                        if ( num_refs>1 )
                            m_me_data_set[2]->Vectors(2)[v][u] = m_me_data_set[level]->Vectors(2)[j][i];

                    }// u
                }// v
 
            }// i
        }// j
      }

}


void ModeDecider::DoME(const int xpos , const int ypos , const int level)
{
    // Do motion estimation for a prediction unit using the 
    // four vectors derived from the next level as a guide

    MEData& me_data = *(m_me_data_set[level]);
    const MEData& guide_data = *(m_me_data_set[level+1]);

    // The corresponding location of the guide data
    const int guide_xpos = xpos<<1; 
    const int guide_ypos = ypos<<1;

    // The location of the lowest level vectors
    const int xblock = xpos << ( 2 - level); 
    const int yblock = ypos << ( 2 - level);

    // The list of potential candidate vectors
    CandidateList cand_list;

    // The lambda to use for motion estimation
    const float lambda = me_data.LambdaMap()[ypos][xpos];

    // The predicting motion vector
    MVector mv_pred;

    for ( int j=0 ; j<2 ; ++j )
        for (int i=0 ; i<2 ; ++i )
            AddNewVlist( cand_list , guide_data.Vectors(1)[guide_ypos+j][guide_xpos+i] , 1 , 1 );

    if (xblock>0 && yblock>0)
        mv_pred = MvMedian( m_me_data_set[2]->Vectors(1)[yblock][xblock-1] ,
                            m_me_data_set[2]->Vectors(1)[yblock-1][xblock-1],
                              m_me_data_set[2]->Vectors(1)[yblock-1][xblock]);
    else if (xblock==0 && yblock>0)
        mv_pred = MvMean( m_me_data_set[2]->Vectors(1)[yblock-1][xblock],
                          m_me_data_set[2]->Vectors(1)[yblock-1][xblock+1]);
    else if (xblock>0 && yblock==0)
        mv_pred = MvMean( m_me_data_set[2]->Vectors(1)[yblock][xblock-1],
                          m_me_data_set[2]->Vectors(1)[yblock+1][xblock-1]);
    else{
        mv_pred.x = 0;
        mv_pred.y = 0;
    }

    BlockMatcher my_bmatch1( *m_pic_data , *m_ref1_updata , m_encparams.LumaBParams(level) ,
                                                     me_data.Vectors(1) , me_data.PredCosts(1) );
    me_data.PredCosts(1)[ypos][xpos].total = 100000000.0f;
    my_bmatch1.FindBestMatchSubp( xpos , ypos , cand_list, mv_pred, lambda );

    if (num_refs>1)
    {//do the same for the other reference

        cand_list.clear();                

        for ( int j=0 ; j<2 ; ++j )
        {
            for (int i=0 ; i<2 ; ++i )
            {
                AddNewVlist( cand_list , guide_data.Vectors(2)[guide_ypos+j][guide_xpos+i] , 1 , 1 );
            }// i
        }// j

        if (xblock>0 && yblock>0)
            mv_pred = MvMedian( m_me_data_set[2]->Vectors(2)[yblock][xblock-1] ,
                                             m_me_data_set[2]->Vectors(2)[yblock-1][xblock-1],
                                               m_me_data_set[2]->Vectors(2)[yblock-1][xblock]);
        else if (xblock==0 && yblock>0)
            mv_pred = MvMean( m_me_data_set[2]->Vectors(2)[yblock-1][xblock],
                                           m_me_data_set[2]->Vectors(2)[yblock-1][xblock+1]);
        else if (xblock>0 && yblock==0)
            mv_pred = MvMean( m_me_data_set[2]->Vectors(2)[yblock][xblock-1],
                                           m_me_data_set[2]->Vectors(2)[yblock+1][xblock-1]);
        else{
             mv_pred.x = 0;
             mv_pred.y = 0;
        }

        BlockMatcher my_bmatch2( *m_pic_data , *m_ref2_updata , m_encparams.LumaBParams(level) ,
                                                     me_data.Vectors(2) , me_data.PredCosts(2) );
        me_data.PredCosts(2)[ypos][xpos].total = 100000000.0f;
        my_bmatch2.FindBestMatchSubp( xpos , ypos , cand_list, mv_pred, lambda );

     }
}
*/

/*
float ModeDecider::DoCommonMode( PredMode& predmode , const int level)
{
    // For a given level, examine the costs in the constituent
    // prediction units of the MB at that level and decide 
    // whether there should be a common prediction mode or not.

    const MEData& me_data = *( m_me_data_set[level] );

    // The total cost for the MB for each possible prediction mode
    OneDArray<float> MB_cost(4);
    for ( int i=0 ; i<4 ; ++i)
        MB_cost[i] = ModeCost( m_xmb_loc<<2 , m_ymb_loc , PredMode(i) )*m_mode_factor[0];

    // The limits of the prediction units
    const int xstart = m_xmb_loc <<level;
    const int ystart = m_ymb_loc <<level;

    const int xend = xstart + (1<<level);
    const int yend = ystart + (1<<level);

    for (int j=ystart ; j<yend ; ++j)
    {
        for (int i=xstart ; i<xend ; ++i)
        {
            MB_cost[INTRA] += me_data.IntraCosts()[j][i];
            MB_cost[REF1_ONLY] += me_data.PredCosts(1)[j][i].total;
            if ( num_refs>1 )
            {
                MB_cost[REF2_ONLY] += me_data.PredCosts(2)[j][i].total;
//                MB_cost[REF1AND2] += me_data.BiPredCosts()[j][i].total;
            }
        }// i
    }// i


    // Find the minimum
    predmode = INTRA;
    if ( MB_cost[REF1_ONLY]<MB_cost[predmode] )
        predmode = REF1_ONLY;

    if ( num_refs>1)
    {
        if ( MB_cost[REF2_ONLY]<MB_cost[predmode] )
            predmode = REF2_ONLY;
//         if ( MB_cost[REF1AND2]<MB_cost[predmode] )
//             predmode = REF1AND2;
    }
 
	//predmode = REF1_ONLY; // added by MARC - must REMOVE LATER !!!

    return MB_cost[predmode];
}

ValueType ModeDecider::GetDCPred( int xblock , int yblock )
{
    ValueType dc_pred = 128;

    if ( xblock>0 && m_me_data_set[2]->Mode()[yblock][xblock-1] == INTRA )
    {
        dc_pred = m_me_data_set[2]->DC( Y_COMP )[yblock][xblock-1];
        if ( yblock>0 && m_me_data_set[2]->Mode()[yblock-1][xblock] == INTRA )
        {
            dc_pred += m_me_data_set[2]->DC( Y_COMP )[yblock-1][xblock];
            dc_pred >>= 1;
        }
    }
     
    return dc_pred;
}

float ModeDecider::ModeCost(const int xindex , const int yindex , 
                 const PredMode predmode )
{
    // Computes the variation of the given mode, predmode, from its immediate neighbours
    // Currently, includes branches to cope with blocks on the edge of the picture.
    int i ,j;
    float diff;
    float var = 0.0;

    i = xindex-1;
    j = yindex;
    if ( i>=0)
    {
        diff = static_cast<float>( m_me_data_set[2]->Mode()[j][i] - predmode );
        var = std::abs( diff );
    }

    i = xindex-1;
    j = yindex-1;
    if ( i>=0 && j>=0)
    {
        diff = static_cast<float>( m_me_data_set[2]->Mode()[j][i] - predmode);
        var += std::abs( diff );
    }

    i = xindex;
    j = yindex-1;
    if ( j>=0 )
    {
        diff = static_cast<float>( m_me_data_set[2]->Mode()[j][i] - predmode );
        var += std::abs( diff );
    }

    return var*m_me_data_set[2]->LambdaMap()[yindex][xindex];
}

float ModeDecider::GetDCVar( const ValueType dc_val , const ValueType dc_pred)
{
    return 8.0*std::abs( static_cast<float>( dc_val - dc_pred ) );
}
*/ 