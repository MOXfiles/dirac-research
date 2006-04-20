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
* Contributor(s): Anuradha Suraparaju (Original Author)
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

/**
* Definition of class MvDataByteIO
*/
#ifndef MV_DATA_BYTEIO_H
#define MV_DATA_BYTEIO_H


// DIRAC INCLUDES
#include <libdirac_common/common.h>             // EncParams

//LOCAL INCLUDES
#include <libdirac_byteio/byteio.h>             // Parent class


namespace dirac
{
             
    /**
    * Represents compressed sequence-parameter data used in an AccessUnit
    */
    class MvDataByteIO : public ByteIO
    {
    public:

        /**
        * Constructor
        *@param fparams    Frame Params
        *@param c_params   Codec parameters
        */
        MvDataByteIO(FrameParams& fparams,
                        CodecParams& c_params);

        /**
        * Constructor
        *@param byte_io    Input/Output Byte stream
        *@param fparams    Frame Params
        *@param c_params   Codec parameters
        */
        MvDataByteIO(ByteIO &byte_io, FrameParams& fparams,
                        CodecParams& c_params);

        /**
        * Destructor
        */
        virtual ~MvDataByteIO();

        /**
        * Gathers byte stats on the motion vector data
        *@param dirac_byte_stats Stat container
        */
        void CollateByteStats(DiracByteStats& dirac_byte_stats);

        /**
        * Outputs motion vector data Dirac byte-format
        */
        void Output();

        /**
        * Inputs motion vector information
        */
        void Input();


        /**
        * Get string containing coded bytes
        */
        virtual const std::string GetBytes();

        /**
        * Return pointer to the block data ByteIO stream
        */
        ByteIO*  BlockData() { return &m_block_data; };

        /**
        * Return the input block data size
        */
        unsigned int BlockDataSize() { return m_block_size; }

        /**
        * Return the size 
        */
        int GetSize() const;

    protected:
    

    private:
        /**
        * Inputs block parameters
        */
        void InputBlockParams();

        /**
        * Inputs Motion vector precision data
        */
        void InputMVPrecision();

        /**
        * Inputs global motion parameters
        */
        void InputGlobalMotionParams();

        /**
        * Inputs frame prediction mode
        */
        void InputFramePredictionMode();

        /**
        * Inputs Frame Weights
        */
        void InputFrameWeights();

        /**
        * Outputs block parameters
        */
        void OutputBlockParams();

        /**
        * Outputs Motion vector precision data
        */
        void OutputMVPrecision();

        /**
        * Outputs global motion parameters
        */
        void OutputGlobalMotionParams();

        /**
        * Outputs frame prediction mode
        */
        void OutputFramePredictionMode();

        /**
        * Outputs Frame Weights
        */
        void OutputFrameWeights();

        /**
        * Sequence paramters for intput/output
        */
        FrameParams&   m_fparams;

        /**
        * Codec params - EncParams for Output and DecParams for input
        */
        CodecParams& m_cparams;
        
        /**
        * Default Codec params - EncParams for Output and DecParams for input
        */
        const CodecParams m_default_cparams;

        /**
        * block data
        */
        ByteIO m_block_data;

        /**
        * In block data size
        */
        unsigned int m_block_size;
    };

} // namespace dirac

#endif
