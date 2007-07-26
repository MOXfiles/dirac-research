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
* Contributor(s): Thomas Davies (Original Author), Scott R Ladd
*                 Andrew Kennedy
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



#ifndef _COMP_DECOMPRESS_H_
#define _COMP_DECOMPRESS_H_

#include <libdirac_common/arrays.h>
#include <libdirac_common/wavelet_utils.h>
#include <libdirac_common/common.h>
#include <libdirac_byteio/component_byteio.h>

namespace dirac
{
    //! Compress a frame component
    /*!
        This class compresses one of the three components (Y, U, or V) of a
        frame according to a given set or parameters. CompCompressor is used
        by FrameCompressor..
    */
    class CompDecompressor
    {
    public:
        //! Constructor
        /*!
            Create and initialize a component decompressor with the given
            characteristics.
            \param  decp    decoding parameters
            \param  fp      frame parameters
        */
        CompDecompressor( DecoderParams& decp, const FrameParams& fp);

        //! Decompress a frame component
        /*!
            Decompress a PicArray containing a frame component (Y, U, or V).

            \param p_component_byteio Bytestream of component data
            \param  pic_data          contains the component data to be decompressed
        */
        void Decompress(ComponentByteIO *p_component_byteio,
                        PicArray& pic_data);

    private:
        //! Copy constructor is private and body-less
        /*!
            Copy constructor is private and body-less. This class should not
            be copied.

        */
        CompDecompressor(const CompDecompressor& cpy);

        //! Assignment = is private and body-less
        /*!
            Assignment = is private and body-less. This class should not be
            assigned.

        */
        CompDecompressor& operator=(const CompDecompressor& rhs);

        //! Sets the data of a specific subband node to a given value
        /*!
            Sets the data of a specific subband node to a given value

            \param  pic_data    contains the component data
            \param  node        subband node
            \param    val            the value to set
        */
        void SetToVal(CoeffArray& pic_data,const Subband& node,CoeffType val);

        //! Set up the code block structures for each subband
        /*!
             Set up the code block structures for each subband
            \param bands    the set of all the subbands
        */
        void SetupCodeBlocks( SubbandList& bands );

        //! Copy of the decompression parameters provided to the constructor
        DecoderParams& m_decparams;

        //! Reference to the frame parameters provided to the constructor
        const FrameParams& m_fparams;

    };

} // namespace dirac

#endif
