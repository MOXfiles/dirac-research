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
* Contributor(s): Thomas Davies (original author),
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

#include <libdirac_common/bit_manager.h>

using std::vector;

////////////////
//Output stuff//
////////////////

//Constructor
BasicOutputManager::BasicOutputManager(std::ostream* out_data ):
    m_num_out_bytes(0),
    m_op_ptr(out_data)
{
    InitOutputStream();
}

void BasicOutputManager::InitOutputStream()
{
    m_current_byte = 0;    // Set byte pointer to start of buffer
    m_output_mask = 0x80; // Set output mask to MSB of byte
    m_buffer.clear();    //reset the output buffer
}

void BasicOutputManager::OutputBit(const bool& bit )
{
    m_current_byte|=(bit? (m_output_mask):0);
    m_output_mask >>= 1; // Shift mask to next bit in the output byte

    if ( m_output_mask == 0 )
    { //if a whole byte has been written, write out
        m_output_mask = 0x80;
        m_buffer.push_back(m_current_byte);
        m_current_byte = 0;
    }    
}

void BasicOutputManager::OutputBit(const bool& bit, int& count)
{
    OutputBit(bit);
    count++;    
}

void BasicOutputManager::OutputByte(const char& byte)
{
    FlushOutput();
    m_buffer.push_back(byte);    
}

void BasicOutputManager::OutputBytes(char* str_array)
{
    FlushOutput();
    while (*str_array!=0)
    {
        m_buffer.push_back(*str_array);
        str_array++;
    }
}

void BasicOutputManager::OutputBytes(char* str_array,int num)
{
    FlushOutput();
    for ( int i=0 ; i<num ; ++i )
        m_buffer.push_back( str_array[i] );
}


void BasicOutputManager::WriteToFile()
{
    FlushOutput();
    for ( vector<char>::iterator it=m_buffer.begin() ; it!=m_buffer.end() ; ++it )
    {
        m_op_ptr->write(&(*it),1);        
    }
    m_num_out_bytes=m_buffer.size();
    InitOutputStream();        
}

void BasicOutputManager::FlushOutput(){
    // Flush the current byte to output buffer and reset
    if (m_output_mask!=0x80){
        m_buffer.push_back(m_current_byte);    
        m_current_byte=0;
        m_output_mask=0x80;
    }
}

BitOutputManager::BitOutputManager(std::ostream* out_data ):
    m_header(out_data),
    m_data(out_data),
    m_total_bytes(0),
    m_total_data_bytes(0),
    m_total_head_bytes(0),
    m_unit_bytes(0),
    m_unit_data_bytes(0),
    m_unit_head_bytes(0)
    {}

void BitOutputManager::WriteToFile()
{
    m_header.WriteToFile();
    m_data.WriteToFile();
    
    // after writing to file, get the number of unit bytes written
    m_unit_data_bytes=m_data.GetNumBytes();
    m_unit_head_bytes=m_header.GetNumBytes();
    m_unit_bytes=m_unit_data_bytes+m_unit_head_bytes;

    // increment the total numbers of bytes
    m_total_data_bytes+=m_unit_data_bytes;
    m_total_head_bytes+=m_unit_head_bytes;
    m_total_bytes+=m_unit_bytes;
}


////////////////
//Input stuff//
////////////////

//Constructor
BitInputManager::BitInputManager(std::istream* in_data ):
    m_ip_ptr(in_data)
{
    InitInputStream();
}


void BitInputManager::InitInputStream()
{
    m_input_bits_left = 0;
}

bool BitInputManager::InputBit()
{
    //assumes mode errors will be caught by iostream class    

    if (m_input_bits_left == 0)
    {
        m_ip_ptr->read(&m_current_byte,1);
        m_input_bits_left = 8;
    }

    m_input_bits_left--;

    return bool( ( m_current_byte >> m_input_bits_left ) & 1 );

}

bool BitInputManager::InputBit(int& count)
{
    count++;
    return InputBit();
}

bool BitInputManager::InputBit(int& count, const int max_count)
{
    if ( count<max_count )
    {
        count++;
        return InputBit();
    }
    else{
        return false;
    }
}

char BitInputManager::InputByte()
{
    FlushInput(); //forget about what's in the current byte    

    char byte;
    m_ip_ptr->read(&byte,1);

    return byte;    
}

void BitInputManager::InputBytes(char* cptr, int num)
{
    FlushInput(); //forget about what's in the current byte    
    m_ip_ptr->read(cptr,num);    
}

void BitInputManager::FlushInput()
{
    m_input_bits_left = 0;    
}

bool BitInputManager::End() const 
{
    return m_ip_ptr->eof();    
}
