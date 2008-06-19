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
#include <libdirac_encoder/prefilter.h>

using namespace dirac;

SequenceCompressor::SequenceCompressor( StreamPicInput* pin ,
                                        EncoderParams& encp,
                                        DiracByteStream& dirac_byte_stream):
    m_all_done(false),
    m_just_finished(true),
    m_srcparams(pin->GetSourceParams()),
    m_encparams(encp),
    m_pparams(m_srcparams.CFormat(),
              m_encparams.Xl(),
              m_encparams.Yl(),
              m_encparams.LumaDepth(),
              m_encparams.ChromaDepth() ),
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

    // Set up generic picture parameters
    m_pparams.SetUsingAC(m_encparams.UsingAC() );

    // Set up a rate controller if rate control being used
    if (m_encparams.TargetRate() != 0)
        m_ratecontrol = new RateController(m_encparams.TargetRate(),
                                           m_pic_in->GetSourceParams(), encp);

    // Copy in the block parameters in case we want to change them dynamically
    m_basic_olb_params2 = m_encparams.LumaBParams(2);
    m_basic_olb_params1 = m_encparams.LumaBParams(1);
    m_basic_olb_params0 = m_encparams.LumaBParams(0);

    SetMotionArraySizes();
}

void SequenceCompressor::SetMotionArraySizes(){

    int xl = m_encparams.Xl();
    int yl = m_encparams.Yl();

    // Make sure we have enough macroblocks to cover the pictures
    m_encparams.SetXNumMB( (xl+m_encparams.LumaBParams(0).Xbsep()-1)/
                                     m_encparams.LumaBParams(0).Xbsep() );
    m_encparams.SetYNumMB( (yl+m_encparams.LumaBParams(0).Ybsep()-1)/
                                     m_encparams.LumaBParams(0).Ybsep() );
    
    m_encparams.SetXNumBlocks( 4 * m_encparams.XNumMB() );
    m_encparams.SetYNumBlocks( 4 * m_encparams.YNumMB() );
}


SequenceCompressor::~SequenceCompressor()
{

    if ( m_encparams.Verbose())
        MakeSequenceReport();

    //TBD: put into the destructor for EncoderParams
    delete &m_encparams.EntropyFactors();

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
            EncPicture& pbuf_picture = m_enc_pbuffer.GetPicture( m_current_display_pnum );
            PictureParams& pbuf_pparams = pbuf_picture.GetPparams();

            // If INTRA pic, no further checks necessary
            if (pbuf_pparams.PicSort().IsIntra())
                return true;

            std::vector<int>& refs = pbuf_pparams.Refs();
            int num_refs = 0;
            if (m_enc_pbuffer.IsPictureAvail(refs[0]))
                ++num_refs;
            if (refs.size() > 1)
            {
                if (m_enc_pbuffer.IsPictureAvail(refs[1]))
                {
                    if (num_refs == 0)
                        refs[0] = refs[1];
                    ++num_refs;
                }
            }
            refs.resize(num_refs);

            if (refs.size() == 0)
                pbuf_picture.SetPictureSort (PictureSort::IntraNonRefPictureSort());

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

const EncPicture* SequenceCompressor::CompressNextPicture()
{

    // This function codes the next picture in coding order and returns the next picture in display order
    // In general these will differ, and because of re-ordering there is a m_delay which needs to be imposed.
    // This creates problems at the start and at the end of the sequence which must be dealt with.
    // At the start we just keep outputting picture 0. At the end you will need to loop for longer to get all
    // the pictures out. It's up to the calling function to do something with the decoded pictures as they
    // come out - write them to screen or to file, or whatever. 

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


        EncPicture& my_picture = m_enc_pbuffer.GetPicture( m_current_display_pnum );

        PictureParams& pparams = my_picture.GetPparams();
        SetPredParams( pparams );

        if (pparams.PicSort().IsRef())
            m_enc_pbuffer.SetRetiredPictureNum( m_show_pnum, m_current_display_pnum );

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
        if ( my_picture.GetPparams().PicSort().IsInter() ){

            // Set the block sizes based on the QF 
            OLBParams new_olb_params=m_basic_olb_params2;
 
            if (m_encparams.Qf()<2.5)
                new_olb_params=m_basic_olb_params1;
            else if (m_encparams.Qf()<1.5)
                new_olb_params=m_basic_olb_params0;
                
            m_encparams.SetBlockSizes(new_olb_params,pparams.CFormat());
            SetMotionArraySizes();

            is_a_cut = m_pcoder.MotionEstimate(  m_enc_pbuffer,
                                                m_current_display_pnum );
            if ( is_a_cut ){
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
        if (m_encparams.TargetRate() == 0){
            PictureByteIO *p_picture_byteio;
            // Coding Without using Rate Control Algorithm
            p_picture_byteio =  m_pcoder.Compress(m_enc_pbuffer ,
                                         m_current_display_pnum);

            // add the picture to the byte stream

            m_dirac_byte_stream.AddPicture(p_picture_byteio);
        }
        else{
            RateControlCompress(my_picture, is_a_cut);
        }

       // Measure the encoded picture quality
       if ( m_encparams.LocalDecode() )
           m_qmonitor.UpdateModel(m_enc_pbuffer.GetPicture( m_current_display_pnum ) );
       
        // Increment our position
        m_current_code_pnum++;

       CleanBuffers();

    }

    // Return the latest picture that can be shown
    if ( m_encparams.Verbose() ){
           std::cout<<std::endl<<"Return " <<
                 (m_encparams.FieldCoding() ? "field " : "frame ")  <<
                  m_show_pnum << " in display order";
    }
    return &m_enc_pbuffer.GetPicture(m_show_pnum );
}

void SequenceCompressor::CleanBuffers()
{
    // If we're not at the beginning, clean the buffer
    if ( m_current_code_pnum != 0 )
        m_enc_pbuffer.CleanRetired( m_show_pnum, m_current_display_pnum );
}

const EncPicture *SequenceCompressor::GetPictureEncoded()
{
    if (m_current_display_pnum >= 0)
        return &m_enc_pbuffer.GetPicture( m_current_display_pnum );

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

FrameSequenceCompressor::FrameSequenceCompressor(
                                  StreamPicInput* pin ,
                                  EncoderParams& encp,
                                  DiracByteStream& dirac_byte_stream):
    SequenceCompressor(pin, encp, dirac_byte_stream)
{
}

void FrameSequenceCompressor::SetPredParams( PictureParams& pparams )
{
    // Set the temporal prediction parameters for frame coding

    const int pnum = pparams.PictureNum();
    const int gop_len = m_encparams.GOPLength();
    const int L1_sep = m_encparams.L1Sep();
    const int num_L1 = m_encparams.NumL1();

    pparams.SetRetiredPictureNum( -1 );
    pparams.Refs().clear();

    if ( num_L1>0 ){

        if ( pnum % gop_len == 0){
            if (gop_len > 1)
                pparams.SetPicSort( PictureSort::IntraRefPictureSort());
            else // I-picture only coding
                pparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
                
            // I picture expires after we've coded the next I picture
            pparams.SetExpiryTime( gop_len );
        }
        else if (pnum % L1_sep == 0){
            pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // Ref the previous I or L1 picture
            pparams.Refs().push_back( pnum - L1_sep );

            // if we don't have the first L1 picture ...
            if ( (pnum-L1_sep) % gop_len>0 )
                // ... other ref is the prior I/L1 picture but one
                pparams.Refs().push_back( pnum - 2*L1_sep  );

            // Expires after the next L1 or I picture
            pparams.SetExpiryTime( 2*L1_sep );
        }
        else if ((pnum+1) % L1_sep == 0){
            pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the previous picture
            pparams.Refs().push_back(pnum-1);
            // Refs are the next I or L1 picture ...
            pparams.Refs().push_back(pnum+1);

            pparams.SetExpiryTime( 1 );
        }
        else{
            pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the previous picture
            pparams.Refs().push_back(pnum-1);
            // Refs are the next I or L1 picture ...
            pparams.Refs().push_back(((pnum/L1_sep)+1)*L1_sep);

            pparams.SetExpiryTime( 2 );
        }

    }
    else{
        pparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
        pparams.SetExpiryTime( 1 );
    }

}

bool FrameSequenceCompressor::LoadNextFrame()
{
    PictureParams pp( m_pparams );
    pp.SetPictureNum( m_last_picture_read+1 );
    
    // Set an initially huge expiry time as we don't know when it will expire yet
    pp.SetExpiryTime(1<<30);

    m_enc_pbuffer.PushPicture( pp );

    m_pic_in->ReadNextPicture( m_enc_pbuffer.GetPicture(m_last_picture_read+1) );

    // Copy into the original data
    m_enc_pbuffer.GetPicture(m_last_picture_read+1).SetOrigData();

    if ( m_pic_in->End() )
    {
        m_all_done = true;
        return false;
    }

    if ( m_encparams.Prefilter()==CWM )
        CWMFilter(m_enc_pbuffer.GetPicture( m_last_picture_read+1 ) , 
                                         m_encparams.PrefilterStrength() );

    m_last_picture_read++;

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

void FrameSequenceCompressor::RateControlCompress(EncPicture& my_frame, bool is_a_cut)
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
    p_picture_byteio =  m_pcoder.Compress(m_enc_pbuffer,
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
    m_delay = 2;
}

bool FieldSequenceCompressor::LoadNextFrame()
{
    PictureParams pp( m_pparams );
    pp.SetExpiryTime( 1<<30 );

    int pnum = m_last_picture_read+1;

    for (int j=pnum; j<=pnum+1; ++j){
        pp.SetPictureNum( j );
        m_enc_pbuffer.PushPicture( pp );
    }

    StreamFieldInput* field_input = (StreamFieldInput*) m_pic_in;
    field_input->ReadNextFrame( m_enc_pbuffer.GetPicture( pnum ), m_enc_pbuffer.GetPicture(pnum+1) );

    // Copy data across
    for (int j=pnum; j<=pnum+1; ++j){
        m_enc_pbuffer.GetPicture( j ).SetOrigData();
    
        if ( m_encparams.Prefilter()==CWM )
            CWMFilter(m_enc_pbuffer.GetPicture( j ), m_encparams.PrefilterStrength() );
    
//FIXME: motion estimator should do the pre-filtering (and only on the luma anyhow)....    
//        PreMotionEstmationFilter(m_mebuffer->GetPicture( j ).Data(Y_COMP));
//        PreMotionEstmationFilter(m_mebuffer->GetPicture( j ).Data(U_COMP));
//        PreMotionEstmationFilter(m_mebuffer->GetPicture( j ).Data(V_COMP));
    }

    if ( m_pic_in->End() ){
        m_all_done = true;
        return false;
    }

    m_last_picture_read +=2;

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

void FieldSequenceCompressor::SetPredParams( PictureParams& pparams )
{
    // Set the temporal prediction parameters for field coding

    const int pnum = pparams.PictureNum();
    const int gop_len = m_encparams.GOPLength();
    const int L1_sep = m_encparams.L1Sep();
    const int num_L1 = m_encparams.NumL1();

    pparams.SetRetiredPictureNum( -1 );
    pparams.Refs().clear();

    if ( num_L1>0 ){

        if ( (pnum/2) % gop_len == 0){
            // Field 1 is Intra Field
            if (gop_len > 1){
                pparams.SetPicSort( PictureSort::IntraRefPictureSort());
                // I picture expires after we've coded the next I picture
                pparams.SetExpiryTime( gop_len * 2);
                if ( pnum%2){
                    pparams.SetPicSort( PictureSort::InterRefPictureSort());
                    // Ref the previous I field
                    pparams.Refs().push_back( pnum-1 );
                }
            }
            else{
                // I-picture only coding
                pparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
                pparams.SetExpiryTime( gop_len );
            }
        }
        else if ((pnum/2) % L1_sep == 0){
            pparams.SetPicSort( PictureSort::InterRefPictureSort());

            if (pnum%2){
                // Field 2
                // Ref the first field of same picture
                pparams.Refs().push_back( pnum - 1);
                // Ref the previous field 2 of I or L1 picture
                pparams.Refs().push_back( pnum - L1_sep*2 );
            }
            else{
                // Field 1
                // Ref the field 1 of previous I or L1 picture
                pparams.Refs().push_back( pnum - L1_sep*2 );
                // Ref the field 2 of previous I or L1 picture
                pparams.Refs().push_back( pnum - L1_sep*2 + 1 );
            }

            // Expires after the next L1 or I picture
            pparams.SetExpiryTime( (L1_sep+1)*2-1 );
        }
        else if ((pnum/2+1) % L1_sep == 0){
            // Bi-directional non-reference fields.
            pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the same parity field of the previous picture
            pparams.Refs().push_back(pnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            pparams.Refs().push_back(pnum+1*2);

            pparams.SetExpiryTime( 1 );
        }
        else{
            // Bi-directional reference fields.
            pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the same parity field of the previous picture
            pparams.Refs().push_back(pnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            pparams.Refs().push_back((((pnum/2)/L1_sep+1)*L1_sep)*2+(pnum%2));

            pparams.SetExpiryTime( 4 );
        }

    }
    else{
        pparams.SetPicSort( PictureSort::IntraNonRefPictureSort());
        pparams.SetExpiryTime( 2 );
    }
}

FieldSequenceCompressor::~FieldSequenceCompressor()
{
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

void FieldSequenceCompressor::RateControlCompress(EncPicture& my_picture, bool is_a_cut)
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
    p_picture_byteio =  m_pcoder.Compress(m_enc_pbuffer,
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

