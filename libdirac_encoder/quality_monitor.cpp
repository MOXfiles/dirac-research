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
    encparams(encp),
    m_cformat( sparams.CFormat() ),

    m_target_wpsnr(3),
    m_last_wpsnr(3),
    m_slope(3),
    m_offset(3),
    m_last_lambda(3),

    m_wtransform(4)
{
    ResetAll();
}

void QualityMonitor::ResetAll()
{
    // set target WPSNRs
 	m_target_wpsnr[I_frame] = 0.28 * encparams.Qf()* encparams.Qf() + 20.0 ;
	m_target_wpsnr[L1_frame] = m_target_wpsnr[I_frame] - 1.5;
	m_target_wpsnr[L2_frame] = m_target_wpsnr[I_frame] - 2.5;

    // assume we hit those targets last time
    m_last_wpsnr = m_target_wpsnr;

    // set defaults for the model
     m_slope[I_frame] = -4.0;
     m_slope[L1_frame] = -4.0;
     m_slope[L2_frame] = -4.0;
     m_offset[I_frame] = 38.5,
     m_offset[L1_frame] = 43.3;
     m_offset[L2_frame] = 43.3;

    for (size_t fsort=0; fsort<3; ++fsort)
    {
        m_last_lambda[fsort] = std::pow( 10.0, (m_target_wpsnr[fsort] - m_offset[fsort])/m_slope[fsort] );
    }// fsort

    // set a default ratio for the motion estimation lambda
    // Exact value TBD - will incorporate stuff about blocks and so on
    // Also need to think about how this can be adapted for sequences for more or less motion 

    m_me_ratio = 0.005;

 	// set up the Lagrangian parameters
    for (size_t fsort=0; fsort<3; ++fsort)
    {
        encparams.SetLambda( FrameSort(fsort), m_last_lambda[fsort] );
    }// fsort

    encparams.SetL1MELambda( encparams.L1Lambda()*m_me_ratio );
    encparams.SetL2MELambda( encparams.L2Lambda()*m_me_ratio );
}

void QualityMonitor::UpdateModel(const Frame& ld_frame, const Frame& orig_frame, float cpd)
{
	const FrameSort& fsort = ld_frame.GetFparams().FSort();	
	double target_wpsnr;	

	//parameters relating to the last frame we measured
	double last_lambda;
	double last_wpsnr;

	//parameters relating to the current frame
	double current_lambda;
	double current_wpsnr;


    //set up local parameters for the particular frame type
    current_lambda = encparams.Lambda(fsort);
    last_lambda = m_last_lambda[fsort];
    last_wpsnr = m_last_wpsnr[fsort];
    target_wpsnr = m_target_wpsnr[fsort];

	// calculate the actual WPSNR that we have for the current frame
//	current_wpsnr = WeightedPSNRDiff( ld_frame.Ydata() , orig_frame.Ydata() , cpd);
    // TBD: Currently using unweighted PSNR since this seems to give a) more stable convergence and b) more bits allocated
    //to I frames at the expense of L1 and L2 frames, which is more efficient. Need to get a better measure, however.
	current_wpsnr = WeightedPSNRDiff( ld_frame.Ydata() , orig_frame.Ydata() , 0.0);

    if ( encparams.Verbose() )
        std::cerr<<std::endl<<"Weighted PSNR for frame is "<<current_wpsnr<<" ; target is "<<target_wpsnr;

    // Copy current data into memory for last frame data
    m_last_lambda[fsort] = encparams.Lambda(fsort);
    m_last_wpsnr[fsort] = current_wpsnr;

	//ok, so we've got an actual WPSNR to use. We know the lambda used before and the resulting
	//WPSNR then allows us to estimate the slope of the curve of WPSNR versus log of lambda

	if ( std::abs(current_wpsnr - last_wpsnr)> 0.2 && 
         std::abs(log10(current_lambda) - log10(last_lambda)) > 0.1 ) 
	{// if we can adjust model accurately, do so

        double slope, offset;

        // Calculate the slope of WPSNR versus log(lambda) from prior measurements
 	    slope = (current_wpsnr - last_wpsnr)/( log10(current_lambda) - log10(last_lambda) );
 
        //Restrict so that the value isn't too extreme
        slope = std::min( std::max( -10.0 , slope ), -0.1);

  		// Calculate the resulting offset
		offset = current_wpsnr - ( log10(current_lambda) * slope );

        // Update the default values using a simple recursive filter
        m_slope[fsort] = (9.0*m_slope[fsort] + slope)/10.0;
        m_offset[fsort] = (9.0*m_offset[fsort] + offset)/10.0;
        m_slope[fsort] = std::min( std::max( -10.0 , m_slope[fsort] ), -0.1);

    }

    // If we need to adjust the lambdas, do so
	if ( std::abs(current_wpsnr - target_wpsnr)> 0.2 )
	{
        // Update the lambdas as appropriate
        float wpsnr_diff = m_target_wpsnr[fsort] - current_wpsnr;

        CalcNewLambdas(fsort , m_slope[fsort] , wpsnr_diff );
    }


}

void QualityMonitor::CalcNewLambdas(const FrameSort fsort, const double slope, const double wpsnr_diff )
{	

     if ( encparams.Lambda(fsort) <= 100001.0 && std::abs(wpsnr_diff/slope <2.0) )
         encparams.SetLambda(fsort, encparams.Lambda(fsort) *
                             std::pow( (double)10.0, wpsnr_diff/slope ) );
     else
         encparams.SetLambda(fsort, 100000.0);

     if (fsort == L1_frame)
 		encparams.SetL1MELambda( encparams.L1Lambda() * m_me_ratio );
     else if (fsort == L2_frame)
 		encparams.SetL2MELambda( encparams.L2Lambda() * m_me_ratio );

}

double QualityMonitor::WeightedPSNRDiff(const PicArray& pic1_data, const PicArray& pic2_data, float cpd)
{
	long double mean_square_diff = 0.0;
	long double diff;

	if ( cpd == 0.0 )
	{
		for (int j=0; j<pic1_data.LengthY(); ++j)
		{
			for (int i=0; i<pic1_data.LengthX(); ++i)
			{
				diff = (long double) ( pic1_data[j][i] - pic2_data[j][i]);
				diff *= diff;
				mean_square_diff += diff;
			}//i
		}//j

		mean_square_diff /= pic1_data.LengthX()*pic1_data.LengthY();
	}
	else
	{
		//we need to do the wavelet transform	
		WaveletTransform wtransform(4);

		PicArray diff_data( pic1_data );

		for (int j=0; j<pic1_data.LengthY(); ++j)
		{
			for (int i=0; i<pic1_data.LengthX(); ++i)
			{
				diff_data[j][i] -= pic2_data[j][i];
			}//i
		}//j

		wtransform.Transform(FORWARD , diff_data);
	    wtransform.SetBandWeights(cpd , I_frame , m_cformat , pic1_data.CSort());

		const SubbandList& bands=wtransform.BandList();
		long double temp_val;

		for ( int B=1 ; B<=bands.Length() ; ++B )
		{

 			//go through each subband, adding in weighted values
			for ( int j=bands(B).Yp() ; j<bands(B).Yp()+bands(B).Yl() ; ++j)
			{
				for ( int i=bands(B).Xp() ; i<bands(B).Xp()+bands(B).Xl() ; ++i)
				{
					temp_val = (long double) diff_data[j][i];
					temp_val /= bands(B).Wt();
					temp_val *= temp_val;
					mean_square_diff += temp_val;
				}//i
			}//j

		}//B

		mean_square_diff /= pic1_data.LengthX()*pic1_data.LengthY();
	}

    // now compensate for the fact that we've got two extra bits
    mean_square_diff /= 16.0;

	return (double) 10.0*std::log10(255.0*255.0 / mean_square_diff);	
}
