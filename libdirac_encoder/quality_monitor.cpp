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
using namespace dirac;

using std::log10;

QualityMonitor::QualityMonitor(EncoderParams& encp, 
                               const SeqParams& sparams)
:
    m_encparams(encp),
    m_cformat( sparams.CFormat() ),
    m_true_xl( sparams.Xl() ),
    m_true_yl( sparams.Yl() ),
    m_quality_average(3),
    m_frame_total(3)
{
    ResetAll();
}

QualityMonitor::~QualityMonitor()
{}

void QualityMonitor::ResetAll()
{
    // Set the lambdas

    if ( !m_encparams.Lossless() )
    {
        m_encparams.SetLambda( FrameSort::IntraRefFrameSort() , std::pow( 10.0 , (10.0-m_encparams.Qf() )/2.5 ) );
        m_encparams.SetLambda( FrameSort::InterRefFrameSort() , m_encparams.ILambda()*128.0 );
        m_encparams.SetLambda( FrameSort::InterNonRefFrameSort() , m_encparams.ILambda()*512.0 );


        // Set the lambdas for motion estimation
        const double me_ratio = 2.0;

        m_encparams.SetL1MELambda( std::sqrt(m_encparams.L1Lambda())*me_ratio );
        m_encparams.SetL2MELambda( std::sqrt(m_encparams.L2Lambda())*me_ratio );

        for (int i=0; i<3 ; ++i )
        {
            m_quality_average[i] = 0.0;
            m_frame_total[i] = 0;
        }// i
    }
    else
    {
        m_encparams.SetLambda( FrameSort::IntraRefFrameSort() , 0.0 );
        m_encparams.SetLambda( FrameSort::InterRefFrameSort() , 0.0 );
        m_encparams.SetLambda( FrameSort::InterNonRefFrameSort() , 0.0 );

        m_encparams.SetL1MELambda( 0.0 );
        m_encparams.SetL2MELambda( 0.0 );
    }
}

void QualityMonitor::WriteLog()
{
    std::cerr<<std::endl<<"Mean quality for Intra frames is "<<m_quality_average[0]/m_frame_total[0];
    std::cerr<<std::endl<<"Mean quality for Inter Ref frames is "<<m_quality_average[1]/m_frame_total[1];
    std::cerr<<std::endl<<"Mean quality for Inter Non-Ref frames is "<<m_quality_average[2]/m_frame_total[2]<<std::endl;
}

void QualityMonitor::UpdateModel(const Frame& ld_frame, const Frame& orig_frame )
{
	const FrameSort& fsort = ld_frame.GetFparams().FSort();	
	int idx = fsort.IsIntra() ? 0 : (fsort.IsRef() ? 1 : 2);

	m_quality_average[idx] += QualityVal( ld_frame.Ydata() , orig_frame.Ydata() , 0.0 , fsort );
    m_frame_total[idx]++;

}


double QualityMonitor::QualityVal(const PicArray& coded_data, const PicArray& orig_data , double cpd , const FrameSort fsort)
{

    // The number of regions to look at in assessing quality
    const int xregions( 4 );
    const int yregions( 3 );

    TwoDArray<long double> diff_array( yregions , xregions);
    TwoDArray<long double> diff_sq_array( yregions , xregions);
	long double diff;
    ValueType filt_orig, filt_coded;

    OneDArray<int> xstart( diff_array.LengthX() );
    OneDArray<int> xend( diff_array.LengthX() );
    OneDArray<int> ystart( diff_array.LengthY() );
    OneDArray<int> yend( diff_array.LengthX() );

    for ( int i=0 ; i<xstart.Length() ; ++i)
    { 
        xstart[i] =( i * (m_true_xl-2) )/xstart.Length()+1;
        xend[i] = ( (i+1) * (m_true_xl-2) )/xstart.Length()+1;
    }

    for ( int i=0 ; i<ystart.Length() ; ++i)
    { 
        ystart[i] =( i * (m_true_yl-2) )/ystart.Length()+1;
        yend[i] = ( (i+1) * (m_true_yl-2) )/ystart.Length()+1;
    }

    for ( int q=0 ; q<diff_array.LengthY() ; q++ )
    { 
        for ( int p=0 ; p<diff_array.LengthX() ; p++ )
        { 
            diff_sq_array[q][p] = 0.0;

            for (int j=ystart[q]; j<yend[q]; j++)
            {
                for (int i=xstart[p]; i<xend[p]; i++)
                {
                    filt_coded = Filter( coded_data , i , j );
                    filt_orig = Filter( orig_data , i , j );

                    diff = static_cast<long double> ( filt_coded - filt_orig );
 
                    diff *= diff;
                    diff *= diff;

                    diff_sq_array[q][p] += diff;
                }//i
            }//j

            diff_sq_array[q][p] /= ( xend[p]-xstart[p] ) * ( yend[q]-ystart[q] );

            // now compensate for the fact that we've got two extra bits
            diff_sq_array[q][p] /= 256.0;

            diff_array[q][p] = std::sqrt( diff_sq_array[q][p] );

        }// p
    }// q
     
    // return the self-weighted average

    long double sum_diff( 0 );
    long double sum_sq_diff( 0 );
    for ( int q=0 ; q<diff_array.LengthY() ; ++q )
    { 
        for ( int p=0 ; p<diff_array.LengthX() ; ++p )
        { 
            sum_diff += diff_array[q][p];
            sum_sq_diff += diff_sq_array[q][p];
        }// p
    }// q

	return static_cast<double> ( 10.0 * std::log10( 255.0*255.0*sum_diff / sum_sq_diff ) );	
}

ValueType QualityMonitor::Filter( const PicArray& data , const int xpos , const int ypos ) const
{
    return (
            int(data[ypos-1][xpos-1]) +6*int(data[ypos-1][xpos]) + int(data[ypos-1][xpos+1]) + 
            6*int(data[ypos][xpos-1]) + 36*int(data[ypos][xpos]) + 6*int(data[ypos][xpos+1]) + 
            int(data[ypos+1][xpos-1]) +6*int(data[ypos+1][xpos]) + int(data[ypos+1][xpos+1]) +
            32 )>>6;
}
