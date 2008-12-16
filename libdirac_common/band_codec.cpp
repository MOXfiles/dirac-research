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
* Portions created by the Initial Developer are Copyright (c) 2004.
* All Rights Reserved.
*
* Contributor(s): Thomas Davies (Original Author),
*                 Scott R Ladd,
*                 Steve Bearcroft
*                 Andrew Kennedy
*                 Anuradha Suraparaju
*                 David Schleef
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

// System includes
#include <sstream>

// Dirac includes
#include <libdirac_common/band_codec.h>
#include <libdirac_byteio/subband_byteio.h>
#include <libdirac_common/dirac_exception.h>
#include <libdirac_common/band_codec_template.cpp>

using namespace dirac;

template
GenericBandCodec<ArithCodec<CoeffArray> >::GenericBandCodec(
    SubbandByteIO* subband_byteio,
    size_t number_of_contexts,
    const SubbandList & band_list,
    int band_num,
    const bool is_intra);

//////////////////////////////////////////////////////////////////////////////////
//Finally,special class incorporating prediction for the DC band of intra frames//
//////////////////////////////////////////////////////////////////////////////////

void IntraDCBandCodec::DoWorkCode(CoeffArray& in_data)
{
    // Residues after prediction, quantisation and inverse quantisation
    m_dc_pred_res.Resize( m_node.Yl() , m_node.Xl() );
    m_dc_pred_res.Fill( 0 );

    BandCodec::DoWorkCode(in_data);
}

void IntraDCBandCodec::CodeCoeff( CoeffArray& in_data, const int xpos, const int ypos)
{
    m_nhood_nonzero = false;
    if (ypos > m_node.Yp())
        m_nhood_nonzero |= bool(m_dc_pred_res[ypos-1][xpos]);
    if (xpos > m_node.Xp())
        m_nhood_nonzero |= bool(m_dc_pred_res[ypos][xpos-1]);
    if (ypos > m_node.Yp() && xpos > m_node.Xp())
        m_nhood_nonzero |= bool(m_dc_pred_res[ypos-1][xpos-1]);

    ValueType prediction = GetPrediction( in_data , xpos , ypos );
    ValueType val = in_data[ypos][xpos] - prediction;
    CodeVal( in_data , xpos , ypos , val );
    m_dc_pred_res[ypos][xpos] = in_data[ypos][xpos];
    in_data[ypos][xpos] += prediction;
}

/* after coding a skipped DC codeblock, reconstruct in_data by predicting the values
 * and not adding any error term (they were all skipped).  This is required to correctly
 * predict the values in the next codeblock */
void IntraDCBandCodec::ClearBlock( const CodeBlock& code_block , CoeffArray& coeff_data)
{
    for (int ypos=code_block.Ystart() ; ypos<code_block.Yend() ; ++ypos)
    {
        for (int xpos=code_block.Xstart() ; xpos<code_block.Xend() ; ++xpos)
        {
            /* NB, it is correct to overwrite the old value */
            coeff_data[ypos][xpos] = GetPrediction( coeff_data , xpos , ypos );
        } // i
    } // j
}

void IntraDCBandCodec::DoWorkDecode(CoeffArray& out_data)
{
    // Residues after prediction, quantisation and inverse quantisation
    m_dc_pred_res.Resize( m_node.Yl() , m_node.Xl() );
    m_dc_pred_res.Fill( 0 );

    BandCodec::DoWorkDecode(out_data);
}

void IntraDCBandCodec::DecodeCoeffBlock(const CodeBlock& code_block , CoeffArray& out_data)
{
    BandCodec::DecodeCoeffBlock(code_block, out_data);
    /* do prediction for this block */
    for ( int ypos=code_block.Ystart() ; ypos<code_block.Yend() ; ++ypos)
    {
        for ( int xpos=code_block.Xstart() ; xpos<code_block.Xend() ; ++xpos)
        {
             out_data[ypos][xpos] += GetPrediction( out_data , xpos , ypos );
        }
    }
}

void IntraDCBandCodec::DecodeCoeff( CoeffArray& out_data, const int xpos, const int ypos)
{
    m_nhood_nonzero = false;
    if (ypos > m_node.Yp())
        m_nhood_nonzero |= bool(m_dc_pred_res[ypos-1][xpos]);
    if (xpos > m_node.Xp())
        m_nhood_nonzero |= bool(m_dc_pred_res[ypos][xpos-1]);
    if (ypos > m_node.Yp() && xpos > m_node.Xp())
        m_nhood_nonzero |= bool(m_dc_pred_res[ypos-1][xpos-1]);

    DecodeVal( out_data , xpos , ypos );
    m_dc_pred_res[ypos][xpos] = out_data[ypos][xpos];
}

CoeffType IntraDCBandCodec::GetPrediction( const CoeffArray& data , const int xpos , const int ypos ) const
{
    /* NB, 4.5.3 integer division
     * numbers are rounded down towards -ve infinity, differing from
     * C's convention that rounds towards 0
    */
    if (ypos!=0)
    {
        if (xpos!=0)
        {
            int sum = data[ypos][xpos-1] + data[ypos-1][xpos-1] + data[ypos-1][xpos] + 3/2;
            if (sum<0)
                return (sum-2)/3;
            else
                return sum/3;
        }
        else
            return data[ypos - 1][0];
    }
    else
    {
        if(xpos!=0)
            return data[0][xpos - 1];
        else
            return 0;
    }
}
