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
* Revision 1.4  2004-05-18 07:46:15  tjdwave
* Added support for I-frame only coding by setting num_L1 equal 0; num_L1 negative gives a single initial I-frame ('infinitely' many L1 frames). Revised quantiser selection to cope with rounding error noise.
*
* Revision 1.3  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/03/22 01:04:28  chaoticcoyote
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

#ifndef _COMP_COMPRESS_H_
#define _COMP_COMPRESS_H_

#include "libdirac_common/arrays.h"
#include "libdirac_common/wavelet_utils.h"
#include "libdirac_common/common.h"

//! Compress a frame component
/*!
    This class compresses one of the three components (Y, U, or V) of a frame
    according to a given set or parameters. CompCompressor is used by FrameCompressor.
*/
class CompCompressor
{
public:
    //! Constructor
    /*!
        Create and initialize a component compressor with the given characteristics.
        \param  encp    encoding parameters
        \param  fp      frame parameters
    */
	CompCompressor(const EncoderParams & encp, const FrameParams& fp);

    //! Compress a frame component
    /*!
        Compress a PicArray containing a frame component (Y, U, or V).
        \param  pic_data    the component data to be compressed
    */
	void Compress(PicArray & pic_data);

private:
	//! Copy constructor is private and body-less
	/*!
		Copy constructor is private and body-less. This class should not be copied.

	*/
	CompCompressor(const CompCompressor& cpy);

	//! Assignment = is private and body-less
	/*!
		Assignment = is private and body-less. This class should not be assigned.

	*/
	CompCompressor& operator=(const CompCompressor& rhs);

	EncoderParams encparams;
	FrameParams fparams;
	float lambda;
	CompSort csort;
	ChromaFormat cformat;
	FrameSort fsort;	
	OneDArray<int> qflist;
	OneDArray<int> qfinvlist;
	OneDArray<int> offset;

	void GenQuantList();
	int SelectQuant(PicArray& pic_data,SubbandList& bands,int band_num);
	ValueType PicAbsMax(const PicArray& pic_data,int xp, int yp ,int xl ,int yl) const;
	ValueType PicAbsMax(const PicArray& pic_data) const;
	void SetToVal(PicArray& pic_data,const Subband& node,ValueType val);
	void AddSubAverage(PicArray& pic_data,int xl,int yl,AddOrSub dirn);

	void Init();
};



#endif
