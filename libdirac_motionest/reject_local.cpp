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
 * Contributor(s): Chris Bowley (Original Author)
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
  
#include <libdirac_motionest/reject_local.h>
using namespace dirac;

RejectLocal::RejectLocal(const FrameBuffer & frame_buffer,
                         const MEData & me_data,
                         unsigned int frame_number,
                         int ref_number,
                         ModelGlobalMotion & model,
                         TestGlobalMotionModel & test,
                         TwoDArray<int> & inliers,
                         const EncoderParams & encparams)
:
RejectMotionVectorOutliers(frame_buffer,
                           me_data,
                           frame_number,
                           ref_number,
                           model,
                           test,
                           inliers,
                           encparams),

m_internal_inliers(inliers),
m_mv(me_data.Vectors(ref_number))
{
    m_debug = true;
}

RejectLocal::~RejectLocal()
{}

void RejectLocal::Reject(int & score)
{
    ResetInliers(m_internal_inliers, m_global_inliers);
    
    // carry out in 5 separate loops:
    // centre block
    // j=0
    // j=m_mv.LengthY()-1
    // i=0
    // i=m_mv.LengthX()-1

    // index array to give automatic rejection without if statement
    int reject[10000];
    for (int i=0;i<10000;++i)   { reject[i] = 0; } // if difference from mean is greater than 1, reject
    for (int i=0;i<2;++i)       { reject[i] = 1; }

    // loop over motion vectors with 1 block perimeter
    for (int j=1;j<m_mv.LengthY()-1;++j)
    {
        for (int i=1;i<m_mv.LengthX()-1;++i)
        {
            int sum_x=0, sum_y=0;
            float block_mean_x, block_mean_y;

            for (int k=-1;k<2;++k)
            {
                for (int l=-1;l<2;++l)
                {
                    sum_x += m_mv[j-l][i-k].x;
                    sum_y += m_mv[j-l][i-k].y;
                }
            }

            block_mean_x = (float)sum_x / 9;
            block_mean_y = (float)sum_y / 9;

            m_internal_inliers[j][i] *= reject[int(std::abs(block_mean_x-m_mv[j][i].x)+0.5)];
            m_internal_inliers[j][i] *= reject[int(std::abs(block_mean_y-m_mv[j][i].y)+0.5)];
        }
    }

    // j=0, not including i=0, i=m_mv.LengthX()-1
    for (int i=1;i<m_mv.LengthX()-1;++i)
    {
        int sum_x=0, sum_y=0;
        float block_mean_x, block_mean_y;

        for (int k=-1;k<2;++k)
        {
            for (int l=-1;l<1;++l)
            {
                sum_x += m_mv[0-l][i-k].x;
                sum_y += m_mv[0-l][i-k].y;
            }
        }

        block_mean_x = (float)sum_x / 6;
        block_mean_y = (float)sum_y / 6;

        m_internal_inliers[0][i] *= reject[int(std::abs(block_mean_x-m_mv[0][i].x)+0.5)];
        m_internal_inliers[0][i] *= reject[int(std::abs(block_mean_y-m_mv[0][i].y)+0.5)];
    }

    // j=m_mv.LengthY()-1, not including i=0, i=m_mv.LengthX()-1
    for (int i=1;i<m_mv.LengthX()-1;++i)
    {
        int sum_x=0, sum_y=0;
        float block_mean_x, block_mean_y;

        for (int k=-1;k<2;++k)
        {
            for (int l=0;l<2;++l)
            {
                sum_x += m_mv[m_mv.LengthY()-1-l][i-k].x;
                sum_y += m_mv[m_mv.LengthY()-1-l][i-k].y;
            }
        }

        block_mean_x = (float)sum_x / 6;
        block_mean_y = (float)sum_y / 6;

        m_internal_inliers[m_mv.LengthY()-1][i] *= reject[int(std::abs(block_mean_x-m_mv[m_mv.LengthY()-1][i].x)+0.5)];
        m_internal_inliers[m_mv.LengthY()-1][i] *= reject[int(std::abs(block_mean_y-m_mv[m_mv.LengthY()-1][i].y)+0.5)];

    }

    // i=0, not including j=0, j=m_mv.LengthY()-1
    for (int j=1;j<m_mv.LengthY()-1;++j)
    {
        int sum_x=0, sum_y=0;
        float block_mean_x, block_mean_y;

        for (int k=-1;k<1;++k)
        {
            for (int l=-1;l<2;++l)
            {
                sum_x += m_mv[j-l][0-k].x;
                sum_y += m_mv[j-l][0-k].y;
            }
        }

        block_mean_x = (float)sum_x / 6;
        block_mean_y = (float)sum_y / 6;

        m_internal_inliers[j][0] *= reject[int(std::abs(block_mean_x-m_mv[j][0].x)+0.5)];
        m_internal_inliers[j][0] *= reject[int(std::abs(block_mean_y-m_mv[j][0].y)+0.5)];
    }

    // i=m_mv.LengthX()-1, not including j=0, j=m_mv.LengthY()-1
    for (int j=1;j<m_mv.LengthY()-1;++j)
    {
        int sum_x=0, sum_y=0;
        float block_mean_x, block_mean_y;

        for (int k=0;k<2;++k)
        {
            for (int l=-1;l<2;++l)
            {
                sum_x += m_mv[j-l][m_mv.LengthX()-1-k].x;
                sum_y += m_mv[j-l][m_mv.LengthX()-1-k].y;
            }
        }

        block_mean_x = (float)sum_x / 6;
        block_mean_y = (float)sum_y / 6;

        m_internal_inliers[j][m_mv.LengthX()-1] *= reject[int(std::abs(block_mean_x-m_mv[j][m_mv.LengthX()-1].x)+0.5)];
        m_internal_inliers[j][m_mv.LengthX()-1] *= reject[int(std::abs(block_mean_y-m_mv[j][m_mv.LengthX()-1].y)+0.5)];
    }

    // do corner blocks
    for (int j=0;j<m_mv.LengthY();j+=m_mv.LengthY()-1)
    {
        for (int i=0;i<m_mv.LengthX();i+=m_mv.LengthX()-1)
        {
            int sum_x=0, sum_y=0;
            float block_mean_x, block_mean_y;
            int k_start, l_start, k_end, l_end;

            if (j==0)                   { l_start=-1; l_end=1;}
            if (j==m_mv.LengthY()-1)    { l_start=0;  l_end=2;}

            if (i==0)                   { k_start=-1; k_end=1;}
            if (i==m_mv.LengthX()-1)       { k_start=0;  k_end=2;}

            for (int k=k_start; k < k_end; k++)
            {
                for (int l=l_start; l < l_end; l++)
                {
                    sum_x += m_mv[j-l][i-k].x;
                    sum_y += m_mv[j-l][i-k].y;
                }
            }

            block_mean_x = (float)sum_x / 4;
            block_mean_y = (float)sum_y / 4;

            m_internal_inliers[j][i] *= reject[int(std::abs(block_mean_x-m_mv[j][i].x)+0.5)];
            m_internal_inliers[j][i] *= reject[int(std::abs(block_mean_y-m_mv[j][i].y)+0.5)];
        }
    }

    // test new outlier mask and update global mask and score if better
    OneDArray<float> model_parameters(10);                                           // create vector to hold model parameters
    MvFloatArray gmv(m_mv.LengthY(), m_mv.LengthX());
    m_model.CalculateModelParameters(m_mv, m_internal_inliers, model_parameters);   // calculate model parameters
    m_model.GenerateGlobalMotionVectors(gmv, model_parameters);                     // generate global motion vectors
    int new_score = m_test.Test(gmv, m_internal_inliers);                           // test new global motion vectors

    if (new_score < score)
    {
        // update cummulative score
        score = new_score;
        
        // update global inliers
        for (int j=0;j<m_mv.LengthY();++j)
        {
            for (int i=0;i<m_mv.LengthX();++i)
            {
                m_global_inliers[j][i] *= m_internal_inliers[j][i];
            }
        }

        if (m_debug) std::cerr << std::endl << "Reject local: Improved model";
    }

    else
        if (m_debug) std::cerr << std::endl << "Reject local: Failed to improve model";
}

