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
* Contributor(s): Thomas Davies (Original Author),
*                 Scott R Ladd,
*                 Tim Borer,
*                 Andrew Kennedy,
*                 Anuradha Suraparaju
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
// Constructor 
MvDataCodec::MvDataCodec(ByteIO* p_byteio,
                         size_t number_of_contexts,
                         const ChromaFormat& cf)
  : ArithCodec <MvData> (p_byteio,number_of_contexts),
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
inline int MvDataCodec::ChooseREF1xFollowContext(const int bin_number) const
{
    switch ( bin_number )
    {
        case 1 :
            return REF1x_FBIN1_CTX; 
        case 2 :
            return REF1x_FBIN2_CTX; 
        case 3 :
            return REF1x_FBIN3_CTX; 
        case 4 :
            return REF1x_FBIN4_CTX; 
        default :
            return REF1x_FBIN5plus_CTX; 

    }
}

inline int MvDataCodec::ChooseREF1xInfoContext() const
{
    return REF1x_INFO_CTX;
}

inline int MvDataCodec::ChooseREF1xSignContext() const
{
    return REF1x_SIGN_CTX;
}

inline int MvDataCodec::ChooseREF1yFollowContext(const int bin_number) const
{
    switch ( bin_number )
    {
        case 1 :
            return REF1y_FBIN1_CTX; 
        case 2 :
            return REF1y_FBIN2_CTX; 
        case 3 :
            return REF1y_FBIN3_CTX; 
        case 4 :
            return REF1y_FBIN4_CTX; 
        default :
            return REF1y_FBIN5plus_CTX; 

    }
}

inline int MvDataCodec::ChooseREF1yInfoContext() const
{
    return REF1y_INFO_CTX;
}

inline int MvDataCodec::ChooseREF1ySignContext() const
{
    return REF1y_SIGN_CTX;
}

inline int MvDataCodec::ChooseREF2xFollowContext(const int bin_number) const
{
    switch ( bin_number )
    {
        case 1 :
            return REF2x_FBIN1_CTX; 
        case 2 :
            return REF2x_FBIN2_CTX; 
        case 3 :
            return REF2x_FBIN3_CTX; 
        case 4 :
            return REF2x_FBIN4_CTX; 
        default :
            return REF2x_FBIN5plus_CTX; 

    }
}

inline int MvDataCodec::ChooseREF2xInfoContext() const
{
    return REF2x_INFO_CTX; 
}

inline int MvDataCodec::ChooseREF2xSignContext() const
{
    return REF2x_SIGN_CTX; 
}

inline int MvDataCodec::ChooseREF2yFollowContext(const int bin_number) const
{
    switch ( bin_number )
    {
        case 1 :
            return REF2y_FBIN1_CTX; 
        case 2 :
            return REF2y_FBIN2_CTX; 
        case 3 :
            return REF2y_FBIN3_CTX; 
        case 4 :
            return REF2y_FBIN4_CTX; 
        default :
            return REF2y_FBIN5plus_CTX; 

    }
}

inline int MvDataCodec::ChooseREF2yInfoContext() const
{
    return REF2y_INFO_CTX; 
}

inline int MvDataCodec::ChooseREF2ySignContext() const
{
    return REF2y_SIGN_CTX; 
}

inline int MvDataCodec::ChooseYDCFollowContext(const int bin_number) const
{
    if (bin_number == 1)
        return YDC_FBIN1_CTX; 
    else
        return YDC_FBIN2plus_CTX; 
}

inline int MvDataCodec::ChooseYDCInfoContext() const
{
    return YDC_INFO_CTX; 
}

inline int MvDataCodec::ChooseUDCFollowContext(const int bin_number) const
{
    if (bin_number == 1)
        return UDC_FBIN1_CTX; 
    else
        return UDC_FBIN2plus_CTX; 
}

inline int MvDataCodec::ChooseUDCInfoContext() const
{
    return UDC_INFO_CTX; 
}

inline int MvDataCodec::ChooseVDCFollowContext(const int bin_number) const
{
    if (bin_number == 1)
        return VDC_FBIN1_CTX; 
    else
        return VDC_FBIN2plus_CTX; 
}

inline int MvDataCodec::ChooseVDCInfoContext() const
{
    return VDC_INFO_CTX; 
}

inline int MvDataCodec::ChooseYDCSignContext() const
{
    return YDC_SIGN_CTX; 
}

inline int MvDataCodec::ChooseUDCSignContext() const
{
    return UDC_SIGN_CTX; 
}

inline int MvDataCodec::ChooseVDCSignContext() const
{
    return VDC_SIGN_CTX; 
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
        unsigned int sum = (unsigned int)( cbm_data[m_mb_yp-1][m_mb_xp] ); 
        sum += (unsigned int)( cbm_data[m_mb_yp-1][m_mb_xp-1] ); 
        sum += (unsigned int)( cbm_data[m_mb_yp][m_mb_xp-1] ); 

        result = (sum>1);     
    }
    else if (m_mb_xp > 0 && m_mb_yp == 0)
        result = cbm_data[0][m_mb_xp-1]; 
    else if (m_mb_xp == 0 && m_mb_yp > 0)
        result = cbm_data[m_mb_yp-1][0]; 
 
    return result; 
}

inline unsigned int MvDataCodec::BlockModePrediction(const TwoDArray < PredMode > & preddata,
                                                     const unsigned int num_refs) const
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

        if ( num_refs==2)
        {
            num_ref2_nbrs += ((unsigned int)( preddata[m_b_yp-1][m_b_xp] ) ) & 2; 
            num_ref2_nbrs += ((unsigned int)( preddata[m_b_yp-1][m_b_xp-1] ) ) & 2; 
            num_ref2_nbrs += ((unsigned int)( preddata[m_b_yp][m_b_xp-1] ) ) & 2; 
            num_ref2_nbrs >>= 1;
            result ^= ( (num_ref2_nbrs>>1)<<1 );
        }
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
    std::vector < unsigned int >  nbrs; 
    PredMode pmode;
    ValueType result = 128; 
    
    if (m_b_xp > 0 && m_b_yp > 0)
    {
        pmode = preddata[m_b_yp-1][m_b_xp]; 
        if (pmode == INTRA) 
            nbrs.push_back( (unsigned int) dcdata[m_b_yp-1][m_b_xp] ); 
        
        pmode = preddata[m_b_yp-1][m_b_xp-1]; 
        if (pmode == INTRA)
            nbrs.push_back((unsigned int)dcdata[m_b_yp-1][m_b_xp-1] ); 
        
        pmode = preddata[m_b_yp][m_b_xp-1]; 
        if (pmode == INTRA)        
            nbrs.push_back( (unsigned int) dcdata[m_b_yp][m_b_xp-1] ); 
        
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
    int pstep,pmax,mode_step; 
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

            if (common_ref)
                mode_step = 4;
            else
                mode_step = pstep;
      
            for (m_b_yp = m_mb_tlb_y; m_b_yp < m_mb_tlb_y+4; m_b_yp += mode_step)
                for (m_b_xp = m_mb_tlb_x; m_b_xp < m_mb_tlb_x+4; m_b_xp += mode_step)
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
    
            if (m_MB_count == m_reset_num)
            {
                 m_MB_count = 0;
                 ResetAll();
            }
            
        }//m_mb_xp
    }//m_mb_yp

}

void MvDataCodec::CodeMBSplit(const MvData& in_data)
{
    const int prediction = MBSplitPrediction( in_data.MBSplit() );
    const bool bit0 = (in_data.MBSplit()[m_mb_yp][m_mb_xp]>0)
                     ^( prediction>0 );
    const bool bit1 = (in_data.MBSplit()[m_mb_yp][m_mb_xp]>1)
                     ^( prediction>1 );

    EncodeSymbol( bit0,MB_SPLIT_BIT0_CTX);

    if ( in_data.MBSplit()[m_mb_yp][m_mb_xp]>0 )
        EncodeSymbol( bit1,MB_SPLIT_BIT1_CTX);    

}

void MvDataCodec::CodeMBCom(const MvData& in_data)
{
    bool val = in_data.MBCommonMode()[m_mb_yp][m_mb_xp]; 

    EncodeSymbol( val ^ int(MBCBModePrediction( in_data.MBCommonMode() )),
                  MB_CMODE_CTX ); 
}

void MvDataCodec::CodePredmode(const MvData& in_data)
{
    // Xor with the prediction so we predict whether REF1 is used or REF2 is
    // used, separately
    unsigned int residue = in_data.Mode()[m_b_yp][m_b_xp] ^ 
        BlockModePrediction( in_data.Mode(), in_data.NumRefs() ); 
    
    // Code REF1 part of the prediction residue (ie the first bit)
    EncodeSymbol( residue & 1 , PMODE_BIT0_CTX );

    // Code REF2 part of the prediction residue (ie the second bit)
    if (in_data.NumRefs()==2)
        EncodeSymbol( residue & 2 , PMODE_BIT1_CTX );

}

/*
Motion vector magnitude and DC values are coded using interleaved exp-Golomb 
coding for binarisation. In this scheme, a value N>=0 is coded by 
writing N+1 in binary form of a 1 followed by K other bits: 1bbbbbbb 
(adding 1 ensures there'll be a leading 1). These K bits ("info bits") 
are interleaved with K zeroes ("follow bits") each of which means 
"another bit coming", followed by a terminating 1:
 
    0b0b0b ...0b1
 
(Conventional exp-Golomb coding has the K zeroes at the beginning, followed
by the 1 i.e 00...01bb .. b, but interleaving allows the decoder to run a
single loop and avoid counting the number of zeroes, sparing a register.)

All bits are arithmetically coded. The follow bits have separate contexts
based on position, and have different contexts from the info bits. 
*/

void MvDataCodec::CodeMv1(const MvData& in_data )
{
    const MvArray& mv_array = in_data.Vectors(1);
    const MVector pred = Mv1Prediction( mv_array , in_data.Mode() ); 

    const int valx = mv_array[m_b_yp][m_b_xp].x - pred.x; 
    const int abs_valx = std::abs(valx); 

    int N = abs_valx+1;
    int num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseREF1xFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseREF1xInfoContext() );
    }
    EncodeSymbol( 1, ChooseREF1xFollowContext( num_follow_zeroes+1 ) );

    if ( valx )
    {
        EncodeSymbol( ( (valx > 0)? 1 : 0) , ChooseREF1xSignContext() ); 
    }


    const int valy = mv_array[m_b_yp][m_b_xp].y - pred.y;         
    const int abs_valy = std::abs( valy );     
    
    N = abs_valy+1;
    num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseREF1yFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseREF1yInfoContext() );
    }
    EncodeSymbol( 1, ChooseREF1yFollowContext( num_follow_zeroes+1 ) );
    
    if (valy != 0)
    {
        EncodeSymbol( ( (valy > 0)? 1 : 0) , ChooseREF1ySignContext() );  
    }
}

void MvDataCodec::CodeMv2(const MvData& in_data)
{
    const MvArray& mv_array = in_data.Vectors(2);
    const MVector pred = Mv2Prediction( mv_array , in_data.Mode() ); 

    const int valx = mv_array[m_b_yp][m_b_xp].x - pred.x; 
    const int abs_valx = std::abs(valx); 

    int N = abs_valx+1;
    int num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseREF2xFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseREF2xInfoContext() );
    }
    EncodeSymbol( 1, ChooseREF2xFollowContext( num_follow_zeroes+1 ) );

    if ( valx )
    {
        EncodeSymbol( ( (valx > 0)? 1 : 0) , ChooseREF2xSignContext() ); 
    }


    const int valy = mv_array[m_b_yp][m_b_xp].y - pred.y;         
    const int abs_valy = std::abs( valy );     
    
    N = abs_valy+1;
    num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseREF2yFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseREF2yInfoContext() );
    }
    EncodeSymbol( 1, ChooseREF2yFollowContext( num_follow_zeroes+1 ) );
    
    if (valy != 0)
    {
        EncodeSymbol( ( (valy > 0)? 1 : 0) , ChooseREF2ySignContext() ); 
    }
}

void MvDataCodec::CodeDC(const MvData& in_data)
{    
    //begin with Y DC value    
    const ValueType valY = in_data.DC( Y_COMP )[m_b_yp][m_b_xp]
                           - DCPrediction( in_data.DC(Y_COMP) , in_data.Mode() ); 
    const ValueType abs_valY = std::abs( valY ); 

    int N = abs_valY+1;
    int num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseYDCFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseYDCInfoContext() );
    }
    EncodeSymbol( 1, ChooseYDCFollowContext( num_follow_zeroes+1 ) );
    
    if (valY != 0)
    {
        EncodeSymbol( ( (valY > 0)? 1 : 0) , ChooseYDCSignContext() ); 
    }

    //continue with U and V DC values
    const int valU =  in_data.DC(U_COMP)[m_b_yp][m_b_xp]
                      - DCPrediction(in_data.DC( U_COMP ) , in_data.Mode()); 
    const int abs_valU = std::abs( valU ); 

    N = abs_valU+1;
    num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseUDCFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseUDCInfoContext() );
    }
    EncodeSymbol( 1, ChooseUDCFollowContext( num_follow_zeroes+1 ) );
    
    if (valU != 0)
    {
        EncodeSymbol( ( (valU > 0) ? 1 : 0) , ChooseUDCSignContext() ); 
    }
    const int valV = in_data.DC( V_COMP )[m_b_yp][m_b_xp] 
                     - DCPrediction( in_data.DC( V_COMP ) , in_data.Mode() ); 
    const int abs_valV = std::abs( valV ); 

    N = abs_valV+1;
    num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseVDCFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseVDCInfoContext() );
    }
    EncodeSymbol( 1, ChooseVDCFollowContext( num_follow_zeroes+1 ) );
    
    if (valV != 0)
    {
        EncodeSymbol( ( (valV > 0)? 1 : 0) , ChooseVDCSignContext() ); 
    }
}

//decoding functions//
//////////////////////

void MvDataCodec::DoWorkDecode( MvData& out_data)
{
    int step,max; 
    int pstep,pmax,mode_step;     
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
     
            if (common_ref)
                mode_step = 4;
            else
                mode_step = pstep;

            // do prediction modes
            for (m_b_yp = m_mb_tlb_y;  m_b_yp < m_mb_tlb_y + 4;  m_b_yp += mode_step)
            {                
                for (m_b_xp = m_mb_tlb_x; m_b_xp < m_mb_tlb_x + 4;  m_b_xp += mode_step)
                {
                    DecodePredmode(out_data); 
                    
                    // propagate throughout prediction unit/MB
                    for (int y = m_b_yp;  y < m_b_yp + mode_step;  y++)
                        for (int x = m_b_xp;  x < m_b_xp + mode_step;  x++)
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

                            if (out_data.NumRefs()==2)
                            {
                                out_data.Vectors(2)[m_b_yp][m_b_xp].x = out_data.Vectors(2)[ystart][xstart].x; 
                                out_data.Vectors(2)[m_b_yp][m_b_xp].y = out_data.Vectors(2)[ystart][xstart].y; 
                            }
                            out_data.DC( Y_COMP )[m_b_yp][m_b_xp] = out_data.DC( Y_COMP )[ystart][xstart]; 
                            out_data.DC( U_COMP )[m_b_yp][m_b_xp] = out_data.DC( U_COMP )[ystart][xstart]; 
                            out_data.DC( V_COMP )[m_b_yp][m_b_xp] = out_data.DC( V_COMP )[ystart][xstart]; 
                        }//m_b_xp
                    }//m_b_yp
                }//i                    
            }//j

            m_MB_count++;
    
            if (m_MB_count == m_reset_num)
            {
                 m_MB_count = 0;
                 ResetAll();
            }

        }//m_mb_xp
    }//m_mb_yp

}

void MvDataCodec::DecodeMBSplit(MvData& out_data)
{
    const int prediction = MBSplitPrediction( out_data.MBSplit() );
    out_data.MBSplit()[m_mb_yp][m_mb_xp] = int( DecodeSymbol( MB_SPLIT_BIT0_CTX)
                                               ^( prediction>0 ) );

    if ( out_data.MBSplit()[m_mb_yp][m_mb_xp]>0 )
    {
        out_data.MBSplit()[m_mb_yp][m_mb_xp] += int( DecodeSymbol( MB_SPLIT_BIT1_CTX)
                                                   ^( prediction>1 ) );
    }

}

void MvDataCodec::DecodeMBCom( MvData& out_data )
{    
    out_data.MBCommonMode()[m_mb_yp][m_mb_xp] = DecodeSymbol( MB_CMODE_CTX ) ^ 
                                                int( MBCBModePrediction( out_data.MBCommonMode() ) );
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
    if (out_data.NumRefs()==2)
    {
        bit = DecodeSymbol( PMODE_BIT1_CTX );
        residue |= ( (unsigned int) bit ) << 1;
    }
    
    out_data.Mode()[m_b_yp][m_b_xp] = 
        PredMode( BlockModePrediction (out_data.Mode() , out_data.NumRefs()) ^ residue ); 
}

/*
Motion vector magnitude values are coded using interleaved exp-Golomb 
coding for binarisation. In this scheme, a value N>=0 is coded by 
writing N+1 in binary form of a 1 followed by K other bits: 1bbbbbbb 
(adding 1 ensures there'll be a leading 1). These K bits ("info bits") 
are interleaved with K zeroes ("follow bits") each of which means 
"another bit coming", followed by a terminating 1:
 
    0b0b0b ...0b1
 
(Conventional exp-Golomb coding has the K zeroes at the beginning, followed
by the 1 i.e 00...01bb .. b, but interleaving allows the decoder to run a
single loop and avoid counting the number of zeroes, sparing a register.)

All bits are arithmetically coded. The follow bits have separate contexts
based on position, and have different contexts from the info bits. 
*/

void MvDataCodec::DecodeMv1( MvData& out_data )
{
    MVector pred = Mv1Prediction( out_data.Vectors(1) , out_data.Mode() );     

    int val = 1;
    int bit_count=1;

    while ( !DecodeSymbol( ChooseREF1xFollowContext( bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseREF1xInfoContext() );
        bit_count++;
    };
    --val;
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF1xSignContext() ) )
            val = -val; 
    }
    
    out_data.Vectors(1)[m_b_yp][m_b_xp].x = val + pred.x; 

    val = 1;
    bit_count=1;

    while ( !DecodeSymbol( ChooseREF1yFollowContext( bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseREF1yInfoContext() );
        bit_count++;
    };
    --val;    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF1ySignContext() )  )
            val = -val; 
    }
    
    out_data.Vectors(1)[m_b_yp][m_b_xp].y = val + pred.y; 
}

void MvDataCodec::DecodeMv2( MvData& out_data )
{
    MVector pred = Mv2Prediction( out_data.Vectors(2) , out_data.Mode() ); 
    
    int val = 1;
    int bit_count=1;

    while ( !DecodeSymbol( ChooseREF2xFollowContext(bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseREF2xInfoContext() );
        bit_count++;
    };
    --val;
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF2xSignContext() ) )
            val = -val; 
    }
    
    out_data.Vectors(2)[m_b_yp][m_b_xp].x = val + pred.x; 

    val = 1;
    bit_count=1;

    while ( !DecodeSymbol( ChooseREF2yFollowContext( bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseREF2yInfoContext() );
        bit_count++;
    };
    --val;
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseREF2ySignContext() )  )
            val = -val; 
    }
    
    out_data.Vectors(2)[m_b_yp][m_b_xp].y = val + pred.y; 
}

void MvDataCodec::DecodeDC( MvData& out_data )
{
    //begin with Y DC value    
    ValueType val = 1;
    int bit_count=1;

    while ( !DecodeSymbol( ChooseYDCFollowContext( bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseYDCInfoContext() );
        bit_count++;
    };
    --val;    

    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseYDCSignContext() )  )
            val = -val; 
    }
    
    out_data.DC( Y_COMP )[m_b_yp][m_b_xp] = val + DCPrediction( out_data.DC( Y_COMP ) , out_data.Mode()); 

    //move onto U and V DC values
    val = 1;
    bit_count=1;

    while ( !DecodeSymbol( ChooseUDCFollowContext( bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseUDCInfoContext() );
        bit_count++;
    };
    --val;
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseUDCSignContext () ) )
            val = -val; 
    }
    
    out_data.DC( U_COMP )[m_b_yp][m_b_xp] = val + DCPrediction( out_data.DC( U_COMP ) , out_data.Mode()); 
    
    val = 1;
    bit_count=1;

    while ( !DecodeSymbol( ChooseVDCFollowContext(bit_count ) ) )
    {
        val <<= 1;
        val |= DecodeSymbol( ChooseVDCInfoContext() );
        bit_count++;
    };
    --val;    
    
    if (val != 0)
    {
        if ( !DecodeSymbol( ChooseVDCSignContext() )  )
            val = -val; 
    }
    
    out_data.DC( V_COMP )[m_b_yp][m_b_xp] = val + DCPrediction( out_data.DC( V_COMP ) , out_data.Mode() ); 
}
