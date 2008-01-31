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

#ifndef _PICTURE_BUFFER_H_
#define _PICTURE_BUFFER_H_

#include <vector>
#include <map>
#include <libdirac_common/picture.h>
#include <libdirac_common/common.h>

namespace dirac
{
    //! Holds frames both for reference and to overcome reordering delay
    /*!
        The buffer holds frames in a stack to overcome both reordering due to
        bi-directional prediction and use as references for subsequence motion
        estimation. Frames, and components of frames, can be accessed by their
        picture numbers. GOP parameters can be included in the constructors so
        that frames can be given types (I picture, L1 picture or L2 picture) on
        being pushed onto the stack; alternatively, these parameters can be
        overridden.
    */
    class PictureBuffer{
    public:
        //! Default Constructor
        PictureBuffer();

        //! Constructor
        /*!
            Creates a PictureBuffer using the chroma format. Suitable for
            compressing when there are no L2 frames, or when the temporal
            prediction structure is to be determined on the fly.

            \param   cf    the Chroma format of frames in the buffer
            \param   orig_xlen  the original luma width of frames in the buffer
            \param   orig_ylen  the original luma height of frames in the buffer
            \param   dwt_xlen   the padded luma width of frames in the buffer
            \param   dwt_ylen   the padded luma height of frames in the buffer
            \param   dwt_cxlen  the padded chroma width of frames in the buffer
            \param   dwt_cylen  the padded chroma height of frames in the buffer
            \param   luma_depth the video depth of the luma comp in the buffer
            \param   chroma_depth the video depth of the chroma comp in the buffer
            \param   using_ac   True if using Arithmetic coding to code coefficient data

        */
        PictureBuffer(ChromaFormat cf,
                    const int orig_xlen,
                    const int orig_ylen,
                    const int dwt_xlen,
                    const int dwt_ylen,
                    const int dwt_cxlen,
                    const int dwt_cylen,
                    const unsigned int luma_depth,
                    const unsigned int chroma_depth,
                    bool using_ac);

        //! Constructor
        /*!
            Creates a PictureBuffer using the chroma format, the number of L1
            frames between I frames and the separation in frames between L1
            frames. Suitable for compressing when there is a full GOP structure
            or when the temporal prediction structure is to be determined on
            the fly.

            \param  cf    the Chroma format of frames in the buffer
            \param  numL1    the number of Layer 1 frames before the next I picture. 0 means that there is only one I picture.
            \param  L1sep    the number of Layer 2 frames between Layer 1 frames
            \param  orig_xlen  the original luma width of frames in the buffer
            \param  orig_ylen  the original luma height of frames in the buffer
            \param  dwt_xlen   the padded luma width of frames in the buffer
            \param  dwt_ylen   the padded luma height of frames in the buffer
            \param  dwt_cxlen  the padded chroma width of frames in the buffer
            \param  dwt_cylen  the padded chroma height of frames in the buffer
            \param   luma_depth the video depth of the luma comp in the buffer
            \param   chroma_depth the video depth of the chroma comp in the buffer
            \param   interlace Set true if material is being coded in interlaced mode
            \param   using_ac   True if using Arithmetic coding to code coefficient data
        */
        PictureBuffer(ChromaFormat cf,
                    const int numL1,
                    const int L1sep,
                    const int orig_xlen,
                    const int orig_ylen,
                    const int dwt_xlen,
                    const int dwt_ylen,
                    const int dwt_cxlen,
                    const int dwt_cylen,
                    const unsigned int luma_depth,
                    const unsigned int chroma_depth,
                    bool interlace,
                    bool using_ac);

        //! Copy constructor
        /*!
            Copy constructor. Removes the current contents of the picture buffer
            and copies in the contents of the initialising buffer.
        */
        PictureBuffer(const PictureBuffer& cpy);

        //! Operator=.
        /*!
            Operator=. Assigns all elements of the rhs to the lhs.
        */
        PictureBuffer& operator=(const PictureBuffer& rhs);

        //! Destructor
        ~PictureBuffer();

        //! Get picture with a given picture number (NOT with a given position in the buffer)
        Picture& GetPicture(const unsigned int fnum );

        //! Get picture with a given picture number (NOT with a given position in the buffer)
        const Picture& GetPicture(const unsigned int fnum) const;

        //! Get picture with a given picture number, setting a flag to true if it's there
        Picture& GetPicture(const unsigned int fnum, bool& is_present);

        //! Get picture with a given picture number, setting a flag to true if it's there
        const Picture& GetPicture(const unsigned int fnum, bool& is_present) const;

        //! Return true if picture with the particular picture number is available else return false
        bool IsPictureAvail(const unsigned int fnum) const;

        //! Get component with a given component sort and picture number (NOT with a given position in the buffer)
        PicArray& GetComponent(const unsigned int frame_num, CompSort c);

        //! Get component with a given component sort and picture number (NOT with a given position in the buffer)
        const PicArray& GetComponent(const unsigned int frame_num, CompSort c) const;

        //! Get upconverted component with a given component sort and picture number (NOT with a given position in the buffer)
        PicArray& GetUpComponent(const unsigned int frame_num, CompSort c);

        //! Get upconverted component with a given component sort and picture number (NOT with a given position in the buffer)
        const PicArray& GetUpComponent(const unsigned int frame_num, CompSort c) const;

        //! Returns a list of member frames
        std::vector<int> Members() const;

        //! Put a new picture into the top of the buffer
        /*!
            Put a new picture into the top of the buffer. Picture parameters
            associated with the picture will be the built-in parameters for the
            buffer.

            \param    frame_num    the number of the picture being inserted
        */
        void PushPicture(const unsigned int frame_num);

        //! Put a new picture into the top of the buffer
        /*!
            Put a new picture into the top of the buffer. Picture parameters
            associated with the picture will be as given by the picture parameter
            object.
        */
        void PushPicture(const PictureParams& fp);

        //! Put a copy of a new picture into the top of the buffer
        /*!
            Put a copy of a new picture into the top of the buffer.
        */
        void PushPicture( const Picture& picture );

        //! Sets the reference picture number that will be cleaned
        /*!
            Indicate which picture which has been output and which is no longer
            required for reference. Expiry times are set in each picture's
            picture parameters.
            \param show_fnum             picture number in display order that can be output
            \param current_coded_fnum    picture number in display order of picture currently being coded
        */
        void SetRetiredPictureNum(const int show_fnum, const int current_coded_fnum);

        //! Delete all expired frames
        /*!
            Delete frames which have been output and which are no longer
            required for reference. Expiry times are set in each picture's
            picture parameters.
            \param show_fnum             picture number in display order that can be output
            \param current_coded_fnum    picture number in display order of picture currently being coded
        */
        void CleanAll(const int show_fnum, const int current_coded_fnum);

        //! Delete retired reference frames and expired non-ref frames
        /*!
            Delete frames which have been output and retired reference frames.
            Expiry times are set in each picture's picture parameters.
            \param show_fnum             picture number in display order that can be output
            \param current_coded_fnum    picture number in display order of picture currently being coded
        */
        void CleanRetired(const int show_fnum, const int current_coded_fnum);

        //! Delete picture
        /*!
            Delete picture.
            \param fnum             picture number in display order to be deleted from picture buffer
        */
        void Clean(int fnum);

        //! Return the default picture parameters
        const PictureParams& GetPictureParams() const{return m_fparams;}

        //! Returnthe default picture parameters
        PictureParams& GetPictureParams() { return m_fparams; }

        //! Set the picture parameters based on the picture number in display order and internal GOP parameters
        void SetPictureParams(const unsigned int fnum);

    private:
        //! Remove a picture with a given picture number from the buffer
        /*!
            Remove a picture with a given picture number (in display order) from
            the buffer. Searches through the buffer and removes picture(s) with
            that number.
        */
        void Remove(const unsigned int fnum);

        //! Set the picture parameters for a progressive picture based on the picture number in display order and internal GOP parameters
        void SetProgressiveFrameParams(const unsigned int fnum);

        //! Set the picture parameters for an interlaced picture based on the picture number in display order and internal GOP parameters
        void SetInterlacedFrameParams(const unsigned int fnum);

    private:

        //! the count of the number of reference frames in the buffer
        int m_ref_count;

        //! the buffer storing all the values
        std::vector<Picture*> m_frame_data;

        //! the flags that specifies if the picture is currently in use or not
        std::vector<bool> m_frame_in_use;

        //!the map from picture numbers to position in the buffer
        std::map<unsigned int,unsigned int> m_fnum_map;

        //! The picture parameters to use as a default if none are supplied with the picture
        PictureParams m_fparams;

        //! The number of L1 frames before next I picture
        unsigned int m_num_L1;

        //! The distance, in frames, between L1 frames
        unsigned int m_L1_sep;

        //! The length of the group of pictures (GOP)
        unsigned int m_gop_len;

        //! Interlaced coding
        bool m_interlace;

        //! Arithmetic coding flag to code coefficients
        bool m_using_ac;




    };

} // namespace dirac

#endif
