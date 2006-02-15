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


///////////////////////////////////////////
//---------------------------------------//
//Class to manage decompressing sequences//
//---------------------------------------//
///////////////////////////////////////////

#include <libdirac_common/dirac_assertions.h>
#include <libdirac_decoder/seq_decompress.h>
#include <libdirac_common/common.h>
#include <libdirac_common/golomb.h>
#include <libdirac_common/frame_buffer.h>
#include <libdirac_decoder/frame_decompress.h>
using namespace dirac;

SequenceDecompressor::SequenceDecompressor(std::istream* ip,bool verbosity)
: 
m_all_done(false),
m_infile(ip),
m_current_code_fnum(0),
m_delay(1),
m_last_frame_read(-1),
m_show_fnum(-1)
{
    m_decparams.SetBitsIn( new BitInputManager(m_infile) );
    m_decparams.SetVerbose( verbosity );
    ReadStreamHeader();

    //Amount of horizontal padding for Y,U and V components
    int xpad_luma,xpad_chroma;

    //Amount of vertical padding for Y,U and V components
    int ypad_luma,ypad_chroma;

    //scaling factors for chroma based on chroma format
    int x_chroma_fac,y_chroma_fac;

    //First, we need to have sufficient padding to take account of the blocksizes.
    //It's sufficient to check for chroma

    if ( m_sparams.CFormat() == format411 )
    {
        x_chroma_fac = 4; 
        y_chroma_fac = 1;
    }
    else if ( m_sparams.CFormat() == format420 )
    {
        x_chroma_fac = 2; 
        y_chroma_fac = 2;
    }
    else if ( m_sparams.CFormat() == format422 )
    {
        x_chroma_fac = 2; 
        y_chroma_fac = 1;
    }
    else
    {
        x_chroma_fac = 1; 
        y_chroma_fac = 1;
    }

    int xl_chroma=m_sparams.Xl() / x_chroma_fac;
    int yl_chroma=m_sparams.Yl() / y_chroma_fac;

    xpad_chroma = ypad_chroma = 0;
    
    // The frame dimensions must be a multiple of 2^(transform_depth)
    // For the moment, we'll fix the transform depth to be 4, so we need 
    // divisibility by 16. In the future we'll want arbitrary transform 
    // depths.

    if ( xl_chroma%16 != 0 )
        xpad_chroma=( ( xl_chroma/16 ) + 1 )*16 - xl_chroma;
    if ( yl_chroma%16 != 0)
        ypad_chroma = ( ( yl_chroma/16 ) + 1 )*16 - yl_chroma;    

    int xpad_chroma_len = xl_chroma+xpad_chroma;
    int ypad_chroma_len = yl_chroma+ypad_chroma;
    

    //make sure we have enough macroblocks to cover the pictures 
    m_decparams.SetXNumMB( xpad_chroma_len / m_decparams.ChromaBParams(0).Xbsep() );
    m_decparams.SetYNumMB( ypad_chroma_len / m_decparams.ChromaBParams(0).Ybsep() );
    if ( m_decparams.XNumMB() * m_decparams.ChromaBParams(0).Xbsep() < xpad_chroma_len )
    {
        m_decparams.SetXNumMB( m_decparams.XNumMB() + 1 );
    }

    if (m_decparams.YNumMB()*m_decparams.ChromaBParams(0).Ybsep()< ypad_chroma_len)
    {
        m_decparams.SetYNumMB( m_decparams.YNumMB() + 1 );
    }

    int xpad_len = m_sparams.Xl();
    int ypad_len = m_sparams.Yl();
    xpad_luma = ypad_luma = 0;
    
    // The frame dimensions must be a multiple of 2^(transform_depth)
    // For the moment, we'll fix the transform depth to be 4, so we need 
    // divisibility by 16. In the future we'll want arbitrary transform 
    // depths.

    if ( xpad_len%16 != 0 )
        xpad_luma=( ( xpad_len/16 ) + 1 )*16 - xpad_len;
    if ( ypad_len%16 != 0)
        ypad_luma = ( ( ypad_len/16 ) + 1 )*16 - ypad_len;    

    xpad_len += xpad_luma;
    ypad_len += ypad_luma;

    // NOTE: do we need to recalulate the number of MacroBlocks here???

    // Note that we do not have an integral number of macroblocks in a picture
    // So it is possible that part of a macro-block and some blocks can fall
    // of the edge of the true picture. We need to take this into 
    // consideration while doing Motion Compensation
    m_decparams.SetXNumBlocks( 4*m_decparams.XNumMB() );
    m_decparams.SetYNumBlocks( 4*m_decparams.YNumMB() );


    //set up padded picture sizes, based on original picture sizes, the block parameters and the wavelet transform depth
    m_fbuffer= new FrameBuffer( m_sparams.CFormat() , xpad_len, ypad_len, xpad_chroma_len, ypad_chroma_len );

    m_fdecoder = new FrameDecompressor (m_decparams , m_sparams.CFormat() );
}

SequenceDecompressor::~SequenceDecompressor()
{
    delete m_fbuffer;
    delete m_fdecoder;
    delete &m_decparams.BitsIn();
}

bool SequenceDecompressor::ReadNextFrameHeader()
{
    return m_fdecoder->ReadFrameHeader(*m_fbuffer);
}

const FrameParams& SequenceDecompressor::GetNextFrameParams() const
{
    return m_fdecoder->GetFrameParams();
}

Frame& SequenceDecompressor::DecompressNextFrame(bool skip /* = false */)
{
    //this function decodes the next frame in coding order and returns the next frame in display order
    //In general these will differ, and because of re-ordering there is a m_delay which needs to be imposed.
    //This creates problems at the start and at the end of the sequence which must be dealt with.
    //At the start we just keep outputting frame 0. At the end you will need to loop for longer to get all
    //the frames out. It's up to the calling function to do something with the decoded frames as they
    //come out - write them to screen or to file, as required.

    TEST (m_fdecoder != NULL);

    if (m_current_code_fnum!=0){
        //if we're not at the beginning, clean the buffer of frames that can be discarded
        m_fbuffer->Clean(m_show_fnum);
    }

    bool new_frame_to_display=false;
       
    if (!skip)
       new_frame_to_display = m_fdecoder->Decompress(*m_fbuffer);

    //if we've exited with success, there's a new frame to display, so increment
    //the counters. Otherwise, freeze on the last frame shown
    m_show_fnum=std::max(m_current_code_fnum-m_delay,0);
    if (new_frame_to_display || skip)
    {
        m_current_code_fnum++;
    }

    return m_fbuffer->GetFrame(m_show_fnum);
}

Frame& SequenceDecompressor::GetNextFrame()
{
    return m_fbuffer->GetFrame(m_show_fnum);
}

void SequenceDecompressor::ReadStreamHeader()
{    //called from constructor

    //read the stream header parameters
    //begin with the identifying string
    OLBParams bparams;
    //char kwname[9];
    //for (int i=0; i<8; ++i)
    //{
    //    kwname[i]=m_decparams.BitsIn().InputByte();    
    //}    
    //kwname[8]='\0';

    char seq_start[5];
    for (int i=0;i<5;++i)
    {
        seq_start[i]=m_decparams.BitsIn().InputByte();    
    }    
    //TBC: test that kwname="KW-DIRAC"    

    //bit stream version
    m_sparams.SetBitstreamVersion( m_decparams.BitsIn().InputByte() );

    // TODO: test if this bit stream version is supported. Report an error
    // otherwise
    TESTM (m_sparams.BitstreamVersion() == BITSTREAM_VERSION, "Bitstream version match");

    //picture dimensions
    m_sparams.SetXl( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );
    m_sparams.SetYl( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );    

    //picture rate
    m_sparams.SetFrameRate( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );

     //block parameters
    bparams.SetXblen( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );
    bparams.SetYblen( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );    
    bparams.SetXbsep( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );
    bparams.SetYbsep( int(UnsignedGolombDecode( m_decparams.BitsIn() )) );

    //dimensions of block arrays (remember there may need to be padding for some block and picture sizes)
    m_decparams.SetXNumBlocks( int(UnsignedGolombDecode( m_decparams.BitsIn())) );
    m_decparams.SetYNumBlocks( int(UnsignedGolombDecode( m_decparams.BitsIn())) );
    m_decparams.SetXNumMB( m_decparams.XNumBlocks()/4 );
    m_decparams.SetYNumMB( m_decparams.YNumBlocks()/4 );

    //Set the original picture dimensions
    m_decparams.SetOrigXl(m_sparams.Xl());
    m_decparams.SetOrigYl(m_sparams.Yl());

    //chroma format
    m_sparams.SetCFormat( ChromaFormat(UnsignedGolombDecode( m_decparams.BitsIn())) );        
    m_decparams.SetBlockSizes( bparams , m_sparams.CFormat() );

     //interlace marker
    m_decparams.SetInterlace( m_decparams.BitsIn().InputBit() );
    m_sparams.SetInterlace( m_decparams.Interlace() );

    //Flush the input to the end of the header
    m_decparams.BitsIn().FlushInput();    
}
