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
*                 Anuradha Suraparaju
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


#ifndef _PICTURE_COMPRESS_H_
#define _PICTURE_COMPRESS_H_

#include <libdirac_common/picture_buffer.h>
#include <libdirac_common/common.h>
#include <libdirac_common/motion.h>
#include <libdirac_byteio/picture_byteio.h>

namespace dirac
{

    class MvData;

    //! Compress a single image picture
    /*!
        This class compresses a single picture at a time, using parameters
        supplied at its construction. PictureCompressor is used by
        SequenceCompressor.
    */
    class PictureCompressor
    {
    public:
        //! Constructor
        /*!
            Creates a FrameEncoder with specific set of parameters the control
            the compression process. It encodes motion data before encoding
            each component of the picture. 
            \param encp encoder parameters
        */
        PictureCompressor( EncoderParams& encp ); 

        //! Destructor
        ~PictureCompressor( );

        //! Performs motion estimation for a picture and writes the data locally
        /*! Performs motion estimation for a picture and writes the data locally
            \param my_fbuffer picture buffer of uncoded originals
            \param fnum    picture number to compress
            \return true   if a cut is detected.
        */                        
        bool MotionEstimate( const PictureBuffer& my_fbuffer, 
                                        int fnum); 

        //! Compress a specific picture within a group of pictures (GOP)
        /*!
            Compresses a specified picture within a group of pictures. 
            \param my_fbuffer  picture buffer in which the reference frames resides
            \param fnum        picture number to compress
            \return Compressed picture in Dirac bytestream format
        */
        PictureByteIO* Compress(  PictureBuffer& my_fbuffer , 
                                int fnum );

        //! Returns true if the picture has been skipped rather than coded normally
        bool IsSkipped(){ return m_skipped; }

        //! Returns true if Motion estimation data is available
        bool IsMEDataAvail() const { return m_medata_avail; }

        //! Returns the motion estimation data
        const MEData* GetMEData() const;

    private:
        //! Copy constructor is private and body-less
        /*!
            Copy constructor is private and body-less. This class should not
            be copied.
        */
        PictureCompressor( const PictureCompressor& cpy );

        //! Assignment = is private and body-less
        /*!
            Assignment = is private and body-less. This class should not be
            assigned.
        */
        PictureCompressor& operator=(const PictureCompressor& rhs);

        //! Analyses the ME data and returns true if a cut is detected, false otherwise
        void AnalyseMEData( const MEData& );
        
        //! Compresses the motion vector data
        void CompressMVData(MvDataByteIO* mv_data);
        
        //! Returns the value lambda according to picture and component type
        float GetCompLambda( const PictureParams& fparams,
                             const CompSort csort );

        void SelectQuantisers( CoeffArray& coeff_data , 
                               SubbandList& bands ,
                               const float lambda,
                               OneDArray<unsigned int>& est_counts,
                               const CodeBlockMode cb_mode,
                               const PictureSort fsort,
                               const CompSort csort );

        int SelectMultiQuants( CoeffArray& coeff_data , 
                               SubbandList& bands , 
                               const int band_num,
                               const float lambda,
                               const PictureSort fsort, 
                               const CompSort csort );

        void SetupCodeBlocks( SubbandList& bands );


        void AddSubAverage(CoeffArray& coeff_data,int xl,int yl,AddOrSub dirn);

    private:

        //member variables
        // a local copy of the encoder params
        EncoderParams& m_encparams;
     
        // Pointer to the motion vector data
        MEData* m_me_data;

        // True if the picture has been skipped, false otherwise
        bool m_skipped;                

        // True if we use global motion vectors, false otherwise
        bool m_use_global;

        // True if we use block motion vectors, false otherwise
        bool m_use_block_mv;
        
        // Prediction mode to use if we only have global motion vectors
        PredMode m_global_pred_mode;
        
        // True if motion estimation data is available
        bool m_medata_avail;

        // True if we have detected a cut
        bool m_is_a_cut;

        // The proportion of intra blocks that motion estimation has found
        double m_intra_ratio;
    };

} // namespace dirac

#endif
