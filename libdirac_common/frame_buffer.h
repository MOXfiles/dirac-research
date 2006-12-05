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

#ifndef _FRAME_BUFFER_H_
#define _FRAME_BUFFER_H_

#include <vector>
#include <map>
#include <libdirac_common/frame.h>
#include <libdirac_common/common.h>
#include <libdirac_common/pic_io.h>

namespace dirac
{
    //! Holds frames both for reference and to overcome reordering delay
    /*!
        The buffer holds frames in a stack to overcome both reordering due to 
        bi-directional prediction and use as references for subsequence motion 
        estimation. Frames, and components of frames, can be accessed by their 
        frame numbers. GOP parameters can be included in the constructors so 
        that frames can be given types (I frame, L1 frame or L2 frame) on 
        being pushed onto the stack; alternatively, these parameters can be 
        overridden.
    */
    class FrameBuffer{
    public:
        //! Default Constructor
        FrameBuffer();
        
        //! Constructor
        /*!
            Creates a FrameBuffer using the chroma format. Suitable for 
            compressing when there are no L2 frames, or when the temporal 
            prediction structure is to be determined on the fly. 

            \param    cf    the Chroma format of frames in the buffer
            \param    xlen    the luma width of frames in the buffer
            \param    ylen    the luma height of frames in the buffer
            \param    c_xlen  the chroma width of frames in the buffer
            \param    c_ylen  the chroma height of frames in the buffer
            \param    vd      the video depth of the data in the buffer

        */
        FrameBuffer(ChromaFormat cf,
                    const int xlen,
                    const int ylen, 
                    const int c_xlen, 
                    const int c_ylen, 
                    const unsigned int vd);

        //! Constructor
        /*!
            Creates a FrameBuffer using the chroma format, the number of L1 
            frames between I frames and the separation in frames between L1 
            frames. Suitable for compressing when there is a full GOP structure
            or when the temporal prediction structure is to be determined on 
            the fly. 

            \param    cf    the Chroma format of frames in the buffer
            \param    numL1    the number of Layer 1 frames before the next I frame. 0 means that there is only one I frame.
            \param    L1sep    the number of Layer 2 frames between Layer 1 frames
            \param    xlen    the luma width of frames in the buffer
            \param    ylen    the luma height of frames in the buffer
            \param    c_xlen  the chroma width of frames in the buffer
            \param    c_ylen  the chroma height of frames in the buffer
            \param    vd      the video depth of the data in the buffer
        */    
        FrameBuffer(ChromaFormat cf,
                    const int numL1,
                    const int L1sep,
                    const int xlen,
                    const int ylen, 
                    const int c_xlen, 
                    const int c_ylen, 
                    const unsigned int vd);

        //! Copy constructor
        /*!
            Copy constructor. Removes the current contents of the frame buffer 
            and copies in the contents of the initialising buffer.
        */
        FrameBuffer(const FrameBuffer& cpy);

        //! Operator=. 
        /*!
            Operator=. Assigns all elements of the rhs to the lhs.
        */
        FrameBuffer& operator=(const FrameBuffer& rhs);

        //! Destructor
        ~FrameBuffer();

        //! Get frame with a given frame number (NOT with a given position in the buffer)
        Frame& GetFrame(const unsigned int fnum );

        //! Get frame with a given frame number (NOT with a given position in the buffer)
        const Frame& GetFrame(const unsigned int fnum) const;

        //! Get frame with a given frame number, setting a flag to true if it's there
        Frame& GetFrame(const unsigned int fnum, bool& is_present);

        //! Get frame with a given frame number, setting a flag to true if it's there
        const Frame& GetFrame(const unsigned int fnum, bool& is_present) const;

        //! Get component with a given component sort and frame number (NOT with a given position in the buffer)
        PicArray& GetComponent(const unsigned int frame_num, CompSort c);

        //! Get component with a given component sort and frame number (NOT with a given position in the buffer)
        const PicArray& GetComponent(const unsigned int frame_num, CompSort c) const;    

        //! Get upconverted component with a given component sort and frame number (NOT with a given position in the buffer)
        PicArray& GetUpComponent(const unsigned int frame_num, CompSort c);

        //! Get upconverted component with a given component sort and frame number (NOT with a given position in the buffer)
        const PicArray& GetUpComponent(const unsigned int frame_num, CompSort c) const;

        //! Returns a list of member frames
        std::vector<int> Members() const; 

        //! Put a new frame into the top of the buffer
        /*! 
            Put a new frame into the top of the buffer. Frame parameters 
            associated with the frame will be the built-in parameters for the 
            buffer.

            \param    frame_num    the number of the frame being inserted
        */
        void PushFrame(const unsigned int frame_num);    

        //! Put a new frame into the top of the buffer
        /*! 
            Put a new frame into the top of the buffer. Frame parameters 
            associated with the frame will be as given by the frame parameter 
            object.
        */
        void PushFrame(const FrameParams& fp);

        //! Put a copy of a new frame into the top of the buffer
        /*! 
            Put a copy of a new frame into the top of the buffer. 
        */
        void PushFrame( const Frame& frame );

        //! Read a new frame into the buffer.
        /*! 
            Read a new frame into the buffer. Frame parameters associated with 
            the frame will be as given by the frame parameter object.

            \param    picin    the picture input
            \param    fp        the frame parameters to apply to the frame
        */    
        void PushFrame(StreamPicInput* picin,const FrameParams& fp);

        //! Read a new frame into the buffer.
        /*! 
            Read a new frame into the buffer. Frame parameters associated with 
            the frame will be derived from the frame number and the internal 
            GOP parameters in the frame buffer.
            \param    picin    the picture input
            \param    fnum    the frame number
        */    
        void PushFrame(StreamPicInput* picin,const unsigned int fnum);

        //! Set retired list for reference frames that will be cleaned
        /*! 
            Indicate frames which have been output and which are no longer 
            required for reference. Expiry times are set in each frame's 
            frame parameters.
            \param show_fnum             frame number in display order that can be output
            \param current_coded_fnum    frame number in display order of frame currently being coded
        */
        void SetRetiredList(const int show_fnum, const int current_coded_fnum);

        //! Delete expired frames
        /*! 
            Delete frames which have been output and which are no longer 
            required for reference. Expiry times are set in each frame's 
            frame parameters.
            \param show_fnum             frame number in display order that can be output
            \param current_coded_fnum    frame number in display order of frame currently being coded
        */
        void Clean(const int show_fnum, const int current_coded_fnum);

        //! Delete frame
        /*! 
            Delete frame. 
            \param fnum             frame number in display order to be deleted from frame buffer 
        */
        void Clean(int fnum);

        //! Return the default frame parameters
        const FrameParams& GetFParams() const{return m_fparams;}

    private:
        //! Set the frame parameters based on the frame number in display order and internal GOP parameters
        void SetFrameParams(const unsigned int fnum);

        //! Remove a frame with a given frame number from the buffer
        /*!
            Remove a frame with a given frame number (in display order) from 
            the buffer. Searches through the buffer and removes frame(s) with 
            that number.
        */
        void Remove(const unsigned int fnum);

    private:
            
        //! the count of the number of reference frames in the buffer
        int m_ref_count;

        //! the buffer storing all the values
        std::vector<Frame*> m_frame_data;
        
        //! the flags that specifies if the frame is currently in use or not
        std::vector<bool> m_frame_in_use;

        //!the map from frame numbers to position in the buffer
        std::map<unsigned int,unsigned int> m_fnum_map;

        //! The frame parameters to use as a default if none are supplied with the frame
        FrameParams m_fparams;

        //! The number of L1 frames before next I frame
        unsigned int m_num_L1;

        //! The distance, in frames, between L1 frames
        unsigned int m_L1_sep;

        //! The length of the group of pictures (GOP)
        unsigned int m_gop_len;





    };

} // namespace dirac

#endif
