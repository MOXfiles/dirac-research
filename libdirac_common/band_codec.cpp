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

#include <libdirac_common/band_codec.h>
#include <libdirac_byteio/subband_byteio.h>

using namespace dirac;

//! Constructor for encoding.
BandCodec::BandCodec(SubbandByteIO* subband_byteio,
                     size_t number_of_contexts,
                     const SubbandList & band_list,
                     int band_num):
    ArithCodec<PicArray>(subband_byteio,number_of_contexts),
    m_bnum(band_num),
    m_node(band_list(band_num)),
    m_vol(m_node.Xl()*m_node.Yl()),
    m_reset_coeff_num( std::max( 25 , std::min(m_vol/32,800) ) ),
    m_cut_off_point(m_node.Scale()>>1)
{
    if (m_node.Parent()!=0) 
        m_pnode=band_list(m_node.Parent());
}        



void BandCodec::InitContexts()
{
    //initialises the contexts. 
    //If _list does not already have values, then they're set to default values. 
    //This way, the constructor can override default initialisation.
    Context tmp_ctx;
    
    for (size_t i=0; i<m_context_list.size(); ++i)
    {
        if (i>=m_context_list.size())
            m_context_list.push_back(tmp_ctx);
        else
        {
            if (m_context_list[i].Weight()==0)
                m_context_list[i].SetCounts(1,1);
        }
    }
}

void BandCodec::ResetAll()
{
    for (unsigned int c = 0; c < m_context_list.size(); ++c)
        if (m_context_list[c].Weight()>16)
            m_context_list[c].HalveCounts();
}

//encoding function
void BandCodec::DoWorkCode(PicArray& in_data)
{

    const TwoDArray<CodeBlock>& block_list( m_node.GetCodeBlocks() );

    if (m_node.Parent() != 0)
    {
        m_pxp = m_pnode.Xp();
        m_pyp = m_pnode.Yp();
    }
    else
    {
        m_pxp = 0;
        m_pyp = 0;
    }

    // Now loop over the blocks and code
    m_coeff_count = 0;
    for (int j=block_list.FirstY() ; j<=block_list.LastY() ; ++j)
    {
        CodeBlock *block = block_list[j];
        for (int i=block_list.FirstX() ; i<=block_list.LastX() ; ++i)
        {
            EncodeSymbol(block[i].Skipped() , BLOCK_SKIP_CTX );
            if ( !block[i].Skipped() )
                CodeCoeffBlock( block[i] , in_data );
            else
                ClearBlock (block[i] , in_data);
        }// i
    }// j

}

void BandCodec::CodeCoeffBlock( const CodeBlock& code_block , PicArray& in_data )
{
    //main coding function, using binarisation

    const int xbeg = code_block.Xstart();
    const int ybeg = code_block.Ystart();
    const int xend = code_block.Xend();
    const int yend = code_block.Yend();
 
    int xpbeg;
    int ypbeg;

   if (m_node.Parent()!=0)
    {
        xpbeg = m_pnode.Xp() + ( ( code_block.Xstart()-m_node.Xp() )>>1 );
        ypbeg = m_pnode.Yp() + ( ( code_block.Ystart()-m_node.Yp() )>>1 );
    }
    else
    {
        xpbeg = 0; 
        ypbeg = 0;
    }
    
    const int qf_idx = code_block.QIndex();

    if ( m_node.UsingMultiQuants() )
    {
        CodeQIndexOffset( qf_idx-m_node.QIndex() );
    }

    m_qf = dirac_quantiser_lists.QuantFactor( qf_idx );
    m_qfinv =  dirac_quantiser_lists.InverseQuantFactor( qf_idx );
    m_offset =  dirac_quantiser_lists.QuantOffset( qf_idx );

    m_cut_off_point *= m_qf;

    for ( int ypos=ybeg , m_pypos=ypbeg; ypos<yend ;++ypos , m_pypos=(( ypos-ybeg )>>1)+ypbeg)
    {
        for ( int xpos=xbeg , m_pxpos=xpbeg ; xpos<xend ;++xpos , m_pxpos=((xpos-xbeg)>>1)+xpbeg)
        {

            if ( xpos==m_node.Xp() )
                m_nhood_sum = (ypos != m_node.Yp()) ? std::abs(in_data[ypos-1][xpos]) : 0;
            else
                m_nhood_sum = (ypos!=m_node.Yp()) ? 
                (std::abs(in_data[ypos-1][xpos]) + std::abs(in_data[ypos][xpos-1])) 
               : std::abs(in_data[ypos][xpos-1]);

            m_parent_notzero = static_cast<bool> ( in_data[m_pypos][m_pxpos] );

            CodeVal( in_data , xpos , ypos , in_data[ypos][xpos] );

        }// xpos
    }// ypos    

}

/*
Coefficient magnitude value and differential quantiser index magnitude are
coded using interleaved exp-Golomb coding for binarisation. In this scheme, a
value N>=0 is coded by writing N+1 in binary form of a 1 followed by K other
bits: 1bbbbbbb (adding 1 ensures there'll be a leading 1). These K bits ("info
bits") are interleaved with K zeroes ("follow bits") each of which means
"another bit coming", followed by a terminating 1:
 
    0b0b0b ...0b1
 
(Conventional exp-Golomb coding has the K zeroes at the beginning, followed
by the 1 i.e 00...01bb .. b, but interleaving allows the decoder to run a
single loop and avoid counting the number of zeroes, sparing a register.)

All bits are arithmetically coded. The follow bits have separate contexts
based on position, and have different contexts from the info bits. 
*/

inline void BandCodec::CodeVal( PicArray& in_data , 
                                const int xpos , 
                                const int ypos , 
                                const ValueType val )
{
    unsigned int abs_val( std::abs(val) );
    abs_val /= m_qf;

    const int N = abs_val+1;
    int num_follow_zeroes=0;

    while ( N >= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes; 

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0, ChooseFollowContext( c ) );
        EncodeSymbol( N&(1<<i), ChooseInfoContext() );
    }
    EncodeSymbol( 1, ChooseFollowContext( num_follow_zeroes+1 ) );

    in_data[ypos][xpos] = static_cast<ValueType>( abs_val );

    if ( abs_val )
    {
        // Must code sign bits and reconstruct
        in_data[ypos][xpos] *= m_qf;
        in_data[ypos][xpos] += m_offset;

        if ( val>0 )
        {
            EncodeSymbol( 1 , ChooseSignContext( in_data , xpos , ypos ) );
        }
        else
        {
            EncodeSymbol( 0 , ChooseSignContext( in_data , xpos , ypos ) );
            in_data[ypos][xpos]  = -in_data[ypos][xpos];
        }
    }

    m_coeff_count++;
    
    if (m_coeff_count == m_reset_coeff_num)
    {
        m_coeff_count=0;
        ResetAll();
    }
}

void BandCodec::CodeQIndexOffset( const int offset )
{

    const int abs_val = std::abs( offset );

    int N = abs_val+1;
    int num_follow_zeroes=0;

    while ( N>= (1<<num_follow_zeroes) )
        ++num_follow_zeroes;
    --num_follow_zeroes;

    for ( int i=num_follow_zeroes-1, c=1; i>=0; --i, ++c )
    {
        EncodeSymbol( 0 , Q_OFFSET_FOLLOW_CTX );
        EncodeSymbol( N&(1<<i), Q_OFFSET_INFO_CTX );
    }
    EncodeSymbol( 1 , Q_OFFSET_FOLLOW_CTX );

    if ( abs_val )
    {
        if ( offset>0 )
            EncodeSymbol( 1 , Q_OFFSET_SIGN_CTX );
        else
            EncodeSymbol( 0 , Q_OFFSET_SIGN_CTX );
    }
}

void BandCodec::DoWorkDecode( PicArray& out_data )
{
    if (m_node.Parent() != 0)
    {
        m_pxp = m_pnode.Xp();
        m_pyp = m_pnode.Yp();
    }
    else
    {
        m_pxp = 0;
        m_pyp = 0;
    }

    const TwoDArray<CodeBlock>& block_list( m_node.GetCodeBlocks() );

    // Now loop over the blocks and decode
    m_coeff_count = 0;
    for (int j=block_list.FirstY() ; j<=block_list.LastY() ; ++j)
    {
        CodeBlock *block = block_list[j];
        for (int i=block_list.FirstX() ; i<=block_list.LastX() ; ++i)
        {
            block[i].SetSkip( DecodeSymbol( BLOCK_SKIP_CTX ) );
            if ( !block[i].Skipped() )
                DecodeCoeffBlock( block[i] , out_data );
            else
                ClearBlock (block[i] , out_data);

        }// i
    }// j

}

void BandCodec::DecodeCoeffBlock( const CodeBlock& code_block , PicArray& out_data )
{


    const int xbeg = code_block.Xstart();
    const int ybeg = code_block.Ystart();
    const int xend = code_block.Xend();
    const int yend = code_block.Yend();
 
    int xpbeg;
    int ypbeg;

   if (m_node.Parent()!=0)
    {
        xpbeg = m_pnode.Xp() + ( ( code_block.Xstart()-m_node.Xp() )>>1 );
        ypbeg = m_pnode.Yp() + ( ( code_block.Ystart()-m_node.Yp() )>>1 );
    }
    else
    {
        xpbeg = 0; 
        ypbeg = 0;
    }

    int qf_idx = m_node.QIndex();

    if ( m_node.UsingMultiQuants() )
    {
        qf_idx += DecodeQIndexOffset(); 
    }

    m_qf = dirac_quantiser_lists.QuantFactor( qf_idx );
    m_offset =  dirac_quantiser_lists.QuantOffset( qf_idx );

    m_cut_off_point *= m_qf;

    //Work
    
    for ( int ypos=ybeg , m_pypos=ypbeg; ypos<yend ;++ypos , m_pypos=(( ypos-ybeg )>>1)+ypbeg)
    {
        ValueType *p_out_data = out_data[m_pypos];
        ValueType *c_out_data_1 = out_data[ypos-1];
        ValueType *c_out_data_2 = out_data[ypos];
        for ( int xpos=xbeg , m_pxpos=xpbeg ; xpos<xend ;++xpos,m_pxpos=((xpos-xbeg)>>1)+xpbeg)
        {
            if (xpos == m_node.Xp())
                m_nhood_sum=(ypos!=m_node.Yp()) ? std::abs(c_out_data_1[xpos]): 0;
            else
                m_nhood_sum=(ypos!=m_node.Yp()) ? 
                (std::abs(c_out_data_1[xpos]) + std::abs(c_out_data_2[xpos-1])) 
              : std::abs(c_out_data_2[xpos-1]);

            m_parent_notzero = ( p_out_data[m_pxpos] != 0 );            
            DecodeVal( out_data , xpos , ypos );

        }// xpos
    }// ypos
}


/*
Coefficient magnitude value and differential quantiser index value is coded
using interleaved exp-Golomb coding for binarisation. In this scheme, a value
N>=0 is coded by writing N+1 in binary form of a 1 followed by K other bits:
1bbbbbbb (adding 1 ensures there'll be a leading 1). These K bits ("info bits")
are interleaved with K zeroes ("follow bits") each of which means "another bit
coming", followed by a terminating 1:
 
    0b0b0b ...0b1
 
(Conventional exp-Golomb coding has the K zeroes at the beginning, followed
by the 1 i.e 00...01bb .. b, but interleaving allows the decoder to run a
single loop and avoid counting the number of zeroes, sparing a register.)

All bits are arithmetically coded. The follow bits have separate contexts
based on position, and have different contexts from the info bits. 
*/
inline void BandCodec::DecodeVal( PicArray& out_data , const int xpos , const int ypos )
{

    ValueType& out_pixel = out_data[ypos][xpos];

    out_pixel = 1;
    int bit_count=1;
    
    while ( !DecodeSymbol( ChooseFollowContext( bit_count ) ) )
    {
        out_pixel <<= 1;
        out_pixel |= DecodeSymbol( ChooseInfoContext() );
        bit_count++;
    };
    --out_pixel;

    if ( out_pixel )
    {
        out_pixel *= m_qf;
        out_pixel += m_offset;
     
        if ( !DecodeSymbol( ChooseSignContext(out_data, xpos, ypos)) )
            out_pixel = -out_pixel;

    }

    m_coeff_count++;
    
    if ( m_coeff_count == m_reset_coeff_num )
    {
        ResetAll();
        m_coeff_count=0;
    }
}

inline int BandCodec::ChooseFollowContext( const int bin_number ) const 
{
    //condition on neighbouring values and parent values

    if (!m_parent_notzero && (m_pxp != 0 || m_pyp != 0))
    {
        switch ( bin_number )
        {
            case 1 :
                if(m_nhood_sum == 0)
                    return Z_FBIN1z_CTX;

                return Z_FBIN1nz_CTX;

            case 2 :
                return Z_FBIN2_CTX;
            case 3 :
                return Z_FBIN3_CTX;
            case 4 :
                return Z_FBIN4_CTX;
            default :
                return Z_FBIN5plus_CTX;
        }
    }
    else
    {
        switch ( bin_number )
        {
            case 1 :
                if(m_nhood_sum == 0)
                    return NZ_FBIN1z_CTX;
                else if (m_nhood_sum>m_cut_off_point)
                    return NZ_FBIN1b_CTX;

                return NZ_FBIN1a_CTX;

            case 2 :
                return NZ_FBIN2_CTX;
            case 3 :
                return NZ_FBIN3_CTX;
            case 4 :
                return NZ_FBIN4_CTX;
            default :
                return NZ_FBIN5plus_CTX;
        }

    }

}

inline int BandCodec::ChooseInfoContext() const 
{
    if (!m_parent_notzero && (m_pxp != 0 || m_pyp != 0))
        return Z_INFO_CTX;
    else
        return NZ_INFO_CTX;
}

inline int BandCodec::ChooseSignContext( const PicArray& data , const int xpos , const int ypos ) const
{    
    if ( m_node.Yp()==0 && m_node.Xp()!=0 )
    {
        //we're in a vertically oriented subband
        if (ypos == 0)
            return SIGN0_CTX;
        else
        {
            if (data[ypos-1][xpos]>0)
                return SIGN_POS_CTX;        
            else if (data[ypos-1][xpos]<0)
                return SIGN_NEG_CTX;
            else
                return SIGN0_CTX;
        }        
    }
    else if ( m_node.Xp()==0 && m_node.Yp()!=0 )
    {
        //we're in a horizontally oriented subband
        if (xpos == 0)
            return SIGN0_CTX;
        else
        {
            if ( data[ypos][xpos-1] > 0 )
                return SIGN_POS_CTX;                
            else if ( data[ypos][xpos-1] < 0 )
                return SIGN_NEG_CTX;
            else
                return SIGN0_CTX;
        }
    }
    else
        return SIGN0_CTX;
}

int BandCodec::DecodeQIndexOffset()
{
    int offset = 1;

    while ( !DecodeSymbol( Q_OFFSET_FOLLOW_CTX ) )
    {
        offset <<= 1;
        offset |= DecodeSymbol( Q_OFFSET_INFO_CTX );
    }
    --offset;

    if ( offset )
    {
        if ( !DecodeSymbol( Q_OFFSET_SIGN_CTX ) )
            offset = -offset;
    }
    return offset;
}

void BandCodec::SetToVal( const CodeBlock& code_block , PicArray& pic_data , const ValueType val)
{
    for (int j=code_block.Ystart() ; j<code_block.Yend() ; j++)
    {
          for (int i=code_block.Xstart() ; i<code_block.Xend() ; i++)
           {
            pic_data[j][i] = val;
           }// i
    }// j
}

void BandCodec::ClearBlock( const CodeBlock& code_block , PicArray& pic_data)
{
    for (int j=code_block.Ystart() ; j<code_block.Yend() ; j++)
    {
        ValueType *pic = &pic_data[j][code_block.Xstart()];
        memset (pic, 0, (code_block.Xend()-code_block.Xstart())*sizeof(ValueType));
    }// j

}
//////////////////////////////////////////////////////////////////////////////////
//Now for special class for LF bands (since we don't want/can't refer to parent)//
//////////////////////////////////////////////////////////////////////////////////

void LFBandCodec::DoWorkCode(PicArray& in_data)
{

    m_pxp = 0;
    m_pyp = 0;

    const TwoDArray<CodeBlock>& block_list( m_node.GetCodeBlocks() );

    // Now loop over the blocks and code
    m_coeff_count = 0;
    for (int j=block_list.FirstY() ; j<=block_list.LastY() ; ++j)
    {
        for (int i=block_list.FirstX() ; i<=block_list.LastX() ; ++i)
        {
            EncodeSymbol(block_list[j][i].Skipped() , BLOCK_SKIP_CTX );
            if ( !block_list[j][i].Skipped() )
                CodeCoeffBlock( block_list[j][i] , in_data );
            else
                ClearBlock (block_list[j][i] , in_data);
        }// i
    }// j
}

void LFBandCodec::CodeCoeffBlock( const CodeBlock& code_block , PicArray& in_data )
{
    //main coding function, using binarisation
    const int xbeg = code_block.Xstart();
    const int ybeg = code_block.Ystart();
    const int xend = code_block.Xend();
    const int yend = code_block.Yend();

    m_parent_notzero = false; //set parent to always be zero

    const int qf_idx = code_block.QIndex();

    if ( m_node.UsingMultiQuants() )
    {
        CodeQIndexOffset( qf_idx-m_node.QIndex() ); 
    }

    m_qf = dirac_quantiser_lists.QuantFactor( qf_idx );
    m_qfinv =  dirac_quantiser_lists.InverseQuantFactor( qf_idx );
    m_offset =  dirac_quantiser_lists.QuantOffset( qf_idx );

    m_cut_off_point *= m_qf;

    for ( int ypos=ybeg ; ypos<yend ; ++ypos )
    {        
        for ( int xpos=xbeg ; xpos<xend ; ++xpos )
        {
            if ( xpos == m_node.Xp() )
                m_nhood_sum = (ypos!=m_node.Yp()) ? std::abs(in_data[ypos-1][xpos]) : 0;
            else
                m_nhood_sum = (ypos!=m_node.Yp()) ? 
                (std::abs(in_data[ypos-1][xpos]) + std::abs(in_data[ypos][xpos-1])) 
               : std::abs(in_data[ypos][xpos-1]);    
            
            CodeVal( in_data , xpos , ypos , in_data[ypos][xpos] );

        }//xpos
    }//ypos    
}


void LFBandCodec::DoWorkDecode(PicArray& out_data )
{
    m_pxp = 0;
    m_pyp = 0;

    const TwoDArray<CodeBlock>& block_list( m_node.GetCodeBlocks() );

    // Now loop over the blocks and decode
    m_coeff_count = 0;
    for (int j=block_list.FirstY() ; j<=block_list.LastY() ; ++j)
    {
        for (int i=block_list.FirstX() ; i<=block_list.LastX() ; ++i)
        {
            block_list[j][i].SetSkip( DecodeSymbol( BLOCK_SKIP_CTX ) );
            if ( !block_list[j][i].Skipped() )
                DecodeCoeffBlock( block_list[j][i] , out_data );
            else
                ClearBlock (block_list[j][i] , out_data);
        }// i
    }// j

}

void LFBandCodec::DecodeCoeffBlock( const CodeBlock& code_block , PicArray& out_data )
{

    const int xbeg = code_block.Xstart();
    const int ybeg = code_block.Ystart();
    const int xend = code_block.Xend();
    const int yend = code_block.Yend();

    m_parent_notzero = false;//set parent to always be zero    

    int qf_idx = m_node.QIndex();

    if ( m_node.UsingMultiQuants() )
    {
        qf_idx += DecodeQIndexOffset(); 
    }

    m_qf = dirac_quantiser_lists.QuantFactor( qf_idx );
    m_offset =  dirac_quantiser_lists.QuantOffset( qf_idx );

    m_cut_off_point *= m_qf;

    //Work
    
    for ( int ypos=ybeg ; ypos<yend ; ++ypos )
    {
        for ( int xpos=xbeg ; xpos<xend; ++xpos )
        {
            if ( xpos == m_node.Xp() )
                m_nhood_sum=(ypos!=m_node.Yp()) ? std::abs(out_data[ypos-1][xpos]) : 0;
            else
                m_nhood_sum=(ypos!=m_node.Yp()) ? 
                (std::abs(out_data[ypos-1][xpos]) + std::abs(out_data[ypos][xpos-1])) 
               : std::abs(out_data[ypos][xpos-1]);

            DecodeVal( out_data , xpos , ypos );

        }// xpos
    }// ypos
}


//////////////////////////////////////////////////////////////////////////////////
//Finally,special class incorporating prediction for the DC band of intra frames//
//////////////////////////////////////////////////////////////////////////////////

void IntraDCBandCodec::DoWorkCode(PicArray& in_data)
{

    m_pxp = 0;
    m_pyp = 0;

    // Residues after prediction, quantisation and inverse quantisation
    m_dc_pred_res.Resize( m_node.Yl() , m_node.Xl() );

    const TwoDArray<CodeBlock>& block_list( m_node.GetCodeBlocks() );

    // Now loop over the blocks and code. Note that DC blocks can't be skipped
    m_coeff_count = 0;
    for (int j=block_list.FirstY() ; j<=block_list.LastY() ; ++j)
    {
        for (int i=block_list.FirstX() ; i<=block_list.LastX() ; ++i)
        {
            CodeCoeffBlock( block_list[j][i] , in_data );
        }// i
    }// j
}

void IntraDCBandCodec::CodeCoeffBlock( const CodeBlock& code_block , PicArray& in_data)
{
    // Main coding function, using binarisation
    const int xbeg = code_block.Xstart();
    const int ybeg = code_block.Ystart();
    const int xend = code_block.Xend();
    const int yend = code_block.Yend();

    //set parent to always be zero
    m_parent_notzero = false;
    ValueType val;
    
    ValueType prediction;

    const int qf_idx = code_block.QIndex();

    if ( m_node.UsingMultiQuants() )
    {
        CodeQIndexOffset( qf_idx-m_node.QIndex() ); 
    }

    m_qf = dirac_quantiser_lists.QuantFactor( qf_idx );
    m_qfinv =  dirac_quantiser_lists.InverseQuantFactor( qf_idx );
    m_offset =  dirac_quantiser_lists.QuantOffset( qf_idx );

    m_cut_off_point *= m_qf;
    
    for ( int ypos=ybeg ; ypos < yend; ++ypos )
    {
        for (int xpos = xbeg ; xpos < xend; ++xpos )
        {
             if (xpos == m_node.Xp())
                 m_nhood_sum = (ypos!=m_node.Yp()) ? std::abs(m_dc_pred_res[ypos-1][xpos]) : 0;
             else
                 m_nhood_sum = (ypos!=m_node.Yp()) ? 
                               (std::abs(m_dc_pred_res[ypos-1][xpos]) + std::abs(m_dc_pred_res[ypos][xpos-1])) 
                              : std::abs(m_dc_pred_res[ypos][xpos-1]);
          
            prediction = GetPrediction( in_data , xpos , ypos );            
            val = in_data[ypos][xpos] - prediction;
            CodeVal( in_data , xpos , ypos , val );            
            m_dc_pred_res[ypos][xpos] = in_data[ypos][xpos];
            in_data[ypos][xpos] += prediction;
        }//xpos            
    }//ypos    
}


void IntraDCBandCodec::DoWorkDecode(PicArray& out_data)
{

    m_pxp = 0;
    m_pyp = 0;

    m_dc_pred_res.Resize( m_node.Yl() , m_node.Xl() );

    const TwoDArray<CodeBlock>& block_list( m_node.GetCodeBlocks() );

    // Now loop over the blocks and decode
    m_coeff_count = 0;
    for (int j=block_list.FirstY() ; j<=block_list.LastY() ; ++j)
    {
        for (int i=block_list.FirstX() ; i<=block_list.LastX() ; ++i)
        {
            DecodeCoeffBlock( block_list[j][i] , out_data );
        }// i
    }// j
}

void IntraDCBandCodec::DecodeCoeffBlock( const CodeBlock& code_block , PicArray& out_data)
{
    const int xbeg = code_block.Xstart();
    const int ybeg = code_block.Ystart();
    const int xend = code_block.Xend();
    const int yend = code_block.Yend();

    m_parent_notzero = false; //set parent to always be zero

    int qf_idx = m_node.QIndex();

    if ( m_node.UsingMultiQuants() )
    {
        qf_idx += DecodeQIndexOffset();
    }

    m_qf = dirac_quantiser_lists.QuantFactor( qf_idx );
    m_offset =  dirac_quantiser_lists.QuantOffset( qf_idx );

    m_cut_off_point *= m_qf;

    //Work
    
    for ( int ypos=ybeg ; ypos<yend ; ++ypos)
    {
        for ( int xpos=xbeg ; xpos<xend ; ++xpos)
        {
              if ( xpos==m_node.Xp() )
                 m_nhood_sum=(ypos!=m_node.Yp()) ? std::abs(m_dc_pred_res[ypos - 1][xpos]) : 0;
             else
                 m_nhood_sum=(ypos!=m_node.Yp()) ? 
                             (std::abs(m_dc_pred_res[ypos-1][xpos]) + std::abs(m_dc_pred_res[ypos][xpos-1]))
                            : std::abs(m_dc_pred_res[ypos][xpos-1]);

             DecodeVal( out_data , xpos , ypos );
             m_dc_pred_res[ypos][xpos] = out_data[ypos][xpos];
             out_data[ypos][xpos] += GetPrediction( out_data , xpos , ypos );

        }//xpos
    }//ypos
}


ValueType IntraDCBandCodec::GetPrediction( const PicArray& data , const int xpos , const int ypos ) const
{
    if (ypos!=0)
    {
        if (xpos!=0)
            return (data[ypos][xpos - 1] + data[ypos - 1][xpos - 1] + data[ypos - 1][xpos]) / 3;
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
