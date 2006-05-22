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
    m_sparams( sparams ),
    m_quality_averageY(3),
    m_quality_averageU(3),
    m_quality_averageV(3),
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
            m_quality_averageY[i] = 0.0;
            m_quality_averageU[i] = 0.0;
            m_quality_averageV[i] = 0.0;
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
    std::cerr<<std::endl<<"Mean PSNR values by frame type and component";
    std::cerr<<std::endl<<"--------------------------------------------";
    std::cerr<<std::endl;
    
    std::cerr<<std::endl<<"                 ||       Y       ||       U       ||       V       ||";
    std::cerr<<std::endl<<"=================||===================================================";
    std::cerr<<std::endl<<"           Intra ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageY[0]/m_frame_total[0]<<"     ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageU[0]/m_frame_total[0]<<"     ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageV[0]/m_frame_total[0]<<"     ||    ";
    std::cerr<<std::endl<<"-----------------||---------------------------------------------------";
    std::cerr<<std::endl<<"       Inter Ref ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageY[1]/m_frame_total[1]<<"     ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageU[1]/m_frame_total[1]<<"     ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageV[1]/m_frame_total[1]<<"     ||    ";
    std::cerr<<std::endl<<"-----------------||---------------------------------------------------";
    std::cerr<<std::endl<<"   Inter Non Ref ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageY[2]/m_frame_total[2]<<"     ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageU[2]/m_frame_total[2]<<"     ||     ";
    std::cerr.width(5);std::cerr.precision(4);
    std::cerr<<m_quality_averageV[2]/m_frame_total[2]<<"     ||     ";
    std::cerr<<std::endl<<"-----------------||---------------------------------------------------";
}

void QualityMonitor::UpdateModel(const Frame& ld_frame, const Frame& orig_frame )
{
	const FrameSort& fsort = ld_frame.GetFparams().FSort();	
	int idx = fsort.IsIntra() ? 0 : (fsort.IsRef() ? 1 : 2);

	m_quality_averageY[idx] += QualityVal( ld_frame.Ydata() , orig_frame.Ydata(), m_sparams.Xl(), m_sparams.Yl() );
	m_quality_averageU[idx] += QualityVal( ld_frame.Udata() , orig_frame.Udata(), m_sparams.ChromaWidth(), m_sparams.ChromaHeight() );
	m_quality_averageV[idx] += QualityVal( ld_frame.Vdata() , orig_frame.Vdata(), m_sparams.ChromaWidth(), m_sparams.ChromaHeight() );
    m_frame_total[idx]++;

}


double QualityMonitor::QualityVal(const PicArray& coded_data, const PicArray& orig_data, 
const int xlen, const int ylen )
{
    long double sum_sq_diff = 0.0;
    double diff;
    for ( int j=0;j<ylen; ++j )
    {
        for ( int i=0;i<xlen; ++i )
        {
            diff = orig_data[j][i] - coded_data[j][i];
            sum_sq_diff += diff*diff;

        }// i
    }// j

    const double max = double( (1<<m_sparams.GetVideoDepth())-1 );

    sum_sq_diff /= xlen*ylen;

	return static_cast<double> ( 10.0 * std::log10( max*max / sum_sq_diff ) );
}
