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
* Contributor(s):   Tim Borer (Author),
                    Thomas Davies,
                    Scott R Ladd,
                    Peter Bleackley,
                    Steve Bearcroft,
                    Anuradha Suraparaju
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

#include <libdirac_common/arith_codec.h>

namespace dirac{
          
    unsigned int ContextLookupTable::table[256];

    ContextLookupTable::ContextLookupTable() {
        static bool done = false;
        if (!done) {
            unsigned int max_lookup = 1<<16;
            for (int weight=1; weight<256; ++weight)
                table[weight]= (max_lookup+weight/2)/weight;
            done = true;
        };
    }

    ArithCodecBase::ArithCodecBase(ByteIO* p_byteio, size_t number_of_contexts):
        m_context_list( number_of_contexts ),
        m_byteio(p_byteio ),
        m_decode_data_ptr( 0 ),
        m_scount( 0 )
    {
        // nothing needed here
    }

    ArithCodecBase::~ArithCodecBase() {
        delete[] m_decode_data_ptr; }

    
    void ArithCodecBase::InitEncoder()
    {
        // Set the m_code word stuff
        m_low_code  = 0;
        m_high_code = 0xffff;
        m_range = 0x10000;
        m_underflow = 0;

        InitContexts();
    }

    void ArithCodecBase::FlushEncoder()
    {
    	RenormEncoder();
    	
        m_byteio->OutputBit(m_low_code & CODE_2ND_MSB);
        while ( m_underflow >= 0 ) {
            m_byteio->OutputBit(~m_low_code & CODE_2ND_MSB);
            m_underflow -= 1; }

        // byte align
        m_byteio->ByteAlignOutput();
    }
    
    void ArithCodecBase::InitDecoder(int num_bytes)
    {
        InitContexts();
        ReadAllData(num_bytes);
        m_input_bits_left = 8;

        m_code = 0;
        m_low_code = 0;
        m_high_code = 0;
        m_range = 1;
        RenormDecoder();
    }

    int ArithCodecBase::ByteCount() const
    {
        return m_byteio->GetSize();
    }

    void ArithCodecBase::ReadAllData(int num_bytes)
    {
       if ( m_decode_data_ptr )
           delete[] m_decode_data_ptr;

       m_decode_data_ptr = new char[num_bytes+2];
       m_byteio->InputBytes( m_decode_data_ptr , num_bytes );
       m_decode_data_ptr[num_bytes] = 255;
       m_decode_data_ptr[num_bytes+1] = 255;

       m_data_ptr = m_decode_data_ptr;
    }

}// namespace dirac
