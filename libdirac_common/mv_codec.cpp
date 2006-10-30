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

inline unsigned int MvDataCodec::MBSplitPrediction(const TwoDArray<int> & split_data ) const
{    
    int result = 0;
    
    std::vector < unsigned int >  nbrs;
    
    if (m_mb_xp > 0 && m_mb_yp > 0)
    {
        nbrs.push_back( split_data[m_mb_yp-1][m_mb_xp] ); 
        nbrs.push_back( split_data[m_mb_yp-1][m_mb_xp-1] ); 
        nbrs.push_back( split_data[m_mb_yp][m_mb_xp-1] ); 

        result = GetUMean(nbrs);     
    }
    else if (m_mb_xp > 0 && m_mb_yp == 0)
        result = split_data[m_mb_yp][m_mb_xp-1]; 
    else if (m_mb_xp == 0 && m_mb_yp > 0)
        result =  split_data[m_mb_yp-1][m_mb_xp]; 

    return result; 
}

inline unsigned int MvDataCodec::BlockModePrediction(const TwoDArray < PredMode > & preddata,
                                                     const unsigned int num_refs) const
{
#if 0
    // software
    unsigned int result = (unsigned int)(REF1_ONLY);
#else
    // spec
    unsigned int result = (unsigned int)(INTRA);
#endif
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
            result = ValueType(GetUMean(nbrs));     
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
                        
            //now do all the block modes and mvs in the mb            
            for (m_b_yp = m_mb_tlb_y; m_b_yp < m_mb_tlb_y+4; m_b_yp += step)
            {
                for (m_b_xp = m_mb_tlb_x; m_b_xp < m_mb_tlb_x+4; m_b_xp += step)
                {
                    CodePredmode(in_data); 

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
    int val = in_data.MBSplit()[m_mb_yp][m_mb_xp] - MBSplitPrediction( in_data.MBSplit() ); 
    
    if (val < 0) val+=3; //produce prediction mod 3

    EncodeUInt(val, MB_SPLIT_BIN1_CTX, MB_SPLIT_BIN2_CTX);
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
    EncodeSInt(valx, REF1x_FBIN1_CTX, REF1x_FBIN5plus_CTX);

    const int valy = mv_array[m_b_yp][m_b_xp].y - pred.y;
    EncodeSInt(valy, REF1y_FBIN1_CTX, REF1y_FBIN5plus_CTX);
}

void MvDataCodec::CodeMv2(const MvData& in_data )
{
    const MvArray& mv_array = in_data.Vectors(2);
    const MVector pred = Mv2Prediction( mv_array , in_data.Mode() ); 
    const int valx = mv_array[m_b_yp][m_b_xp].x - pred.x;
    EncodeSInt(valx, REF2x_FBIN1_CTX, REF2x_FBIN5plus_CTX);

    const int valy = mv_array[m_b_yp][m_b_xp].y - pred.y;
    EncodeSInt(valy, REF2y_FBIN1_CTX, REF2y_FBIN5plus_CTX);
}
void MvDataCodec::CodeDC(const MvData& in_data)
{    
    //begin with Y DC value
    const int valY = in_data.DC( Y_COMP )[m_b_yp][m_b_xp] -
                         DCPrediction( in_data.DC(Y_COMP) , in_data.Mode() );
    EncodeSInt(valY, YDC_FBIN1_CTX, YDC_FBIN2plus_CTX);

    //continue with U and V DC values
    const int valU =  in_data.DC(U_COMP)[m_b_yp][m_b_xp]
                      - DCPrediction(in_data.DC( U_COMP ) , in_data.Mode());
    EncodeSInt(valU, UDC_FBIN1_CTX, UDC_FBIN2plus_CTX);

    const int valV = in_data.DC( V_COMP )[m_b_yp][m_b_xp] 
                     - DCPrediction( in_data.DC( V_COMP ) , in_data.Mode() );
    EncodeSInt(valV, VDC_FBIN1_CTX, VDC_FBIN2plus_CTX);
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

            //now do all the block mvs in the mb
            for (int j = 0; j < max; ++j)
            {                
                for (int i = 0; i < max; ++i)
                {
                    xstart = m_b_xp = m_mb_tlb_x + i * step; 
                    ystart = m_b_yp = m_mb_tlb_y + j * step;                                             
                    
                    DecodePredmode(out_data); 

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
                            out_data.Mode()[m_b_yp][m_b_xp] = out_data.Mode()[ystart][xstart]; 

                            out_data.Vectors(1)[m_b_yp][m_b_xp].y = out_data.Vectors(1)[ystart][xstart].y; 
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
    out_data.MBSplit()[m_mb_yp][m_mb_xp] =
        (DecodeUInt(MB_SPLIT_BIN1_CTX, MB_SPLIT_BIN2_CTX) +
            MBSplitPrediction(out_data.MBSplit())) % 3;
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
    out_data.Vectors(1)[m_b_yp][m_b_xp].x =
        DecodeSInt(REF1x_FBIN1_CTX, REF1x_FBIN5plus_CTX) + pred.x;
    out_data.Vectors(1)[m_b_yp][m_b_xp].y =
        DecodeSInt(REF1y_FBIN1_CTX, REF1y_FBIN5plus_CTX) + pred.y; 
}

void MvDataCodec::DecodeMv2( MvData& out_data )
{
    MVector pred = Mv2Prediction( out_data.Vectors(2) , out_data.Mode() );
    out_data.Vectors(2)[m_b_yp][m_b_xp].x =
        DecodeSInt(REF2x_FBIN1_CTX, REF2x_FBIN5plus_CTX) + pred.x;
    out_data.Vectors(2)[m_b_yp][m_b_xp].y =
        DecodeSInt(REF2y_FBIN1_CTX, REF2y_FBIN5plus_CTX) + pred.y; 
}

void MvDataCodec::DecodeDC( MvData& out_data )
{
    //begin with Y DC value
    out_data.DC( Y_COMP )[m_b_yp][m_b_xp] = DecodeSInt(YDC_FBIN1_CTX, YDC_FBIN2plus_CTX) +
        DCPrediction(out_data.DC( Y_COMP ), out_data.Mode());

    //move onto U and V DC values
    out_data.DC( U_COMP )[m_b_yp][m_b_xp] = DecodeSInt(UDC_FBIN1_CTX, UDC_FBIN2plus_CTX) +
        DCPrediction( out_data.DC( U_COMP ) , out_data.Mode()); 

    out_data.DC( V_COMP )[m_b_yp][m_b_xp] = DecodeSInt(VDC_FBIN1_CTX, VDC_FBIN2plus_CTX) +
        DCPrediction( out_data.DC( V_COMP ) , out_data.Mode() ); 
}
