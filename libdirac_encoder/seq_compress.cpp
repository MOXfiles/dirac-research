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

    m_intra_olbp = new OLBParams( 2*m_basic_olb_params2.Xbsep() , 
                                  2*m_basic_olb_params2.Ybsep() , 
  		                  m_basic_olb_params2.Xbsep() , 
  		                  m_basic_olb_params2.Ybsep() );
 
    SetMotionParameters();

}

void SequenceCompressor::SetMotionParameters(){

    if ( m_encparams.TargetRate() != 0 ){
        OLBParams new_olb_params=m_basic_olb_params2;
/* 
        if (m_encparams.Qf()<2.5)
            new_olb_params=m_basic_olb_params1;
        else if (m_encparams.Qf()<1.5)
            new_olb_params=m_basic_olb_params0;
                
        m_encparams.SetBlockSizes( new_olb_params , m_srcparams.CFormat() );
*/
    }

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
    delete m_intra_olbp;

    if ( m_encparams.Verbose())
        MakeSequenceReport();

    //TBD: put into the destructor for EncoderParams
    delete &m_encparams.EntropyFactors();

    if (m_encparams.TargetRate()!=0)
        delete m_ratecontrol;
}

bool SequenceCompressor::CanEncode()
{
    const int queue_size = 20;

    if (m_eos_signalled)
    {
        if (m_last_picture_read >= m_current_display_pnum )
            return true;

        /*
        * Encode the remaining picture in the frame buffer. We check if the
        * reference pictures are available and modify the picture sort
        * accordingly.
        */
        if (m_current_code_pnum <= m_last_picture_read)
        {
            m_current_display_pnum = m_current_code_pnum;
            return true;
        }
    }
    else
    {
        if (m_last_picture_read >= m_current_display_pnum + queue_size)
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
    {

        if(IsNewAccessUnit())
        {
            SequenceHeaderByteIO *p_seqheader_byteio = new SequenceHeaderByteIO
                                        ( m_pic_in->GetSourceParams(),
                                            m_encparams);
            p_seqheader_byteio->Output();

            m_dirac_byte_stream.AddSequenceHeader(p_seqheader_byteio);
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
	
        const std::vector<int> queue_members = m_enc_pbuffer.Members();
/*
        if (m_encparams.Verbose() ){
            std::cout<<std::endl<<std::endl<<"Queue size is "<<m_enc_pbuffer.Size();

            std::cout<<std::endl<<"Queue members are : ";
            for (size_t i=0; i<queue_members.size(); ++i)
                std::cout<<queue_members[i]<<" ";
        }
*/
        EncPicture& current_pic = m_enc_pbuffer.GetPicture( m_current_display_pnum );
	PictureParams& current_pp = current_pic.GetPparams();

        // 1. Set the picture type and refs for all the pictures in the queue not already encoded 
        for (size_t i=0; i<queue_members.size(); ++i){
            int pnum = queue_members[i];
      	    EncPicture& enc_pic = m_enc_pbuffer.GetPicture(pnum);

            if ( (enc_pic.GetStatus() & DONE_SET_PTYPE) == 0 ){
                PictureParams& pparams = enc_pic.GetPparams();

                if (pparams.PictureNum() < m_current_display_pnum + 20 - m_encparams.L1Sep() ){
                    SetPicTypeAndRefs( pparams );
  		    enc_pic.UpdateStatus( DONE_SET_PTYPE );
		}
	    }
	}
     

        if ( current_pp.PicSort().IsRef() )
            m_enc_pbuffer.SetRetiredPictureNum( m_show_pnum, m_current_display_pnum );

        /* Do motion estimation and compensation if inter*/
        bool is_a_cut( false );

        //2. Set up block sizes etc
        SetMotionParameters();// FIXME can't change block sizes now - block params need
	                      // to be part of the EncPicture class

        // Loop over the whole queue and ...
        for (size_t i=0; i<queue_members.size(); ++i){
            int pnum = queue_members[i];
      	    EncPicture& enc_pic = m_enc_pbuffer.GetPicture(pnum);

            if ( ( enc_pic.GetStatus() & DONE_SET_PTYPE) != 0 ){
                PictureParams& pparams = enc_pic.GetPparams();

                if ( pparams.PicSort().IsInter() ){
	            // 3.Initialise motion data
	            if ( ( enc_pic.GetStatus() & DONE_ME_INIT) == 0 ){
                        enc_pic.InitMEData( m_encparams.XNumMB() , m_encparams.YNumMB() , 
                                                                   pparams.NumRefs() );
  		        enc_pic.UpdateStatus( DONE_ME_INIT );				   
  	            }

                    // 4. Do pixel-accurate motion estimation
		    if ( ( enc_pic.GetStatus() & DONE_PEL_ME) == 0 ){
                        m_pcoder.PixelME( m_enc_pbuffer, pnum );
  		        enc_pic.UpdateStatus( DONE_PEL_ME );				   
	            }

//		    // 5. Set picture complexity
//  		    if ( (enc_pic.GetStatus() & DONE_PIC_COMPLEXITY ) == 0 ){
//                        m_pcoder.CalcComplexity( m_enc_pbuffer, pnum, m_encparams.LumaBParams(2) );
// 		        enc_pic.UpdateStatus( DONE_PIC_COMPLEXITY );
//		    }
	        }
	    }

	}
        if ( current_pp.PicSort().IsInter() ){
//            // 6. Normalise complexity for the current picture
//	    m_pcoder.NormaliseComplexity( m_enc_pbuffer, m_current_display_pnum );
//
//	    std::cout<<std::endl<<"Complexity is : "<<current_pic.GetComplexity();
//	    std::cout<<std::endl<<"Normalised complexity is : "<<current_pic.GetNormComplexity();


	    //5. Do subpel refinement
	    m_pcoder.SubPixelME( m_enc_pbuffer, m_current_display_pnum );

	    //6. Do mode decision
	    m_pcoder.ModeDecisionME( m_enc_pbuffer, m_current_display_pnum );

	    //7. Do cut detection 
	    is_a_cut = m_pcoder.CutDetect( m_enc_pbuffer, m_current_display_pnum );

            //8. Motion compensate
            if ( is_a_cut ){
                if ( current_pp.PicSort().IsRef() ) // Set the picture type to intra
                    current_pic.SetPictureSort (PictureSort::IntraRefPictureSort());
                else
                    current_pic.SetPictureSort (PictureSort::IntraNonRefPictureSort());

                if ( m_encparams.Verbose() )
                    std::cout<<std::endl<<"Cut detected and I-picture inserted!";
            }
	    else{
	    
	        MEData& me_data = current_pic.GetMEData();

                if (me_data.IntraBlockRatio()>0.1)
                    m_encparams.SetBlockSizes(*m_intra_olbp, m_srcparams.CFormat() );
                
	        m_pcoder.MotionCompensate(m_enc_pbuffer, m_current_display_pnum, SUBTRACT );
		current_pic.UpdateStatus( DONE_MC );
		// 5. Set picture complexity
  		if ( (current_pic.GetStatus() & DONE_PIC_COMPLEXITY ) == 0 ){
                    m_pcoder.CalcComplexity2( m_enc_pbuffer, m_current_display_pnum );
 	            current_pic.UpdateStatus( DONE_PIC_COMPLEXITY );
  	        }	
  	        m_pcoder.NormaliseComplexity( m_enc_pbuffer, m_current_display_pnum );
	    std::cout<<std::endl<<"Complexity is : "<<current_pic.GetComplexity();
	    std::cout<<std::endl<<"Normalised complexity is : "<<current_pic.GetNormComplexity();
	    }
	
        }

        // 9. Now code the residual data and motion data
        if (m_encparams.TargetRate() != 0)
	    UpdateIntraPicCBRModel( current_pp, is_a_cut );

        // 10.  Write the picture header. 
        PictureByteIO* p_picture_byteio = new PictureByteIO(current_pp, m_current_display_pnum );
        p_picture_byteio->Output();
	
	if ( current_pp.PicSort().IsInter() ){

            if (m_encparams.Verbose() ){
	        std::cout<<std::endl<<"References "
                     << (m_encparams.FieldCoding() ? "field " : "frame ")
                     << current_pp.Refs()[0];
                if (current_pp.Refs().size() > 1)
                    std::cout<<" and "<< current_pp.Refs()[1];
            }
        }


        // 11. Code the motion vectors
        if ( current_pp.PicSort().IsInter() )
            m_pcoder.CodeMVData(m_enc_pbuffer , m_current_display_pnum, p_picture_byteio);

        // 12. Do prefiltering on the residue if necessary
	if (m_encparams.Prefilter() != NO_PF )
  	    m_pcoder.Prefilter( m_enc_pbuffer, m_current_display_pnum );
 
        // 13. Do the transform on the 3 components
	m_pcoder.DoDWT( m_enc_pbuffer, m_current_display_pnum , FORWARD);

	// 14. Select the quantisers
	
        // 15. Code the residue	
        m_pcoder.CodeResidue(m_enc_pbuffer , m_current_display_pnum, p_picture_byteio);

        const PictureSort& psort = current_pp.PicSort();

        /* All coding is done - so output and reconstruct */

        m_dirac_byte_stream.AddPicture(p_picture_byteio);

        // 16. Do the inverse DWT if necessary
	m_pcoder.DoDWT( m_enc_pbuffer, m_current_display_pnum , BACKWARD);


        // 17. Motion compensate back if necessary
        if (psort.IsInter() && !is_a_cut )
            m_pcoder.MotionCompensate(m_enc_pbuffer, m_current_display_pnum, ADD );
	
	// Reset block sizes for next picture
        m_encparams.SetBlockSizes(m_basic_olb_params2, m_srcparams.CFormat() );
 
        // 18. Clip the data to keep it in range
        current_pic.Clip();

        // Use the results of encoding to update the CBR model
        if (m_encparams.TargetRate() != 0 )
	    UpdateCBRModel(current_pic, p_picture_byteio);
         
        // 19. Measure the encoded picture quality
        if ( m_encparams.LocalDecode() )
            m_qmonitor.UpdateModel( current_pic );
       
        // Increment our position
        m_current_code_pnum++;

        CleanBuffers();

        current_pic.SetStatus( ALL_ENC );
	
    }

    // Return the latest picture that can be shown
    if ( m_enc_pbuffer.GetPicture(m_show_pnum).GetStatus() == ALL_ENC ){
        if ( m_encparams.Verbose() ){
           std::cout<<std::endl<<"Return " <<
                        (m_encparams.FieldCoding() ? "field " : "frame ")  <<
                          m_show_pnum << " in display order";
        }
        return &m_enc_pbuffer.GetPicture(m_show_pnum );
    }
    else
        return NULL;
}



void SequenceCompressor::CleanBuffers()
{
    // If we're not at the beginning, clean the buffer
    if ( m_current_code_pnum != 0 )
//        m_enc_pbuffer.CleanRetired( m_show_pnum, m_current_display_pnum );
        m_enc_pbuffer.CleanRetired( m_show_pnum, std::max(m_current_display_pnum-10, 0 ) );
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

void SequenceCompressor::UpdateIntraPicCBRModel( const PictureParams& pparams, const bool is_a_cut ){
    // For intra pictures we want to update before coding
    // especially if they're inserted

    if ( pparams.PicSort().IsIntra() && m_current_display_pnum > 0 &&
             m_encparams.NumL1() != 0){
        // Calculate the new QF for encoding the following I picture 
        if ( is_a_cut ) 
            m_ratecontrol->SetCutPictureQualFactor();
        else  
            m_ratecontrol->CalcNextIntraQualFactor();
    }
}

FrameSequenceCompressor::FrameSequenceCompressor(
                                  StreamPicInput* pin ,
                                  EncoderParams& encp,
                                  DiracByteStream& dirac_byte_stream):
    SequenceCompressor(pin, encp, dirac_byte_stream)
{
}

void FrameSequenceCompressor::SetPicTypeAndRefs( PictureParams& pparams )
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
/*
            // if we don't have the first L1 picture ...
            if ( (pnum-L1_sep) % gop_len>0 )
                // ... other ref is the prior I/L1 picture but one
                pparams.Refs().push_back( pnum - 2*L1_sep  );
*/
            // Expires after the next L1 or I picture
            pparams.SetExpiryTime( 2*L1_sep );
        }
        else if ((pnum+1) % L1_sep == 0){
            pparams.SetPicSort( PictureSort::InterNonRefPictureSort());

            // .. and the previous picture
            pparams.Refs().push_back(pnum-1);
            // Refs are the next I or L1 picture ...
            if (m_enc_pbuffer.IsPictureAvail(pnum+1))
                pparams.Refs().push_back(pnum+1);

            pparams.SetExpiryTime( 1 );
        }
        else{
            pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the previous picture
            pparams.Refs().push_back(pnum-1);
            // Refs are the next I or L1 picture ...
            int next_ref = ((pnum/L1_sep)+1)*L1_sep;
            if (m_enc_pbuffer.IsPictureAvail(next_ref))
                pparams.Refs().push_back(next_ref);

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

    if ( m_encparams.Prefilter()==CWM )
        CWMFilter(m_enc_pbuffer.GetPicture( m_last_picture_read+1 ) , 
                                         m_encparams.PrefilterStrength() );

    if ( m_pic_in->End() )
    {
        m_all_done = true;
        return false;
    }

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

void FrameSequenceCompressor::UpdateCBRModel(EncPicture& my_frame,
                                                  const PictureByteIO* p_picture_byteio)
{
    // Update the quality factor
    m_ratecontrol->CalcNextQualFactor(my_frame.GetPparams(), p_picture_byteio->GetSize()*8);

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

void FieldSequenceCompressor::SetPicTypeAndRefs( PictureParams& pparams )
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
            if (m_enc_pbuffer.IsPictureAvail(pnum+1*2))
                pparams.Refs().push_back(pnum+1*2);

            pparams.SetExpiryTime( 1 );
        }
        else{
            // Bi-directional reference fields.
            pparams.SetPicSort( PictureSort::InterRefPictureSort());

            // .. and the same parity field of the previous picture
            pparams.Refs().push_back(pnum-1*2);
            // Refs are the same parity fields in the next I or L1 picture ...
            int next_ref = (((pnum/2)/L1_sep+1)*L1_sep)*2+(pnum%2);
            if (m_enc_pbuffer.IsPictureAvail(next_ref))
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

void FieldSequenceCompressor::UpdateCBRModel(EncPicture& my_picture, 
                                                  const PictureByteIO* p_picture_byteio)
{
    if (m_current_display_pnum%2 == 0)
        m_field1_bytes = p_picture_byteio->GetSize();
    else
        m_field2_bytes = p_picture_byteio->GetSize();

    // Update the quality factor
    if (my_picture.GetPparams().PictureNum()%2)
        m_ratecontrol->CalcNextQualFactor(my_picture.GetPparams(), (m_field1_bytes+m_field2_bytes)*8);

}

