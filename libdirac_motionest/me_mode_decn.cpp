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
* Contributor(s): Thomas Davies (Original Author),
*                 Tim Borer
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

#include <libdirac_motionest/me_mode_decn.h>
#include <libdirac_common/picture_buffer.h>
using namespace dirac;

#include <algorithm>

using std::vector;

ModeDecider::ModeDecider( const EncoderParams& encp):
    m_encparams( encp ),
    m_level_factor(3),
    m_mode_factor(3),
    m_me_data_set(3)
{

    // The following factors normalise costs for sub-MBs and MBs to those of 
    // blocks, so that the overlap is take into account (e.g. a sub-MB has
    // length XBLEN+XBSEP and YBLEN+YBSEP). The MB costs for a 1x1 
    // decomposition are not directly comprable to those for other decompositions
    // because of the block overlaps. These factors remove these effects, so that
    // all SAD costs are normalised to the area corresponding to non-overlapping
    // 16 blocks of size XBLEN*YBLEN.    

   m_level_factor[0] = float( 16 * m_encparams.LumaBParams(2).Xblen() * m_encparams.LumaBParams(2).Yblen() )/
       float( m_encparams.LumaBParams(0).Xblen() * m_encparams.LumaBParams(0).Yblen() );

   m_level_factor[1] = float( 4 * m_encparams.LumaBParams(2).Xblen() * m_encparams.LumaBParams(2).Yblen() )/
       float( m_encparams.LumaBParams(1).Xblen() * m_encparams.LumaBParams(1).Yblen() );

   m_level_factor[2] = 1.0f;

    for (int i=0 ; i<=2 ; ++i)
        m_mode_factor[i] = 80.0*std::pow(0.8 , 2-i);
}


ModeDecider::~ModeDecider()
{
    if (fsort.IsInter())
    {
        delete m_me_data_set[0];
        delete m_me_data_set[1];
    }
}

void ModeDecider::DoModeDecn(const PictureBuffer& my_buffer, int frame_num, MEData& me_data)
{

     // We've got 'raw' block motion vectors for up to two reference frames. Now we want
     // to make a decision as to mode. In this initial implementation, this is bottom-up
    // i.e. find mvs for MBs and sub-MBs and see whether it's worthwhile merging.    

    int ref1,ref2;

    // Initialise // 
    ////////////////

    fsort = my_buffer.GetPicture(frame_num).GetPparams().PicSort();
    if (fsort.IsInter())
    {
        // Extract the references
        const vector<int>& refs = my_buffer.GetPicture(frame_num).GetPparams().Refs();
        num_refs = refs.size();
        ref1 = refs[0];

        // The picture we're doing estimation from
        m_pic_data = &(my_buffer.GetComponent( frame_num , Y_COMP));

        // Set up the hierarchy of motion vector data objects
        m_me_data_set[0] = new MEData( m_encparams.XNumMB() , m_encparams.YNumMB() , 
                                       m_encparams.XNumBlocks()/4 , m_encparams.YNumBlocks()/4, num_refs );
        m_me_data_set[1] = new MEData( m_encparams.XNumMB() , m_encparams.YNumMB() , 
                                       m_encparams.XNumBlocks()/2 , m_encparams.YNumBlocks()/2, num_refs );

        m_me_data_set[2] = &me_data;

        // Set up the lambdas to use per block
        m_me_data_set[0]->SetLambdaMap( 0 , me_data.LambdaMap() , 1.0/m_level_factor[0] );
        m_me_data_set[1]->SetLambdaMap( 1 , me_data.LambdaMap() , 1.0/m_level_factor[1] );

        // Set up the reference pictures
        m_ref1_updata = &(my_buffer.GetUpComponent( ref1 , Y_COMP));

        if (num_refs>1)
        {
            ref2 = refs[1];
            m_ref2_updata = &(my_buffer.GetUpComponent( ref2 , Y_COMP));
            // Create an object for computing bi-directional prediction calculations            

            if ( m_encparams.MVPrecision()==MV_PRECISION_EIGHTH_PIXEL )
                m_bicheckdiff = new BiBlockEighthPel( *m_ref1_updata ,
                                                      *m_ref2_updata ,
                                                      *m_pic_data );
            else if ( m_encparams.MVPrecision()==MV_PRECISION_QUARTER_PIXEL )
                m_bicheckdiff = new BiBlockQuarterPel( *m_ref1_updata ,
                                                       *m_ref2_updata ,
                                                       *m_pic_data );
            else
                m_bicheckdiff = new BiBlockHalfPel( *m_ref1_updata ,
                                                    *m_ref2_updata ,
                                                    *m_pic_data );
        }
        else
        {    
            ref2 = ref1;
        }


        // Create an object for doing intra calculations
        m_intradiff = new IntraBlockDiff( *m_pic_data );

        // Loop over all the macroblocks, doing the work //
        ///////////////////////////////////////////////////

        for (m_ymb_loc=0 ; m_ymb_loc<m_encparams.YNumMB() ; ++m_ymb_loc )
        {
            for (m_xmb_loc=0 ; m_xmb_loc<m_encparams.XNumMB(); ++m_xmb_loc )
            {
                DoMBDecn();
            }//m_xmb_loc        
        }//m_ymb_loc

        delete m_intradiff;
        if (num_refs>1)
            delete m_bicheckdiff;
    }
}

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
        m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] = MB_cost;
    }

    if ( level<2 && MB_cost <= m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] )
    {
        m_me_data_set[2]->MBCosts()[m_ymb_loc][m_xmb_loc] = MB_cost;
        m_me_data_set[2]->MBSplit()[m_ymb_loc][m_xmb_loc] = level;
        
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
            AddNewVlist( cand_list , guide_data.Vectors(1)[guide_ypos+j][guide_xpos+i] , 0 , 0 );

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

    BlockMatcher my_bmatch1( *m_pic_data , 
                             *m_ref1_updata , 
                              m_encparams.LumaBParams(level) ,
                              m_encparams.MVPrecision(),
                              me_data.Vectors(1) , me_data.PredCosts(1) );
    me_data.PredCosts(1)[ypos][xpos].total = 100000000.0f;
    my_bmatch1.FindBestMatchSubp( xpos , ypos , cand_list, mv_pred, lambda );

    if (num_refs>1)
    {//do the same for the other reference

        cand_list.clear();                

        for ( int j=0 ; j<2 ; ++j )
            for (int i=0 ; i<2 ; ++i )
                AddNewVlist( cand_list , guide_data.Vectors(2)[guide_ypos+j][guide_xpos+i] , 0 , 0 );

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

        BlockMatcher my_bmatch2( *m_pic_data , 
                                 *m_ref2_updata , 
                                 m_encparams.LumaBParams(level) ,
                                 m_encparams.MVPrecision(),
                                 me_data.Vectors(2) , me_data.PredCosts(2) );
        me_data.PredCosts(2)[ypos][xpos].total = 100000000.0f;
        my_bmatch2.FindBestMatchSubp( xpos , ypos , cand_list, mv_pred, lambda );

     }
}



float ModeDecider::DoUnitDecn(const int xpos , const int ypos , const int level )
{
    // For a given prediction unit (MB, subMB or block) find the best
    // mode, given that the REF1 and REF2 motion estimation has
    // already been done.

    MEData& me_data = *( m_me_data_set[level] );

    // Coords of the top-leftmost block belonging to this unit
//    const int xblock = xpos<<(2-level);
//    const int yblock = ypos<<(2-level);

    const float loc_lambda = me_data.LambdaMap()[ypos][xpos];

    float unit_cost;
    float mode_cost(0.0);
    float min_unit_cost;
    float best_SAD_value;
 
    BlockDiffParams dparams;

    dparams.SetBlockLimits( m_encparams.LumaBParams( level ) , *m_pic_data, xpos , ypos);

     // First check REF1 costs //
    /**************************/

//    mode_cost = ModeCost( xblock , yblock )*m_mode_factor[level];
    me_data.Mode()[ypos][xpos] = REF1_ONLY;
    me_data.PredCosts(1)[ypos][xpos].total *= m_level_factor[level];
    min_unit_cost = me_data.PredCosts(1)[ypos][xpos].total + mode_cost;
    best_SAD_value = me_data.PredCosts(1)[ypos][xpos].SAD;

    if (num_refs>1)
    {
       // Next check REF2 costs //
       /*************************/

//        mode_cost = ModeCost( xblock , yblock )*m_mode_factor[level];
        me_data.PredCosts(2)[ypos][xpos].total *= m_level_factor[level];
        unit_cost = me_data.PredCosts(2)[ypos][xpos].total + mode_cost;
        if ( unit_cost<min_unit_cost )
        {
            me_data.Mode()[ypos][xpos] = REF2_ONLY;
            min_unit_cost = unit_cost;
            best_SAD_value = me_data.PredCosts(2)[ypos][xpos].SAD;
        }

        // Calculate the cost if we were to use bi-predictions //
        /****************************************************************/
//        mode_cost = ModeCost( xpos , ypos )*m_mode_factor[level];

        me_data.BiPredCosts()[ypos][xpos].mvcost =
                                       me_data.PredCosts(1)[ypos][xpos].mvcost+
                                       me_data.PredCosts(2)[ypos][xpos].mvcost;

        me_data.BiPredCosts()[ypos][xpos].SAD = m_bicheckdiff->Diff(dparams , 
                                                  me_data.Vectors(1)[ypos][xpos] , 
                                                  me_data.Vectors(2)[ypos][xpos] );

        me_data.BiPredCosts()[ypos][xpos].SetTotal( loc_lambda );

        me_data.BiPredCosts()[ypos][xpos].total *= m_level_factor[level];
        unit_cost = me_data.BiPredCosts()[ypos][xpos].total + mode_cost;

        if ( unit_cost<min_unit_cost )
        {
            me_data.Mode()[ypos][xpos] = REF1AND2;
            min_unit_cost = unit_cost;
            best_SAD_value = me_data.BiPredCosts()[ypos][xpos].SAD;
        }

    }

    // Calculate the cost if we were to code the block as intra //
    /************************************************************/

    if (best_SAD_value> 4.0*m_encparams.LumaBParams( level ).Xblen()*m_encparams.LumaBParams( level ).Yblen() )
    {
//        mode_cost = ModeCost( xblock , yblock ) * m_mode_factor[level];
        me_data.IntraCosts()[ypos][xpos] = m_intradiff->Diff( dparams , me_data.DC( Y_COMP )[ypos][xpos] );
//        me_data.IntraCosts()[ypos][xpos] += loc_lambda * 
//                                       GetDCVar( me_data.DC( Y_COMP )[ypos][xpos] , GetDCPred( xblock , yblock ) );
        me_data.IntraCosts()[ypos][xpos] *= m_level_factor[level];
        unit_cost = me_data.IntraCosts()[ypos][xpos] +  mode_cost;

        if ( unit_cost<min_unit_cost && me_data.IntraCosts()[ypos][xpos]<0.9*best_SAD_value)
        {
            me_data.Mode()[ypos][xpos] = INTRA;
            min_unit_cost = unit_cost;
        }
    }

    return min_unit_cost;
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

float ModeDecider::ModeCost(const int xindex , const int yindex)
{
    // Computes the variation of the given mode, predmode, from its immediate neighbours
    // First, get a prediction for the mode

    unsigned int mode_predictor = (unsigned int)(REF1_ONLY);
    const TwoDArray<PredMode>& preddata( m_me_data_set[2]->Mode() );
    
    unsigned int num_ref1_nbrs( 0 ); 
    unsigned int num_ref2_nbrs( 0 );
    
    if (xindex > 0 && yindex > 0)
    {
        num_ref1_nbrs += ((unsigned int)( preddata[yindex-1][xindex] ) ) & 1; 
        num_ref1_nbrs += ((unsigned int)( preddata[yindex-1][xindex-1] ) ) & 1; 
        num_ref1_nbrs += ((unsigned int)( preddata[yindex][xindex-1] ) ) & 1;

        mode_predictor = num_ref1_nbrs>>1;

        num_ref2_nbrs += ((unsigned int)( preddata[yindex-1][xindex] ) ) & 2; 
        num_ref2_nbrs += ((unsigned int)( preddata[yindex-1][xindex-1] ) ) & 2; 
        num_ref2_nbrs += ((unsigned int)( preddata[yindex][xindex-1] ) ) & 2; 
        num_ref2_nbrs >>= 1;

        mode_predictor ^= ( (num_ref2_nbrs>>1)<<1 );
    }
    else if (xindex > 0 && yindex == 0)
        mode_predictor = (unsigned int)( preddata[0][xindex-1] ); 
    else if (xindex == 0 && yindex > 0)
        mode_predictor = (unsigned int)( preddata[yindex-1][0] ); 

    unsigned int var = (mode_predictor & 1)+((mode_predictor>>1) &1);

    return var*m_me_data_set[2]->LambdaMap()[yindex][xindex];
}


float ModeDecider::GetDCVar( const ValueType dc_val , const ValueType dc_pred)
{
    return 4.0*std::abs( static_cast<float>( dc_val - dc_pred ) );
}
