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
  
#include <libdirac_motionest/filter_weighted_local.h>
using namespace dirac;

FilterWeightedLocal::FilterWeightedLocal(const FrameBuffer & frame_buffer,
                                         const MEData & me_data,
                                         unsigned int frame_number,
                                         int ref_number,
                                         ModelGlobalMotion & model,
                                         TestGlobalMotionModel & test,
                                         TwoDArray<int> & inliers)
:
RefineMotionVectorField(frame_buffer, me_data, frame_number, ref_number,
                        model, test, inliers),
m_mv(me_data.Vectors(ref_number)),
m_internal_inliers(inliers)
{
    m_debug = true;
}

FilterWeightedLocal::~FilterWeightedLocal()
{}

void FilterWeightedLocal::Refine(MvFloatArray & gmv, int & score)
{
    // create float version of motion vector array
    MvFloatArray mv_filt(gmv.LengthY(), gmv.LengthX());

    for (int j=0;j<gmv.LengthY();++j)
    {
        for (int i=0;i<gmv.LengthX();++i)
        {
            mv_filt[j][i].x = (float)m_mv[j][i].x;
            mv_filt[j][i].y = (float)m_mv[j][i].y;
        }
    }

    int xnum = m_mv.LengthX();
    int ynum = m_mv.LengthY();

    // weight neighbours for proximity and value
    //
    //              k
    //          -1  0   1
    //     -1   .   .   .
    //  l   0   . (m,n) .
    //      1   .   .   .

    // proximity weighting based on |k| + |l|:
    // |k| + |l| = 0, h = 4
    // |k| + |l| = 1, h = 2
    // |k| + |l| = 2, h = 1

    int h[3];
    h[0] = 4;
    h[1] = 2;
    h[2] = 1;

    // value weighting based on motion vector difference *** can be adjusted ***
    // dMV <= 1/8 pixel x and 1/8 pixel y, w = 1
    // dMV < 1/8 pixel x and 1/8 pixel y, w = 0

    // weighting array for each component can be indexed by motion vector difference for speed
    int w[100000];
    for (int i=0;i<1000;++i) { w[i] = 0; }   // zero all weights
    for (int i=0;i<2;++i)    { w[i] = 1; }   // include maximum allowed difference

    // loop over motion vectors with 1 block perimeter
    for (int j=1;j<m_mv.LengthY()-1;++j)
    {
        for (int i=1;i<m_mv.LengthX()-1;++i)
        {
            int numerator_x=0, numerator_y=0, denominator=0, denominator_x=0, denominator_y=0;
            int rejectBlock=m_internal_inliers[j][i]; // do not include centre block in loop
            m_internal_inliers[j][i]=0;

            for (int k=-1;k<2;++k)
            {
                for (int l=-1;l<2;++l)
                {
                    int currentDenominator = 0, currentDenominator_x = 0, currentDenominator_y = 0;
                    
                    currentDenominator += w[int(std::abs(m_mv[j-l][i-k].x-m_mv[j][i].x))] *
                                          w[int(std::abs(m_mv[j-l][i-k].y-m_mv[j][i].y))] *
                                          h[int(std::abs(k)+std::abs(l))] *
                                          m_internal_inliers[j-l][i-k];

                    currentDenominator_x += w[int(std::abs(m_mv[j-l][i-k].x-m_mv[j][i].x))] *
                                            h[int(std::abs(k)+std::abs(l))] *
                                            m_internal_inliers[j-l][i-k];
                                               
                    currentDenominator_y += w[int(std::abs(m_mv[j-l][i-k].y-m_mv[j][i].y))] *
                                            h[int(std::abs(k)+std::abs(l))] *
                                            m_internal_inliers[j-l][i-k];

                    int currentNumerator_x = currentDenominator * m_mv[j-l][i-k].x;
                    int currentNumerator_y = currentDenominator * m_mv[j-l][i-k].y;

                    numerator_x += currentNumerator_x;
                    numerator_y += currentNumerator_y;
                    denominator += currentDenominator;

                    denominator_x += currentDenominator_x;
                    denominator_y += currentDenominator_y;
                } // l
            }// k

            // now add centre block
            denominator += 4;
            denominator_x += 4;
            denominator_y += 4;

            numerator_x += 4 * m_mv[j][i].x;
            numerator_y += 4 * m_mv[j][i].y;

            // reset centre block rejection
            m_internal_inliers[j][i]=rejectBlock;

            // finish processing current motion vector
            mv_filt[j][i].x = (float)numerator_x / (float)denominator;
            mv_filt[j][i].y = (float)numerator_y / (float)denominator;
        }// i
    }// j

    // do j = 0, not including i = 0, i = xlength-1
    for (int i=1;i<xnum-1;++i)
    {
        int numerator_x=0, numerator_y=0, denominator=0, denominator_x=0, denominator_y=0;

        int rejectBlock=m_internal_inliers[0][i]; // do not include centre block in loop
        m_internal_inliers[0][i]=0;

        for (int k=-1;k<2;++k)
        {
            for (int l=-1;l<1;++l)
            {
                int currentDenominator = w[int(std::abs(m_mv[0-l][i-k].x-m_mv[0][i].x))] *
                    w[int(std::abs(m_mv[0-l][i-k].y-m_mv[0][i].y))] * h[int(std::abs(k)+std::abs(l))] *
                    m_internal_inliers[0-l][i-k];

                int currentDenominator_x = w[int(std::abs(m_mv[0-l][i-k].x-m_mv[0][i].x))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[0-l][i-k];
                int currentDenominator_y = w[int(std::abs(m_mv[0-l][i-k].y-m_mv[0][i].y))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[0-l][i-k];

                int currentNumerator_x = currentDenominator * m_mv[0-l][i-k].x;
                int currentNumerator_y = currentDenominator * m_mv[0-l][i-k].y;

                numerator_x += currentNumerator_x;
                numerator_y += currentNumerator_y;
                denominator += currentDenominator;

                denominator_x += currentDenominator_x;
                denominator_y += currentDenominator_y;
            } // l
        }// k

        // now add centre block
        denominator += 4;
        denominator_x += 4;
        denominator_y += 4;

        numerator_x += 4 * m_mv[0][i].x;
        numerator_y += 4 * m_mv[0][i].y;

        // reset centre block rejection
        m_internal_inliers[0][i]=rejectBlock;

        // finish processing current motion vector
        mv_filt[0][i].x = (float)numerator_x / (float)denominator;
        mv_filt[0][i].y = (float)numerator_y / (float)denominator;

    }// i

    // do j = ylength-1, not including i = 0, i = xlength-1
    for (int i=1;i<xnum-1;++i)
    {
        int numerator_x=0, numerator_y=0, denominator=0, denominator_x=0, denominator_y=0;

        int rejectBlock=m_internal_inliers[ynum-1][i]; // do not include centre block in loop
        m_internal_inliers[ynum-1][i]=0;

        for (int k=-1;k<2;++k)
        {
            for (int l=0;l<2;++l)
            {
                int currentDenominator = w[int(std::abs(m_mv[ynum-1-l][i-k].x-m_mv[ynum-1][i].x))] *
                    w[int(std::abs(m_mv[ynum-1-l][i-k].y-m_mv[ynum-1][i].y))] * h[int(std::abs(k)+std::abs(l))] *
                    m_internal_inliers[ynum-1-l][i-k];

                int currentDenominator_x = w[int(std::abs(m_mv[ynum-1-l][i-k].x-m_mv[ynum-1][i].x))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[ynum-1-l][i-k];
                int currentDenominator_y = w[int(std::abs(m_mv[ynum-1-l][i-k].y-m_mv[ynum-1][i].y))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[ynum-1-l][i-k];

                int currentNumerator_x = currentDenominator * m_mv[ynum-1-l][i-k].x;
                int currentNumerator_y = currentDenominator * m_mv[ynum-1-l][i-k].y;

                numerator_x += currentNumerator_x;
                numerator_y += currentNumerator_y;
                denominator += currentDenominator;

                denominator_x += currentDenominator_x;
                denominator_y += currentDenominator_y;
            } // l
        }// k

        // now add centre block
        denominator += 4;
        denominator_x += 4;
        denominator_y += 4;

        numerator_x += 4 * m_mv[ynum-1][i].x;
        numerator_y += 4 * m_mv[ynum-1][i].y;

        // reset centre block rejection
        m_internal_inliers[ynum-1][i]=rejectBlock;

        // finish processing current motion vector
        mv_filt[ynum-1][i].x = float(numerator_x) / float(denominator);
        mv_filt[ynum-1][i].y = float(numerator_y) / float(denominator);
    }// i

    // do i = 0, not including j = 0, j = ylength-1
    for (int j=1;j<ynum-1;++j)
    {
        int numerator_x=0, numerator_y=0, denominator=0, denominator_x=0, denominator_y=0;

        int rejectBlock=m_internal_inliers[j][0]; // do not include centre block in loop
        m_internal_inliers[j][0]=0;

        for (int k=-1;k<1;++k)
        {
            for (int l=-1;l<2;++l)
            {
                int currentDenominator = w[int(std::abs(m_mv[j-l][0-k].x-m_mv[j][0].x))] *
                    w[int(std::abs(m_mv[j-l][0-k].y-m_mv[j][0].y))] * h[int(std::abs(k)+std::abs(l))] *
                    m_internal_inliers[j-l][0-k];

                int currentDenominator_x = w[int(std::abs(m_mv[j-l][0-k].x-m_mv[j][0].x))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[j-l][0-k];
                int currentDenominator_y = w[int(std::abs(m_mv[j-l][0-k].y-m_mv[j][0].y))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[j-l][0-k];

                int currentNumerator_x = currentDenominator * m_mv[j-l][0-k].x;
                int currentNumerator_y = currentDenominator * m_mv[j-l][0-k].y;

                numerator_x += currentNumerator_x;
                numerator_y += currentNumerator_y;
                denominator += currentDenominator;

                denominator_x += currentDenominator_x;
                denominator_y += currentDenominator_y;
            } // l
        }// k

        // now add centre block
        denominator += 4;
        denominator_x += 4;
        denominator_y += 4;

        numerator_x += 4 * m_mv[j][0].x;
        numerator_y += 4 * m_mv[j][0].y;

        // reset centre block rejection
        m_internal_inliers[j][0]=rejectBlock;

        // finish processing current motion vector
        mv_filt[j][0].x = float(numerator_x) / float(denominator);
        mv_filt[j][0].y = float(numerator_y) / float(denominator);
    }// j

    // do i = xnum-1, not including j = 0, j = ylength-1
    for (int j=1;j<ynum-1;++j)
    {
        int numerator_x=0, numerator_y=0, denominator=0, denominator_x=0, denominator_y=0;

        int rejectBlock=m_internal_inliers[j][xnum-1]; // do not include centre block in loop
        m_internal_inliers[j][xnum-1]=0;

        for (int k=0;k<2;++k)
        {
            for (int l=-1;l<2;++l)
            {
                int currentDenominator = w[int(std::abs(m_mv[j-l][xnum-1-k].x-m_mv[j][xnum-1].x))] *
                    w[int(std::abs(m_mv[j-l][xnum-1-k].y-m_mv[j][xnum-1].y))] * h[int(std::abs(k)+std::abs(l))] *
                    m_internal_inliers[j-l][xnum-1-k];

                int currentDenominator_x = w[int(std::abs(m_mv[j-l][xnum-1-k].x-m_mv[j][xnum-1].x))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[j-l][xnum-1-k];
                int currentDenominator_y = w[int(std::abs(m_mv[j-l][xnum-1-k].y-m_mv[j][xnum-1].y))] *
                    h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[j-l][xnum-1-k];

                int currentNumerator_x = currentDenominator * m_mv[j-l][xnum-1-k].x;
                int currentNumerator_y = currentDenominator * m_mv[j-l][xnum-1-k].y;

                numerator_x += currentNumerator_x;
                numerator_y += currentNumerator_y;
                denominator += currentDenominator;

                denominator_x += currentDenominator_x;
                denominator_y += currentDenominator_y;
            } // l
        }// k

        // now add centre block
        denominator += 4;
        denominator_x += 4;
        denominator_y += 4;

        numerator_x += 4 * m_mv[j][xnum-1].x;
        numerator_y += 4 * m_mv[j][xnum-1].y;

        // reset centre block rejection
        m_internal_inliers[j][xnum-1]=rejectBlock;

        // finish processing current motion vector
        mv_filt[j][xnum-1].x = float(numerator_x) / float(denominator);
        mv_filt[j][xnum-1].y = float(numerator_y) / float(denominator);
    }// j

    // do corner blocks
    for (int j=0;j<ynum;j+=ynum-1)
    {
        for (int i=0;i<xnum;i+=xnum-1)
        {
            int numerator_x=0, numerator_y=0, denominator=0, denominator_x=0, denominator_y=0;
            int k_start, l_start, k_end, l_end;

            int rejectBlock=m_internal_inliers[j][i]; // do not include centre block in loop
            m_internal_inliers[j][i]=0;

            if (j==0)           { l_start=-1; l_end=1;}
            if (j==ynum-1)      { l_start=0;  l_end=2;}

            if (i==0)           { k_start=-1; k_end=1;}
            if (i==xnum-1)      { k_start=0;  k_end=2;}

            for (int k=k_start; k < k_end; k++)
            {
                for (int l=l_start; l < l_end; l++)
                {
                    int currentDenominator = w[int(std::abs(m_mv[j-l][i-k].x-m_mv[j][i].x))] *
                        w[int(std::abs(m_mv[j-l][i-k].y-m_mv[j][i].y))] * h[int(std::abs(k)+std::abs(l))] *
                        m_internal_inliers[j-l][i-k];

                    int currentDenominator_x = w[int(std::abs(m_mv[j-l][i-k].x-m_mv[j][i].x))] *
                        h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[j-l][i-k];
                    int currentDenominator_y = w[int(std::abs(m_mv[j-l][i-k].y-m_mv[j][i].y))] *
                        h[int(std::abs(k)+std::abs(l))] * m_internal_inliers[j-l][i-k];

                    int currentNumerator_x = currentDenominator * m_mv[j-l][i-k].x;
                    int currentNumerator_y = currentDenominator * m_mv[j-l][i-k].y;

                    numerator_x += currentNumerator_x;
                    numerator_y += currentNumerator_y;
                    denominator += currentDenominator;

                    denominator_x += currentDenominator_x;
                    denominator_y += currentDenominator_y;
                } // l
            }// k

            // now add centre block
            denominator += 4;
            denominator_x += 4;
            denominator_y += 4;

            numerator_x += 4 * m_mv[j][i].x;
            numerator_y += 4 * m_mv[j][i].y;

            // reset centre block rejection
            m_internal_inliers[j][i]=rejectBlock;

            // finish processing current motion vector
            mv_filt[j][i].x = float(numerator_x) / float(denominator);
            mv_filt[j][i].y = float(numerator_y) / float(denominator);
        }
    }

    // test refined motion vector field
    OneDArray<float> model_parameters(10);                                           // create vector to hold model parameters
    m_model.CalculateModelParameters(mv_filt, m_global_inliers, model_parameters);  // calculate model parameters
	
	m_model.GenerateGlobalMotionVectors(gmv, model_parameters);                     // generate global motion vectors
    int new_score = m_test.Test(gmv, m_global_inliers);                             // test new global motion vectors

    //if (m_debug) std::cerr<<std::endl<<"Filter model parameters: "<<model_parameters[0]<<" "<<model_parameters[1]<<" "<<model_parameters[2]<<" "<<model_parameters[3]
    //<<" "<<model_parameters[4]<<" "<<model_parameters[5];

    if (new_score < score)
    {
        // update cummulative score
        score = new_score;

        // update global motion vectors
        for (int j=0;j<gmv.LengthY();++j)
        {
            for (int i=0;i<gmv.LengthX();++i)
            {
                gmv[j][i].x = mv_filt[j][i].x;
                gmv[j][i].y = mv_filt[j][i].y;
            }
        }

        //if (m_debug) std::cerr << std::endl << "Filter: Improved model";
    }

    else
    {
        //if (m_debug) std::cerr << std::endl << "Filter: Failed to improve model";

        // update global motion vectors
        for (int j=0;j<gmv.LengthY();++j)
        {
            for (int i=0;i<gmv.LengthX();++i)
            {
                gmv[j][i].x = (float)m_mv[j][i].x;
                gmv[j][i].y = (float)m_mv[j][i].y;
            }
        }
    }
}

