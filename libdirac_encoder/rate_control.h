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
*		  School of Engineering and Design, Brunel University, UK
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
//////////////////////////////////////////

#ifndef _RATE_CONTROL_H_
#define _RATE_CONTROL_H_

#include <libdirac_common/common.h>

namespace dirac
{
	class FrameComplexity
    {
	public:
		//! Default constructor   
        FrameComplexity();

        //! Return the complexity of I frame
		int IComplexity() {return m_XI;} 

		//! Return the complexity of L1 frame
		int L1Complexity() {return m_XL1;} 

		//! Return the complexity of L2 frame
		int L2Complexity() {return m_XL2;} 

		//! Set the complexity of I frame
		void SetIComplexity(int cpx) {m_XI = cpx;} 

		//! Set the complexity of L1 frame
		void SetL1Complexity(int cpx) {m_XL1 = cpx;} 

		//! Set the complexity of L2 frame
		void SetL2Complexity(int cpx) {m_XL2 = cpx;} 


	private:

		//! Complexity of I frame
		int m_XI; 

		//! Complexity of L1 frame
		int m_XL1; 

		//! Complexity of L2 frame
		int m_XL2; 
    };

	
	//! A clas for allocation the bits to each and every types of frames in a GOP
	class RateController
    {
	public:

		//! Default constructor   
        RateController(int trate, SourceParams& srcp, EncoderParams& encp);

		//! Allocate the bits to each type of frame in a GOP    
        void Allocate(const FrameParams& fparams, int num_bits);

		//! Calculate the Quality factor of the next frame to encode
		void CalcNextQualFactor(const FrameParams& fparams, int num_bits);

		//! Calculate the Quality factor of the next I frame to encode
		void CalcNextIntraQualFactor();

		//! Calculate the total number of bits in a GOP
		void CalcTotalBits();

		//! Set the value of Current IQF
		void SetIntraQualFactor(double value){m_I_qf = value;}

		//! Reset the number of I, L1 and L2 frames
		void ResetNumFrame();

		//! Return I frame qf
		double IntraQualFactor() {return m_I_qf;}

		//! Return qf
		double QualFactor() {return m_qf;}

		
	private:

		//! Current Quality Factor
		double m_qf; 

		//! I frame Quality Factor
		double m_I_qf; 

		//! Target bit rate in kbps
		int m_target_rate;

		//! Number of bits for I frame
		int m_Iframe_bits;

		//! Number of Encoded bits for I frame
		int m_enc_Iframe_bits;

		//! Number of bits for L1 frame
		int m_L1frame_bits;

		//! Number of bits for L2 frame
		int m_L2frame_bits;

		//! Number of I frames
		int m_num_Iframe;

		//! Number of L1 frames
		int m_num_L1frame;

		//! Number of L2 frames
		int m_num_L2frame;

		//! Total Number of bits in a GOP
		int m_total_GOP_bits;

		//! The Number of bits currently left for allocating the remaining frames in a GOP
		int m_current_GOP_bits;

		//! Total Number of bits for a sub_GOP, e.g. IPBB or PBB
		//! Corresponds to actual encoded bits
		int m_enc_num_bits;

		//! Total Number of bits for a sub_GOP, e.g. IPBB or PBB
		//! Corresponds to allocated bits based upon target bit rate
		int m_t_num_bits;

		//! The duration of a GOP 
		double m_GOP_duration;

		//! A reference to the source parameters
        SourceParams& m_srcparams;

		//! A reference to the encoder parameters
        EncoderParams& m_encparams;

		//! A class to hold the frame complexity object
        FrameComplexity m_frame_complexity;

		//! frame counter
		int m_fcount;

		//! Temp. Complexity of I frame
		int m_TXI; 

		//! Temp. Complexity of L1 frame
		int m_TXL1; 

		//! Temp. Complexity of L2 frame
		int m_TXL2; 

    };

	
}// namespace dirac
#endif
