/* ***** BEGIN LICENSE BLOCK *****
*
* $Id$ $Name$
*
* Version: MPL 1.1/GPL 2.0/LGPL 2.1
*
* The contents of this file are subject to the Mozilla Public License
* Version 1.1 (the "License");  you may not use this file except in compliance
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
* Contributor(s): Thomas Davies (Original Author), Scott R Ladd
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

#include <libdirac_common/mv_codec.h>

using namespace dirac;

//public functions//
////////////////////
// Constructor for encoding
MvDataCodec::MvDataCodec(BasicOutputManager* bits_out,
                         size_t number_of_contexts,
                         const ChromaFormat& cf)
  : ArithCodec <MvData> (bits_out,number_of_contexts),
    m_MB_count( 0 ),
    m_reset_num( 32 ),
    m_cformat(cf)
{}        

// Constructor for decoding
MvDataCodec::MvDataCodec(BitInputManager* bits_in,
                         size_t number_of_contexts,
                         const ChromaFormat& cf)
  : ArithCodec <MvData> (bits_in,number_of_contexts),
    m_MB_count( 0 ),
    m_reset_num( 32 ),
	m_cformat(cf)
{}    

void MvDataCodec::InitContexts() 
{
    for (size_t i = 0;  i < m_context_list.size();  ++i)
        m_context_list[i].SetCounts(1,1);     
}

    

//protected functions//
///////////////////////

inline void MvDataCodec::Update( const bool symbol , const int context_num )
{
    Context& ctx = m_context_list[context_num];

    ctx.IncrCount( symbol );
    
    if ( ctx.Weight() >= 1024 )
        ctx.HalveCounts();
}

inline void MvDataCodec::ResetAll()
{
    for ( size_t c = 0; c < m_context_list.size(); ++c)
        if ( m_context_list[c].Weight() > 16 )
            m_context_list[c].HalveCounts();
}

//coding functions//
////////////////////

//prediction functions

//proper context functions

inline int MvDataCodec::ChooseMBSContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return MB_SPLIT_BIN1_CTX; 
    else
        return MB_SPLIT_BIN2_CTX; 
}

inline int MvDataCodec::ChooseMBCContext(const MvData& data) const
{
    return MB_CMODE_CTX; 
}

inline int MvDataCodec::ChooseREF1xContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return REF1x_BIN1_CTX; 
    else if (BinNumber == 2)
        return REF1x_BIN2_CTX; 
    else if (BinNumber == 3)
        return REF1x_BIN3_CTX; 
    else if (BinNumber == 4)
        return REF1x_BIN4_CTX; 
    else
        return REF1x_BIN5plus_CTX; 
}

inline int MvDataCodec::ChooseREF1xSignContext(const MvData& data) const
{
    return REF1x_SIGN0_CTX;
}

inline int MvDataCodec::ChooseREF1yContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return REF1y_BIN1_CTX; 
    else if (BinNumber == 2)
        return REF1y_BIN2_CTX; 
    else if (BinNumber == 3)
        return REF1y_BIN3_CTX; 
    else if (BinNumber == 4)
        return REF1y_BIN4_CTX; 
    else
        return REF1y_BIN5plus_CTX; 
}

inline int MvDataCodec::ChooseREF1ySignContext(const MvData& data) const
{
    return REF1y_SIGN0_CTX;
}

inline int MvDataCodec::ChooseREF2xContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return REF2x_BIN1_CTX; 
    else if (BinNumber == 2)
        return REF2x_BIN2_CTX; 
    else if (BinNumber == 3)
        return REF2x_BIN3_CTX; 
    else if (BinNumber == 4)
        return REF2x_BIN4_CTX; 
    else
        return REF2x_BIN5plus_CTX; 
}

inline int MvDataCodec::ChooseREF2xSignContext(const MvData& data) const
{
    return REF2x_SIGN0_CTX; 
}

inline int MvDataCodec::ChooseREF2yContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return REF2y_BIN1_CTX; 
    else if (BinNumber == 2)
        return REF2y_BIN2_CTX; 
    else if (BinNumber == 3)
        return REF2y_BIN3_CTX; 
    else if (BinNumber == 4)
        return REF2y_BIN4_CTX; 
    else
        return REF2y_BIN5plus_CTX; 
}

inline int MvDataCodec::ChooseREF2ySignContext(const MvData& data) const
{
    return REF2y_SIGN0_CTX; 
}

inline int MvDataCodec::ChooseYDCContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return YDC_BIN1_CTX; 
    else
        return YDC_BIN2plus_CTX; 
}

inline int MvDataCodec::ChooseUDCContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return UDC_BIN1_CTX; 
    else
        return UDC_BIN2plus_CTX; 
}

inline int MvDataCodec::ChooseVDCContext(const MvData& data, const int BinNumber) const
{
    if (BinNumber == 1)
        return VDC_BIN1_CTX; 
    else
        return VDC_BIN2plus_CTX; 
}

inline int MvDataCodec::ChooseYDCSignContext(const MvData& data) const
{
    return YDC_SIGN0_CTX; 
}

inline int MvDataCodec::ChooseUDCSignContext(const MvData& data) const
{
    return UDC_SIGN0_CTX; 
}

inline int MvDataCodec::ChooseVDCSignContext(const MvData& data) const
{
    return VDC_SIGN0_CTX; 
}

inline unsigned int MvDataCodec::MBSplitPrediction(const TwoDArray<int> & split_data ) const
{    
    int result = 0;
    
    std::vector < unsigned int >  nbrs;
    
    if (m_mb_xp > 0 && m_mb_yp > 0)
    {
        nbrs.push_back( split_data[m_mb_yp-1][m_mb_xp] ); 
        nbrs.push_back( split_data[m_mb_yp-1][m_mb_xp-1] ); 
        nbrs.push_back( split_data[m_mb_yp][m_mb_xp-1] ); 

        result = GetMean(nbrs);     
    }
    else if (m_mb_xp > 0 && m_mb_yp == 0)
        result = split_data[m_mb_yp][m_mb_xp-1]; 
    else if (m_mb_xp == 0 && m_mb_yp > 0)
        result =  split_data[m_mb_yp-1][m_mb_xp]; 

    return result; 
}

inline bool MvDataCodec::MBCBModePrediction(const TwoDArray <bool> & cbm_data) const
{
    bool result = true;
    std::vector < unsigned int >  nbrs; 
    
    if (m_mb_xp > 0 && m_mb_yp > 0)
    {
        nbrs.push_back( (unsigned int)( cbm_data[m_mb_yp-1][m_mb_xp] ) ); 
        nbrs.push_back( (unsigned int)( cbm_data[m_mb_yp-1][m_mb_xp-1] ) ); 
        nbrs.push_back( (unsigned int)( cbm_data[m_mb_yp][m_mb_xp-1] ) ); 

        result = bool(GetMean(nbrs));     
    }
    else if (m_mb_xp > 0 && m_mb_yp == 0)
        result = cbm_data[m_mb_yp][m_mb_xp-1]; 
    else if (m_mb_xp == 0 && m_mb_yp > 0)
        result = cbm_data[m_mb_yp-1][m_mb_xp]; 
    
    return result; 
}

inline unsigned int MvDataCodec::BlockModePrediction(const TwoDArray < PredMode > & preddata) const
{
    unsigned int result = (unsigned int)(REF1_ONLY);
    
    unsigned int num_ref1_nbrs( 0 ); 
    unsigned int num_ref2_nbrs( 0 );
    
    if (m_b_xp > 0 && m_b_yp > 0)
    {
        num_ref1_nbrs += ((unsigned int)( preddata[m_b_yp-1][m_b_xp] ) ) & 1; 
        num_ref1_nbrs += ((unsigned int)( preddata[m_b_yp-1][m_b_xp-1] ) ) & 1; 
        num_ref1_nbrs += ((unsigned int)( preddata[m_b_yp][m_b_xp-1] ) ) & 1;

        result = num_ref1_nbrs>>1;

        num_ref2_nbrs += ((unsigned int)( preddata[m_b_yp-1][m_b_xp] ) ) & 2; 
        num_ref2_nbrs += ((unsigned int)( preddata[m_b_yp-1][m_b_xp-1] ) ) & 2; 
        num_ref2_nbrs += ((unsigned int)( preddata[m_b_yp][m_b_xp-1] ) ) & 2; 
        num_ref2_nbrs >>= 1;

        result ^= ( (num_ref2_nbrs>>1)<<1 );
    }
    else if (m_b_xp > 0 && m_b_yp == 0)
        result = (unsigned int)( preddata[0][m_b_xp-1] ); 
    else if (m_b_xp == 0 && m_b_yp > 0)
        result = (unsigned int)( preddata[m_b_yp-1][0] ); 

    return result; 
}

inline MVector MvDataCodec::Mv1Prediction(const MvArray& mvarray,
                                          const TwoDArray < PredMode > & preddata) const
{
    std::vector <MVector>  nbrs; 
    PredMode pmode;     
    MVector result; 
    
    if (m_b_xp > 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][m_b_xp]; 
        if (pmode == REF1_ONLY || pmode == REF1AND2) 
            nbrs.push_back(mvarray[m_b_yp-1][m_b_xp]); 
        
        pmode = preddata[m_b_yp-1][m_b_xp-1]; 
        if (pmode == REF1_ONLY || pmode == REF1AND2)
            nbrs.push_back(mvarray[m_b_yp-1][m_b_xp-1]); 
        
        pmode = preddata[m_b_yp][m_b_xp-1]; 
        if (pmode == REF1_ONLY || pmode == REF1AND2)        
            nbrs.push_back(mvarray[m_b_yp][m_b_xp-1]); 
        
        if (nbrs.size() > 0)
            result = MvMedian(nbrs); 
    }
    else if (m_b_xp > 0 && m_b_yp == 0)
    {
        pmode = preddata[0][m_b_xp-1]; 
        if (pmode == REF1_ONLY || pmode == REF1AND2)
            result = mvarray[0][m_b_xp-1]; 
    }
    else if (m_b_xp == 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][0]; 
        if (pmode == REF1_ONLY || pmode == REF1AND2)
            result = mvarray[m_b_yp-1][0]; 
    }

    return result; 
}

inline MVector MvDataCodec::Mv2Prediction(const MvArray & mvarray,
                                          const TwoDArray < PredMode >  & preddata) const
{
    std::vector <MVector>  nbrs; 
    PredMode pmode; 
    MVector result; 
    
    if (m_b_xp > 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][m_b_xp]; 
        if (pmode == REF2_ONLY || pmode == REF1AND2)
            nbrs.push_back(mvarray[m_b_yp-1][m_b_xp]); 
        
        pmode = preddata[m_b_yp-1][m_b_xp-1]; 
        if (pmode == REF2_ONLY || pmode == REF1AND2)
            nbrs.push_back(mvarray[m_b_yp-1][m_b_xp-1]); 
        
        pmode = preddata[m_b_yp][m_b_xp-1]; 
        if (pmode == REF2_ONLY || pmode == REF1AND2)
            nbrs.push_back(mvarray[m_b_yp][m_b_xp-1]); 
        
        if (nbrs.size() > 0)
            result = MvMedian(nbrs); 
    }
    else if (m_b_xp > 0 && m_b_yp == 0)
    {
        pmode = preddata[0][m_b_xp-1]; 
        if(pmode == REF2_ONLY || pmode == REF1AND2)
            result = mvarray[0][m_b_xp-1]; 
    }
    else if (m_b_xp == 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][0]; 
        if(pmode == REF2_ONLY || pmode == REF1AND2)
            result = mvarray[m_b_yp-1][0]; 
    }

    return result; 
}

inline ValueType MvDataCodec::DCPrediction(const TwoDArray < ValueType > & dcdata,
                                           const TwoDArray < PredMode > & preddata) const
{
    std::vector < int >  nbrs; 
    PredMode pmode;
    ValueType result = 128; 
    
    if (m_b_xp > 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][m_b_xp]; 
        if (pmode == INTRA) 
            nbrs.push_back(int(dcdata[m_b_yp-1][m_b_xp])); 
        
        pmode = preddata[m_b_yp-1][m_b_xp-1]; 
        if (pmode == INTRA)
            nbrs.push_back(int(dcdata[m_b_yp-1][m_b_xp-1])); 
        
        pmode = preddata[m_b_yp][m_b_xp-1]; 
        if (pmode == INTRA)        
            nbrs.push_back(int(dcdata[m_b_yp][m_b_xp-1])); 
        
        if (nbrs.size() > 0)
            result = ValueType(GetMean(nbrs));     
    }
    else if (m_b_xp > 0 && m_b_yp == 0)
    {
        pmode = preddata[0][m_b_xp-1]; 
        if (pmode == INTRA)
            result = dcdata[0][m_b_xp-1]; 
    }
    else if (m_b_xp == 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][0]; 
        if (pmode == INTRA)
            result = dcdata[m_b_yp-1][0]; 
    }

    return result;
}


void MvDataCodec::DoWorkCode( MvData& in_data )
{
    int step,max; 
    int pstep,pmax; 
    int split_depth; 
    bool common_ref; 
    
    for (m_mb_yp = 0, m_mb_tlb_y = 0;  m_mb_yp < in_data.MBSplit().LengthY();  ++m_mb_yp, m_mb_tlb_y += 4)
    {
        for (m_mb_xp = 0,m_mb_tlb_x = 0; m_mb_xp < in_data.MBSplit().LengthX(); ++m_mb_xp,m_mb_tlb_x += 4)
        {
             //start with split mode
            CodeMBSplit(in_data); 
            split_depth = in_data.MBSplit()[m_mb_yp][m_mb_xp]; 

            step = 4  >>  (split_depth); 
            max = (1 << split_depth); 

            //next do common_ref
            if(split_depth != 0)
            {
                CodeMBCom(in_data); 
                pstep = step; 
                pmax = max; 
            }
            else
            {
                pstep = 4; 
                pmax = 1; 
            }
            common_ref = in_data.MBCommonMode()[m_mb_yp][m_mb_xp]; 


            //do prediction modes            
            for (m_b_yp = m_mb_tlb_y; m_b_yp < m_mb_tlb_y+4; m_b_yp += pstep)
                for (m_b_xp = m_mb_tlb_x; m_b_xp < m_mb_tlb_x+4; m_b_xp += pstep)
                    CodePredmode(in_data); 
            
            step = 4 >> (split_depth);             
            
               //now do all the block mvs in the mb            
            for (m_b_yp = m_mb_tlb_y; m_b_yp < m_mb_tlb_y+4; m_b_yp += step)
            {
                for (m_b_xp = m_mb_tlb_x; m_b_xp < m_mb_tlb_x+4; m_b_xp += step)
                {
                    if (in_data.Mode()[m_b_yp][m_b_xp] == REF1_ONLY || in_data.Mode()[m_b_yp][m_b_xp] == REF1AND2 )
                        CodeMv1(in_data); 
                    
                    if (in_data.Mode()[m_b_yp][m_b_xp] == REF2_ONLY || in_data.Mode()[m_b_yp][m_b_xp] == REF1AND2 )
                        CodeMv2(in_data); 
                    
                    if(in_data.Mode()[m_b_yp][m_b_xp] == INTRA)
                        CodeDC(in_data);                     
                }//m_b_xp
            }//m_b_yp    
            
            m_MB_count++;
    
            if (m_MB_count > m_reset_num)
            {
                 m_MB_count = 0;
                 ResetAll();
            }
            
        }//m_mb_xp
    }//m_mb_yp

}


void MvDataCodec::CodeMBSplit(const MvData& in_data)
{
    int val = in_data.MBSplit()[m_mb_yp][m_mb_xp] - MBSplitPrediction( in_data.MBSplit() ); 
    
    if (val < 0)
        val += 3; //produce prediction mod 3    
    
    int ctx; 
    
    for (int bin = 1; bin <= val; ++bin)
    {
        ctx = ChooseMBSContext(in_data,bin); 
        EncodeSymbol(0,ctx); 
    }
    
    if (val != 2)//if we've had two zeroes, know we must have value 2
        EncodeSymbol(1,ChooseMBSContext(in_data,val+1)); 
}

void MvDataCodec::CodeMBCom(const MvData& in_data)
{
    bool val = in_data.MBCommonMode()[m_mb_yp][m_mb_xp]; 
    
    if (val != MBCBModePrediction( in_data.MBCommonMode() ))
        EncodeSymbol( 1 , ChooseMBCContext( in_data ) ); 
    else
        EncodeSymbol( 0 , ChooseMBCContext( in_data ) ); 
}

void MvDataCodec::CodePredmode(const MvData& in_data)
{
    // Xor with the prediction so we predict whether REF1 is used or REF2 is
    // used, separately
    unsigned int residue = in_data.Mode()[m_b_yp][m_b_xp] ^ BlockModePrediction( in_data.Mode() ); 
    
    // Code REF1 part of the prediction residue (ie the first bit)
    EncodeSymbol( residue & 1 , PMODE_BIT0_CTX );

    // Code REF2 part of the prediction residue (ie the second bit)
    EncodeSymbol( residue & 2 , PMODE_BIT1_CTX );

}

void MvDataCodec::CodeMv1(const MvData& in_data )
{
    const MvArray& mv_array = in_data.Vectors(1);
    const MVector pred = Mv1Prediction( mv_array , in_data.Mode() ); 

    const int valx = mv_array[m_b_yp][m_b_xp].x - pred.x; 
    const int abs_valx = std::abs(valx); 
    
    for (int bin = 1;  bin  <= abs_valx;  ++bin)
        EncodeSymbol( 0 , ChooseREF1xContext( in_data , bin ) ); 

    EncodeSymbol( 1 , ChooseREF1xContext( in_data , abs_valx + 1 ) ); 
    
    if (valx != 0)
        EncodeSymbol( ( (valx > 0)? 1 : 0) , ChooseREF1xSignContext( in_data ) ); 

    const int valy = mv_array[m_b_yp][m_b_xp].y - pred.y;         
    const int abs_valy = std::abs( valy );     
    
    for (int bin = 1; bin<=abs_valy ; ++bin )
        EncodeSymbol( 0 , ChooseREF1yContext( in_data , bin ) );         
    
    EncodeSymbol( 1 , ChooseREF1yContext( in_data , abs_valy + 1 ) ); 
    
    if (valy != 0)
        EncodeSymbol( ( (valy > 0)? 1 : 0) , ChooseREF1ySignContext( in_data ) );  
}

void MvDataCodec::CodeMv2(const MvData& in_data)
{
    const MvArray& mv_array = in_data.Vectors(2);
    const MVector pred = Mv2Prediction( mv_array , in_data.Mode() );     

    const int valx = mv_array[m_b_yp][m_b_xp].x - pred.x; 
    const int abs_valx = abs(valx); 
    
    for (int bin = 1; bin <= abs_valx; ++bin)
        EncodeSymbol( 0 , ChooseREF2xContext( in_data , bin ) ); 

    EncodeSymbol( 1 , ChooseREF2xContext( in_data , abs_valx + 1 ) ); 
    
    if (valx != 0)
        EncodeSymbol( ( (valx > 0)? 1 : 0) , ChooseREF2xSignContext( in_data ) ); 

    const int valy = mv_array[m_b_yp][m_b_xp].y-pred.y; 
    const int abs_valy = std::abs(valy);     
    
    for (int bin = 1; bin<=abs_valy; ++bin )
        EncodeSymbol( 0 , ChooseREF2yContext( in_data , bin ) ); 
    
    EncodeSymbol( 1 , ChooseREF2yContext( in_data , abs_valy + 1 ) ); 
    
    if (valy != 0)
        EncodeSymbol( ( (valy > 0)? 1 : 0) , ChooseREF2ySignContext( in_data ) ); 
}

void MvDataCodec::CodeDC(const MvData& in_data)
{    
    //begin with Y DC value    
    const ValueType valY = in_data.DC( Y_COMP )[m_b_yp][m_b_xp]
                           - DCPrediction( in_data.DC(Y_COMP) , in_data.Mode() ); 
    const ValueType abs_valY = std::abs( valY ); 

    for (ValueType bin = 1; bin <= abs_valY; ++bin)
        EncodeSymbol( 0 , ChooseYDCContext( in_data , bin ) ); 

    EncodeSymbol( 1 , ChooseYDCContext (in_data , abs_valY + 1 ) ); 
    
    if (valY != 0)
        EncodeSymbol( ( (valY > 0)? 1 : 0) , ChooseYDCSignContext( in_data ) ); 

    //now do U and V if necessary
    if (m_cformat != Yonly)
    {
        //continue with U and V DC values
        const int valU =  in_data.DC(U_COMP)[m_b_yp][m_b_xp]
                          - DCPrediction(in_data.DC( U_COMP ) , in_data.Mode()); 
        const int abs_valU = std::abs( valU ); 

        for (ValueType bin = 1;  bin<=abs_valU ; ++bin)
            EncodeSymbol( 0 , ChooseUDCContext( in_data , bin ) ); 
        
        EncodeSymbol( 1 , ChooseUDCContext( in_data , abs_valU + 1 ) ); 
        
        if (valU != 0)
            EncodeSymbol( ( (valU > 0) ? 1 : 0) , ChooseUDCSignContext( in_data ) ); 
        
        const int valV = in_data.DC( V_COMP )[m_b_yp][m_b_xp] 
                         - DCPrediction( in_data.DC( V_COMP ) , in_data.Mode() ); 
        const int abs_valV = std::abs( valV ); 

        for (ValueType bin = 1; bin<=abs_valV ; ++bin)
            EncodeSymbol( 0 , ChooseVDCContext( in_data , bin ) ); 

        EncodeSymbol( 1 , ChooseVDCContext( in_data , abs_valV + 1 ) ); 
        
        if (valV != 0)
            EncodeSymbol( ( (valV > 0)? 1 : 0) , ChooseVDCSignContext( in_data ) ); 
    }
}

//decoding functions//
//////////////////////

void MvDataCodec::DoWorkDecode( MvData& out_data)
{
    int step,max; 
    int pstep,pmax;     
    int split_depth; 
    bool common_ref; 
    int xstart,ystart;     

    for (m_mb_yp = 0,m_mb_tlb_y = 0; m_mb_yp < out_data.MBSplit().LengthY(); ++m_mb_yp,m_mb_tlb_y += 4)
    {
        for (m_mb_xp = 0,m_mb_tlb_x = 0; m_mb_xp < out_data.MBSplit().LengthX(); ++m_mb_xp,m_mb_tlb_x += 4)
        {
             //start with split mode
            DecodeMBSplit( out_data ); 
            split_depth = out_data.MBSplit()[m_mb_yp][m_mb_xp]; 
            step =  4  >>  (split_depth); 
            max  = (1 << split_depth); 

            //next do common_ref
            if(split_depth  !=  0)
            {
                DecodeMBCom( out_data ); 
                pstep = step; 
                pmax = max; 
            }
            else
            {
                out_data.MBCommonMode()[m_mb_yp][m_mb_xp] = true; 
                pstep = 4; 
                pmax = 1; 
            }
            
            common_ref = out_data.MBCommonMode()[m_mb_yp][m_mb_xp]; 

            // do prediction modes
            for (m_b_yp = m_mb_tlb_y;  m_b_yp < m_mb_tlb_y + 4;  m_b_yp += pstep)
            {                
                for (m_b_xp = m_mb_tlb_x; m_b_xp < m_mb_tlb_x + 4;  m_b_xp += pstep)
                {
                    DecodePredmode(out_data); 
                    
                    // propagate throughout MB                
                    for (int y = m_b_yp;  y < m_b_yp + pstep;  y++)
                        for (int x = m_b_xp;  x < m_b_xp + pstep;  x++)
                            out_data.Mode()[y][x] = out_data.Mode()[m_b_yp][m_b_xp];                                                         
                }
            }

               //now do all the block mvs in the mb
            for (int j = 0; j < max; ++j)
            {                
                for (int i = 0; i < max; ++i)
                {
                    xstart = m_b_xp = m_mb_tlb_x + i * step; 
                    ystart = m_b_yp = m_mb_tlb_y + j * step;                                             
                    
                    if (out_data.Mode()[m_b_yp][m_b_xp] == REF1_ONLY || out_data.Mode()[m_b_yp][m_b_xp] == REF1AND2 )
                        DecodeMv1( out_data ); 
                    
                    if (out_data.Mode()[m_b_yp][m_b_xp] == REF2_ONLY || out_data.Mode()[m_b_yp][m_b_xp] == REF1AND2 )
                        DecodeMv2( out_data ); 
                    
                    if(out_data.Mode()[m_b_yp][m_b_xp] == INTRA)
                        DecodeDC( out_data ); 
                    
                      //propagate throughout MB    
                    for (m_b_yp = ystart; m_b_yp < ystart+step; m_b_yp++)
                    {
                        for (m_b_xp = xstart; m_b_xp < xstart+step; m_b_xp++)
                        {                    
                            out_data.Vectors(1)[m_b_yp][m_b_xp].x = out_data.Vectors(1)[ystart][xstart].x; 
                            out_data.Vectors(1)[m_b_yp][m_b_xp].y = out_data.Vectors(1)[ystart][xstart].y; 
                            out_data.Vectors(2)[m_b_yp][m_b_xp].x = out_data.Vectors(2)[ystart][xstart].x; 
                            out_data.Vectors(2)[m_b_yp][m_b_xp].y = out_data.Vectors(2)[ystart][xstart].y; 
                            out_data.DC( Y_COMP )[m_b_yp][m_b_xp] = out_data.DC( Y_COMP )[ystart][xstart]; 
                            out_data.DC( U_COMP )[m_b_yp][m_b_xp] = out_data.DC( U_COMP )[ystart][xstart]; 
                            out_data.DC( V_COMP )[m_b_yp][m_b_xp] = out_data.DC( V_COMP )[ystart][xstart]; 
                        }//m_b_xp
                    }//m_b_yp
                }//i                    
            }//j

            m_MB_count++;
    
            if (m_MB_count > m_reset_num)
            {
                 m_MB_count = 0;
                 ResetAll();
            }

        }//m_mb_xp
    }//m_mb_yp

}

void MvDataCodec::DecodeMBSplit(MvData& out_data)
{
    int val = 0; 
    int bin = 1; 
    bool bit; 

    do
    {
        bit = DecodeSymbol( ChooseMBSContext( out_data , bin ) ); 
        
        if ( !bit )
            val++; 
        
        bin++; 
    }
    while (!bit && val != 2);  
    
    out_data.MBSplit()[m_mb_yp][m_mb_xp] = ( val + MBSplitPrediction( out_data.MBSplit() ) ) % 3;     
}

void MvDataCodec::DecodeMBCom( MvData& out_data )
{    
    if ( DecodeSymbol( ChooseMBCContext( out_data ) ) )
        out_data.MBCommonMode()[m_mb_yp][m_mb_xp] = !MBCBModePrediction( out_data.MBCommonMode() ); 
    else
        out_data.MBCommonMode()[m_mb_yp][m_mb_xp] = MBCBModePrediction( out_data.MBCommonMode() ); 
}

void MvDataCodec::DecodePredmode( MvData& out_data )
{
    // Xor with the prediction so we predict whether REF1 is used or REF2 is
    // used, separately
    unsigned int residue;
    
    // Decode REF1 part of the prediction residue (ie the first bit)
    bool bit;
    bit = DecodeSymbol( PMODE_BIT0_CTX );
    residue = (unsigned int) bit;

    // Decode REF2 part of the prediction residue (ie the second bit)
    bit = DecodeSymbol( PMODE_BIT1_CTX );
    residue |= ( (unsigned int) bit ) << 1;
    
    out_data.Mode()[m_b_yp][m_b_xp] = PredMode( BlockModePrediction (out_data.Mode() ) ^ residue ); 
}

void MvDataCodec::DecodeMv1( MvData& out_data )
{
    MVector pred = Mv1Prediction( out_data.Vectors(1) , out_data.Mode() );     
    int val = 0;
    int bin = 1; 
    bool bit; 
    
    do
    {
        bit = DecodeSymbol( ChooseREF1xContext( out_data , bin ) ); 
        
        if ( !bit )
            val++; 
        
        bin++; 
    }
    while ( !bit ); 
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF1xSignContext( out_data ) ) )
            val = -val; 
    }
    
    out_data.Vectors(1)[m_b_yp][m_b_xp].x = val + pred.x; 

    val = 0; 
    bin = 1; 
    
    do
    {
        bit = DecodeSymbol( ChooseREF1yContext( out_data , bin ) ); 
        
        if ( !bit )
            val++; 
        
        bin++; 
    }
    while ( !bit ); 
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF1ySignContext( out_data ) )  )
            val = -val; 
    }
    
    out_data.Vectors(1)[m_b_yp][m_b_xp].y = val + pred.y; 
}

void MvDataCodec::DecodeMv2( MvData& out_data )
{
    MVector pred = Mv2Prediction( out_data.Vectors(2) , out_data.Mode() ); 
    int val = 0; 
    int bin = 1; 
    bool bit; 
    
    do
    {
        bit = DecodeSymbol( ChooseREF2xContext( out_data , bin ) ); 
        
        if ( !bit )
            val++; 
        
        bin++; 
    }
    while ( !bit ); 
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF2xSignContext( out_data ) ) )
            val = -val; 
    }
    
    out_data.Vectors(2)[m_b_yp][m_b_xp].x = val + pred.x; 

    val = 0; 
    bin = 1; 
    
    do
    {
        bit = DecodeSymbol( ChooseREF2yContext( out_data , bin ) ); 
        
        if ( !bit )
            val++; 
        
        bin++; 
    }
    while ( !bit ); 
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF2ySignContext( out_data ) )  )
            val = -val; 
    }
    
    out_data.Vectors(2)[m_b_yp][m_b_xp].y = val + pred.y; 
}

void MvDataCodec::DecodeDC( MvData& out_data )
{
    //begin with Y DC value    
    ValueType val = 0; 
    int bin = 1; 
    bool bit; 
    
    do
    {
        bit = DecodeSymbol( ChooseYDCContext( out_data , bin ) ); 
        
        if ( !bit )
            val++; 
        
        bin++; 
    }
    while ( !bit ); 
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseYDCSignContext( out_data ) )  )
            val = -val; 
    }
    
    out_data.DC( Y_COMP )[m_b_yp][m_b_xp] = val + DCPrediction( out_data.DC( Y_COMP ) , out_data.Mode()); 

    if (m_cformat != Yonly)
    {
        //move onto U and V DC values
        val = 0; 
        bin = 1; 
        
        do
        {
            bit = DecodeSymbol( ChooseUDCContext( out_data , bin ) ); 
            
            if (!bit)
                val++; 
            
            bin++; 
        }
        while (!bit); 
        
        if (val != 0)
        {
            if ( !DecodeSymbol( ChooseUDCSignContext ( out_data ) ) )
                val = -val; 
        }
        
        out_data.DC( U_COMP )[m_b_yp][m_b_xp] = val + DCPrediction( out_data.DC( U_COMP ) , out_data.Mode()); 

        val = 0; 
        bin = 1; 
        
        do
        {
            bit = DecodeSymbol( ChooseVDCContext( out_data , bin ) ); 
            
            if ( !bit )
                val++; 
            
            bin++; 
        }
        while ( !bit ); 
        
        if (val != 0)
        {
            if ( !DecodeSymbol( ChooseVDCSignContext( out_data ) )  )
                val = -val; 
        }
        
        out_data.DC( V_COMP )[m_b_yp][m_b_xp] = val + DCPrediction( out_data.DC( V_COMP ) , out_data.Mode() ); 
    }
}
