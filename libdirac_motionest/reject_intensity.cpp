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
 
#include <libdirac_motionest/reject_intensity.h>
using namespace dirac;

RejectIntensity::RejectIntensity(const FrameBuffer & frame_buffer,
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

RejectIntensity::~RejectIntensity()
{}

void RejectIntensity::Reject(int & score)
{
    // luminance frame data
    const PicArray & Ydata = m_current_frame.Ydata();
    ResetInliers(m_internal_inliers, m_global_inliers);

    // find 8 x 8 block intensity variance
    for (int j=0;j<m_mv.LengthY();++j)
    {
        for (int i=0;i<m_mv.LengthX();++i)
        {
            int sum = 0;
            
            for (int j_px=(j*8);j_px<((j+1)*8);++j_px)
            {
                for (int i_px=(i*8);i_px<((i+1)*8);++i_px)
                {
                    sum += Ydata[j_px][i_px];
                }
            }

            float mean = (float)sum / 64;
            float sqr_sum = 0;
            
            for (int j_px=(j*8);j_px<((j+1)*8);++j_px)
            {
                for (int i_px=(i*8);i_px<((i+1)*8);++i_px)
                {
                    sqr_sum += ( Ydata[j_px][i_px] - mean ) * ( Ydata[j_px][i_px] - mean );
                }
            }

            float var = sqr_sum / 64;

            if (var<100) // equates to an 8-bit variance of 25 grey levels
            {
                m_internal_inliers[j][i] = 0;
            }
        }
    }

    OneDArray<float> model_parameters(10);                                       // create vector to hold model parameters
    MvFloatArray gmv( m_mv.LengthY(), m_mv.LengthX() );                          // create array to hold global motion vectors
    m_model.CalculateModelParameters(m_mv, m_internal_inliers, model_parameters);// calculate model parameters
    m_model.GenerateGlobalMotionVectors(gmv, model_parameters);                  // generate global motion vectors
    int new_score = m_test.Test(gmv, m_internal_inliers);                        // test new global motion vectors

    if (new_score < score)
    {
        score = new_score;                                  // update cummulative score
        ResetInliers(m_global_inliers, m_internal_inliers); // update global inliers

        //if (m_debug) std::cerr << std::endl << "Reject intensity: Improved model";
    }

	/*
	else
        if (m_debug) std::cerr << std::endl << "Reject intensity: Failed to improve model";
	*/
}

