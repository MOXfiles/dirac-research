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
* Revision 1.3  2004-04-11 22:54:13  chaoticcoyote
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



//! 
/*!

 */
class BasicOutputManager{
public:

    //! 
    /*!
        
     */
	//Constructors
	BasicOutputManager(std::ostream* OutData ): num_out_bytes(0),op_ptr(OutData){InitOutputStream();}

    //! 
    /*!
        
     */
	//Destructors
	~BasicOutputManager(){}


    //! 
    /*!
        
     */
	//output functions	
	void OutputBit(const bool& bit);// Write bit to data char and output to buffer if full

    //! 
    /*!
        
     */
	void OutputBit(const bool& bit,int& count);// Ditto, incrementing count

    //! 
    /*!
        
     */
	void OutputByte(const char& byte);

    //! 
    /*!
        
     */
	void OutputBytes(char* str_array);	

    //! 
    /*!
        
     */
	void WriteToFile();	// Write the buffer to the file 

    //! 
    /*!
        
     */
	unsigned int GetNumBytes(){return num_out_bytes;}//return the number of output bytes written at last output

    //! 
    /*!
        
     */
	unsigned int Size(){return buffer.size();}//return the current size of the output buffer

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



//! 
/*!

 */
class BitOutputManager{
public:

    //! 
    /*!
        
     */
	BitOutputManager(std::ostream* OutData )
        : header(OutData),
          data(OutData),
          total_bytes(0),
          total_data_bytes(0),
          total_head_bytes(0),
          unit_bytes(0),
          unit_data_bytes(0),
          unit_head_bytes(0){}

    //! 
    /*!
        
     */
	BasicOutputManager header;

    //! 
    /*!
        
     */
	BasicOutputManager data;

    //! 
    /*!
        
     */
	void WriteToFile();


    //! 
    /*!
        
     */
	unsigned int GetUnitBytes(){return unit_bytes;}	

    //! 
    /*!
        
     */
	unsigned int GetUnitDataBytes(){return unit_data_bytes;}

    //! 
    /*!
        
     */
	unsigned int GetUnitHeadBytes(){return unit_head_bytes;}

    //! 
    /*!
        
     */
	unsigned int GetTotalBytes(){return total_bytes;}

    //! 
    /*!
        
     */
	unsigned int GetTotalHeadBytes(){return total_head_bytes;}

    //! 
    /*!
        
     */
	unsigned int GetTotalDataBytes(){return total_data_bytes;}

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



//! 
/*!

 */
class BitInputManager{

public:

    //! 
    /*!
        
     */
	//Constructors
	BitInputManager(std::istream* InData ): ip_ptr(InData){InitInputStream();}

    //! 
    /*!
        
     */
	//Destructors
	~BitInputManager(){}


    //! 
    /*!
        
     */
	//input functions	
	bool InputBit();			// Obtain the next bit	

    //! 
    /*!
        
     */
	bool InputBit(int& count);	// Ditto, incrementing count	

    //! 
    /*!
        
     */
	bool InputBit(int& count, const int max_count);// Ditto, returns 0 if >=max_count

    //! 
    /*!
        
     */
	char input_byte();

    //! 
    /*!
        
     */
	void FlushInput();	// Reset ip current byte - needs to be public so we can read 

private:

	std::istream* ip_ptr;
	char current_byte;		// Char used for temporary storage of ip bits	
	int InputBitsLeft;		// The number of bits left withint the current input byte being decoded

	//functions 	
	void InitInputStream();	// Initialise the input stream	
};

#endif
