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
* Revision 1.2  2004-04-11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "bit_manager.h"

using std::vector;

////////////////
//Output stuff//
////////////////

void BasicOutputManager::InitOutputStream(){

	current_byte= 0;	// Set byte pointer to start of buffer
	OutputMask = 0x80;		// Set output mask to MSB of byte
	buffer.clear();		//reset the output buffer
}

void BasicOutputManager::OutputBit(const bool& bit ){

	current_byte|=(bit? (OutputMask):0);
	OutputMask >>= 1;						// Shift mask to next bit in the output byte

	if ( OutputMask == 0 ){//if a whole byte has been written, write out
		OutputMask = 0x80;
		buffer.push_back(current_byte);
		current_byte= 0;
	}	
}

void BasicOutputManager::OutputBit(const bool& bit, int& count){
	OutputBit(bit);
	count++;	
}

void BasicOutputManager::OutputByte(const char& byte){
	FlushOutput();
	buffer.push_back(byte);	
}
void BasicOutputManager::OutputBytes(char* str_array){
	FlushOutput();
	while (*str_array!=0){
		buffer.push_back(*str_array);
		str_array++;
	}
}


void BasicOutputManager::WriteToFile(){
	FlushOutput();
	for (std::vector<char>::iterator it=buffer.begin();it!=buffer.end();++it){
		op_ptr->write(&(*it),1);		
	}
	num_out_bytes=buffer.size();
	InitOutputStream();		
}

void BasicOutputManager::FlushOutput(){

	// Flush the current byte to output buffer and reset

	if (OutputMask!=0x80){
		buffer.push_back(current_byte);	
		current_byte=0;
		OutputMask=0x80;
	}
}

void BitOutputManager::WriteToFile(){

	header.WriteToFile();data.WriteToFile();

	//after writing to file, get the number of unit bytes written
	unit_data_bytes=data.GetNumBytes();
	unit_head_bytes=header.GetNumBytes();
	unit_bytes=unit_data_bytes+unit_head_bytes;
	total_data_bytes+=unit_data_bytes;
	total_head_bytes+=unit_head_bytes;
	total_bytes+=unit_bytes;
}


////////////////
//Input stuff//
////////////////

void BitInputManager::InitInputStream(){
	InputBitsLeft = 0;
}

bool BitInputManager::InputBit(){
	//assumes mode errors will be caught by iostream class	
	if (InputBitsLeft == 0){
		//		if(*ip_ptr){
		ip_ptr->read(&current_byte,1);
		InputBitsLeft = 8;
	// 	}
	// 	else{
	// 		std::cerr<<"eof";
	// 		return false;
	// 	}
	}
	InputBitsLeft--;
	return bool( ( current_byte >> InputBitsLeft ) & 1 );
}

bool BitInputManager::InputBit(int& count){
	count++;
	return InputBit();
}

bool BitInputManager::InputBit(int& count, const int max_count){
	if (count<max_count){
		count++;
		return InputBit();
	}
	else{
		return false;
	}
}

char BitInputManager::input_byte(){
	FlushInput();//forget about what's in the current byte	
	char byte;
	ip_ptr->read(&byte,1);
	return byte;	
}

void BitInputManager::FlushInput(){
	InputBitsLeft=0;	
}
