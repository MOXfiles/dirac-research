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

#include <libdirac_motionest/model_projective.h>
using namespace dirac;
 
ModelProjective::ModelProjective(const MEData & me_data,
                                 const EncoderParams & encparams)
:
ModelGlobalMotion(me_data, encparams)
{
    m_debug = true;
}

ModelProjective::~ModelProjective()
{}

void ModelProjective::CalculateModelParameters(const MvArray & mv,
                                          TwoDArray<int> & mv_inliers,
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
    CalculateModelParameters(m_mv_float, mv_inliers, parameters);
}

void ModelProjective::CalculateModelParameters(MvFloatArray & mv,
                                               TwoDArray<int> & inliers,
                                               OneDArray<float> & parameters)
{
    /*
    // ****** DEBUG ******
    // give fixed, small array of motion vectors
    float a11 = 0.0;
    float a12 = 0.0;
    float a21 = 0.0;
    float a22 = 0.0;

    float b1 = 0.0;
    float b2 = 0.0;

    float c1 = 0.0;
    float c2 = 0.0;

    std::cerr<<std::endl<<"Scrub that, using fixed value MVs,";
    std::cerr<<std::endl<<"A: "<<a11<<" "<<a12<<" "<<a21<<" "<<a22;
    std::cerr<<std::endl<<"B: "<<b1<<" "<<b2;
    std::cerr<<std::endl<<"C: "<<c1<<" "<<c2;

    */
    int count = 0;

    for (int j=0;j<m_y_mvs;j++)
    {
        for (int i=0;i<m_x_mvs;i++)
        {
            if (inliers[j][i])
            {
                ++count;
            }
        }
    }

    OneDArray<float> xi( count );
    OneDArray<float> yi( count );
    OneDArray<float> xo( count );
    OneDArray<float> yo( count );
    OneDArray<float> t( ( xi.Length() + yi.Length() ) * ( ( xi.Length() + yi.Length() ) +1 ) );
    
    double coeffs[8];

    count = 0;
                            
    for (int j=0;j<m_y_mvs;j++)
    {
        for (int i=0;i<m_y_mvs;i++)
        {
            if (inliers[j][i])
            {
                float x = i + 0.5 - ( m_x_mvs / 2 );
                float y = j + 0.5 - ( m_y_mvs / 2 );


                //mv[j][i].x = ( ( a11 * x ) + ( a12 * y ) + b1 ) / ( ( c1 * x ) + ( c2 * y ) + 1 );
                //mv[j][i].y = ( ( a21 * x ) + ( a22 * y ) + b2 ) / ( ( c1 * x ) + ( c2 * y ) + 1 );

                //std::cerr<<std::endl<<"Motion vectors: "<<i<<","<<j<<": "<<mv[j][i].x<<","<<mv[j][i].y;

                xi[count] = x;
                yi[count] = y;

                xo[count] = mv[j][i].x;
                yo[count] = mv[j][i].y;

                ++count;
            }
        }
    }

    // ******* END DEBUG *******

    Calc_mapping_coeffs( xi, yi, xo, yo, t, coeffs);

    // set parameters for calling function    
    parameters[0] = coeffs[0];
    parameters[1] = coeffs[1];
    parameters[2] = coeffs[3];
    parameters[3] = coeffs[4];
    parameters[4] = coeffs[2];
    parameters[5] = coeffs[5];
    parameters[6] = coeffs[6];
    parameters[7] = coeffs[7];
}


void ModelProjective::CalculateModelParametersOld(MvFloatArray & mv,
                                               TwoDArray<int> & inliers,
                                               OneDArray<float> & parameters)
{
    /*
    // ****** DEBUG ******
    // give fixed, small array of motion vectors
    float a11 = 0.0;
    float a12 = 0.0;
    float a21 = 0.0;
    float a22 = 0.0;

    float b1 = 0.0;
    float b2 = 0.0;

    float c1 = 0.0;
    float c2 = 0.0;

    std::cerr<<std::endl<<"Scrub that, using fixed value MVs,";
    std::cerr<<std::endl<<"A: "<<a11<<" "<<a12<<" "<<a21<<" "<<a22;
    std::cerr<<std::endl<<"B: "<<b1<<" "<<b2;
    std::cerr<<std::endl<<"C: "<<c1<<" "<<c2;

    */
    int count = 0;

    for (int j=0;j<m_y_mvs;j++)
    {
        for (int i=0;i<m_x_mvs;i++)
        {
            if (inliers[j][i])
            {
                ++count;
            }
        }
    }

    OneDArray<float> xi( count );
    OneDArray<float> yi( count );
    OneDArray<float> xo( count );
    OneDArray<float> yo( count );
    OneDArray<float> t( ( xi.Length() + yi.Length() ) * ( ( xi.Length() + yi.Length() ) +1 ) );
    
    double coeffs[8];

    count = 0;
                            
    for (int j=0;j<m_y_mvs;j++)
    {
        for (int i=0;i<m_y_mvs;i++)
        {
            if (inliers[j][i])
            {
                float x = i + 0.5 - ( m_x_mvs / 2 );
                float y = j + 0.5 - ( m_y_mvs / 2 );


                //mv[j][i].x = ( ( a11 * x ) + ( a12 * y ) + b1 ) / ( ( c1 * x ) + ( c2 * y ) + 1 );
                //mv[j][i].y = ( ( a21 * x ) + ( a22 * y ) + b2 ) / ( ( c1 * x ) + ( c2 * y ) + 1 );

                //std::cerr<<std::endl<<"Motion vectors: "<<i<<","<<j<<": "<<mv[j][i].x<<","<<mv[j][i].y;

                xi[count] = x;
                yi[count] = y;

                xo[count] = mv[j][i].x;
                yo[count] = mv[j][i].y;

                ++count;
            }
        }
    }

    // ******* END DEBUG *******

    Calc_mapping_coeffs( xi, yi, xo, yo, t, coeffs);

    // set parameters for calling function    
    parameters[0] = coeffs[0];
    parameters[1] = coeffs[1];
    parameters[2] = coeffs[3];
    parameters[3] = coeffs[4];
    parameters[4] = coeffs[2];
    parameters[5] = coeffs[5];
    parameters[6] = coeffs[6];
    parameters[7] = coeffs[7];
}

int ModelProjective::Calc_mapping_coeffs( OneDArray<float>& xi,
                                          OneDArray<float>& yi, 
                                          OneDArray<float>& xo,
                                          OneDArray<float>& yo,
                                          OneDArray<float>& t,
                                          double *coeffs)         // 8 computed coeffs
{
/*
    Function to calculate the mapping coefficients, using this transformation:
    xout = (a11x + a12y + b1) / (c1x + c2y + 1)
    yout = (a21x + a22y + b2) / (c1x + c2y + 1)

    inputs:   xi[], yi[] are input coordinates
              xo[], yo[] are where these should map to

    output: array coeffs[0..7] corresponding to a..h in the above mapping

    return code:  0 for success
                 -1 if equations can't be solved (coeffs[0..7] set to zero)

    
    Set up a matrix M such that

    M * (a, b, c, d, e, f, g, h)(transposed) - B  = 0

    and then solve for a..h

    In general,
    
    xout * c1.x + xout * c2.y + xout = a11.x + a12.y + b1

    or
    
    a11   a12   a21   a22   b1    b2    c1       c2       =
    x     y     1     0     0     0     -xout.x  -xout.y     -xout

    and
        
    yout * c1.x + yout * c2.y + yout = a21.x + a22.y + b2

    or

    a11   a12   a21   a22   b1    b2    c1       c2       =
    0     0     0     x     y     1     -yout.x  -yout.y     -yout

*/

    
    OneDArray<float> A( 8 * ( xi.Length() + yi.Length() ) );
    OneDArray<float> B( ( xi.Length() + yi.Length() ) );

    int j=0;  // pointer to unknown to put in
    int k=0;  // pointer to right-hand side

    for (int i=0; i<xi.Length(); i++)
    {
        // equation terms for eqn. based on x output coord:
        A[j++] = xi[i];
        A[j++] = yi[i];
        A[j++] = 1.0;
        A[j++] = 0;
        A[j++] = 0;
        A[j++] = 0;
        A[j++] = -xo[i] * xi[i];
        A[j++] = -xo[i] * yi[i];
        B[k++] = xo[i];      // minus right-hand side

        // equation terms for eqn. based on y output coord:
        A[j++] = 0;
        A[j++] = 0;
        A[j++] = 0;
        A[j++] = xi[i];
        A[j++] = yi[i];
        A[j++] = 1.0;
        A[j++] = -yo[i] * xi[i];
        A[j++] = -yo[i] * yi[i];
        B[k++] = yo[i];      // minus right-hand side

    }

    // solve Ax-B=0
    //int result = Solve(A, xi.Length(), 8, B, coeffs);
    
    Calc_t_matrix_tr(A, B, t, 8, ( xi.Length() + yi.Length() ) );

    int result = Solve_t_matrix(t, coeffs, 8 );
  
    if (result != 0)
    {
        for (int i=0; i<8; i++) coeffs[i]=0;
        return (-1);
    }

    else
    {
        /*
        std::cerr<<std::endl<<"Coefficients are: ";

        std::cerr<<std::endl<<"a11: "<<coeffs[0];
        std::cerr<<std::endl<<"a12: "<<coeffs[1];
        std::cerr<<std::endl<<"a21: "<<coeffs[3];
        std::cerr<<std::endl<<"a22: "<<coeffs[4];
        std::cerr<<std::endl<<"b1: "<<coeffs[2];
        std::cerr<<std::endl<<"b2: "<<coeffs[5];
        std::cerr<<std::endl<<"c1: "<<coeffs[6];
        std::cerr<<std::endl<<"c2: "<<coeffs[7];
        */
        return(0);
    }
}


int ModelProjective::Calc_t_matrix_tr(OneDArray<float>& cc, OneDArray<float>& value, OneDArray<float>& t, int n, int ns)
{
/*
    This function takes a matrix holding over-determined simultaneous equations
    and multiplies it by its transpose to obtain a normalised equation matrix.
    (The _tr version has the matrix addressed round the other way from the
    non-_tr version: the _tr version has each eqn stored together, ie. coeff
    index varies fastest)

    Inputs: n                   number of unknowns
            ns                  number of equations
            cc[ i * n + c]      coefficient of unknown c (=0..n-1) in equation i (=0..ns-1)
            value[ i ]          right hand side of eqn. i (0..ns-1)
            
    Output: t[ r + n * c]       coefficient of unknown c in equation r
                                (t[ r + n * n ] is the right hand side of equation r)

                                (NB this is 'round the other way' from the input
                                array, but this means that you multiply by n instead
                                of n+1 when doing the row addressing)
*/

    double a;
    int    r, c, i;

    for (r = 0; r < n; r++)
    {
        for (c = r; c < n; c++)
        {
            a = 0.0;

            for (i = 0; i < ns; i++)
            {
                a += cc[ (i * n) + r ] * cc[ ( i * n ) + c ];
            }

            t[r+n*c] = a;
            t[c+n*r] = a;
        }
    }

    for (r = 0; r < n; r++)
    {
        a = 0.0;

        for (i = 0; i < ns; i++)
        {
            a += cc[i*n+r] * value[i];
        }

        t[r+n*n] = a;
    }

	return 0;

}

int ModelProjective::Solve_t_matrix(OneDArray<float>& t, double *s, int n)
{
/* 
    This function solves the normalised equation matrix. If solvable, the
    solutions are placed in s[] and "0" is returned. If the equations cannot be
    solved a "1" is returned.

    Inputs:  n                  size of equation matrix (= number of eqns and unknowns)
             t[ r + n * c]      coefficient of unknown c in equation r
                                (t[ r + n * n ] is the right hand side of equation c)

    Output:  s[r]               value of unknown r (if solvable)

*/
    double x, y;
    int    i, j, k, no_solution;

    for (j = 0; j < n; j++)
    {
        no_solution = 1;
        i = j;

        while ( no_solution && (i < n) )
        {
            if ((t[i+n*j] > 1e-50) || (t[i+n*j] < -1e-50))
            {
                no_solution = 0;
            }
            else
            {
                i++;
            }
        }
        
        if (no_solution)
        {
            break;
        }
        
        for (k = 0; k <= n; k++)
        {
            x = t[j+n*k];
            t[j+n*k] = t[i+n*k];
            t[i+n*k] = x;
        }

        y = 1 / t[j+n*j];
        
        for (k = 0; k <= n; k++)
        {
            t[j+n*k] = y * t[j+n*k];
        }
        
        for (i = 0; i < n; i++)
        {
            if (i != j)
            {
                y = -t[i+n*j];

                for (k = j; k <= n; k++)
                {
                    t[i+n*k] = t[i+n*k] + (y * t[j+n*k]);
                }
            }
        }
    }

    if (no_solution)
    {
        std::cerr<<std::endl<<"Solve_t_matrix: no solution";
        return (SOLVE_NO_SOLUTION);
    }

    else
    {
        for (i = 0; i < n; i++)
        {
            s[i] = t[i+n*n];
        }

        return (SOLVE_SUCCESS);
    }
}


void ModelProjective::GenerateGlobalMotionVectors(MvFloatArray & gmv,
                                                  OneDArray<float> & parameters)
{
    // DEBUG
    //if (m_debug) std::cerr<<std::endl<<"Using params: "<<parameters[0]<<" "<<parameters[1]<<" "<<parameters[2]<<" "<<parameters[3]<<" "<<parameters[4]<<" "<<parameters[5]<<" "<<parameters[6]<<" "<<parameters[7];

    for (int j=0;j<m_y_mvs;j++)
    {
        for (int i=0;i<m_x_mvs;i++)
        {
                float x = i + 0.5 - ( m_x_mvs / 2 );
                float y = j + 0.5 - ( m_y_mvs / 2 );
            
            gmv[j][i].x = ( ( ( parameters[0] * x ) + ( parameters[1] * y ) + parameters[4] )
                        / ( ( parameters[6] * x ) + ( parameters[7] * y ) + 1 ) );
                        
            gmv[j][i].y = ( ( ( parameters[2] * x ) + ( parameters[3] * y ) + parameters[5] )
                        / ( ( parameters[6] * x ) + ( parameters[7] * y ) + 1 ) );

            // DEBUG
            //if (m_debug) std::cerr<<std::endl<<"Global motion vectors: "<<i<<","<<j<<": "<<gmv[j][i].x<<","<<gmv[j][i].y;
        }
    }
}

void ModelProjective::GenerateGlobalMotionVectors(MvArray & gmv,
                                                  OneDArray<float> & parameters)
{
    for (int j=0;j<m_y_mvs;j++)
    {
        for (int i=0;i<m_x_mvs;i++)
        {
                float x = i + 0.5 - ( m_x_mvs / 2 );
                float y = j + 0.5 - ( m_y_mvs / 2 );
            
            gmv[j][i].x = int( ( ( parameters[0] * x ) + ( parameters[1] * y ) + parameters[4] )
                        / ( ( parameters[6] * x ) + ( parameters[7] * y ) + 1 ) );
            gmv[j][i].y = int( ( ( parameters[2] * x ) + ( parameters[3] * y ) + parameters[5] )
                        / ( ( parameters[6] * x ) + ( parameters[7] * y ) + 1 ) );

            // DEBUG
            //if (m_debug) std::cerr<<std::endl<<"Global motion vectors: "<<i<<","<<j<<": "<<gmv[j][i].x<<","<<gmv[j][i].y;
        }
    }
}
