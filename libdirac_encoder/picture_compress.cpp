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
*                 Chris Bowley,
*                 Anuradha Suraparaju,
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

//Compression of pictures//
/////////////////////////

#include <libdirac_encoder/picture_compress.h>
#include <libdirac_encoder/comp_compress.h>
#include <libdirac_common/mot_comp.h>
#include <libdirac_motionest/motion_estimate.h>
#include <libdirac_common/mv_codec.h>
#include <libdirac_encoder/quant_chooser.h>
#include <libdirac_common/dirac_assertions.h>
using namespace dirac;

#include <iostream>
#include <sstream>

PictureCompressor::PictureCompressor( EncoderParams& encp ) :
    m_encparams(encp),
    m_me_data(0),
    m_skipped(false),
    m_use_global(false),
    m_use_block_mv(true),
    m_global_pred_mode(REF1_ONLY),
    m_medata_avail(false),
    m_is_a_cut(false)
{}

PictureCompressor::~PictureCompressor()
{
    if (m_me_data)
        delete m_me_data;
}

bool PictureCompressor::MotionEstimate(const  PictureBuffer& my_fbuffer ,
                                                            int pnum )
{
    m_is_a_cut = false;

    if (m_me_data)
    {
        delete m_me_data;
        m_me_data = 0;
    }
    
    m_me_data = new MEData( m_encparams.XNumMB() , 
                            m_encparams.YNumMB(), 
                            my_fbuffer.GetPicture( pnum).GetPparams().NumRefs() );

    MotionEstimator my_motEst( m_encparams );
    my_motEst.DoME( my_fbuffer , pnum , *m_me_data );

    // If we have a cut....
    AnalyseMEData( *m_me_data );
        
    // Set me data available flag
    if ( m_is_a_cut==false )
        m_medata_avail = true;
    else
    {
        m_medata_avail = false;
        delete m_me_data;
        m_me_data = 0;
    }
    
    return m_is_a_cut;
}

PictureByteIO* PictureCompressor::Compress( PictureBuffer& my_buffer ,
                                        int pnum)
{
    Picture& my_picture = my_buffer.GetPicture( pnum );

    PictureParams& fparams = my_picture.GetPparams();
    const PictureSort& fsort = fparams.PicSort();
    
    // Set the wavelet filter
    if ( fsort.IsIntra() )
    {
        m_encparams.SetTransformFilter( m_encparams.IntraTransformFilter() );
        m_encparams.SetUsualCodeBlocks( INTRA_PICTURE );
    }
    else
    {
        m_encparams.SetTransformFilter( m_encparams.InterTransformFilter() );
        m_encparams.SetUsualCodeBlocks( INTER_PICTURE );
    }

    // Write the picture header. We wait until after motion estimation, since
    // this allows us to do cut-detection and (possibly) to decide whether
    // or not to skip a picture before actually encoding anything. However we
    // can do this at any point prior to actually writing any picture data.
    //WritePictureHeader( my_picture.GetPparams() );
    PictureByteIO* p_picture_byteio = new PictureByteIO(fparams,
                                                  pnum);
   
    p_picture_byteio->Output();

    if ( !m_skipped )
    {    // If not skipped we continue with the coding ...
        if (m_encparams.Verbose() )
            std::cout<<std::endl<<"Using QF: "<<m_encparams.Qf();

        if (fsort.IsInter() )
        {
             // Code the MV data

            // If we're using global motion parameters, code them
            if (m_use_global)
            {
                /*
                    Code the global motion parameters
                    TBC ....
                */
            }

            // If we're using block motion vectors, code them
            if ( m_use_block_mv )
            {
                MvDataByteIO *mv_data = new MvDataByteIO(fparams, 
                                        static_cast<CodecParams&>(m_encparams));
                p_picture_byteio->SetMvData(mv_data);
                
                CompressMVData( mv_data );
            }

             // Then motion compensate
            MotionCompensator::CompensatePicture( m_encparams , SUBTRACT , 
                                                my_buffer , pnum , 
                                                *m_me_data );
 
        }//?fsort

        //Write Transform Header
        TransformByteIO *p_transform_byteio = new TransformByteIO(fparams, 
                                static_cast<CodecParams&>(m_encparams));
        p_picture_byteio->SetTransformData(p_transform_byteio);
        p_transform_byteio->Output();

        /* Code component data */
        /////////////////////////
        
        CompCompressor my_compcoder(m_encparams , fparams );

        PicArray* comp_data[3];
        CoeffArray* comp_coeff_data[3];
        WaveletTransform* comp_transform[3];
        SubbandList* comp_bands[3];
        OneDArray<unsigned int>* est_bits[3];


        const int depth=m_encparams.TransformDepth();
        const WltFilter filter = m_encparams.TransformFilter();

        // Construction and definition of objects
        for (int i=0;i<3;++i){
            comp_data[i] = &my_buffer.GetComponent( pnum , (CompSort) i );
            comp_transform[i] = new WaveletTransform( depth, filter );
            comp_coeff_data[i] = new CoeffArray(comp_data[i]->LengthY(),
                                           comp_data[i]->LengthX(), (CompSort) i );
        }// i

        // Do the wavelet transforms for Y, U and V

        for (int i=0;i<3;++i){
            comp_transform[i]->Transform( FORWARD , *comp_data[i], *comp_coeff_data[i] );
            comp_transform[i]->SetBandWeights( m_encparams.CPD() , fsort , 
                            fparams.CFormat(), (CompSort) i, m_encparams.FieldCoding());
            comp_bands[i] = &comp_transform[i]->BandList();
            SetupCodeBlocks( *comp_bands[i] );

        }//i


        // Select the component quantisers
        float lambda;
        for (int i=0; i<3; ++i){
            lambda = GetCompLambda( fparams, (CompSort) i );

            est_bits[i] = new OneDArray<unsigned int>( Range( 1, comp_bands[i]->Length() ) );

            SelectQuantisers( *comp_coeff_data[i] , *comp_bands[i] , lambda, 
                              *est_bits[i] , m_encparams.GetCodeBlockMode(), 
                              fsort, (CompSort) i );
        }
            


        // Quantise and code the component subbands
        for (int i=0; i<3; ++i){
            p_transform_byteio->AddComponent( my_compcoder.Compress( 
                *comp_coeff_data[i], *comp_bands[i], *est_bits[i] ) );
        }


        // Transform back into the picture domain if required
        if ( fsort.IsIntra() || fsort.IsRef() || m_encparams.LocalDecode() )
        {
            for (int i=0; i<3; ++i){
                comp_transform[i]->Transform( BACKWARD , *comp_data[i], *comp_coeff_data[i] );
            }// i
        }
        
        
        // Destruction of objects
        for (int i=0; i<3; ++i)
        {
            delete comp_transform[i];
            delete comp_coeff_data[i];
            delete est_bits[i];
            
        }
                                          

        //motion compensate again if necessary
        if (fsort.IsInter() )
        {
            if ( fsort.IsRef() || m_encparams.LocalDecode() )
            {
                MotionCompensator::CompensatePicture( m_encparams , ADD , 
                                                    my_buffer , pnum , 
                                                    *m_me_data );   
            }
        }//?fsort

         //finally clip the data to keep it in range
        my_buffer.GetPicture( pnum ).Clip();

    }//?m_skipped

    // return compressed picture
    return p_picture_byteio;
}

const MEData* PictureCompressor::GetMEData() const
{
    TESTM (m_me_data != NULL, "m_medata allocated");
    TESTM (m_medata_avail == true, "ME Data available");

    return m_me_data;
}

void PictureCompressor::CompressMVData(MvDataByteIO* mv_data)
{
    SplitModeCodec smode_coder( mv_data->SplitModeData()->DataBlock(), TOTAL_MV_CTXS);
    smode_coder.Compress( *m_me_data );
    mv_data->SplitModeData()->Output();
    
    PredModeCodec pmode_coder( mv_data->PredModeData()->DataBlock(), TOTAL_MV_CTXS);
    pmode_coder.Compress( *m_me_data );
    mv_data->PredModeData()->Output();

    VectorElementCodec vcoder1h( mv_data->MV1HorizData()->DataBlock(), 1, 
                                 HORIZONTAL, TOTAL_MV_CTXS);
    vcoder1h.Compress( *m_me_data );
    mv_data->MV1HorizData()->Output();
    
    VectorElementCodec vcoder1v( mv_data->MV1VertData()->DataBlock(), 1, 
                                 VERTICAL, TOTAL_MV_CTXS);
    vcoder1v.Compress( *m_me_data );
    mv_data->MV1VertData()->Output();

    if ( m_me_data->NumRefs()>1 )
    {
        VectorElementCodec vcoder2h( mv_data->MV2HorizData()->DataBlock(), 2, 
                                     HORIZONTAL, TOTAL_MV_CTXS);
        vcoder2h.Compress( *m_me_data );
        mv_data->MV2HorizData()->Output();
        
        VectorElementCodec vcoder2v( mv_data->MV2VertData()->DataBlock(), 2, 
                                     VERTICAL, TOTAL_MV_CTXS);
        vcoder2v.Compress( *m_me_data );
        mv_data->MV2VertData()->Output();
    }

    DCCodec ydc_coder( mv_data->YDCData()->DataBlock(), Y_COMP, TOTAL_MV_CTXS);
    ydc_coder.Compress( *m_me_data );
    mv_data->YDCData()->Output();

    DCCodec udc_coder( mv_data->UDCData()->DataBlock(), U_COMP, TOTAL_MV_CTXS);
    udc_coder.Compress( *m_me_data );
    mv_data->UDCData()->Output();
    
    DCCodec vdc_coder( mv_data->VDCData()->DataBlock(), V_COMP, TOTAL_MV_CTXS);
    vdc_coder.Compress( *m_me_data );
    mv_data->VDCData()->Output();

    mv_data->Output();    
}

void PictureCompressor::AnalyseMEData( const MEData& me_data )
{
    // Count the number of intra blocks
    const TwoDArray<PredMode>& modes = me_data.Mode();

    int count_intra = 0;
    for ( int j=0 ; j<modes.LengthY() ; ++j )
    {
        for ( int i=0 ; i<modes.LengthX() ; ++i )
        {
            if ( modes[j][i] == INTRA )
                count_intra++;
        }
    }// j
    
    m_intra_ratio = 100.0*static_cast<double>( count_intra ) / 
                          static_cast<double>( modes.LengthX() * modes.LengthY() );

    if ( m_encparams.Verbose() )
        std::cout<<std::endl<<m_intra_ratio<<"% of blocks are intra   ";
   
    if ( m_intra_ratio > 33.33 )
        m_is_a_cut = true;
    else
        m_is_a_cut = false;
  
}

float PictureCompressor::GetCompLambda( const PictureParams& fparams,
                                      const CompSort csort )
{
    const PictureSort fsort = fparams.PicSort();
    
    float lambda;
    
    if ( fsort.IsIntra() )
    {
        lambda= m_encparams.ILambda();
        if ( m_is_a_cut )
        {
            // The intra picture is inserted so we can lower the quality
            lambda *= 5;

        }
    }
    else
    {
        double log_intra_lambda = std::log10( m_encparams.ILambda() );
        double log_picture_lambda;

        if (fparams.IsBPicture() )
            log_picture_lambda= std::log10( m_encparams.L2Lambda() );
        else
            log_picture_lambda= std::log10( m_encparams.L1Lambda() );


        lambda= std::pow(10.0, ( (1.7*m_intra_ratio*log_intra_lambda+
                         (100.0-2*m_intra_ratio)*log_picture_lambda )/100.0) );
    }


    if (csort == U_COMP)
        lambda*= m_encparams.UFactor();
    if (csort == V_COMP)
        lambda*= m_encparams.VFactor();
        
    return lambda;
}

void PictureCompressor::SetupCodeBlocks( SubbandList& bands )
{
    int xregions;
    int yregions;

    // The maximum number of regions horizontally and vertically


    for (int band_num = 1; band_num<=bands.Length() ; ++band_num)
    {
        if (m_encparams.SpatialPartition())
        {
            int level = m_encparams.TransformDepth() - (band_num-1)/3;
            const CodeBlocks &cb = m_encparams.GetCodeBlocks(level);
            xregions = cb.HorizontalCodeBlocks();
            yregions = cb.VerticalCodeBlocks();
        }
        else
        {
               xregions = 1;
               yregions = 1;
        }

        bands( band_num ).SetNumBlocks( yregions , xregions );
    }// band_num
}

void PictureCompressor::SelectQuantisers( CoeffArray& coeff_data ,
                                       SubbandList& bands ,
                                       const float lambda,
                                       OneDArray<unsigned int>& est_bits,
                                       const CodeBlockMode cb_mode,
                                       const PictureSort fsort,
                                       const CompSort csort )
{

   // Set up the multiquantiser mode
    for ( int b=bands.Length() ; b>=1 ; --b )
    {
        // Set multiquants flag in the subband only if
        // a. Global m_cb_mode flag is set to QUANT_MULTIPLE in encparams
        //           and
        // b. Current subband has more than one block
        if (
            cb_mode == QUANT_MULTIPLE &&
            (bands(b).GetCodeBlocks().LengthX() > 1  ||
            bands(b).GetCodeBlocks().LengthY() > 1)
           )
            bands(b).SetUsingMultiQuants( true );
        else
            bands(b).SetUsingMultiQuants( false );
    }// b

    // Select all the quantizers
    if ( !m_encparams.Lossless() )
    {
        // Set the DC band quantiser to be 1
        bands( bands.Length() ).SetQIndex( 0 );
        bands( bands.Length() ).SetSkip( false );
        bands( bands.Length() ).SetUsingMultiQuants( false );
        est_bits[ bands.Length()] = 0;
        TwoDArray<CodeBlock>& blocks = bands( bands.Length() ).GetCodeBlocks();
        for (int j=0; j<blocks.LengthY(); ++j)
            for (int i=0 ; i<blocks.LengthX(); ++i )
                blocks[j][i].SetQIndex( 0 );

        // Now do the rest of the bands.
        for ( int b=bands.Length()-1 ; b>=1 ; --b )
        {            est_bits[b] = SelectMultiQuants( coeff_data , bands , b, lambda, 
                                      fsort, csort );
        }// b
    }
    else
    {
        for ( int b=bands.Length() ; b>=1 ; --b )
        {
            bands(b).SetQIndex( 0 );
            est_bits[b] = 0;
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

int PictureCompressor::SelectMultiQuants( CoeffArray& coeff_data , SubbandList& bands , 
    const int band_num , const float lambda, const PictureSort fsort, const CompSort csort)
{
    Subband& node( bands( band_num ) );

    // Now select the quantisers //
    ///////////////////////////////

    QuantChooser qchooser( coeff_data , lambda );

    // For the DC band in I pictures, remove the average
    if ( band_num == bands.Length() && fsort.IsIntra() )
        AddSubAverage( coeff_data , node.Xl() , node.Yl() , SUBTRACT);

    // The total estimated bits for the subband
    int band_bits( 0 );
    qchooser.SetEntropyCorrection( m_encparams.EntropyFactors().Factor( band_num , fsort , csort ) );
    band_bits = qchooser.GetBestQuant( node );

    // Put the DC band average back in if necessary
    if ( band_num == bands.Length() && fsort.IsIntra() )
        AddSubAverage( coeff_data , node.Xl() , node.Yl() , ADD);

    if ( band_bits == 0 )
        node.SetSkip( true );
    else
        node.SetSkip( false );

    return band_bits;
}


void PictureCompressor::AddSubAverage( CoeffArray& coeff_data, int xl, int yl ,
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
                last_val2 = coeff_data[j][i];
                coeff_data[j][i] -= last_val;
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
                coeff_data[j][i] += last_val;
                last_val = coeff_data[j][i];
            }// i
        }// j

    }
}
