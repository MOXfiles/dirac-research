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
*                 Robert Scott Ladd,
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

#ifndef _BIT_MANAGER_H_
#define _BIT_MANAGER_H_

#include <cstring>
#include <vector>
#include <iostream>

////////////////////////////////////////////////
//--------------Bit output stuff--------------//
////////////////////////////////////////////////

//! Class for managing bit- and byte-oriented output.
/*!
    A class for managing bit- and byte-oriented output. Wraps around an ostream object but stores 
    data in memory until told told to write out in order to support data re-ordering - for example
    writing a header once the subsequent data has been obtained. Implementation to be reviewed in
    future. TJD 13 April 2004.
*/
class BasicOutputManager{
    public:
        //! Constructor
        /*!
        Constructor requires an ostream object pointer.
        /param OutData the output stream object pointer
        */
        BasicOutputManager(std::ostream* OutData );

        //Copy constructor is default shallow copy

        //Operator= is default shallow=

        //! Destructor
        ~BasicOutputManager(){}

        //! Write a bit out. 
        /*!
        Write a bit out to the internal data cache.
        */ 
        void OutputBit(const bool& bit);

        //! Write a bit out and increment count 
        /*!
        Write a bit out to the internal data cache and increment the count of bits written.
        */
        void OutputBit(const bool& bit,int& count);// Ditto, incrementing count

        //! Write a byte out.
        /*!
        Write a byte out to the internal data cache.
        */
        void OutputByte(const char& byte);

        //! Write a null-terminated set of bytes out.
        /*!
        Write a null-terminated set of bytes out to the internal data cache.
        */   
        void OutputBytes(char* str_array);

        //! Write a number of bytes out.
        /*!
        Write a number of bytes out to the internal data cache.
        */   
        void OutputBytes(char* str_array,int num);

        //! Write all data to file.
        /*!
        Dump the internal data cache to the internal ostream object.
        */   
        void WriteToFile();    // Write the buffer to the file 

        //! Return the number of bytes last output to file.
        /*!
        Return the number of bytes last output to file.
        */   
        size_t GetNumBytes() const {return m_num_out_bytes;}

        //! Size of the internal data cache in bytes.
        /*!
        Size of the internal data cache in bytes.
        */   
        size_t Size() const {return m_buffer.size();}

    private:
        size_t m_num_out_bytes;       //number of output bytes written
        std::ostream* m_op_ptr;
        std::vector<char> m_buffer; //buffer used to store output prior to saving to file
        char m_current_byte;         //Char used for temporary storage of op data bits
        int m_output_mask;           //Used to set individual bit within the current header byte

        //functions  
        void InitOutputStream();    //Initialise the output stream.
        void FlushOutput();         //Clean out any remaining output bits to the buffer
};

//! A class for handling data output, including headers.
/*!
A class for handling data output, including headers and reordering.
*/
class BitOutputManager{
    public:
        //! Constructor.
        /*!
        Constructor wraps around a pointer to an ostream object, and initialises 
        /param header a BasicOutputManager object to handle the header
        /param data a BasicOutputManager object to handle the data
        */
        BitOutputManager(std::ostream* out_data ); 

        //Copy constructor is default shallow copy

        //Operator= is default shallow=

        //! Destructor
        ~BitOutputManager(){}

        //! Handles the header bits.
        /*!
        A BasicOutputManager object for handling the header bits.
        */
        BasicOutputManager& Header(){return m_header;}

        //! Handles the data bits.
        /*!
        A BasicOutputManager object for handling the data bits.
        */
        BasicOutputManager& Data(){return m_data;}

        //! Writes the bit caches to file.
        /*!
        Writes the header bits to the ostream, followed by the data bits.
        */
        void WriteToFile();

        //! Returns the total number of bytes written in the last unit coded.
        /*!
        Returns the total number of bytes written in the last unit coded - header + data.
        */
        size_t GetUnitBytes() const {return m_unit_bytes;}

        //! Returns the total number of data bytes written in the last unit coded.
        size_t GetUnitDataBytes() const {return m_unit_data_bytes;}

        //! Returns the total number of header bytes written in the last unit coded. 
        size_t GetUnitHeadBytes() const {return m_unit_head_bytes;}

        //! Returns the total number of bytes written to date (header and data). 
        size_t GetTotalBytes() const {return m_total_bytes;}

        //! Returns the total number of header bytes written to date. 
        size_t GetTotalHeadBytes() const {return m_total_head_bytes;}

        //! Returns the total number of data bytes written to date.
        size_t GetTotalDataBytes() const {return m_total_data_bytes;}

    private:
        //basic output managers for the header and data
        BasicOutputManager m_header,m_data;

        size_t m_total_bytes;     //total number of bytes written to date- sum of:
        size_t m_total_data_bytes; //1) total number of data bytes written to date
        size_t m_total_head_bytes; //and 2) total number of header bytes written to date
        size_t m_unit_bytes;      //total number of bytes written in the last unit coded - sum of: 
        size_t m_unit_data_bytes;  //1) number of data bytes for the last unit coded
        size_t m_unit_head_bytes;  //and 2) number of data bytes for the last unit coded
};

///////////////////////////////////////////////
//--------------Bit input stuff--------------//
///////////////////////////////////////////////

//! A class for managing bit-wise and byte-wise input. 
class BitInputManager{

    public:
        //! Constructor. 
        /*!
        Constructor. Wraps around an istream object.
        */
        BitInputManager(std::istream* in_data );

        //Copy constructor is default shallow copy

        //Operator= is default shallow=

        //! Destructor
        ~BitInputManager(){}

        //input functions
        //! Obtain the next bit.
        bool InputBit(); // Obtain the next bit

        //! Obtain the next bit, incrementing count. 
        bool InputBit(int& count); // Ditto, incrementing count

        //! Obtain the next bit, incrementing count, if count<max_count; else return 0 (false).
        bool InputBit(int& count, const int max_count);// Ditto, returns 0 if >=max_count

        //! Obtain the next byte. 
        char InputByte();

        //! Obtain a number of bytes. 
        void InputBytes(char* cptr,int num);

        //! Move onto the next byte. Needed if a data unit is not an exact number of bytes.
        void FlushInput();

        //! Returns true if we're at the end of the input, false otherwise
        bool End() const ;

    private:

        std::istream* m_ip_ptr;
        char m_current_byte;     // Char used for temporary storage of ip bits
        int m_input_bits_left;    // The number of bits left withint the current input byte being decoded

        //functions 
        void InitInputStream(); // Initialise the input stream
};

#endif
