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
*                 Scott Robert Ladd,
*                 Stuart Cunningham,
*                 Tim Borer 
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

#ifndef _PIC_IO_H_
#define _PIC_IO_H_

#include <iostream>
#include <fstream>

#include <libdirac_common/common.h>
#include <libdirac_common/frame.h>

//////////////////////////////////////////
//--------------------------------------//
//-                                    -//
//-Uncompressed picture file IO wrapper-//
//-                                    -//
//--------------------------------------//
//////////////////////////////////////////

//Currently just a temporary fix to write raw bytes to file with a separate header. Horrible.
//Should be extended for interfacing with players (DirectShow etc) and for wrapping frames
//in MXF with suitable metadata as a single file/stream of uncompressed video. TJD 10Feb04.

//Subclass these to provide functionality for different file formats and for streaming.

//! Class for outputting pictures
/*!
    Outputs pictures to a file
*/

class PicOutput{
    public:

        //! Constructor
        /*!
            Constructor, takes
            /param output_name the name of the output file
            /param sp the sequence parameters
            /param write_header_only optionally write only the header
         */  
        PicOutput(const char* output_name,
          const SeqParams& sp,
          bool write_header_only = false);

        //! Destructor
        virtual ~PicOutput();

        //! Write the next frame to the output
        virtual bool WriteNextFrame(const Frame& myframe);

        //! Write the picture sequence header
        virtual bool WritePicHeader();
        
		//! Get the sequence parameters 
        SeqParams& GetSeqParams() {return m_sparams;}

    protected:

        SeqParams m_sparams;
        std::ofstream* m_op_pic_ptr;
        std::ofstream* m_op_head_ptr;

        //! Write a component to file
        virtual bool WriteComponent(const PicArray& pic_data, const CompSort& cs);

        //! Open picture's header file for output
        virtual bool OpenHeader(const char* output_name);

        //! Open picture's YUV data file for output
        virtual bool OpenYUV(const char* output_name);
};

//! Picture input class
/*!
    Class for reading picture data from a file.
 */
class PicInput
{
    public:

        //! Constructor
        /*!
            Constructor, takes
            /param input_name the name of the input picture file
         */
        PicInput(const char* input_name);

        //! Destructor
        virtual ~PicInput();

        //! Set padding values to take into account block and transform sizes
        void SetPadding(const int xpd, const int ypd);

        //! Read the next frame from the file
        virtual bool ReadNextFrame(Frame& myframe);

        //! Read the picture header
        virtual bool ReadPicHeader();

        //! Skip n frames of input
        void Skip( const int n);


        //! Get the sequence parameters (got from the picture header)
        const SeqParams& GetSeqParams() const {return m_sparams;}

        //! Returns true if we're at the end of the input, false otherwise
        bool End() const ;

    protected:

        SeqParams m_sparams;
        std::ifstream* m_ip_pic_ptr;
        std::ifstream* m_ip_head_ptr;

        //padding values
        int m_xpad,m_ypad;

        //! Read a component from the file
        virtual bool ReadComponent(PicArray& pic_data,const CompSort& cs);
};

#endif
