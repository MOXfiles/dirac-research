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
* Revision 1.2  2004-03-29 01:52:08  chaoticcoyote
* Added Doxygen comments
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _SEQ_DECOMPRESS_H_
#define _SEQ_DECOMPRESS_H_

///////////////////////////////////////////
//---------------------------------------//
//Class to manage decompressing sequences//
//---------------------------------------//
///////////////////////////////////////////

#include "libdirac_common/common.h"
#include <iostream>
#include <fstream>

class Gop;
class Frame;

//! Decompresses a sequence of frames from a stream.
/*!
    This class decompresses a sequence of frames, frame by frame.
*/
class SequenceDecompressor{
public:

    //! Constructor
    /*!
        Initializes the decompressor with an input stream and level of output detail.
        
        /param  ip      input data stream containing a sequnce of compressed images
        /param  verbosity   when true, increases the amount of information displayed during decompression
     */
	SequenceDecompressor(std::ifstream * ip, bool verbosity);

    //! Destructor
    /*!
        Closes files and releases resources. 
    */
	~SequenceDecompressor();

    //! Decompress the next frame in sequence
    /*!
        This function decodes the next frame in coding order and returns the next frame in
        display order. In general these will differ, and because of re-ordering there is a
        delay which needs to be imposed. This creates problems at the start and at the end
        of the sequence which must be dealt with. At the start we just keep outputting
        frame 0. At the end you will need to loop for longer to get all the frames out. It's
        up to the calling function to do something with the decoded frames as they come out
        -- write them to screen or to file, as required.
        
        \return     reference to the next locally decoded frame available for display
    */
	Frame& DecompressNextFrame();

    //! Determine if decompression is complete.
    /*!
        Indicates whether or not the last frame in the sequence has been decompressed.
        \return     true if last frame has been compressed; false if not
    */
	bool Finished() { return all_done; }
    
    //! Interrogates for decompression parameters.
    /*!
        Returns the parameters used for this decompression run.
        
        /return decompression parameters originally provide din the constructor.
     */
	SeqParams & GetSeqParams() { return decparams.sparams; }

private:
    //! Read a sequence from bitstream
    /*!
        Reads the sequence data from the bitstream. This contains all the block and GOP information. 
		In the long term we want to be able to reset all this stuff on the fly by sending a
		parameter sets. We also want to move away from simple GOPs, to a picture buffer model.
     */
	void ReadStreamHeader();	

    //! Completion flag, returned via the Finished method
	bool all_done;
    
    //! Parameters for the decompression, as provided in constructor
	DecoderParams decparams;
    
    //! Pointer to the current group of pictores (Gop)
	Gop * my_gop;
    
    //! Input file object
	std::ifstream* infile;			//

    //! Number of the frame in display order which is to be displayed
    int current_display_fnum;	

    //! Number of the frame in coded order which is to be coded
    int current_code_fnum;		

    //! ??? Value so that when converted to display order, current_code_fnum is <= current_display_fnum
	int delay;					

    //! Index, in display order, of the last frame read
	int last_frame_read;
};

#endif
