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
* Revision 1.4  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/04/11 22:54:13  chaoticcoyote
* Additional comments
*
* Revision 1.2  2004/04/06 18:06:53  chaoticcoyote
* Boilerplate for Doxygen comments; testing ability to commit into SF CVS
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

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
		/param	OutData	the output stream object pointer
     */
	BasicOutputManager(std::ostream* OutData ): 
	num_out_bytes(0),
	op_ptr(OutData)
	{
		InitOutputStream();
	}

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
	void WriteToFile();	// Write the buffer to the file 

	//! Return the number of bytes last output to file.
    /*!
        Return the number of bytes last output to file.
     */	
	unsigned int GetNumBytes() const {return num_out_bytes;}

    //! Size of the internal data cache in bytes.
    /*!
        Size of the internal data cache in bytes.
     */	
	unsigned int Size() const {return buffer.size();}

private:
	unsigned int num_out_bytes;//number of output bytes written
	std::ostream* op_ptr;
	std::vector<char> buffer;//buffer used to store output prior to saving to file
	char current_byte;		// Char used for temporary storage of op data bits
	int OutputMask;		// Used to set individual bit within the current header byte

	//functions 	
	void InitOutputStream();// Initialise the output stream.
	void FlushOutput();	// Clean out any remaining output bits to the buffer
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
		/param 	header	a BasicOutputManager object to handle the header      
		/param 	data	a BasicOutputManager object to handle the data
     */
	BitOutputManager(std::ostream* OutData ): 
	header(OutData),
	data(OutData),
	total_bytes(0),
	total_data_bytes(0),
	total_head_bytes(0),
	unit_bytes(0),
	unit_data_bytes(0),
	unit_head_bytes(0)
	{}

	//Copy constructor is default shallow copy

	//Operator= is default shallow=

	//! Destructor
	~BitOutputManager(){}

	//! Handles the header bits.
    /*!
        A BasicOutputManager object for handling the header bits.
     */
	BasicOutputManager header;

    //! Handles the data bits.
    /*!
        A BasicOutputManager object for handling the data bits.
     */	
	BasicOutputManager data;

    //! Writes the bit caches to file.
    /*!
        Writes the header bits to the ostream, followed by the data bits.
     */	
	void WriteToFile();

	    //! Returns the total number of bytes written in the last unit coded.
    /*!
        Returns the total number of bytes written in the last unit coded - header + data.
     */
	unsigned int GetUnitBytes() const {return unit_bytes;}	

    //! Returns the total number of data bytes written in the last unit coded.
	unsigned int GetUnitDataBytes() const {return unit_data_bytes;}

    //! Returns the total number of header bytes written in the last unit coded. 
	unsigned int GetUnitHeadBytes() const {return unit_head_bytes;}

    //! Returns the total number of bytes written to date (header and data). 
	unsigned int GetTotalBytes() const {return total_bytes;}

    //! Returns the total number of header bytes written to date. 
	unsigned int GetTotalHeadBytes() const {return total_head_bytes;}

    //! Returns the total number of data bytes written to date.
	unsigned int GetTotalDataBytes() const {return total_data_bytes;}

private:
	unsigned int total_bytes;//total number of bytes written to date- sum of:
	unsigned int total_data_bytes;//1) total number of data bytes written to date
	unsigned int total_head_bytes;//and 2) total number of header bytes written to date
	unsigned int unit_bytes;//total number of bytes written in the last unit coded - sum of: 	
	unsigned int unit_data_bytes;//1) number of data bytes for the last unit coded
	unsigned int unit_head_bytes;//and 2) number of data bytes for the last unit coded
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
	BitInputManager(std::istream* InData ): 
	ip_ptr(InData)
	{
		InitInputStream();
	}

	//Copy constructor is default shallow copy

	//Operator= is default shallow=	

	//! Destructor
	~BitInputManager(){}

	//input functions	
    //! Obtain the next bit.
	bool InputBit();			// Obtain the next bit	

    //! Obtain the next bit, incrementing count. 
	bool InputBit(int& count);	// Ditto, incrementing count	

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

	std::istream* ip_ptr;
	char current_byte;		// Char used for temporary storage of ip bits	
	int InputBitsLeft;		// The number of bits left withint the current input byte being decoded

	//functions 	
	void InitInputStream();	// Initialise the input stream	
};

#endif
