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

#include <libdirac_motionest/reject_edge.h>
using namespace dirac;

RejectEdge::RejectEdge(const FrameBuffer & frame_buffer,
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

RejectEdge::~RejectEdge()
{}

void RejectEdge::Reject(int & score)
{ 
    // loop over motion vectors and reject them if they point to region outside the frame
    for (int j=0; j<m_mv.LengthY(); ++j)
    {
        for (int i=0; i<m_mv.LengthX(); ++i)
        {
            int x = (i*8) + 3 + int(m_mv[j][i].x / 8); // x position after motion
            int y = (j*8) + 3 + int(m_mv[j][i].y / 8); // y position after motion

            if (x >= m_encparams.OrigXl() || x < 0 ||
                y >= m_encparams.OrigYl() || y < 0)
            {
                m_internal_inliers[j][i] = 0;
            }
            else
                m_internal_inliers[j][i] = 1;
        }
    }

    // reject edge motion vectors
    for (int j=0; j<m_mv.LengthY(); ++j)
    {
        m_internal_inliers[j][0] = 0;
        m_internal_inliers[j][m_mv.LengthX()-1] = 0;
    }

    for (int i=0; i<m_mv.LengthX(); ++i)
    {
        m_internal_inliers[0][i] = 0;
        m_internal_inliers[m_mv.LengthY()-1][i] = 0;
    }
    
    // remove padded areas
    if (m_mv.LengthX() * 8 > m_encparams.OrigXl())
    {        
        for (int j=0; j<m_mv.LengthY(); ++j)
        {
            for (int i=int(m_encparams.OrigXl()/8)-1; i<m_mv.LengthX(); ++i)
            {
                m_internal_inliers[j][i] = 0;
            }
        }
    }

    if (m_mv.LengthY() * 8 > m_encparams.OrigYl())
    {        
        for (int j=int(m_encparams.OrigYl()/8)-1; j<m_mv.LengthY(); ++j)
        {
            for (int i=0; i<m_mv.LengthX(); ++i)
            {
                m_internal_inliers[j][i] = 0;
            }
        }
    }
        
    OneDArray<float> model_parameters(10);                                         // create array to hold model parameters
    MvFloatArray gmv(m_mv.LengthY(), m_mv.LengthX());                             // create global motion vector array
    m_model.CalculateModelParameters(m_mv, m_internal_inliers, model_parameters); // calculate model parameters
    m_model.GenerateGlobalMotionVectors(gmv, model_parameters);                   // generate global motion vectors
    int new_score = m_test.Test(gmv, m_internal_inliers);                         // test new global motion vectors
    score = new_score;                                                            // update cummulative score
    ResetInliers(m_global_inliers, m_internal_inliers);                           // update global inliers
  
}
