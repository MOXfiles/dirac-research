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
* Revision 1.2  2004-03-22 01:04:28  chaoticcoyote
* Added API documentation to encoder library
* Moved large constructors so they are no longer inlined
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _FRAME_COMPRESS_H_
#define _FRAME_COMPRESS_H_

#include "libdirac_common/gop.h"
#include "libdirac_common/common.h"

class MvData;

//! Compress a single image frame
/*!
    This class compresses a single frame at a time, using parameters supplied at
    its construction. FrameCompressor is used by SequenceCompressor.
*/
class FrameCompressor{
public:
    //! Constructor
    /*!
        Creates a FrameEncoder with specific set of parameters the control
        the compression process. It encodes motion data before encoding each
        component of the frame. 
        \param encp encoder parameters
    */
	FrameCompressor(EncoderParams& encp) : encparams(encp) { }

    //! Compress a specific frame within a group of pictures (GOP)
    /*!
        Compresses a specified frame within a group of pictures. 
        \param my_gop   group of pictures in which the frame resides
        \param fnum     frame number to compress
    */
	void Compress(Gop& my_gop, int fnum);

private:
	EncoderParams encparams;
	MvData* mv_data;
};


#endif
