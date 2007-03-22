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
* Contributor(s): Myo Tun (Original Author, myo.tun@brunel.ac.uk)
*                 Jonathan Loo (Jonathan.Loo@brunel.ac.uk)
*                 School of Engineering and Design, Brunel University, UK
*                 Thomas Davies
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


//Compression of an individual component,
//after motion compensation if appropriate
//////////////////////////////////////////#

#include <math.h>
#include <libdirac_encoder/rate_control.h>
using namespace dirac;

//Default constructor    
FrameComplexity::FrameComplexity():
    m_XI(169784),
    m_XL1(36016),
	m_XL2(4824)
{}

//Default constructor    
RateController::RateController(int trate, SourceParams& srcp, EncoderParams& encp):
	m_qf (7.0),
	m_I_qf (7.0),
	m_target_rate(trate),
	m_enc_num_bits(0),
	m_t_num_bits(0),	
	m_srcparams(srcp),
	m_encparams(encp),
	m_fcount(3),
	m_TXL2(0)
{
	ResetNumFrame();
	CalcTotalBits();
}

void RateController::ResetNumFrame()
{
	m_num_L1frame = m_encparams.NumL1();

	if (m_num_L1frame > 0) 
        m_num_Iframe = 1;
	else if (m_num_L1frame == 0) 
        m_num_Iframe = 10;
	else 
        m_num_Iframe = 0;

	m_num_L2frame = m_encparams.GOPLength() - m_num_Iframe - m_num_L1frame;
}

void RateController::Allocate (const FrameParams& fparams, int num_bits)
{
	const FrameSort& framesort = fparams.FSort();;
	int XI = m_frame_complexity.IComplexity();
	int XL1 = m_frame_complexity.L1Complexity();
	int XL2 = m_frame_complexity.L2Complexity();

	//Allocation for I frame only coding
	if ( framesort.IsIntra() )
	{
        m_Iframe_bits = (int) (m_current_GOP_bits
                      / (m_num_Iframe
                        +(float)(m_num_L1frame*XL1)/XI
                        +(float)(m_num_L2frame*XL2)/XI));
		m_current_GOP_bits -= num_bits;
		m_num_Iframe--;
	}

	//Allocation for L1 and L2 frames coding
	if ( framesort.IsInter() )
	{
		if ( fparams.IsBFrame() )
		{
            m_L2frame_bits = (int) (m_current_GOP_bits
                           / (m_num_L2frame
                             +(float)(m_num_Iframe*XI)/XL2
                             +(float)(m_num_L1frame*XL1)/XL2));
			m_current_GOP_bits -= num_bits;
			m_num_L2frame--;
		}
		else
		{
            m_L1frame_bits = (int) (m_current_GOP_bits
                           / (m_num_L1frame
                             +(float)(m_num_Iframe*XI)/XL1
                             +(float)(m_num_L2frame*XL2)/XL1));
			m_current_GOP_bits -= num_bits;
			m_num_L1frame--;
		}
    }
}

void RateController::CalcTotalBits()
{
	const Rational& frame_rate = m_srcparams.FrameRate();
	double f_rate = frame_rate.m_num/frame_rate.m_denom;
	int GOP_length = m_encparams.GOPLength();

	m_GOP_duration = GOP_length/f_rate;
	m_total_GOP_bits = int(m_GOP_duration*1000.0)*m_target_rate; //Unit in bits
	m_current_GOP_bits = m_total_GOP_bits;
	
	std::cout<<"\nRate Control Encoding with target bit rate = ";
    std::cout<<m_target_rate<<" kbps"<< std::endl;

	std::cout<<"GOP Length = "<<GOP_length<< std::endl;

	std::cout<<"Frame Rate = "<<f_rate<< std::endl;

	std::cout<<"GOP Duration = "<<m_GOP_duration<< std::endl;

	std::cout<<"Total Allocated Num. of bits for each GOP = ";
    std::cout<<m_total_GOP_bits<<"\n"<< std::endl;
}
void RateController::CalcNextQualFactor(const FrameParams& fparams, int num_bits)
{
	const FrameSort& fsort = fparams.FSort(); 
	
	if ( fsort.IsIntra() ) 
	{
		//I frame only coding
		if (m_encparams.NumL1() == 0)
		{
			double rate = num_bits/(1000*m_GOP_duration);
			double K = std::pow(rate, 2)*std::pow(10.0, ((double)2/5*(10-m_I_qf)))/16;
	
			Allocate (fparams, num_bits);

			double trate = m_Iframe_bits/(1000*m_GOP_duration);
			m_I_qf = 10 - (double)5/2*log10(16*K/std::pow(trate, 2));
	
//			SetLambda(m_I_qf);
			m_encparams.SetQf(m_I_qf);
		}
		//Normal Coding
		else
		{
			m_enc_Iframe_bits = num_bits;
			if (m_num_Iframe != 0)
                m_current_GOP_bits -= num_bits;
			m_num_Iframe--;
		}
	}	
    else
	{
		m_fcount--;
		Allocate (fparams, num_bits);//Allocation for L1 and L2 frames

		if ( fparams.IsBFrame() ) 
		{
			//L2 frame
			m_enc_num_bits += num_bits;
			m_t_num_bits += m_L2frame_bits;
			m_TXL2 += num_bits; //store temporarily for new complexity
		}
		else
		{
			//L1 Frame
			m_enc_num_bits += num_bits;
			m_t_num_bits += m_L1frame_bits;
			m_TXL1 = num_bits; //store temporarily for new complexity
		}
	}


	if (m_fcount == 0 ) 
	{
		//Calculating the QF of L1L2L2 frames
		if (m_num_L2frame != 0)
		{
			double rate = (double)(m_enc_num_bits)/(1000.0*m_GOP_duration);
			double K = std::pow(rate, 2)*std::pow(10.0, ((double)2/5*(10-m_qf)))/16;
			double trate = (double)(m_t_num_bits)/(1000.0*m_GOP_duration);

			m_qf = 10 - (double)5/2*log10(16*K/std::pow(trate, 2));
			
            m_encparams.SetQf(m_qf);
		}

		if (m_num_L2frame == 0) 
		{
			m_qf = m_I_qf;
            m_encparams.SetQf(m_qf);
		}
		
		//Updating the complexity
		m_frame_complexity.SetL1Complexity(m_TXL1);
		m_frame_complexity.SetL2Complexity(m_TXL2/2);//Taking the average
		

		//ReInitialization
		if (m_num_L2frame == 2)
            m_fcount = 2;
		else
            m_fcount = 3;

		m_enc_num_bits = 0;
		m_t_num_bits = 0;
		m_TXL2 = 0;
	}//End of fcount

	// Normal (long GOP) coding
	if (m_num_L2frame == 0 && m_encparams.NumL1() != 0)
	{
		ResetNumFrame();
		m_num_Iframe--;
		m_current_GOP_bits = m_total_GOP_bits;
		m_current_GOP_bits -= m_enc_Iframe_bits;
	}

	// I frame only coding
	if (m_num_Iframe == 0 && m_encparams.NumL1() == 0)
	{
		ResetNumFrame();
		m_current_GOP_bits = m_total_GOP_bits;
	}

}

void RateController::CalcNextIntraQualFactor()
{
	double delta_qf = std::abs(m_qf - m_I_qf);

	double new_qf;
	if (m_I_qf < m_qf) 
        new_qf = m_I_qf + delta_qf/2;
	else 
        new_qf = m_I_qf - delta_qf/2;

	m_I_qf = new_qf;
	m_encparams.SetQf(m_I_qf);
}
