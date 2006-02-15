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


#include <libdirac_encoder/seq_compress.h>
#include <libdirac_common/dirac_assertions.h>
#include <libdirac_common/golomb.h>
using namespace dirac;

SequenceCompressor::SequenceCompressor( StreamPicInput* pin , 
                                        std::ostream* outfile ,
                                        EncoderParams& encp
                                        ): 
    m_all_done(false),
    m_just_finished(true),
    m_encparams(encp),
    m_pic_in(pin),
    m_current_display_fnum(-1),
    m_current_code_fnum(0),
    m_show_fnum(-1),m_last_frame_read(-1), 
    m_delay(1),
    m_qmonitor( m_encparams , m_pic_in->GetSeqParams() ),
    m_fcoder( m_encparams )
{
    // Set up the compression of the sequence

    const SeqParams& sparams=m_pic_in->GetSeqParams();

    //TBD: put into the constructor for EncoderParams
    m_encparams.SetEntropyFactors( new EntropyCorrector(4) );
    m_encparams.SetBitsOut( new SequenceOutputManager( outfile ) );
    WriteStreamHeader();

    //We have to set up the block parameters and file padding. This needs to take into
    //account both blocks for motion compensation and also wavelet transforms

    //Amount of horizontal padding for Y,U and V components
    int xpad_luma,xpad_chroma;

    //Amount of vertical padding for Y,U and V components
    int ypad_luma,ypad_chroma;

    //scaling factors for chroma based on chroma format
    int x_chroma_fac,y_chroma_fac;    

    //First, we need to have sufficient padding to take account of the blocksizes.
    //It's sufficient to check for chroma

    
    if (sparams.CFormat() == format411)
    {
        x_chroma_fac = 4; 
        y_chroma_fac = 1;
    }
    else if (sparams.CFormat()==format420)
    {
        x_chroma_fac = 2;
        y_chroma_fac = 2;
    }
    else if (sparams.CFormat() == format422)
    {
        x_chroma_fac = 2;
        y_chroma_fac = 1;
    }
    else
    {
        x_chroma_fac = 1;
        y_chroma_fac = 1;
    }

    int xl_chroma = sparams.Xl() / x_chroma_fac;
    int yl_chroma = sparams.Yl() / y_chroma_fac;
     xpad_chroma = ypad_chroma = 0;  
       // The pic dimensions must be a multiple of 2^(transform depth). 
    // For the moment, we'll fix the transform depth to be 4, so we need 
    // divisibility by 16. In the future we'll want arbitrary transform 
    // depths. 
    if ( xl_chroma%16 != 0 )
        xpad_chroma = ( (xl_chroma/16)+1 ) *16 - xl_chroma;
    if ( yl_chroma%16 != 0 )
        ypad_chroma = ( (yl_chroma/16)+1 ) * 16 - yl_chroma;

    int xpad_chroma_len = xl_chroma+xpad_chroma;
    int ypad_chroma_len = yl_chroma+ypad_chroma;

    // Make sure we have enough macroblocks to cover the pictures
    m_encparams.SetXNumMB( xpad_chroma_len/m_encparams.ChromaBParams(0).Xbsep() );
    m_encparams.SetYNumMB( ypad_chroma_len/m_encparams.ChromaBParams(0).Ybsep() );
    if ( m_encparams.XNumMB() * m_encparams.ChromaBParams(0).Xbsep() < xpad_chroma_len )
    {
        m_encparams.SetXNumMB( m_encparams.XNumMB() + 1 );
    }

    if ( m_encparams.YNumMB() * m_encparams.ChromaBParams(0).Ybsep() < ypad_chroma_len )
    {
        m_encparams.SetYNumMB( m_encparams.YNumMB() + 1 );
    }


     xpad_luma = ypad_luma = 0;
    int xpad_len = sparams.Xl();
    int ypad_len = sparams.Yl();
       // The pic dimensions must be a multiple of 2^(transform depth). 
    // For the moment, we'll fix the transform depth to be 4, so we need 
    // divisibility by 16. In the future we'll want arbitrary transform 
    // depths. 
    if ( xpad_len%16 != 0 )
        xpad_luma = ( (xpad_len/16)+1 ) *16 - xpad_len;
    if ( ypad_len%16 != 0 )
        ypad_luma = ( (ypad_len/16)+1 ) * 16 - ypad_len;

    xpad_len += xpad_luma;
    ypad_len += ypad_luma;

    // NOTE: Do we need to recalculate the number of Macro blocks!!!

    // Note that we do not have an integral number of macroblocks in a picture
    // So it is possible that part of a macro-block and some blocks can fall
    // of the edge of the padded picture. We need to take this into 
    // consideration while doing Motion Estimation
    m_encparams.SetXNumBlocks( 4 * m_encparams.XNumMB() );
    m_encparams.SetYNumBlocks( 4 * m_encparams.YNumMB() );

    // NOTE: Do we still need this set padding function
    //Set the resulting padding values
    m_pic_in->SetPadding(xpad_luma,ypad_luma);

    // Set up the frame buffers with the PADDED picture sizes
    m_fbuffer = new FrameBuffer( sparams.CFormat() , m_encparams.NumL1() , m_encparams.L1Sep() , 
            xpad_len , ypad_len, xpad_chroma_len, ypad_chroma_len);

    m_origbuffer = new FrameBuffer( sparams.CFormat() , m_encparams.NumL1() , m_encparams.L1Sep() , 
            xpad_len, ypad_len, xpad_chroma_len, ypad_chroma_len);
}

SequenceCompressor::~SequenceCompressor()
{

    if ( m_encparams.Verbose())
        MakeSequenceReport();

    //TBD: put into the destructor for EncoderParams 
    delete &m_encparams.BitsOut();
    delete &m_encparams.EntropyFactors();

    delete m_fbuffer;
    delete m_origbuffer;
}

bool SequenceCompressor::LoadNextFrame()
{
    m_fbuffer->PushFrame( m_pic_in , m_last_frame_read+1 );

    if ( m_pic_in->End() )
    {
        m_all_done = true;
        return false;
    }
    m_last_frame_read++;
    m_origbuffer->PushFrame( m_fbuffer->GetFrame( m_last_frame_read ) );
    return true;
}

Frame& SequenceCompressor::CompressNextFrame()
{

    // This function codes the next frame in coding order and returns the next frame in display order
    // In general these will differ, and because of re-ordering there is a m_delay which needs to be imposed.
    // This creates problems at the start and at the end of the sequence which must be dealt with.
    // At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
    // the frames out. It's up to the calling function to do something with the decoded frames as they
    // come out - write them to screen or to file, or whatever. TJD 13Feb04.

    // current_fnum is the number of the current frame being coded in display order
    // m_current_code_fnum is the number of the current frame in coding order. This function increments
    // m_current_code_fnum by 1 each time and works out what the number is in display order.
    // m_show_fnum is the index of the frame number that can be shown when current_fnum has been coded.
    // Var m_delay is the m_delay caused by reordering (as distinct from buffering)


    TESTM (m_last_frame_read >= 0, "Data loaded before calling CompressNextFrame");
    m_current_display_fnum = CodedToDisplay( m_current_code_fnum );

    // If we're not at the beginning, clean the buffer
    if ( m_current_code_fnum != 0 )
    {
        m_fbuffer->Clean( m_show_fnum );
        m_origbuffer->Clean( m_show_fnum );
    }

    m_show_fnum = std::max( m_current_code_fnum - m_delay , 0 );

    bool can_encode = false;

    if (m_last_frame_read >= m_current_display_fnum )
        can_encode = true;

    if ( can_encode )
    {   // We haven't coded everything, so compress the next frame

        if ( m_encparams.Verbose() )
        {
             std::cerr<<std::endl<<std::endl<<"Compressing frame "<<m_current_code_fnum<<", ";
            std::cerr<<m_current_display_fnum<<" in display order";
        }
 
    
        // Compress the frame//
        ///////////////////////

        m_fcoder.Compress( *m_fbuffer , *m_origbuffer , m_current_display_fnum );

       // Measure the encoded frame quality
       if ( m_encparams.LocalDecode() )
           m_qmonitor.UpdateModel( m_fbuffer->GetFrame( m_current_display_fnum ) , 
                                   m_origbuffer->GetFrame( m_current_display_fnum ) );

       // Finish by writing the compressed data out to file ...
       m_encparams.BitsOut().WriteFrameData();

       if ( m_encparams.Verbose() )
       {
           MakeFrameReport();
       }
    
        // Increment our position
        m_current_code_fnum++;

    }

    // Return the latest frame that can be shown
    return m_fbuffer->GetFrame(m_show_fnum);
}

const Frame *SequenceCompressor::GetFrameEncoded()
{
    if (m_current_display_fnum >= 0)
        return &m_fbuffer->GetFrame( m_current_display_fnum );

    return 0;
}

const MEData *SequenceCompressor::GetMEData()
{
    if ( m_fcoder.IsMEDataAvail())
        return m_fcoder.GetMEData();

    return 0;
}
void SequenceCompressor::EndSequence()
{
    if (m_just_finished)
    {
        //Write end of sequence
        unsigned char seq_end[5] = { START_CODE_PREFIX_BYTE0, 
                                     START_CODE_PREFIX_BYTE1, 
                                     START_CODE_PREFIX_BYTE2, 
                                     START_CODE_PREFIX_BYTE3, 
                                     SEQ_END_CODE };
        m_encparams.BitsOut().TrailerOutput().OutputBytes((char *)seq_end, 5);
        m_encparams.BitsOut().WriteSeqTrailerToFile();
        m_just_finished = false;
        m_all_done = true;
    }
}



void SequenceCompressor::MakeSequenceReport()
{

    std::cerr<<"Total bits for sequence="<<m_encparams.BitsOut().SequenceBytes() * 8;
    std::cerr<<" ( "<<m_encparams.BitsOut().SequenceHeadBytes() * 8<<" header )";
    
    std::cerr<<std::endl<<"Of these: "<<std::endl<<std::endl;
    std::cerr<<m_encparams.BitsOut().ComponentBytes( Y_COMP ) * 8<<" were Y, ";
    std::cerr<<std::endl<<m_encparams.BitsOut().ComponentBytes( U_COMP ) * 8<<" were U, ";
    std::cerr<<std::endl<<m_encparams.BitsOut().ComponentBytes( V_COMP ) * 8<<" were V, and ";
    std::cerr<<std::endl<<m_encparams.BitsOut().MVBytes() * 8<<" were motion vector data.";

    if ( m_encparams.LocalDecode() )
        m_qmonitor.WriteLog();

    std::cerr<<std::endl;

}

void SequenceCompressor::MakeFrameReport()
{
    // Write out to screen a report of the number of bits written
    const FrameOutputManager& foutput = m_encparams.BitsOut().FrameOutput();

    unsigned int unit_bits = foutput.MVBytes() * 8;            
    unsigned int unit_head_bits = foutput.MVHeadBytes() * 8;

    std::cerr<<std::endl<<"Number of MV bits="<<unit_bits;
    std::cerr<<" ( "<<unit_head_bits<<" header bits)";

    unit_bits = foutput.ComponentBytes( Y_COMP ) * 8;
    unit_head_bits = foutput.ComponentHeadBytes( Y_COMP ) * 8;

    std::cerr<<std::endl<<"Number of bits for Y="<<unit_bits;
    std::cerr<<" ( "<<unit_head_bits<<" header bits)";

    unit_bits = foutput.ComponentBytes( U_COMP ) * 8;
    unit_head_bits = foutput.ComponentHeadBytes( U_COMP ) * 8;

    std::cerr<<std::endl<<"Number of bits for U="<<unit_bits;
    std::cerr<<" ( "<<unit_head_bits<<" header bits)";

    unit_bits = foutput.ComponentBytes( V_COMP ) * 8;
    unit_head_bits = foutput.ComponentHeadBytes( V_COMP ) * 8;

    std::cerr<<std::endl<<"Number of bits for V="<<unit_bits;
    std::cerr<<" ( "<<unit_head_bits<<" header bits)";

    unit_bits = foutput.FrameBytes() * 8;
    unit_head_bits = foutput.FrameHeadBytes() * 8;

    std::cerr<<std::endl<<std::endl<<"Total frame bits="<<unit_bits;
    std::cerr<<" ( "<<unit_head_bits<<" header bits)"<<std::endl<<std::endl;

}


void SequenceCompressor::WriteStreamHeader()
{
     // Write out all the header data
    BasicOutputManager& stream_header = m_encparams.BitsOut().HeaderOutput();

        // Begin with the ID of the codec
     stream_header.OutputBytes("KW-DIRAC");
    
    unsigned char seq_start[5] = { START_CODE_PREFIX_BYTE0,
                                   START_CODE_PREFIX_BYTE1,
                                   START_CODE_PREFIX_BYTE2,
                                   START_CODE_PREFIX_BYTE3,
                                   RAP_START_CODE };
    
    stream_header.OutputBytes((char *)seq_start, 5);

    // bit stream version
    stream_header.OutputByte((char)BITSTREAM_VERSION);

        // Picture dimensions
     UnsignedGolombCode( stream_header ,(unsigned int) m_pic_in->GetSeqParams().Xl() );
     UnsignedGolombCode( stream_header ,(unsigned int) m_pic_in->GetSeqParams().Yl() );

     // Picture rate
     UnsignedGolombCode( stream_header , (unsigned int) m_pic_in->GetSeqParams().FrameRate());

     // Block parameters
     UnsignedGolombCode( stream_header ,(unsigned int) m_encparams.LumaBParams(2).Xblen() );
     UnsignedGolombCode( stream_header ,(unsigned int) m_encparams.LumaBParams(2).Yblen() );
     UnsignedGolombCode( stream_header ,(unsigned int) m_encparams.LumaBParams(2).Xbsep() );
     UnsignedGolombCode( stream_header ,(unsigned int) m_encparams.LumaBParams(2).Ybsep() );

     // Also send the number of blocks horizontally and vertically
     UnsignedGolombCode( stream_header ,(unsigned int) m_encparams.XNumBlocks() );
     UnsignedGolombCode( stream_header ,(unsigned int) m_encparams.YNumBlocks() );

     // Chroma format
     UnsignedGolombCode( stream_header ,(unsigned int) m_pic_in->GetSeqParams().CFormat() );

     // Interlace marker
     stream_header.OutputBit(m_pic_in->GetSeqParams().Interlace() );

     m_encparams.BitsOut().WriteSeqHeaderToFile();
}

int SequenceCompressor::CodedToDisplay( const int fnum )
{
    int div;

    if (m_encparams.L1Sep()>0)
    {
        // We have L1 and L2 frames
        if (fnum==0)
            return 0;
        else if ((fnum-1)% m_encparams.L1Sep()==0)
        {//we have L1 or subsequent I frames
            div=(fnum-1)/m_encparams.L1Sep();
            return fnum+m_encparams.L1Sep()-1;
        }
        else//we have L2 frames
            return fnum-1;
    }
    else
    {//we just have I-frames, so no re-ordering
        return fnum;
    }
}
