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

#include <libdirac_motionest/model_affine.h>
using namespace dirac;

ModelAffine::ModelAffine(const MEData & me_data,
                         const EncoderParams & encparams)
:
ModelGlobalMotion(me_data, encparams)
{}

ModelAffine::~ModelAffine()
{
    m_debug = true;   
}

void ModelAffine::CalculateModelParameters(const MvArray & mv,
                                           TwoDArray<int> & inliers,
                                           OneDArray<float> & parameters)
{    
    // create float version of motion vectors
    for (int j=0;j<m_mv_float.LengthY();j++)
    {
        for (int i=0;i<m_mv_float.LengthX();i++)
        {
            m_mv_float[j][i].x = (float)mv[j][i].x;
            m_mv_float[j][i].y = (float)mv[j][i].y;
        }
    }

    // call calculating function
    CalculateModelParameters(m_mv_float, inliers, parameters);
}

void ModelAffine::CalculateModelParameters(MvFloatArray & mv,
                                           TwoDArray<int> & inliers,
                                           OneDArray<float> & parameters)
{
    float b1, b2, a11, a12, a21, a22;

    float x_sum = 0.0;
    float y_sum = 0.0;
    float xy_sum = 0.0;
    float xsquared_sum = 0.0;
    float ysquared_sum = 0.0;
    float Vx_sum = 0.0;
    float Vy_sum = 0.0;
    float xVx_sum = 0.0;
    float xVy_sum = 0.0;
    float yVx_sum = 0.0;
    float yVy_sum = 0.0;

    int count = 0;

    for (int j=0; j<m_y_mvs; ++j)
    {
        for (int i=0; i<m_x_mvs; ++i)
        {
            if (inliers[j][i])
            {
                float x = (float)i + 0.5;
                float y = (float)j + 0.5;

                x_sum += x;
                y_sum += y;
                xsquared_sum += ( x * x );
                ysquared_sum += ( y * y );
                xy_sum += ( x * y );

                xVx_sum += ( x * (float)mv[j][i].x );
                xVy_sum += ( x * (float)mv[j][i].y );
                yVx_sum += ( y * (float)mv[j][i].x );
                yVy_sum += ( y * (float)mv[j][i].y );

                Vx_sum += (float)mv[j][i].x;
                Vy_sum += (float)mv[j][i].y;

                ++count;
            }
        }
    }

    float x = ( x_sum / (float)count );
    float y = ( y_sum / (float)count );
    float xsquared = ( xsquared_sum / (float)count );
    float ysquared = ( ysquared_sum / (float)count );
    float xy = ( xy_sum / (float)count );

    float xVx = ( xVx_sum / (float)count );
    float xVy = ( xVy_sum / (float)count );
    float yVx = ( yVx_sum / (float)count );
    float yVy = ( yVy_sum / (float)count );

    float Vx = ( Vx_sum / (float)count );
    float Vy = ( Vy_sum / (float)count );

    float m1_11 = xsquared - ( x * x );
    float m1_12 = xy - ( x * y );
    float m1_21 = xy - ( x * y );
    float m1_22 = ysquared - ( y * y );

    float m2_11 = xVx - ( x * Vx );
    float m2_12 = yVx - ( y * Vx );
    float m2_21 = xVy - ( x * Vy );
    float m2_22 = yVy - ( y * Vy );

    // now to evaluate matrix A...

    // { a11 a12 }                     1                     { (m1_22*m2_11 - m1_12*m2_21) (m1_22*m2_12 - m1_12*m2_22) }
    // {         } = ------------------------------------- * {                                                         }
    // { a21 a22 }   ( m1_11 * m1_22 ) - ( m1_12 * m1_21 )   ( (m1_11*m2_21 - m1_21*m2_11) (m1_11*m2_22 - m1_21*m2_12} }
    //
    // (a b) (q p) = (aq+br ap+bs)
    // (c d) (r s)   (cq+dr cp+ds)

    float det = ( m1_11 * m1_22 ) - ( m1_12 * m1_21 );

    a11 = ( ( m1_22 * m2_11 ) / det ) - ( ( m1_21 * m2_12 ) / det );
    a12 = ( ( m1_11 * m2_12 ) / det ) - ( ( m1_12 * m2_11 ) / det );
    a21 = ( ( m1_22 * m2_21 ) / det ) - ( ( m1_21 * m2_22 ) / det );
    a22 = ( ( m1_11 * m2_22 ) / det ) - ( ( m1_12 * m2_21 ) / det );

    b1 = Vx - ( ( a11 * x ) + ( a12 * y ) );
    b2 = Vy - ( ( a21 * x ) + ( a22 * y ) );
    
    parameters[0] = a11;
    parameters[1] = a12;
    parameters[2] = a21;
    parameters[3] = a22;
    parameters[4] = b1;
    parameters[5] = b2;
    parameters[6] = 0.0; // affine model requires 6 parameters
    parameters[7] = 0.0; // place holders for future models - projective?
}

void ModelAffine::GenerateGlobalMotionVectors(MvFloatArray & gmv,
                                              OneDArray<float> & parameters)
{
    for (int j=0;j<gmv.LengthY();j++)
    {
        for (int i=0;i<gmv.LengthX();i++)
        {
            float x = (float)i + 0.5;
            float y = (float)j + 0.5;
            
            gmv[j][i].x = ( parameters[0] * x ) + ( parameters[1] * y );
            gmv[j][i].y = ( parameters[2] * x ) + ( parameters[3] * y );
            gmv[j][i].x += parameters[4];
            gmv[j][i].y += parameters[5];
        }
    }
}

void ModelAffine::GenerateGlobalMotionVectors(MvArray & gmv,
                                              OneDArray<float> & parameters)
{
    for (int j=0;j<gmv.LengthY();j++)
    {
        for (int i=0;i<gmv.LengthX();i++)
        {
            float x = (float)i + 0.5;
            float y = (float)j + 0.5;
            
            gmv[j][i].x = int( parameters[0] * x ) + int( parameters[1] * y );
            gmv[j][i].y = int( parameters[0] * y ) + int( parameters[3] * y );
            gmv[j][i].x += int( parameters[4] );
            gmv[j][i].y += int( parameters[5] );
        }
    }
}
