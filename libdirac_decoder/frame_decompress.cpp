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


//Decompression of frames
/////////////////////////

#include <libdirac_common/dirac_assertions.h>
#include <libdirac_decoder/frame_decompress.h>
#include <libdirac_decoder/comp_decompress.h>
#include <libdirac_common/mot_comp.h>
#include <libdirac_common/mv_codec.h>
#include <libdirac_byteio/frame_byteio.h>
#include <libdirac_common/dirac_exception.h>
using namespace dirac;

#include <iostream>
#include <memory>

using std::vector;
using std::auto_ptr;

FrameDecompressor::FrameDecompressor(DecoderParams& decp, ChromaFormat cf, unsigned int vd)
: 
m_decparams(decp),
m_cformat(cf),
m_video_depth(vd)
{
     
    
     

}

FrameDecompressor::~FrameDecompressor()
{
}


bool FrameDecompressor::Decompress(ParseUnitByteIO& parseunit_byteio,
                                   FrameBuffer& my_buffer,
                                   int au_fnum)
{
    // get current byte position
    //int start_pos = parseunit_byteio.GetReadBytePosition();
    try {

    // read frame data
    FrameByteIO frame_byteio(m_fparams,
                             parseunit_byteio,
                             au_fnum);
    frame_byteio.Input();

    // Do frame buffer cleaning
    std::vector<int>& retd_list = m_fparams.RetiredFrames();
    for (size_t i = 0; i < retd_list.size(); ++i)
        my_buffer.Clean(retd_list[i]);

    FrameSort fs;
    
    if (m_fparams.GetFrameType() == INTRA_FRAME)
        fs.SetIntra();
    else
        fs.SetInter();
    
    if (m_fparams.GetReferenceType() == REFERENCE_FRAME)
        fs.SetRef();
    else
        fs.SetNonRef();

    m_fparams.SetFSort(fs);
        

    // Check if the frame can be decoded
    if (m_fparams.FSort().IsInter())
    {
        const std::vector<int>& refs = m_fparams.Refs();
        for (unsigned int i = 0; i < refs.size(); ++i)
        {
            const Frame &ref_frame = my_buffer.GetFrame(refs[i]);
            if (ref_frame.GetFparams().FrameNum() != refs[i])
                return false;
        }
    }

    m_skipped=false;
    if ( !m_skipped )
    {//if we're not m_skipped then we can decode the rest of the frame

       if ( m_decparams.Verbose() )
             std::cerr<<std::endl<<"Decoding frame "<<m_fparams.FrameNum()<<" in display order";        

       FrameSort fsort = m_fparams.FSort();
       auto_ptr<MvData> mv_data;
       unsigned int num_mv_bits;

       if ( fsort.IsInter() )
       {//do all the MV stuff 
            MvDataByteIO mvdata_byteio (frame_byteio, m_fparams, m_decparams);
            // Read in the frame prediction parameters
            mvdata_byteio.Input();
            num_mv_bits = mvdata_byteio.BlockDataSize();

            SetMVBlocks();
            mv_data.reset(new MvData( m_decparams.XNumMB() , m_decparams.YNumMB(), m_fparams.NumRefs() ));
            //decode mv data
            if (m_decparams.Verbose())
                std::cerr<<std::endl<<"Decoding motion data ...";        
            MvDataCodec my_mv_decoder( mvdata_byteio.BlockData(), TOTAL_MV_CTXS , m_cformat );
            my_mv_decoder.InitContexts();//may not be necessary

            //Decompress the MV bits
            my_mv_decoder.Decompress( *(mv_data.get()) , num_mv_bits );                
        }
           
        // Read the  transform header
        TransformByteIO transform_byteio(frame_byteio, m_fparams, m_decparams);
        transform_byteio.Input();
        
        if (m_fparams.FSort().IsIntra() && m_decparams.ZeroTransform())
        {
            DIRAC_THROW_EXCEPTION(
                ERR_UNSUPPORTED_STREAM_DATA,
                "Intra frames cannot have Zero-Residual",
                SEVERITY_FRAME_ERROR);
        }
            
        // Set frame dimensions based on the transform depth. If zero residual
        // then use the actual picture dimensions
        PushFrame(my_buffer);

        Frame& my_frame = my_buffer.GetFrame(m_fparams.FrameNum());//Reference to the frame being decoded

        if (!m_decparams.ZeroTransform())
        {
            //decode components
            CompDecompress( &transform_byteio, my_buffer,m_fparams.FrameNum() , Y_COMP );
            CompDecompress( &transform_byteio, my_buffer , m_fparams.FrameNum() , U_COMP );        
            CompDecompress( &transform_byteio, my_buffer , m_fparams.FrameNum() , V_COMP );
        }

        if ( fsort.IsInter() )
        //motion compensate to add the data back in if we don't have an I frame
            MotionCompensator::CompensateFrame( m_decparams , ADD , 
                                                my_buffer , m_fparams.FrameNum() ,
                                                *(mv_data.get()) );
        my_frame.Clip();

        if (m_decparams.Verbose())
            std::cerr<<std::endl;        

        }//?m_skipped,!End()
        else if (m_skipped){
         //TBD: decide what to return if we're m_skipped. Nearest frame in temporal order??    

        }

         //exit success
        return true;
    }// try
    catch (const DiracException& e) {
        // skip frame
        throw e;
    }

     //exit failure
    return false;
}

void FrameDecompressor::CompDecompress(TransformByteIO *p_transform_byteio,
                                       FrameBuffer& my_buffer, int fnum,CompSort cs)
{
    if ( m_decparams.Verbose() )
        std::cerr<<std::endl<<"Decoding component data ...";
    
    ComponentByteIO component_byteio(cs, *p_transform_byteio);
    CompDecompressor my_compdecoder( m_decparams , my_buffer.GetFrame(fnum).GetFparams() );    
    PicArray& comp_data=my_buffer.GetComponent( fnum , cs );
    my_compdecoder.Decompress(&component_byteio, 
                              comp_data );
}

void FrameDecompressor::SetMVBlocks()
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

void FrameDecompressor::PushFrame(FrameBuffer &my_buffer)
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
        // Use the transform depth to pad the frame

        //Amount of horizontal padding for Y,U and V components
        int xpad_luma,xpad_chroma;

        //Amount of vertical padding for Y,U and V components
        int ypad_luma,ypad_chroma;

        xpad_chroma = ypad_chroma = 0;
    
        // The frame dimensions must be a multiple of 2^(transform_depth)
        int tx_mul = 1<<m_decparams.TransformDepth();

        if ( xl_chroma%tx_mul != 0 )
            xpad_chroma=( ( xl_chroma/tx_mul ) + 1 )*tx_mul - xl_chroma;
        if ( yl_chroma%tx_mul != 0)
            ypad_chroma = ( ( yl_chroma/tx_mul ) + 1 )*tx_mul - yl_chroma;    

        xl_chroma += xpad_chroma;
        yl_chroma += ypad_chroma;
    
        xpad_luma = ypad_luma = 0;
    
        // The frame dimensions must be a multiple of 2^(transform_depth)
        if ( xl_luma%tx_mul != 0 )
            xpad_luma=( ( xl_luma/tx_mul ) + 1 )*tx_mul - xl_luma;
        if ( yl_luma%tx_mul != 0)
            ypad_luma = ( ( yl_luma/tx_mul ) + 1 )*tx_mul - yl_luma;    

        xl_luma += xpad_luma;
        yl_luma += ypad_luma;
    }
    else
    {
        // Use the original frame dimensions.
    }
        
    m_fparams.SetCFormat(m_cformat);

    m_fparams.SetXl(xl_luma);
    m_fparams.SetYl(yl_luma);

    m_fparams.SetChromaXl(xl_chroma);
    m_fparams.SetChromaYl(yl_chroma);

    m_fparams.SetVideoDepth(m_video_depth);

    my_buffer.PushFrame(m_fparams);
}
