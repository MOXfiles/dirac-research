/* ***** BEGIN LICENSE BLOCK *****
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
* Contributor(s):
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

/*
*
* $Author$
* $Revision$
* $Log$
* Revision 1.4  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/03/30 15:52:40  chaoticcoyote
* New Doxygen comments
*
* Revision 1.2  2004/03/22 01:04:28  chaoticcoyote
* Added API documentation to encoder library
* Moved large constructors so they are no longer inlined
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _SEQ_COMPRESS_H_
#define _SEQ_COMPRESS_H_

/////////////////////////////////////////
//-------------------------------------//
//Class to manage compressing sequences//
//-------------------------------------//
/////////////////////////////////////////

#include "libdirac_common/common.h"
#include "libdirac_common/frame_buffer.h"
#include "libdirac_common/pic_io.h"
#include <fstream>

//! Compresses a sequence of frames from a stream.
/*!
    This class compresses a sequence of frames, frame by frame. It currently uses GOP parameters set
	in the encoder parameters in order to define the temporal prediction structure. A version to 
	incorporate non-GOP structures is TBC.
*/
class SequenceCompressor{
public:
    //! Constructor
    /*!
        Creates a sequence compressor, and prepares to begin compressing with
        the first frame.Sets up frame padding in the picture input if necesary
        /param      pin     an input stream containing a sequence of frames
        /param		outfile	an output stream for the compressed output
        /param      encp    parameters for the encoding process
    */
	SequenceCompressor(PicInput* pin, std::ofstream* outfile, const EncoderParams& encp);	

	//! Destructor
	/*!
		Destructor. Must delete IO objects created by constructor.
	*/
	~SequenceCompressor();

    //! Compress the next frame in sequence
    /*!
	    This function codes the next frame in coding order and returns the next frame in display
        order. In general these will differ, and because of re-ordering there is a delay which
        needs to be imposed. This creates problems at the start and at the end of the sequence
        which must be dealt with. At the start we just keep outputting frame 0. At the end you
        will need to loop for longer to get all the frames out. It's up to the calling function
        to do something with the decoded frames as they come out -- write them to screen or to
        file, for example.

        If coding is fast enough the compressed version could be watched
        real-time (with suitable buffering in the calling function to account for
        encode-time variations).

        \return     reference to the next locally decoded frame available for display
    */
	Frame & CompressNextFrame();

    //! Determine if compression is complete.
    /*!
        Indicates whether or not the last frame in the sequence has been compressed.
        \return     true if last frame has been compressed; false if not
    */
	bool Finished(){
		return all_done;
	}

private:
	//! Copy constructor is private and body-less
	/*!
		Copy constructor is private and body-less. This class should not be copied.

	*/
	SequenceCompressor(const SequenceCompressor& cpy);

	//! Assignment = is private and body-less
	/*!
		Assignment = is private and body-less. This class should not be assigned.

	*/
	SequenceCompressor& operator=(const SequenceCompressor& rhs);

	//! Writes the sequence header data to the bitstream.
	/*!
		This contains all the block information used for motion compensation. However, temporal prediction 
		structures are defined via the frame headers, and so a simple GOP need not be used or, if so, can
		be altered on the fly.
	*/
	void WriteStreamHeader();	

	//! Completion flag, returned via the Finished method.
	bool all_done;
	//! Flag indicating whether we've just finished.
	/*!
		Flag which is false if we've been all-done for more than one frame, true otherwise 
		(so that we can take actions on finishing once only).
	*/
	bool just_finished;

	//! Pointer pointing at the picture input.
	PicInput* picIn;
	//! The parameters used for encoding.
	EncoderParams encparams;

	//! A picture buffer used for local storage of frames whilst pending re-ordering or being used for reference.
	FrameBuffer* my_buffer;

	//! Uses the GOP parameters to convert frame numbers in coded order to display order.
	int CodedToDisplay(int fnum);

	//state variables for CompressNextFrame
	//! The number of the current frame to be coded, in display order
	int current_display_fnum;
	//! The number of the current frame to be coded, in coded order
	int current_code_fnum;
	//! The number of the frame which should be output for concurrent display or storage
	int show_fnum;
	//! The index, in display order, of the last frame read
	int last_frame_read;		
	//! A delay so that we don't display what we haven't coded
	int delay;
};

#endif
