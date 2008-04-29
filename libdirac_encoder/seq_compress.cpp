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
*                 Andrew Kennedy
*                 Myo Tun (Brunel University, myo.tun@brunel.ac.uk)
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

using namespace dirac;

SequenceCompressor::SequenceCompressor( StreamPicInput* pin ,
                                        EncoderParams& encp,
                                        DiracByteStream& dirac_byte_stream):
    m_all_done(false),
    m_just_finished(true),
    m_srcparams(pin->GetSourceParams()),
    m_encparams(encp),
    m_pic_in(pin),
    m_current_display_pnum(-1),
    m_current_code_pnum(0),
    m_show_pnum(-1),m_last_picture_read(-1),
    m_delay(1),
    m_qmonitor( m_encparams ),
    m_pcoder( m_encparams ),
    m_dirac_byte_stream(dirac_byte_stream),
    m_eos_signalled(false)
{
    // Set up the compression of the sequence

    //TBD: put into the constructor for EncoderParams
    m_encparams.SetEntropyFactors( new EntropyCorrector(m_encparams.TransformDepth()) );

    int xl_chroma = m_encparams.ChromaXl();
    int yl_chroma = m_encparams.ChromaYl();

    // Make sure we have enough macroblocks to cover the pictures
    m_encparams.SetXNumMB( xl_chroma/m_encparams.ChromaBParams(0).Xbsep() );
    m_encparams.SetYNumMB( yl_chroma/m_encparams.ChromaBParams(0).Ybsep() );
    
    if ( m_encparams.XNumMB() * m_encparams.ChromaBParams(0).Xbsep() < xl_chroma )
        m_encparams.SetXNumMB( m_encparams.XNumMB() + 1 );
    if ( m_encparams.YNumMB() * m_encparams.ChromaBParams(0).Ybsep() < yl_chroma )
        m_encparams.SetYNumMB( m_encparams.YNumMB() + 1 );

    m_encparams.SetXNumBlocks( 4 * m_encparams.XNumMB() );
    m_encparams.SetYNumBlocks( 4 * m_encparams.YNumMB() );

    // Set up the picture buffers
    m_fbuffer = new PictureBuffer( m_srcparams.CFormat() ,
                                 m_encparams.NumL1() , m_encparams.L1Sep() ,
                                 m_encparams.Xl(), m_encparams.Yl(),
                                 m_encparams.LumaDepth(),
                                 m_encparams.ChromaDepth(),
                                 m_encparams.FieldCoding(),
                                 m_encparams.UsingAC());

    // Retain the original pictures the motion estimation buffer
    m_mebuffer = new PictureBuffer( m_srcparams.CFormat() ,
                                    m_encparams.NumL1() , m_encparams.L1Sep() ,
                                    m_encparams.Xl(), m_encparams.Yl(),
                                    m_encparams.LumaDepth(),
                                    m_encparams.ChromaDepth(),
                                    m_encparams.FieldCoding(),
                                    m_encparams.UsingAC());

    // Set up a rate controller if rate control being used
    if (m_encparams.TargetRate() != 0)
        m_ratecontrol = new RateController(m_encparams.TargetRate(),
                                           m_pic_in->GetSourceParams(), encp);
}

SequenceCompressor::~SequenceCompressor()
{

    if ( m_encparams.Verbose())
        MakeSequenceReport();

    //TBD: put into the destructor for EncoderParams
    delete &m_encparams.EntropyFactors();

    delete m_fbuffer;
    delete m_mebuffer;

    if (m_encparams.TargetRate()!=0)
        delete m_ratecontrol;
}

bool SequenceCompressor::CanEncode()
{
    if (m_eos_signalled)
    {
        /*
        * Encode the remaining picture in the frame buffer. We check if the
        * reference pictures are available and modify the picture sort
        * accordingly.
        */
        if (m_current_code_pnum <= m_last_picture_read)
        {
            m_current_display_pnum = m_current_code_pnum;
            Picture& fbuf_picture = m_fbuffer->GetPicture( m_current_display_pnum );
            PictureParams& fbuf_pparams = fbuf_picture.GetPparams();
            Picture& me_picture = m_mebuffer->GetPicture( m_current_display_pnum );
            PictureParams& me_pparams = me_picture.GetPparams();
            // If INTRA pic, no further checks necessary
            if (fbuf_pparams.PicSort().IsIntra())
                return true;

            // Bit of a hack since same frame info is available in two different
            // buffers - decoded pic buffer and motion estimation buffer.
            std::vector<int>& fbuf_refs = fbuf_pparams.Refs();
            std::vector<int>& me_refs = me_pparams.Refs();
            int num_refs = 0;
            if (m_fbuffer->IsPictureAvail(fbuf_refs[0]))
                ++num_refs;
            if (fbuf_refs.size() > 1)
            {
                if (m_fbuffer->IsPictureAvail(fbuf_refs[1]))
                {
                    if (num_refs == 0)
                    {
                        fbuf_refs[0] = fbuf_refs[1];
                        me_refs[0] = me_refs[1];
                    }
                    ++num_refs;
                }
            }
            fbuf_refs.resize(num_refs);
            me_refs.resize(num_refs);

            if (fbuf_refs.size() == 0)
            {
                fbuf_picture.SetPictureSort (PictureSort::IntraNonRefPictureSort());
                me_picture.SetPictureSort (PictureSort::IntraNonRefPictureSort());
            }

            return true;
        }
    }
    else
    {
        if (m_last_picture_read >= m_current_display_pnum)
            return true;
    }
    return false;
}

Picture& SequenceCompressor::CompressNextPicture()
{

    // This function codes the next picture in coding order and returns the next picture in display order
    // In general these will differ, and because of re-ordering there is a m_delay which needs to be imposed.
    // This creates problems at the start and at the end of the sequence which must be dealt with.
    // At the start we just keep outputting picture 0. At the end you will need to loop for longer to get all
    // the pictures out. It's up to the calling function to do something with the decoded pictures as they
    // come out - write them to screen or to file, or whatever. TJD 13Feb04.

    // current_pnum is the number of the current picture being coded in display order
    // m_current_code_pnum is the number of the current picture in coding order. This function increments
    // m_current_code_pnum by 1 each time and works out what the number is in display order.
    // m_show_pnum is the index of the picture number that can be shown when current_pnum has been coded.
    // Var m_delay is the m_delay caused by reordering (as distinct from buffering)

    TESTM (m_last_picture_read >= 0, "Data loaded before calling CompressNextPicture");
    m_current_display_pnum = CodedToDisplay( m_current_code_pnum );

    m_show_pnum = std::max( m_current_code_pnum - m_delay , 0 );

    if ( CanEncode() )
    {   // We haven't coded everything, so compress the next picture

        // stream access-unit data if first picture in unit
        if(IsNewAccessUnit())
        {
            // The access unit picture number must be equal to the picture
            // number in display order of the I_Picture that follows it and
            // not the coded order.
            AccessUnitByteIO *p_accessunit_byteio = new AccessUnitByteIO
                                        (
                                            m_pic_in->GetSourceParams(),
                                            m_encparams
                                        );
            p_accessunit_byteio->Output();

            // add the unit to the byte stream
            m_dirac_byte_stream.AddAccessUnit(p_accessunit_byteio);

        }

        if ( m_encparams.Verbose() )
        {
            if (m_encparams.TargetRate()!=0 )
                m_ratecontrol->Report();

            if (m_encparams.FieldCoding())
                std::cout<<std::endl<<std::endl<<"Compressing field "<<m_current_code_pnum<<", ";
            else
                std::cout<<std::endl<<std::endl<<"Compressing frame "<<m_current_code_pnum<<", ";
            std::cout<<m_current_display_pnum<<" in display order";


        }


        // Compress the picture//
        ///////////////////////



        Picture& my_picture = m_fbuffer->GetPicture( m_current_display_pnum );

        PictureParams& pparams = my_picture.GetPparams();
        
        if (pparams.PicSort().IsRef())
            m_fbuffer->SetRetiredPictureNum( m_show_pnum, m_current_display_pnum );

        // Do motion estimation using the original (not reconstructed) data
        if (m_encparams.Verbose() && my_picture.GetPparams().PicSort().IsInter())
        {
            std::cout<<std::endl<<"References "
                     << (m_encparams.FieldCoding() ? "field " : "frame ")
                     << pparams.Refs()[0];
            if (pparams.Refs().size() > 1)
            {
                std::cout<<" and "<< pparams.Refs()[1];
            }
        }
        bool is_a_cut( false );
        if ( my_picture.GetPparams().PicSort().IsInter() )
        {
            is_a_cut = m_pcoder.MotionEstimate(  *m_mebuffer,
                                                m_current_display_pnum );
            if ( is_a_cut )
            {
                // Set the picture type to intra
                if (my_picture.GetPparams().PicSort().IsRef())
                    my_picture.SetPictureSort (PictureSort::IntraRefPictureSort());
                else
                    my_picture.SetPictureSort (PictureSort::IntraNonRefPictureSort());

                if ( m_encparams.Verbose() )
                    std::cout<<std::endl<<"Cut detected and I-picture inserted!";
            }
        }


        // Now code the residual data
        if (m_encparams.TargetRate() == 0)
        {
            PictureByteIO *p_picture_byteio;
            // Coding Without using Rate Control Algorithm
            p_picture_byteio =  m_pcoder.Compress(*m_fbuffer ,
                                         m_current_display_pnum);

            // add the picture to the byte stream

            m_dirac_byte_stream.AddPicture(p_picture_byteio);
        }
        else
        {
            RateControlCompress(my_picture, is_a_cut);
        }

       // Measure the encoded picture quality
       if ( m_encparams.LocalDecode() )
       {
           const Picture &orig_picture = OriginalPicture( m_current_display_pnum );
           if (m_current_display_pnum != orig_picture.GetPparams().PictureNum())
           {
               std::cerr << "Error in picture buffer:"
                         << " Requested : " << m_current_display_pnum
                         << "  Retrieved : " << orig_picture.GetPparams().PictureNum() << std::endl;
           }
           m_qmonitor.UpdateModel(
               m_fbuffer->GetPicture( m_current_display_pnum ) ,
               OriginalPicture(m_current_display_pnum) );
       }
        // Increment our position
        m_current_code_pnum++;

       CleanBuffers();

    }

    // Return the latest picture that can be shown
    if ( m_encparams.Verbose() )
    {
           std::cout<<std::endl<<"Return " <<
                 (m_encparams.FieldCoding() ? "field " : "frame ")  <<
                  m_show_pnum << " in display order";
    }
    return m_fbuffer->GetPicture(m_show_pnum );
}

void SequenceCompressor::CleanBuffers()
{
    // If we're not at the beginning, clean the buffer
    if ( m_current_code_pnum != 0 )
    {
        m_fbuffer->CleanRetired( m_show_pnum, m_current_display_pnum );
        m_mebuffer->CleanAll( m_show_pnum, m_current_display_pnum );
    }
}

const Picture *SequenceCompressor::GetPictureEncoded()
{
    if (m_current_display_pnum >= 0)
        return &m_fbuffer->GetPicture( m_current_display_pnum );

    return 0;
}

const MEData *SequenceCompressor::GetMEData()
{
    if ( m_pcoder.IsMEDataAvail())
        return m_pcoder.GetMEData();

    return 0;
}
DiracByteStats SequenceCompressor::EndSequence()
{
    DiracByteStats seq_stats;

    if (m_just_finished)
    {
        seq_stats=m_dirac_byte_stream.EndSequence();
        m_just_finished = false;
        m_all_done = true;
    }

    return seq_stats;
}



void SequenceCompressor::MakeSequenceReport()
{
    if ( m_encparams.LocalDecode() )
        m_qmonitor.WriteLog();

    std::cout<<std::endl;

}

void SequenceCompressor::Denoise( Picture& picture )
{
    DenoiseComponent( picture.Ydata() );
    DenoiseComponent( picture.Udata() );
    DenoiseComponent( picture.Vdata() );

}

void SequenceCompressor::DenoiseComponent( PicArray& pic_data )
{
    // Do centre-weighted median denoising

    PicArray pic_copy( pic_data );

    const int centre_weight = 5;
    const int list_length = centre_weight+8;
    ValueType val_list[list_length];

    for (int j=1; j<pic_data.LastY(); ++j)
    {
        for (int i=1; i<pic_data.LastX(); ++i)
        {
            // Make the value list
            int pos=0;
            for (; pos<centre_weight-1; ++pos)
                val_list[pos] = pic_copy[j][i];

            for (int s=-1; s<=1; ++s)
            {
                for (int r=-1; r<=1; ++r)
                {
                    val_list[pos]=pic_copy[j+s][i+r];
                    pos++;
                }// r
            }// s

            pic_data[j][i] = Median( val_list, list_length );
        }// i
    }// j

}

ValueType SequenceCompressor::Median( const ValueType* val_list, const int length)
{


    OneDArray<ValueType> ordered_vals( length );

    // Place the values in order
    int pos=0;
    ordered_vals[0] = val_list[0];
    for (int i=1 ; i<length ; ++i )
    {
        for (int k=0 ; k<i ; ++k)
        {
            if (val_list[i]<ordered_vals[k])
            {
                pos=k;
                break;
            }
            else
                pos=k+1;
        }// k

        if ( pos==i)
            ordered_vals[i] = val_list[i];
        else
        {
            for (int k=i-1 ; k>=pos ; --k )
            {
                ordered_vals[k+1] = ordered_vals[k];
            }// k
            ordered_vals[pos] = val_list[i];
        }
    }// i

    // return the middle value
    if ( length%2!=0 )
        return ordered_vals[(length-1)/2];
    else
        return (ordered_vals[(length/2)-1]+ordered_vals[length/2]+1)>>1;

}

FrameSequenceCompressor::FrameSequenceCompressor(
                                  StreamPicInput* pin ,
                                  EncoderParams& encp,
                                  DiracByteStream& dirac_byte_stream):
    SequenceCompressor(pin, encp, dirac_byte_stream)
{
}

bool FrameSequenceCompressor::LoadNextFrame()
{
    m_pic_in->ReadNextFrame( *m_fbuffer, m_last_picture_read+1 );

    if ( m_pic_in->End() )
    {
        m_all_done = true;
        return false;
    }

    if ( m_encparams.Denoise() )
        Denoise(m_fbuffer->GetPicture( m_last_picture_read+1 ) );

    m_last_picture_read++;
    m_mebuffer->PushPicture( m_fbuffer->GetPicture( m_last_picture_read ) );
    return true;
}

int FrameSequenceCompressor::CodedToDisplay( const int pnum )
{
    int div;

    if (m_encparams.L1Sep()>0)
    {
        // We have L1 and L2 pictures
        if (pnum==0)
            return 0;
        else if ((pnum-1)% m_encparams.L1Sep()==0)
        {//we have L1 or subsequent I pictures
            div=(pnum-1)/m_encparams.L1Sep();
            return pnum+m_encparams.L1Sep()-1;
        }
        else//we have L2 pictures
            return pnum-1;
    }
    else
    {//we just have I-pictures, so no re-ordering
        return pnum;
    }
}

bool FrameSequenceCompressor::IsNewAccessUnit()
{
    return (m_current_display_pnum % m_encparams.GOPLength()==0);
}

void FrameSequenceCompressor::RateControlCompress(Picture& my_frame, bool is_a_cut)
{
    if (m_encparams.TargetRate() == 0)
        return;

    PictureParams& pparams = my_frame.GetPparams();
    const PictureSort& fsort = pparams.PicSort();

    // Coding using Rate Control Algorithm

    if ( fsort.IsIntra() && m_current_display_pnum != 0 &&
         m_encparams.NumL1() != 0){
        // Calculate the new QF for encoding the following I pictures in the sequence
        // in normal coding

        if ( is_a_cut ) // Recompute the QF based on long-term history since recent history is bunk
            m_ratecontrol->SetCutPictureQualFactor();
        else
            m_ratecontrol->CalcNextIntraQualFactor();
    }

    PictureByteIO *p_picture_byteio;
    p_picture_byteio =  m_pcoder.Compress(*m_fbuffer,
                                            m_current_display_pnum);

    // add the picture to the byte stream
    m_dirac_byte_stream.AddPicture(p_picture_byteio);


    // Update the quality factor
    m_ratecontrol->CalcNextQualFactor(pparams, p_picture_byteio->GetSize()*8);

}


FieldSequenceCompressor::FieldSequenceCompressor(
                                  StreamPicInput* pin ,
                                  EncoderParams& encp,
                                  DiracByteStream& dirac_byte_stream):
    SequenceCompressor(pin, encp, dirac_byte_stream)
{
    if ( m_encparams.LocalDecode() )
    {
        m_origbuffer = new PictureBuffer(*m_mebuffer);
    }
    m_delay = 2;
}

bool FieldSequenceCompressor::LoadNextFrame()
{
    m_pic_in->ReadNextFrame( *m_fbuffer, m_last_picture_read+1 );

    if ( m_pic_in->End() ){
        m_all_done = true;
        return false;
    }

    ++m_last_picture_read;
    if ( m_encparams.Denoise() )
    {
        Denoise(m_fbuffer->GetPicture( m_last_picture_read ) );
        Denoise(m_fbuffer->GetPicture( m_last_picture_read+1 ) );
    }
    m_mebuffer->PushPicture( m_fbuffer->GetPicture( m_last_picture_read ) );

    Picture &field1 = m_mebuffer->GetPicture( m_last_picture_read );
    PreMotionEstmationFilter(field1.Ydata());
    PreMotionEstmationFilter(field1.Udata());
    PreMotionEstmationFilter(field1.Vdata());

    if ( m_encparams.LocalDecode() )
        m_origbuffer->PushPicture( m_fbuffer->GetPicture( m_last_picture_read ) );

    m_mebuffer->PushPicture( m_fbuffer->GetPicture( m_last_picture_read + 1 ) );

    Picture &field2 = m_mebuffer->GetPicture( m_last_picture_read + 1 );
    PreMotionEstmationFilter(field2.Ydata());
    PreMotionEstmationFilter(field2.Udata());
    PreMotionEstmationFilter(field2.Vdata());

    if ( m_encparams.LocalDecode() )
        m_origbuffer->PushPicture( m_fbuffer->GetPicture( m_last_picture_read + 1 ) );

    ++m_last_picture_read;
    return true;
}

void FieldSequenceCompressor::PreMotionEstmationFilter(PicArray& comp)
{
    //Special case for first row
    for (int i = comp.FirstX(); i <= comp.LastX(); ++i)
    {
        comp[comp.FirstY()][i] = (3*comp[comp.FirstY()][i] +
                                  comp[comp.FirstY()+1][i] +2 )>>2;
    }
    //Middle section
    for (int j = comp.FirstY()+1; j < comp.LastY(); ++j)
    {
        for (int i = comp.FirstX(); i <= comp.LastX(); ++i)
        {
            comp[j][i] = (comp[j-1][i] + 2*comp[j][i] + comp[j+1][i] + 2)>>2;
        }
    }
    //Special case for last row
    for (int i = comp.FirstX(); i <= comp.LastX(); ++i)
    {
        comp[comp.LastY()][i] = (comp[comp.LastY()-1][i] +
                                 3*comp[comp.LastY()][i] + 2)>>2;
    }
}

const Picture& FieldSequenceCompressor::OriginalPicture(int picture_num)
{
    if ( m_encparams.LocalDecode() )
        return m_origbuffer->GetPicture(picture_num);
    else
        return m_mebuffer->GetPicture(picture_num);
}

void FieldSequenceCompressor::CleanBuffers()
{
    // If we're not at the beginning, clean the buffer
    if ( m_current_code_pnum != 0 )
    {
        SequenceCompressor::CleanBuffers();
        if (m_encparams.LocalDecode())
            m_origbuffer->CleanAll( m_show_pnum, m_current_display_pnum );
    }
}

FieldSequenceCompressor::~FieldSequenceCompressor()
{
    if ( m_encparams.LocalDecode() )
        delete m_origbuffer;
}

int FieldSequenceCompressor::CodedToDisplay( const int pnum )
{
     // Frame the field pnum belongs to
    int fnum = pnum>>1;
    if (m_encparams.L1Sep()>0)
    {
        // We have L1 and L2 frames
        if (fnum==0)
            return pnum;
        else if ((fnum-1)% m_encparams.L1Sep()==0)
        {//we have L1 or subsequent I frames
            return (pnum+(m_encparams.L1Sep()-1)*2);
        }
        else//we have L2 frames
            return (pnum - 2);
    }
    else
    {//we just have I-frames, so no re-ordering
        return (pnum);
    }
}

bool FieldSequenceCompressor::IsNewAccessUnit( )
{
    return ((m_current_display_pnum) % (m_encparams.GOPLength()<<1)==0);
}

void FieldSequenceCompressor::RateControlCompress(Picture& my_picture, bool is_a_cut)
{
    if (m_encparams.TargetRate() == 0)
        return;

    PictureParams& pparams = my_picture.GetPparams();
    const PictureSort& fsort = pparams.PicSort();

    // Coding using Rate Control Algorithm
    if ( fsort.IsIntra() &&
         m_current_display_pnum > 1 &&
         m_encparams.NumL1() != 0)
    {
        // Calculate the new QF for encoding the following I pictures in the sequence
        // in normal coding

        if ( is_a_cut )
            // Recompute the QF based on long-term history since recent history is bunk
            m_ratecontrol->SetCutPictureQualFactor();
        else if (m_current_display_pnum%2 == 0)
                m_ratecontrol->CalcNextIntraQualFactor();
    }

    PictureByteIO *p_picture_byteio;
    p_picture_byteio =  m_pcoder.Compress(*m_fbuffer,
                                            m_current_display_pnum);

    if (m_current_display_pnum%2 == 0)
        m_field1_bytes = p_picture_byteio->GetSize();
    else
        m_field2_bytes = p_picture_byteio->GetSize();

    // Update the quality factor
    if (pparams.PictureNum()%2)
        m_ratecontrol->CalcNextQualFactor(pparams, (m_field1_bytes+m_field2_bytes)*8);

    // add the picture to the byte stream
    m_dirac_byte_stream.AddPicture(p_picture_byteio);
}

