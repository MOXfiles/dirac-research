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

#include <libdirac_motionest/reject_value.h>
using namespace dirac;

RejectValue::RejectValue(const FrameBuffer & frame_buffer,
                         const MEData & me_data,
                         unsigned int frame_number,
                         int ref_number,
                         ModelGlobalMotion & model,
                         TestGlobalMotionModel & test,
                         TwoDArray<int> & inliers,
                         const EncoderParams & encparams)
:
RejectMotionVectorOutliers(frame_buffer, me_data, frame_number,
                           ref_number, model, test, inliers,
                           encparams),

m_internal_inliers(inliers),
m_mv(me_data.Vectors(ref_number))
{
    m_debug = true;
}

RejectValue::~RejectValue()
{}

void RejectValue::Reject(int & score)
{
    // calculate standard deviation for x and y
    float sum_x = 0.0, sum_y = 0.0;
    int count = 0;
    for (int j=0;j<m_mv.LengthY();j++)
    {
        for (int i=0;i<m_mv.LengthX();i++)
        {
            if (m_global_inliers[j][i])
            {
                sum_x += m_mv[j][i].x;
                sum_y += m_mv[j][i].y;
                count++;
            }
        }
    }

    // if there are no inliers, do not continue
    if (!count)
    {
        if (m_debug) std::cerr << std::endl << "Reject value bailing! No inliers!";
        return;
    }

    float mean_x = sum_x / count;
    float mean_y = sum_y / count;
    float sqr_sum_x = 0, sqr_sum_y = 0;

    for (int j=0;j<m_mv.LengthY();j++)
    {
        for (int i=0;i<m_mv.LengthX();i++)
        {
            if (m_global_inliers[j][i])
            {
                sqr_sum_x += (m_mv[j][i].x - mean_x) * (m_mv[j][i].x - mean_x);
                sqr_sum_y += (m_mv[j][i].y - mean_y) * (m_mv[j][i].y - mean_y);
            }
        }
    }

    float sd_x = std::sqrt( sqr_sum_x / count );
    float sd_y = std::sqrt( sqr_sum_y / count );
    
    // calculate x threshold
    // set initial y threshold very high so no motion vectors are rejected for y value
    float threshold_y = 1000000;
    float threshold_x = 1000000;
    float best_threshold_x = 1000000;
    float best_threshold_y = 1000000;

    OneDArray<float> model_parameters(10);             // create vector to hold model parameters    
    MvFloatArray gmv(m_mv.LengthY(), m_mv.LengthX()); // create array to hold global motion vectors

    int best_score = score;

    // iterate x threshold
    for ( threshold_x = (1.6 * sd_x); threshold_x < (2.4 * sd_x); threshold_x += 0.5)
    {
        ResetInliers(m_internal_inliers, m_global_inliers);
        UpdateInliers(m_internal_inliers, threshold_x, threshold_y);
        m_model.CalculateModelParameters(m_mv, m_internal_inliers, model_parameters);
        m_model.GenerateGlobalMotionVectors(gmv, model_parameters);

        int new_score = m_test.Test(gmv, m_internal_inliers);

        if (new_score < best_score)
        {
            best_score = new_score;
            best_threshold_x = threshold_x;
        }
    }

    // calculate y threshold
    // iterate y threshold
    for ( threshold_y = (1.6 * sd_y); threshold_y < (2.4 * sd_y); threshold_y += 0.5)
    {
        ResetInliers(m_internal_inliers, m_global_inliers);
        UpdateInliers(m_internal_inliers, best_threshold_x, threshold_y);
        m_model.CalculateModelParameters(m_mv, m_internal_inliers, model_parameters);
        m_model.GenerateGlobalMotionVectors(gmv, model_parameters);

        int new_score = m_test.Test(gmv, m_internal_inliers);

        if (new_score < best_score)
        {
            best_score = new_score;
            best_threshold_y = threshold_y;
        }
    }

    // get best value for y threshold
    if (best_score < score)
    {
        // calculate score with best thresholds
        UpdateInliers(m_global_inliers, best_threshold_x, best_threshold_y);
        m_model.CalculateModelParameters(m_mv, m_global_inliers, model_parameters);
        m_model.GenerateGlobalMotionVectors(gmv, model_parameters);
        score = m_test.Test(gmv, m_internal_inliers);

        if (m_debug)
        {
            std::cerr << std::endl << "Reject value: Improved model, thresholds = " << threshold_x << ", " << threshold_y;
            std::cerr << ", score = " << best_score;
        }
    }
    else
        if (m_debug) std::cerr << std::endl << "Reject value: Failed to improve model";
}

void RejectValue::UpdateInliers(TwoDArray<int> & inliers, float threshold_x, float threshold_y)
{
    for (int j=0;j<m_mv.LengthY();j++)
    {
        for (int i=0;i<m_mv.LengthX();i++)
        {
            // value is greater than either component threshold
            if (std::abs(m_mv[j][i].x) >= threshold_x || std::abs(m_mv[j][i].y) >= threshold_y)
                inliers[j][i] = 0;
        }
    }
}
