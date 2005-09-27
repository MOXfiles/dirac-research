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


//Compression of an individual component,
//after motion compensation if appropriate
//////////////////////////////////////////

#include <libdirac_encoder/comp_compress.h>
#include <libdirac_encoder/quant_chooser.h>
#include <libdirac_common/band_codec.h>
#include <libdirac_common/golomb.h>

using namespace dirac;

#include <ctime>
#include <vector>
#include <iostream>

CompCompressor::CompCompressor( EncoderParams& encp,const FrameParams& fp)
: m_encparams(encp),
  m_fparams(fp),
  m_fsort( m_fparams.FSort() ),
  m_cformat( m_fparams.CFormat() )
{}

void CompCompressor::Compress(PicArray& pic_data)
{
    //need to transform, select quantisers for each band, and then compress each component in turn
    m_csort=pic_data.CSort();    
    const int depth=4;
    unsigned int num_band_bytes( 0 );

    // A pointer to an object  for coding the subband data
    BandCodec* bcoder;

    // A pointer to an object for outputting the subband data
    UnitOutputManager* band_op;

    const size_t CONTEXTS_REQUIRED = 24;

    Subband node;

    //set up Lagrangian params    
/*
    if (m_fsort == I_frame) 
        m_lambda= m_encparams.ILambda();
    else
    {
        if (m_fsort == L1_frame) 
            m_lambda= m_encparams.L1Lambda();
        else 
            m_lambda= m_encparams.L2Lambda();
    }
*/
    m_lambda= m_encparams.Lambda( m_fsort );

    if (m_csort == U_COMP)
        m_lambda*= m_encparams.UFactor();
    if (m_csort == V_COMP) 
        m_lambda*= m_encparams.VFactor();

    WaveletTransform wtransform( depth , m_encparams.TransformFilter() );
    wtransform.Transform( FORWARD , pic_data );

    // Choose all the quantisers //
    ///////////////////////////////
    SubbandList& bands = wtransform.BandList();

    // Set up the code blocks
    SetupCodeBlocks( bands );

    wtransform.SetBandWeights( m_encparams.CPD() , m_fparams.FSort() , m_fparams.CFormat(), m_csort);

    OneDArray<unsigned int> estimated_bits( Range( 1 , bands.Length() ) );

    // Default coding is that we're not using multiple quantisers in coding subbands
    const bool using_multi_quants( false );
    
    SelectQuantisers( pic_data , bands , estimated_bits , using_multi_quants );  

    // Loop over all the bands (from DC to HF) quantising and coding them
    for (int b=bands.Length() ; b>=1 ; --b )
    {
        band_op = & m_encparams.BitsOut().FrameOutput().BandOutput( m_csort , b );

        if ( !bands(b).Skipped() )
        {   // If not skipped ...

             // Pick the right codec according to the frame type and subband
            if (b >= bands.Length()-3)
            {
                if ( m_fsort == I_frame && b == bands.Length() )
                    bcoder=new IntraDCBandCodec( &( band_op->Data() ) , CONTEXTS_REQUIRED , bands );
                else
                    bcoder=new LFBandCodec( &( band_op->Data() ) ,CONTEXTS_REQUIRED, bands , b);
            }
            else
                bcoder=new BandCodec( &( band_op->Data() ) , CONTEXTS_REQUIRED , bands , b);

            num_band_bytes = bcoder->Compress(pic_data);

             // Update the entropy correction factors
            m_encparams.EntropyFactors().Update(b , m_fsort , m_csort , estimated_bits[b] , 8*num_band_bytes);

            delete bcoder;            
        }
        else
        {   // ... skipped
            if (b == bands.Length() && m_fsort == I_frame)
                SetToVal( pic_data , bands(b) , wtransform.GetMeanDCVal() );
            else
                SetToVal( pic_data , bands(b) , 0 );
        }
        WriteBandHeader( band_op->Header() , bands(b) , num_band_bytes );


    }//b

    if ( m_fsort!= L2_frame || m_encparams.LocalDecode() )
    {
        // Transform back into the picture domain
        wtransform.Transform( BACKWARD , pic_data );
    }
}

void CompCompressor::WriteBandHeader( BasicOutputManager& hdr_out , const Subband& band , 
                                      const int num_band_bytes )
{
    hdr_out.OutputBit( band.Skipped() );

    if ( !band.Skipped() )
    {
        // If we're not skipped, we need a quantisation index for the subband
        UnsignedGolombCode( hdr_out , band.QIndex() );

        // We also need to say whether there are multiple quantisers in the subband.
        // If so, offsets from the band QIndex will be coded in the code block
        // headers in the arithmetic coded subband data.
        hdr_out.OutputBit( band.UsingMultiQuants() );

        // Write the length of the data chunk into the header
        UnsignedGolombCode( hdr_out , num_band_bytes);

    }

}

void CompCompressor::SetupCodeBlocks( SubbandList& bands )
{
    int xregions;
    int yregions;

    // The minimum x and y dimensions of a block
    const int min_dim( 4 );
  
    // The maximum number of regions horizontally and vertically
    int max_xregion, max_yregion;

    for (int band_num = 1; band_num<=bands.Length() ; ++band_num)
    {
        if ( band_num < bands.Length()-6 )
        {
            if (m_fsort != I_frame )
            {
                xregions = 12;
                yregions = 8;
            }
            else
            {
                xregions = 4;
                yregions = 3;
            }
        }
        else if (band_num < bands.Length()-3)
        {
            if (m_fsort != I_frame )
            {
                xregions = 8;
                yregions = 6;
            }
            else
            {
                xregions = 1;
                yregions = 1;
            }
        }
        else
        {
            xregions = 1;
            yregions = 1;
        }

        max_xregion = bands( band_num ).Xl() / min_dim;
        max_yregion = bands( band_num ).Yl() / min_dim;

        bands( band_num ).SetNumBlocks( std::min( yregions , max_yregion ), 
                                        std::min( xregions , max_xregion ) );

    }// band_num
        
}

void CompCompressor::SelectQuantisers( PicArray& pic_data , 
                                       SubbandList& bands ,
                                       OneDArray<unsigned int>& est_bits,
                                       const bool using_multi_quants )
{
    // Select all the quantizers
    if ( !m_encparams.Lossless() )
    {
        for ( int b=bands.Length() ; b>=1 ; --b )
        {
            bands(b).SetUsingMultiQuants( using_multi_quants );
            est_bits[b] = SelectMultiQuants( pic_data , bands , b );
        }// b
    }
    else
    {
        for ( int b=bands.Length() ; b>=1 ; --b )
        {
            bands(b).SetUsingMultiQuants( false );
            bands(b).SetQIndex( 0 );
            TwoDArray<CodeBlock>& blocks = bands(b).GetCodeBlocks();
            for (int j=0; j<blocks.LengthY() ;++j)
            {
                for (int i=0; i<blocks.LengthX() ;++i)
                {
                    blocks[j][i].SetQIndex( 0 );
                }// i
            }// j
        }// b
    }
}

int CompCompressor::SelectMultiQuants( PicArray& pic_data , SubbandList& bands , const int band_num )
{
    Subband& node( bands( band_num ) );

    // Now select the quantisers //
    ///////////////////////////////

    QuantChooser qchooser( pic_data , m_lambda );

    // For the DC band in I frames, remove the average
    if ( band_num == bands.Length() && m_fsort == I_frame)
        AddSubAverage( pic_data , node.Xl() , node.Yl() , SUBTRACT);

    // The total estimated bits for the subband 
    int band_bits( 0 );
    qchooser.SetEntropyCorrection( m_encparams.EntropyFactors().Factor( band_num , m_fsort , m_csort ) );
    band_bits = qchooser.GetBestQuant( node );

    // Put the DC band average back in if necessary   
    if ( band_num == bands.Length() && m_fsort == I_frame)
        AddSubAverage( pic_data , node.Xl() , node.Yl() , ADD);

    if ( band_bits == 0 )
        node.SetSkip( true );
    else
        node.SetSkip( false );

    return band_bits;
}



void CompCompressor::SetToVal(PicArray& pic_data,const Subband& node,ValueType val)
{

    for (int j=node.Yp() ; j<node.Yp() + node.Yl() ; ++j)
    {    
        for (int i=node.Xp(); i<node.Xp() + node.Xl() ; ++i)
        {
            pic_data[j][i] = val;
        }// i
    }// j

}


void CompCompressor::AddSubAverage( PicArray& pic_data ,
                                    int xl ,
                                    int yl , 
                                    AddOrSub dirn)
{

    ValueType last_val=0;
    ValueType last_val2;
 
    if ( dirn == SUBTRACT )
    {
        for ( int j=0 ; j<yl ; j++)
            {
            for ( int i=0 ; i<xl ; i++)
                {
                last_val2 = pic_data[j][i];        
                pic_data[j][i] -= last_val;
                last_val = last_val2;
            }// i
        }// j
    }
    else
    {
        for ( int j=0 ; j<yl ; j++)
        {
            for ( int i=0 ; i<xl; i++ )
            {
                pic_data[j][i] += last_val;
                last_val = pic_data[j][i];
            }// i
        }// j

    }
}
