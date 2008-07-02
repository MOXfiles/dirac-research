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
* Contributor(s): Richard Felton (Original Author),
*                 Thomas Davies
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

//  Motion Compensation routines.
//  Supports different sizes of blocks as long as the parameters
//     describing them are 'legal'. Blocks overlap the edge of the image
//     being written to but blocks in the reference image are forced to
//     lie completely within the image bounds.

#ifndef _INCLUDED_MOT_COMP
#define _INCLUDED_MOT_COMP

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <vector>
#include <libdirac_common/common.h>
#include <libdirac_common/upconvert.h>
#include <libdirac_common/motion.h>
#include <libdirac_common/frame_buffer.h>

namespace dirac
{
    class FrameBuffer;
    class Frame;

 
    //! Abstract Motion compensator class. 
    /*!
        Motion compensator class, for doing motion compensation with two 
        references and overlapped blocks, using raised-cosine roll-off.
        This is an abstract class. It must be sub-classed and the 
        CompensateBlock must be defined in the sub-classes.
    */
    class MotionCompensator
    {

    public:
        //! Constructor.
        /*!
            Constructor initialises using codec parameters.
         */
        MotionCompensator( const CodecParams &cp );
        //! Destructor
        virtual ~MotionCompensator();

        //! Convenience function to perform motion compensation on a frame
        /*!
            Static function that motion compensates a frame. It uses the
            MV precision value in the CodecParams to instantiate the 
            appropriate MotionCompensation sub-class.
            \param    cp        Encoder/decoder parameters
            \param    direction whether we're subtracting or adding
            \param    buffer    the FrameBuffer object containing the frame and the reference frames
            \param    fnum    number of frame in the frame buffer to be compensated
    `       \param    mv_data    the motion vector data
         */
        static void CompensateFrame ( const CodecParams &cp, 
                                      const AddOrSub direction , 
                                      FrameBuffer& buffer , 
                                      const int fnum, 
                                      const MvData& mv_data );

        //! Compensate a frame
        /*!
            Perform motion compensated addition/subtraction on a frame using 
            parameters
            \param    direction whether we're subtracting or adding
            \param    fnum    number of frame in the frame buffer to be compensated
            \param    my_buffer    the FrameBuffer object containing the frame and the reference frames
    `       \param    mv_data    the motion vector data
         */
        void CompensateFrame( const AddOrSub direction , 
                              FrameBuffer& my_buffer , 
                              int fnum , 
                              const MvData& mv_data );

    private:
        //private, body-less copy constructor: this class should not be copied
        MotionCompensator( const MotionCompensator& cpy );
        //private, body-less assignment=: this class should not be assigned
        MotionCompensator& operator=( const MotionCompensator& rhs );

        //functions

        //! Motion-compensate a component
        void CompensateComponent( Frame& picframe , 
                                  const Frame& ref1frame , 
                                  const Frame& ref2frame ,
                                  const MvData& mv_data , const CompSort cs);

        //! Recalculate the weight matrix and store other key block related parameters.
        //! DC-compensate an individual block
        void DCBlock( TwoDArray<CalcValueType> &pic_data ,
                      const ValueType dc ,
                      const ImageCoords& Pos , 
                      const TwoDArray<CalcValueType>& Weights );
        void ReConfig();

        // Overlapping blocks are acheived by applying a 2D raised cosine shape
        // to them. This function facilitates the calculations
        float RaisedCosine(float t, float B);

        //! Calculates a weighting block.
        /*! 
            Params defines the block parameters so the relevant weighting 
            arrays can be created.  FullX and FullY refer to whether the 
            weight should be adjusted for the edge of an image.  eg. 1D 
            Weighting shapes in x direction
              FullX true        FullX false
                ***           ********
              *     *                  *
             *       *                  *
           *           *                  *
        */
        void CreateBlock(const OLBParams &bparams, bool FullX, bool FullY, TwoDArray<CalcValueType>& WeightArray);

        //! Flips the values in an array in the x direction
        void FlipX(const TwoDArray<CalcValueType>& Original, const OLBParams &bparams, TwoDArray<CalcValueType>& Flipped);

        //! Flips the values in an array in the y direction.
        void FlipY(const TwoDArray<CalcValueType>& Original, const OLBParams &bparams, TwoDArray<CalcValueType>& Flipped);

        //! Motion-compensate a block. Pure virtual. SubClasses need to define it
        virtual void CompensateBlock( TwoDArray<CalcValueType>& pic_data , 
                              const PicArray& refup_data , 
                              const MVector& Vec ,
                              const ImageCoords& Pos , 
                              const TwoDArray<CalcValueType>& Weights ) = 0;
        
    protected:
        //variables    

        //! The codec parameters
        CodecParams m_cparams;

        //! The chroma format
        ChromaFormat m_cformat;
        bool luma_or_chroma;    //true if we're doing luma, false if we're coding chroma  
        
        // A marker saying whether we're doing MC addition or subtraction
        AddOrSub m_add_or_sub;                    

        // Block information
        OLBParams m_bparams;
        TwoDArray<CalcValueType>* m_block_weights;
        TwoDArray<CalcValueType>* m_half_block_weights;

    };

    //! Pixel precision Motion compensator class. 
    class MotionCompensator_Pixel : public MotionCompensator
    {

    public:
        //! Constructor.
        /*!
            Constructor initialises using codec parameters.
         */
        MotionCompensator_Pixel (const CodecParams &cp);

    private:
        //! Motion-compensate a block. 
        virtual void CompensateBlock( TwoDArray<CalcValueType>& pic_data , 
                              const PicArray& refup_data , 
                              const MVector& Vec ,
                              const ImageCoords& Pos , 
                              const TwoDArray<CalcValueType>& Weights );
    };

    //! Half Pixel precision Motion compensator class. 
    class MotionCompensator_HalfPixel : public MotionCompensator
    {
    public:
        //! Constructor.
        /*!
            Constructor initialises using codec parameters.
         */
        MotionCompensator_HalfPixel (const CodecParams &cp);
    private:
        //! Motion-compensate a block. 
        virtual void CompensateBlock( TwoDArray<CalcValueType>& pic_data , 
                              const PicArray& refup_data , 
                              const MVector& Vec ,
                              const ImageCoords& Pos , 
                              const TwoDArray<CalcValueType>& Weights );
    };

    //! Quarter Pixel precision Motion compensator class. 
    class MotionCompensator_QuarterPixel : public MotionCompensator
    {
    public:
        //! Constructor.
        /*!
            Constructor initialises using codec parameters.
         */
        MotionCompensator_QuarterPixel (const CodecParams &cp);
    private:
        //! Motion-compensate a block. 
        virtual void CompensateBlock( TwoDArray<CalcValueType>& pic_data , 
                              const PicArray& refup_data , 
                              const MVector& Vec ,
                              const ImageCoords& Pos , 
                              const TwoDArray<CalcValueType>& Weights );
    };

    //! Eighth Pixel precision Motion compensator class. 
    class MotionCompensator_EighthPixel : public MotionCompensator
    {
    public:
        //! Constructor.
        /*!
            Constructor initialises using codec parameters.
         */
        MotionCompensator_EighthPixel (const CodecParams &cp);
    private:
        //! Motion-compensate a block. 
        virtual void CompensateBlock( TwoDArray<CalcValueType>& pic_data , 
                              const PicArray& refup_data , 
                              const MVector& Vec ,
                              const ImageCoords& Pos , 
                              const TwoDArray<CalcValueType>& Weights );
    };


} // namespace dirac

#endif
