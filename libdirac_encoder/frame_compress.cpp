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
                  Scott R Ladd,
                  Chris Bowley
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

//Compression of frames//
/////////////////////////

#include <libdirac_encoder/frame_compress.h>
#include <libdirac_encoder/comp_compress.h>
#include <libdirac_common/mot_comp.h>
#include <libdirac_motionest/motion_estimate.h>
#include <libdirac_common/mv_codec.h>
#include <libdirac_common/golomb.h>
#include <libdirac_common/bit_manager.h>
#include <iostream>
#include <sstream>



FrameCompressor::FrameCompressor( EncoderParams& encp ) :
m_encparams(encp),
m_skipped(false),
m_use_global(false),
m_use_block_mv(true),
m_global_pred_mode(REF1_ONLY)
{}

void FrameCompressor::Compress(FrameBuffer& my_buffer, int fnum)
{
    FrameOutputManager& foutput = m_encparams.BitsOut().FrameOutput();

	Frame& my_frame = my_buffer.GetFrame( fnum );
	const FrameSort& fsort = my_frame.GetFparams().FSort();
	const ChromaFormat cformat = my_frame.GetFparams().CFormat();
	MEData* me_data;

	unsigned int num_mv_bits;//number of bits written, without byte alignment

	CompCompressor my_compcoder(m_encparams , my_frame.GetFparams());

	if ( fsort != I_frame )
    {
		me_data = new MEData( m_encparams.XNumMB() , m_encparams.YNumMB() );

		// Motion estimate first
		if (m_encparams.Verbose()) std::cerr<<std::endl<<"Motion estimating ...";
		MotionEstimator my_motEst( m_encparams );
		my_motEst.DoME( my_buffer , fnum , *me_data );
	}

    // Write the motion data out to file for instrumentation purposes
    WriteMotionData(fnum, my_frame.GetFparams(), *me_data, fsort);

	// Write the frame header. We wait until after motion estimation, since
    // this allows us to do cut-detection and (possibly) to decide whether
    // or not to skip a frame before actually encoding anything. However we
	// can do this at any point prior to actually writing any frame data.
	WriteFrameHeader( my_frame.GetFparams() );


	if ( !m_skipped )
	{	// If not skipped we continue with the coding ...

		if ( fsort != I_frame){

 			//code the MV data
			if ( m_encparams.Verbose() ) std::cerr<<std::endl<<"Coding motion data ...";

			//if we're using global motion parameters, code them
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
				MvDataCodec my_mv_coder( &( foutput.MVOutput().Data() ) , 50 , cformat);

				my_mv_coder.InitContexts();//may not be necessary
				num_mv_bits = my_mv_coder.Compress( *me_data );			

				UnsignedGolombCode( foutput.MVOutput().Header() , num_mv_bits);
			}

 			//then motion compensate
			if ( m_encparams.Verbose() )
				std::cerr<<std::endl<<"Motion compensating ...";

			MotionCompensator mycomp( m_encparams );
			mycomp.SetCompensationMode( SUBTRACT );
			mycomp.CompensateFrame( my_buffer , fnum , *me_data );

		}//?fsort

 		//code component data
		my_compcoder.Compress( my_buffer.GetComponent( fnum , Y_COMP) );
  		if (cformat != Yonly)
          {
  			my_compcoder.Compress( my_buffer.GetComponent( fnum , U_COMP) );
  			my_compcoder.Compress( my_buffer.GetComponent( fnum , V_COMP) );
  		}

 		//motion compensate again if necessary
		if ( fsort != I_frame )
		{
			MotionCompensator mycomp( m_encparams );
			mycomp.SetCompensationMode( ADD );
			mycomp.CompensateFrame( my_buffer , fnum , *me_data );

			delete me_data;	
		}//?fsort

 		//finally clip the data to keep it in range
		my_buffer.GetFrame(fnum).Clip();

	}//?m_skipped

    // Finish by writing everything out to file ...
    m_encparams.BitsOut().WriteFrameData();


    if ( m_encparams.Verbose() )
        MakeFrameReport();
}

void FrameCompressor::MakeFrameReport()
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

void FrameCompressor::WriteFrameHeader( const FrameParams& fparams )
{

	//write the frame number
	int temp_int=fparams.FrameNum();

    BasicOutputManager& frame_header_op = m_encparams.BitsOut().FrameOutput().HeaderOutput();

	frame_header_op.OutputBytes(  (char*) &temp_int , 4 );

	//write whether the frame is m_skipped or not
	frame_header_op.OutputBit( m_skipped );

	if (!m_skipped)
	{// If we're not m_skipped, then we write the rest of the metadata

		// Write the expiry time relative to the frame number
		UnsignedGolombCode( frame_header_op , fparams.ExpiryTime() );

		// Write the frame sort
		UnsignedGolombCode( frame_header_op , (unsigned int) fparams.FSort() );		

		if (fparams.FSort() != I_frame)
		{		
			// If not an I-frame, write how many references there are		
			UnsignedGolombCode( frame_header_op , (unsigned int) fparams.Refs().size() );

			// For each reference, write the reference number relative to the frame number
			for ( size_t i=0 ; i<fparams.Refs().size() ; ++i )
				GolombCode( frame_header_op , fparams.Refs()[i]-fparams.FrameNum() );

			// Indicate whether or not there is global motion vector data
			frame_header_op.OutputBit( m_use_global );

			// Indicate whether or not there is block motion vector data
			frame_header_op.OutputBit( m_use_block_mv );

			// If there is global but no block motion vector data, indicate the 
            // prediction mode to use for the whole frame
			if ( m_use_global && !m_use_block_mv )
			{
				UnsignedGolombCode( frame_header_op , (unsigned int) m_global_pred_mode );
			}
		}

	}// ?m_skipped

}

// Write motion data (MvData object) to file.
// uses overloaded operator<< defined in libdirac_common/motion.cpp
void FrameCompressor::WriteMotionData(int fnum, const FrameParams & fparams, MEData & me_data, const FrameSort & fsort)
{
    if (m_encparams.Verbose())
        std::cerr<<std::endl<<"Writing motion data to file: ";

    char file[150];
    std::strcpy(file, m_encparams.OutputPath());

    std::ofstream out(file, std::ios::out | std::ios::app);

    out << std::endl << "[frame:" << fnum << "]";

    if (fsort == I_frame)
    {
        out << ">intra" << std::endl;
    }
    else
    {
        out << ">mo_comp";
        out << std::endl << std::endl << fparams.Refs().size() << " ";

        for (int i=0; i<int(fparams.Refs().size()); ++i)
            out << fparams.Refs()[i] << " ";

        OLBParams block_params = m_encparams.LumaBParams(2);
        out << block_params << " ";
        out << me_data;
    }
    
    out.close();
}
