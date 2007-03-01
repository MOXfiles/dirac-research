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
* Contributor(s): Thomas Davies (Original Author),
*                 Scott R Ladd,
*                 Tim Borer,
*                 Andrew Kennedy
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

#ifndef _MV_CODEC_H_
#define _MV_CODEC_H_

/////////////////////////////////////////////////
//Class to do motion vector coding and decoding//
//------using adaptive arithmetic coding-------//
/////////////////////////////////////////////////

#include <libdirac_common/arith_codec.h>
#include <libdirac_common/common.h>
#include <libdirac_common/motion.h>
#include <libdirac_common/wavelet_utils.h>
#include <vector>

namespace dirac
{

    //! Codes and decodes all the Motion Vector data
    /*!
        Derived from the ArithCodec class, this codes and decodes all the motion vector data.
     */
    class MvDataCodec: public ArithCodec<MvData>
    {
    public:
        //! Constructor
            /*!
            Creates a MvDataCodec object to encode MV data, based on parameters
            \param    p_byteio   Input/output for the encoded bits
            \param    number_of_contexts the number of contexts used
            \param     cf            the chroma format
         */    
        MvDataCodec(ByteIO* p_byteio,
                    size_t number_of_contexts,
                    const ChromaFormat & cf);

       

        //! Initialises the contexts    
        void InitContexts();
    
    private:

        const ChromaFormat & m_cformat;

        // Position of current block
        int m_b_xp, m_b_yp;
        // Position of current MB
        int m_mb_xp, m_mb_yp;
        // Position of top-left block of current MB
        int m_mb_tlb_x, m_mb_tlb_y;

    private:

        // functions   
        //! Private, bodyless copy constructor: class should not be copied
        MvDataCodec(const MvDataCodec& cpy);
        //! Private, bodyless copy operator=: class should not be assigned
        MvDataCodec& operator=(const MvDataCodec& rhs);

        // coding functions   
        // Code the MB splitting mode
        void CodeMBSplit(const MvData& in_data);
        // Code the block prediction mode
        void CodePredmode(const MvData& in_data);
        // Code the first motion vector
        void CodeMv1(const MvData& in_data);
        // Code the second motion vector
        void CodeMv2(const MvData& in_data);
        // Code the dc value of intra blocks
        void CodeDC(const MvData& in_data);

        // decoding functions
        // Decode the MB splitting mode
        void DecodeMBSplit( MvData& out_data);
        // Decode the block prediction mode
        void DecodePredmode(MvData& out_data);
        // Decode the first motion vector
        void DecodeMv1( MvData& out_data);
        // Decode the second motion vector
        void DecodeMv2( MvData& out_data);
        // Decode the dc value of intra blocks  
        void DecodeDC( MvData& out_data);

        void DoWorkCode( MvData& in_data );
        void DoWorkDecode(MvData& out_data);

        // Context stuff   
        void ResetAll();

        //prediction stuff
        unsigned int MBSplitPrediction(const TwoDArray<int>& mbdata) const;

        unsigned int BlockModePrediction(const TwoDArray<PredMode>& preddata,
                                         const unsigned int num_refs) const;

        MVector Mv1Prediction( const MvArray& mvarray,
                               const TwoDArray<PredMode>& preddata) const;

        MVector Mv2Prediction( const MvArray& mvarray, 
                               const TwoDArray<PredMode>& preddata) const;

        ValueType DCPrediction( const TwoDArray<ValueType>& dcdata,
                                const TwoDArray<PredMode>& preddata) const;
    };

}// end namepace dirac

#endif
