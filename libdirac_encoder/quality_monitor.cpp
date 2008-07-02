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

#include <libdirac_encoder/quality_monitor.h>
#include <libdirac_common/wavelet_utils.h>

using std::log10;

QualityMonitor::QualityMonitor(EncoderParams& encp, 
                               const SeqParams& sparams)
:
    m_encparams(encp),
    m_cformat( sparams.CFormat() ),
    m_true_xl( sparams.Xl() ),
    m_true_yl( sparams.Yl() ),
    m_target_quality(3),
    m_last_quality(3),
    m_slope(3),
    m_offset(3),
    m_last_lambda(3)
{
    ResetAll();
}

void QualityMonitor::ResetAll()
{
    // set target WPSNRs
 	m_target_quality[I_frame] = 0.28 * m_encparams.Qf()* m_encparams.Qf() + 20.0 ;
	m_target_quality[L1_frame] = m_target_quality[I_frame] - 1.5;
	m_target_quality[L2_frame] = m_target_quality[I_frame] - 2.5;

    // assume we hit those targets last time
    m_last_quality = m_target_quality;

    // set defaults for the model
     m_slope[I_frame] = -4.0;
     m_slope[L1_frame] = -4.0;
     m_slope[L2_frame] = -4.0;
     m_offset[I_frame] = 38.5,
     m_offset[L1_frame] = 43.3;
     m_offset[L2_frame] = 43.3;

    for (size_t fsort=0; fsort<3; ++fsort)
    {
        m_last_lambda[fsort] = std::pow( 10.0, (m_target_quality[fsort] - m_offset[fsort])/m_slope[fsort] );
    }// fsort

    // set a default ratio for the motion estimation lambda
    // Exact value TBD - will incorporate stuff about blocks and so on
    // Also need to think about how this can be adapted for sequences with more or less motion 

    m_me_ratio = 0.1;

 	// set up the Lagrangian parameters
    for (size_t fsort=0; fsort<3; ++fsort)
    {
        m_encparams.SetLambda( FrameSort(fsort), m_last_lambda[fsort] );
    }// fsort

    m_encparams.SetL1MELambda( std::sqrt(m_encparams.L1Lambda())*m_me_ratio );
    m_encparams.SetL2MELambda( std::sqrt(m_encparams.L2Lambda())*m_me_ratio );

}

bool QualityMonitor::UpdateModel(const Frame& ld_frame, const Frame& orig_frame , const int count)
{
    // The return value - true if we need to recode, false otherwise
    bool recode = false;

	const FrameSort& fsort = ld_frame.GetFparams().FSort();	
	double target_quality;	

	// Parameters relating to the last frame we measured
	double last_lambda;
	double last_quality;

	// Parameters relating to the current frame
	double current_lambda;
	double current_quality;


    //set up local parameters for the particular frame type
    current_lambda = m_encparams.Lambda(fsort);
    last_lambda = m_last_lambda[fsort];
    last_quality = m_last_quality[fsort];
    target_quality = m_target_quality[fsort];

    // TBD: Currently using unweighted PSNR since this seems to give a) more stable convergence and b) more bits allocated
    // to I frames at the expense of L1 and L2 frames, which is more efficient. Need to get a better measure, however.
	current_quality = QualityVal( ld_frame.Ydata() , orig_frame.Ydata() , 0.0);

    // Copy current data into memory for last frame data
    m_last_lambda[fsort] = m_encparams.Lambda(fsort);
    m_last_quality[fsort] = current_quality;

	// ok, so we've got an actual WPSNR to use. We know the lambda used before and the resulting
	// WPSNR then allows us to estimate the slope of the curve of WPSNR versus log of lambda

	if ( std::abs(current_quality - last_quality)> 0.2 && 
         std::abs(log10(current_lambda) - log10(last_lambda)) > 0.1 ) 
	{// if we can adjust model accurately, do so

        double slope, offset;

        // Calculate the slope of WPSNR versus log(lambda) from prior measurements
 	    slope = (current_quality - last_quality)/( log10(current_lambda) - log10(last_lambda) );
 
        //Restrict so that the value isn't too extreme
        slope = std::min( std::max( -10.0 , slope ), -0.1);

  		// Calculate the resulting offset
		offset = current_quality - ( log10(current_lambda) * slope );

        if ( count != 1 )
        {
            // Update the default values using a simple recursive filter ...
            m_slope[fsort] = (3.0*m_slope[fsort] + slope)/4.0;
            m_offset[fsort] = (3.0*m_offset[fsort] + offset)/4.0;
            m_slope[fsort] = std::min( std::max( -10.0 , m_slope[fsort] ), -0.1);
        }
        else
        {
            // .. unless we're recoding a frame for the first time            
            m_slope[fsort] = slope;
            m_offset[fsort] = offset;
            m_slope[fsort] = std::min( std::max( -10.0 , m_slope[fsort] ), -0.1);
        }

    }

    // If we need to adjust the lambdas, do so
	if ( std::abs(current_quality - target_quality)> 0.2 )
	{
        // Update the lambdas as appropriate
        float quality_diff = m_target_quality[fsort] - current_quality;

        CalcNewLambdas(fsort , m_slope[fsort] , quality_diff );
    }

    // if we have a large difference in WPSNR, recode)
    if ( std::abs( current_quality - target_quality )>1.5 )
        recode = true;

    return recode;
}

void QualityMonitor::CalcNewLambdas(const FrameSort fsort, const double slope, const double quality_diff )
{	
     if ( m_encparams.Lambda(fsort) <= 100001.0 && std::abs(quality_diff/slope <2.0) )
         m_encparams.SetLambda(fsort, m_encparams.Lambda(fsort) *
                             std::pow( (double)10.0, quality_diff/slope ) );
     else
         m_encparams.SetLambda(fsort, 100000.0);

     if (fsort == L1_frame)
 		m_encparams.SetL1MELambda( std::sqrt(m_encparams.L1Lambda()) * m_me_ratio );
     else if (fsort == L2_frame)
 		m_encparams.SetL2MELambda( std::sqrt(m_encparams.L2Lambda()) * m_me_ratio );

}

double QualityMonitor::QualityVal(const PicArray& coded_data, const PicArray& orig_data , double cpd)
{
    TwoDArray<long double> ms_diff(3,4);
	long double diff;

    OneDArray<int> xstart( ms_diff.LengthX() );
    OneDArray<int> xend( ms_diff.LengthX() );
    OneDArray<int> ystart( ms_diff.LengthY() );
    OneDArray<int> yend( ms_diff.LengthX() );

    for ( int i=0 ; i<xstart.Length() ; ++i)
    { 
        xstart[i] =( i * m_true_xl )/xstart.Length();
        xend[i] = ( (i+1) * m_true_xl )/xstart.Length();
    }

    for ( int i=0 ; i<ystart.Length() ; ++i)
    { 
        ystart[i] =( i * m_true_yl )/ystart.Length();
        yend[i] = ( (i+1) * m_true_yl )/ystart.Length();
    }

    for ( int q=0 ; q<ms_diff.LengthY() ; ++q )
    { 
        for ( int p=0 ; p<ms_diff.LengthX() ; ++p )
        { 
            ms_diff[q][p] = 0.0;

            for (int j=ystart[q]; j<yend[q]; ++j)
            {
                for (int i=xstart[p]; i<xend[p]; ++i)
                {
                    diff = static_cast<long double> ( coded_data[j][i] - orig_data[j][i]);
                    diff *= diff;
                    diff *= diff;

                    ms_diff[q][p] += diff;
                }//i
            }//j

            ms_diff[q][p] /= ( xend[p]-xstart[p] ) * ( yend[q]-ystart[q] );
            ms_diff[q][p] = std::sqrt( ms_diff[q][p] );

            // now compensate for the fact that we've got two extra bits
            ms_diff[q][p] /= 16.0;
        }// p
    }// q
     
    // return the worst quarter
    long double worst_mse = ms_diff[0][0];
    for ( int q=0 ; q<ms_diff.LengthY() ; ++q )
    { 
        for ( int p=0 ; p<ms_diff.LengthX() ; ++p )
        { 
            if ( ms_diff[q][p] > worst_mse )          
                worst_mse = ms_diff[q][p];
        }// p
    }// q

	return static_cast<double> ( 10.0 * std::log10( 255.0*255.0 / worst_mse ) );	
}
