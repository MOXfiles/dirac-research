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
 
#include <libdirac_motionest/reject_sad.h>
using namespace dirac;

RejectSAD::RejectSAD(const FrameBuffer & frame_buffer,
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
                           
m_internal_inliers(inliers.LengthY(), inliers.LengthX()),
m_SAD(me_data.PredCosts(ref_number)),
m_mv(me_data.Vectors(ref_number))
{
    m_debug = true;
}

RejectSAD::~RejectSAD()
{}

//! Do work
void RejectSAD::Reject(int & score)
{
    // calculate standard deviation
    float sum = 0.0;
    int count = 0;
    
    for (int j=0;j<m_SAD.LengthY();j++)
    {
        for (int i=0;i<m_SAD.LengthX();i++)
        {
            if (m_global_inliers[j][i])
            {
                sum += m_SAD[j][i].SAD;
                ++count;
            }
        }
    }
    
    float mean = sum / float(count);
    float sqr_sum = 0.0;

    for (int j=0;j<m_SAD.LengthY();j++)
    {
        for (int i=0;i<m_SAD.LengthX();i++)
        {
            if (m_global_inliers[j][i])
            {
                sqr_sum += (m_SAD[j][i].SAD - mean) * (m_SAD[j][i].SAD - mean);
            }
        }
    }

    float sd = std::sqrt(sqr_sum/float(count));

    OneDArray<float> model_parameters(10);                // create vector to hold model parameters
    MvFloatArray gmv(m_SAD.LengthY(), m_SAD.LengthX());  // create array to hold global motion vectors
    int best_score = score;
    float best_threshold;
    
    // iterate over SAD thresholds values to find the value which yields the lowest score
    for (float threshold = mean; threshold < (mean+(2*sd)); threshold += 100)
    {
        ResetInliers(m_internal_inliers, m_global_inliers);
        UpdateInliers(m_internal_inliers, threshold);
        m_model.CalculateModelParameters(m_mv, m_internal_inliers, model_parameters);
        m_model.GenerateGlobalMotionVectors(gmv, model_parameters);

        int new_score = m_test.Test(gmv, m_internal_inliers);
        
        if (new_score < best_score)
        {
            best_score = new_score;
            best_threshold = threshold;
        }
    }

    // threshold found, update global inlier array
    if (best_score < score)
    {
        UpdateInliers(m_global_inliers, best_threshold);    // update global outlier array
        score = best_score;                                 // update cummulative score
        
        /*
		if (m_debug)
        {
            std::cerr << std::endl << "Reject SAD: Improved model, threshold = " << best_threshold;
            std::cerr << ", score = " << best_score;
        }
		*/
    }
    /*
	else
        if (m_debug) std::cerr << std::endl << "Reject SAD: Failed to improve model";
	*/
}

void RejectSAD::UpdateInliers(TwoDArray<int> & inliers, float threshold)
{
    // index array to give automatic rejection without if statement
    int SAD_reject[10000];
    for (int i=0;i<10000;++i)   { SAD_reject[i] = 0; } // if block SAD is greater than or equal to the
    for (int i=0;i<1;++i)       { SAD_reject[i] = 1; } // threshold, reject

    for (int j=0;j<m_SAD.LengthY();++j)
    {
        for (int i=0;i<m_SAD.LengthX();++i)
        {
            //int temp = SAD_reject[int(m_SAD[j][i].SAD / threshold)] & m_global_inliers[j][i];
            inliers[j][i] *= SAD_reject[int(m_SAD[j][i].SAD / threshold)]; //temp; // update array
        }
    }
}
