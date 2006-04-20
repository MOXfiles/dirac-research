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
* Contributor(s): Andrew Kennedy (Original Author),
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

#include <libdirac_byteio/byteio.h>
using namespace dirac;
using namespace std;

ByteIO::ByteIO(bool new_stream):
m_current_byte(0),
m_current_pos(0),
m_num_bytes(0),
m_new_stream(true)
{
    if(new_stream)
        mp_stream = new stringstream(stringstream::in | stringstream::out |
                                     stringstream::binary);

                                    
}

ByteIO::ByteIO(const ByteIO& stream_data):
m_current_byte(0),
m_current_pos(0),
m_num_bytes(0),
m_new_stream(false)
{
     mp_stream=stream_data.mp_stream;
}


ByteIO::~ByteIO()
{
    if (m_new_stream)
        delete mp_stream;
}

const string ByteIO::GetBytes() 
{
    return mp_stream->str();
}

int ByteIO::GetSize() const
{
    return m_num_bytes;
}

void ByteIO::SetByteParams(const ByteIO& byte_io)
{
    mp_stream=byte_io.mp_stream;
    m_current_byte=byte_io.m_current_byte;
    m_current_pos=byte_io.m_current_pos;
}

//----------protected---------------------------------------------------------------

void ByteIO::ByteAlignInput()
{
    m_current_pos=0;
    m_current_byte=0;
}

void ByteIO::ByteAlignOutput()
{
    if(m_current_pos!=0)
        OutputCurrentByte();
}

bool ByteIO::InputBit()
{
    if(m_current_pos == CHAR_BIT)
        m_current_pos=0;

    if (m_current_pos == 0)
        m_current_byte = InputUnByte();
#if 1
    // MSB to LSB
    return GetBit(m_current_byte, (CHAR_BIT-1-m_current_pos++));
#else
    // LSB to MSB
    return GetBit(m_current_byte, m_current_pos++);
#endif
}

int ByteIO::InputSignedGolombValue()
{

    int val = InputUnGolombValue();
    bool bit;

     //get the sign
    if (val != 0)
    {
        bit = InputBit();
        if (!bit )
            val = -val;
    }
    return val;        
}

int ByteIO::InputUnGolombValue()
{
    unsigned int M = 0;
    unsigned int info = 0;
    bool bit = 0;
    unsigned int val = 0;

    do
    {
        bit = InputBit();
        if (bit == BIT_ZERO)
            M++;
    }
    while( bit==BIT_ZERO && M<64 );//terminate if the number is too big!

    //now get the M info bits    
    for ( unsigned int i=0 ; i<M ; ++i)
    {
        bit = InputBit();
        if (bit == BIT_ONE )
            info |= (1<<i);
    }// i    
    val = (1<<M) -1 + info;

    return static_cast<int>(val);
}


void ByteIO::OutputBit(const bool& bit)
{
    if(bit)
#if 1
        // MSB to LSB
        SetBit(m_current_byte, CHAR_BIT-1-m_current_pos);
#else
        // LSB to MSB
        SetBit(m_current_byte, m_current_pos);
#endif

    if ( m_current_pos == CHAR_BIT-1)
    { 
        // If a whole byte has been written, output to stream
        OutputCurrentByte();
        m_current_byte = 0;
        m_current_pos = 0;
    }    
    else
      // Shift mask to next bit in the output byte
        ++m_current_pos;
}

void ByteIO::OutputSignedGolombValue(const int val)
{
    //output magnitude
    OutputUnGolombValue(abs(val));

    //do sign
    if (val>0) OutputBit(1);
    else if (val<0) OutputBit(0);
}

void ByteIO::OutputUnGolombValue(const int& value)
{
    // convert to colomb format
    unsigned int u_value = static_cast<const unsigned int>(value);
    unsigned int M = 0;
    unsigned int info;

    u_value++;
    while (u_value>1)
        {//get the log base 2 of val.
        u_value >>= 1;
        M++;        
    }
    info = value - (1<<M) + 1;

    //add length M+1 prefix
    for ( unsigned int i=1 ; i<=M ; ++i)
        OutputBit(BIT_ZERO);
    
    OutputBit(BIT_ONE);

    //add info bits
    for (unsigned int i=0 ; i<M ;++i)
        OutputBit( info & (1<<i) );        
}

void ByteIO::RemoveRedundantBytes(const int size)
{
    int prev_pos = mp_stream->tellg();
    string data=mp_stream->str();
    data.erase(0, size);
    mp_stream->str(data);
    m_num_bytes=data.size();
    if(data.size())
        SeekGet(max(prev_pos-size, 0), ios_base::beg);
}
