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
*                 Anuradha Suraparaju,
*                 Andrew Kennedy,
*                 Tim Borer
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


//Decompression of pictures
/////////////////////////

#include <libdirac_common/dirac_assertions.h>
#include <libdirac_decoder/picture_decompress.h>
#include <libdirac_decoder/comp_decompress.h>
#include <libdirac_common/mot_comp.h>
#include <libdirac_common/mv_codec.h>
#include <libdirac_byteio/picture_byteio.h>
#include <libdirac_common/dirac_exception.h>
using namespace dirac;

#include <iostream>
#include <memory>

using std::vector;
using std::auto_ptr;

PictureDecompressor::PictureDecompressor(DecoderParams& decp, ChromaFormat cf)
: 
m_decparams(decp),
m_cformat(cf)
{
}

PictureDecompressor::~PictureDecompressor()
{
}


bool PictureDecompressor::Decompress(ParseUnitByteIO& parseunit_byteio,
                                   PictureBuffer& my_buffer)
{
    // get current byte position
    //int start_pos = parseunit_byteio.GetReadBytePosition();
    try {

    // read picture data
    PictureByteIO picture_byteio(m_fparams,
                             parseunit_byteio);

    picture_byteio.Input();
    
    PictureSort fs;
    
    if (m_fparams.GetPictureType() == INTRA_PICTURE)
        fs.SetIntra();
    else
        fs.SetInter();
    
    if (m_fparams.GetReferenceType() == REFERENCE_PICTURE)
        fs.SetRef();
    else
        fs.SetNonRef();

    m_fparams.SetPicSort(fs);

    if (m_fparams.GetReferenceType() == REFERENCE_PICTURE)
    {
        // Now clean the reference pictures from the buffer
        CleanReferencePictures( my_buffer );
    }
        
    // Check if the picture can be decoded
    if (m_fparams.PicSort().IsInter())
    {
        const std::vector<int>& refs = m_fparams.Refs();

        for (unsigned int i = 0; i < refs.size(); ++i)
        {
            if ( !my_buffer.IsPictureAvail(refs[i]) )
            {
                return false;
            }
        }
    }

    m_skipped=false;
    if ( !m_skipped )
    {//if we're not m_skipped then we can decode the rest of the picture

       if ( m_decparams.Verbose() )
       {
             std::cout<<std::endl<<"Decoding picture "<<m_fparams.PictureNum()<<" in display order";        
             if ( m_fparams.PicSort().IsInter() )
             {
                 std::cout<<std::endl<<"References: "<<m_fparams.Refs()[0];
                 if ( m_fparams.Refs().size()>1 )
                     std::cout<<" and "<<m_fparams.Refs()[1];
             }
       }   

       PictureSort fsort = m_fparams.PicSort();
       auto_ptr<MvData> mv_data;

       if ( fsort.IsInter() )
       {    //do all the MV stuff 
            DecompressMVData( mv_data, picture_byteio );                
       }
           
        // Read the  transform header
        TransformByteIO transform_byteio(picture_byteio, m_fparams, m_decparams);
        transform_byteio.Input();
        
        if (m_fparams.PicSort().IsIntra() && m_decparams.ZeroTransform())
        {
            DIRAC_THROW_EXCEPTION(
                ERR_UNSUPPORTED_STREAM_DATA,
                "Intra pictures cannot have Zero-Residual",
                SEVERITY_PICTURE_ERROR);
        }
            
        // Set picture dimensions based on the transform depth. If zero residual
        // then use the actual picture dimensions
        PushPicture(my_buffer);

        //Reference to the picture being decoded
        Picture& my_picture = my_buffer.GetPicture(m_fparams.PictureNum());

        if (!m_decparams.ZeroTransform())
        {
            //decode components
            CompDecompress( &transform_byteio, my_buffer,m_fparams.PictureNum() , Y_COMP );
            CompDecompress( &transform_byteio, my_buffer , m_fparams.PictureNum() , U_COMP );        
            CompDecompress( &transform_byteio, my_buffer , m_fparams.PictureNum() , V_COMP );
        }
        else
        {
            my_picture.Fill(0);
        }

        if ( fsort.IsInter() )
        //motion compensate to add the data back in if we don't have an I picture
            MotionCompensator::CompensatePicture( m_decparams , ADD , 
                                                my_buffer , m_fparams.PictureNum() ,
                                                *(mv_data.get()) );
        my_picture.Clip();

        if (m_decparams.Verbose())
            std::cout<<std::endl;        

        }//?m_skipped,!End()
        else if (m_skipped){
         //TBD: decide what to return if we're m_skipped. Nearest picture in temporal order??    

        }

        //exit success
        return true;
    }// try
    catch (const DiracException& e) {
        // skip picture
        throw e;
    }

     //exit failure
    return false;
}

void PictureDecompressor::CleanReferencePictures( PictureBuffer& my_buffer )
{
    if ( m_decparams.Verbose() )
        std::cout<<std::endl<<"Cleaning reference buffer: ";
    // Do picture buffer cleaning
    int retd_pnum = m_fparams.RetiredPictureNum();

    if ( retd_pnum >= 0 && my_buffer.IsPictureAvail(retd_pnum) && my_buffer.GetPicture(retd_pnum).GetPparams().PicSort().IsRef() )
    {
        my_buffer.Clean(retd_pnum);
        if ( m_decparams.Verbose() )
            std::cout<<retd_pnum<<" ";    
    }
}

void PictureDecompressor::CompDecompress(TransformByteIO *p_transform_byteio,
                                       PictureBuffer& my_buffer, int pnum,CompSort cs)
{
    if ( m_decparams.Verbose() )
        std::cout<<std::endl<<"Decoding component data ...";
    
    ComponentByteIO component_byteio(cs, *p_transform_byteio);
    CompDecompressor my_compdecoder( m_decparams , my_buffer.GetPicture(pnum).GetPparams() );    
    PicArray& comp_data=my_buffer.GetComponent( pnum , cs );
    my_compdecoder.Decompress(&component_byteio, 
                              comp_data );
}

void PictureDecompressor::SetMVBlocks()
{
    OLBParams olb_params = m_decparams.LumaBParams(2);
    m_decparams.SetBlockSizes(olb_params, m_cformat);

    // Calculate the number of macro blocks
    int xnum_mb = m_decparams.OrigXl()/(4 * m_decparams.LumaBParams(2).Xbsep());
    
    int ynum_mb = m_decparams.OrigYl()/(4 * m_decparams.LumaBParams(2).Ybsep());
    
    if ( 4* xnum_mb *  m_decparams.LumaBParams(2).Xbsep() < m_decparams.OrigXl() )
        ++xnum_mb;
    
    if ( 4* ynum_mb *  m_decparams.LumaBParams(2).Ybsep() < m_decparams.OrigYl() )
        ++ynum_mb;

    m_decparams.SetXNumMB(xnum_mb);
    m_decparams.SetYNumMB(ynum_mb);

    // Set the number of blocks
    m_decparams.SetXNumBlocks(4*xnum_mb);
    m_decparams.SetYNumBlocks(4*ynum_mb);
    
    // Note that we do not have an integral number of macroblocks in a picture
    // So it is possible that part of a macro-block and some blocks can fall
    // of the edge of the true picture. We need to take this into 
    // consideration while doing Motion Compensation
}

void PictureDecompressor::PushPicture(PictureBuffer &my_buffer)
{
    int xl_luma = m_decparams.OrigXl();
    int yl_luma = m_decparams.OrigYl();
      //scaling factors for chroma based on chroma format
       int x_chroma_fac,y_chroma_fac;

       //First, we need to have sufficient padding to take account of the blocksizes.
       //It's sufficient to check for chroma

       if ( m_cformat == format420 )
       {
           x_chroma_fac = 2; 
           y_chroma_fac = 2;
       }
       else if ( m_cformat == format422 )
       {
           x_chroma_fac = 2; 
           y_chroma_fac = 1;
       }
       else
       {
           x_chroma_fac = 1; 
           y_chroma_fac = 1;
       }

       int xl_chroma=xl_luma / x_chroma_fac;
       int yl_chroma=yl_luma / y_chroma_fac;


    if (!m_decparams.ZeroTransform())
    {
        // Use the transform depth to pad the picture

        //Amount of horizontal padding for Y,U and V components
        int xpad_luma,xpad_chroma;

        //Amount of vertical padding for Y,U and V components
        int ypad_luma,ypad_chroma;

        xpad_chroma = ypad_chroma = 0;
    
        // The picture dimensions must be a multiple of 2^(transform_depth)
        int tx_mul = 1<<m_decparams.TransformDepth();

        if ( xl_chroma%tx_mul != 0 )
            xpad_chroma=( ( xl_chroma/tx_mul ) + 1 )*tx_mul - xl_chroma;
        if ( yl_chroma%tx_mul != 0)
            ypad_chroma = ( ( yl_chroma/tx_mul ) + 1 )*tx_mul - yl_chroma;    

        xl_chroma += xpad_chroma;
        yl_chroma += ypad_chroma;
    
        xpad_luma = ypad_luma = 0;
    
        // The picture dimensions must be a multiple of 2^(transform_depth)
        if ( xl_luma%tx_mul != 0 )
            xpad_luma=( ( xl_luma/tx_mul ) + 1 )*tx_mul - xl_luma;
        if ( yl_luma%tx_mul != 0)
            ypad_luma = ( ( yl_luma/tx_mul ) + 1 )*tx_mul - yl_luma;    

        xl_luma += xpad_luma;
        yl_luma += ypad_luma;
    }
    else
    {
        // Use the original picture dimensions.
    }
        
    m_fparams.SetCFormat(m_cformat);

    m_fparams.SetDwtXl(xl_luma);
    m_fparams.SetDwtYl(yl_luma);

    m_fparams.SetOrigXl(m_decparams.OrigXl());
    m_fparams.SetOrigYl(m_decparams.OrigYl());
    
    m_fparams.SetDwtChromaXl(xl_chroma);
    m_fparams.SetDwtChromaYl(yl_chroma);

    m_fparams.SetLumaDepth(m_decparams.LumaDepth());
    m_fparams.SetChromaDepth(m_decparams.ChromaDepth());

    my_buffer.PushPicture(m_fparams);
}

void PictureDecompressor::DecompressMVData( std::auto_ptr<MvData>& mv_data, 
                                          PictureByteIO& picture_byteio )
{
    MvDataByteIO mvdata_byteio (picture_byteio, m_fparams, m_decparams);

    // Read in the picture prediction parameters
    mvdata_byteio.Input();

    SetMVBlocks();
    mv_data.reset(new MvData( m_decparams.XNumMB() , 
                              m_decparams.YNumMB(), m_fparams.NumRefs() ));

    // decode mv data
    if (m_decparams.Verbose())
        std::cout<<std::endl<<"Decoding motion data ...";        

    int num_bits;

    // Read in the split mode data header
    mvdata_byteio.SplitModeData()->Input();
    // Read the mode data
    num_bits = mvdata_byteio.SplitModeData()->DataBlockSize();
    SplitModeCodec smode_decoder( mvdata_byteio.SplitModeData()->DataBlock(), TOTAL_MV_CTXS);
    smode_decoder.Decompress( *(mv_data.get()) , num_bits);
    
    // Read in the prediction mode data header
    mvdata_byteio.PredModeData()->Input();
    // Read the mode data
    num_bits = mvdata_byteio.PredModeData()->DataBlockSize();
    PredModeCodec pmode_decoder( mvdata_byteio.PredModeData()->DataBlock(), TOTAL_MV_CTXS);
    pmode_decoder.Decompress( *(mv_data.get()) , num_bits);
    
    // Read in the MV1 horizontal data header
    mvdata_byteio.MV1HorizData()->Input();
    // Read the MV1 horizontal data
    num_bits = mvdata_byteio.MV1HorizData()->DataBlockSize();
    VectorElementCodec vdecoder1h( mvdata_byteio.MV1HorizData()->DataBlock(), 1, 
                                   HORIZONTAL, TOTAL_MV_CTXS);
    vdecoder1h.Decompress( *(mv_data.get()) , num_bits);
    
    // Read in the MV1 vertical data header
    mvdata_byteio.MV1VertData()->Input();
    // Read the MV1 data
    num_bits = mvdata_byteio.MV1VertData()->DataBlockSize();
    VectorElementCodec vdecoder1v( mvdata_byteio.MV1VertData()->DataBlock(), 1, 
                                   VERTICAL, TOTAL_MV_CTXS);
    vdecoder1v.Decompress( *(mv_data.get()) , num_bits);
    
    if ( (mv_data.get())->NumRefs()>1 )
    { 
        // Read in the MV2 horizontal data header
        mvdata_byteio.MV2HorizData()->Input();
        // Read the MV2 horizontal data
        num_bits = mvdata_byteio.MV2HorizData()->DataBlockSize();
        VectorElementCodec vdecoder2h( mvdata_byteio.MV2HorizData()->DataBlock(), 2, 
                                       HORIZONTAL, TOTAL_MV_CTXS);
        vdecoder2h.Decompress( *(mv_data.get()) , num_bits);
        
        // Read in the MV2 vertical data header
        mvdata_byteio.MV2VertData()->Input();
        // Read the MV2 vertical data
        num_bits = mvdata_byteio.MV2VertData()->DataBlockSize();
        VectorElementCodec vdecoder2v( mvdata_byteio.MV2VertData()->DataBlock(), 2, 
                                       VERTICAL, TOTAL_MV_CTXS);
        vdecoder2v.Decompress( *(mv_data.get()) , num_bits);
    }

    // Read in the Y DC data header
    mvdata_byteio.YDCData()->Input();
    // Read the Y DC data
    num_bits = mvdata_byteio.YDCData()->DataBlockSize();
    DCCodec ydc_decoder( mvdata_byteio.YDCData()->DataBlock(), Y_COMP, TOTAL_MV_CTXS);
    ydc_decoder.Decompress( *(mv_data.get()) , num_bits);

    // Read in the U DC data header
    mvdata_byteio.UDCData()->Input();
    // Read the U DC data
    num_bits = mvdata_byteio.UDCData()->DataBlockSize();
    DCCodec udc_decoder( mvdata_byteio.YDCData()->DataBlock(), U_COMP, TOTAL_MV_CTXS);
    udc_decoder.Decompress( *(mv_data.get()) , num_bits);
    
    // Read in the Y DC data header
    mvdata_byteio.YDCData()->Input();
    // Read the Y DC data
    num_bits = mvdata_byteio.YDCData()->DataBlockSize();
    DCCodec vdc_decoder( mvdata_byteio.VDCData()->DataBlock(), V_COMP, TOTAL_MV_CTXS);
    vdc_decoder.Decompress( *(mv_data.get()) , num_bits);
}
