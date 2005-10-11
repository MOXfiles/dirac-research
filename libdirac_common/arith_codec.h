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
* The Original Code is BBC Research and Development m_code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s):    Richard Felton (Original Author),
                    Thomas Davies,
                    Scott R Ladd,
                    Peter Bleackley,
                    Steve Bearcroft
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


#ifndef _ARITH_CODEC_H_
#define _ARITH_CODEC_H_

/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
////                                                                 ////
////-----------Abstract binary arithmetic coding class---------------////
////subclass this for coding motion vectors, subband residues etc ...////
////                                                                 ////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////

#include <libdirac_common/common.h>
#include <libdirac_common/bit_manager.h>
#include <vector>

#ifdef _MSC_VER // define types for MSVC compiler on Windows
    typedef unsigned short    uint16_t;
    typedef unsigned _int32    uint32_t;
#else // include header file for types for Linux
    #include <inttypes.h>
#endif

namespace dirac
{
    //! Abstract binary arithmetic coding class
    /*!
        This is an abtract binary arithmetic encoding class, used as the base
        for concrete classes that encode motion vectors and subband residues.
        \param        T        a container (most probably, or array) type
    */


    template<class T> //T is container/array type
    class ArithCodec
    {
    public:

        //! Constructor for encoding
        /*!
            Creates an ArithCodec object to decode input based on a set of
            parameters.
            \param        bits_out     output for encoded bits
            \param    number_of_contexts    the number of contexts used
        */    
        ArithCodec(BasicOutputManager * bits_out, size_t number_of_contexts);

        //! Constructor for decoding
        /*!
            Creates an ArithCodec object to decode input based on a set of 
            parameters.
            \param    bits_in               source of bits to be decoded
            \param    number_of_contexts    the number of contexts used
         */
        ArithCodec(BitInputManager * bits_in, size_t number_of_contexts);

        //! Destructor
        /*!
            Destructor is virtual as this class is abstract.
         */ 
        virtual ~ArithCodec();

        //! Compresses the input and returns the number of bits written. 
        /*!
            Compress takes a type T object (a container or array) and 
            compresses it using the abstract function DoWorkCode() which
            is overridden in subclasses. It returns the number of 
            bits written. 
            \param    in_data    the input to be compressed. Non-const, 
            since the compression may be lossy.
         */
        int Compress(T & in_data);
    
        //! Decompresses the bitstream and writes into the output.
        /*!
            Decompresses the  bitstream, up to the number of bytes 
            specified and writes into the output subclasses.
            \param    out_data the output into which the decompressed data
            is written.
            \param    num_bytes    the number of bytes to be read from the 
            bitstream.
         */ 
        void Decompress(T & out_data, const int num_bytes);

    protected:

        // use explicity type sizes for portability
        typedef uint16_t code_t;
        typedef uint32_t calc_t;

        // NOTE: These macros imply an unsigned 16-bit operand
        static const code_t CODE_MAX     = 0xffff;
        static const code_t CODE_MSB     = ((0xffff + 1) >> 1);
        static const code_t CODE_2ND_MSB = ((0xffff + 1) >> 2);


        //! A class for binary contexts.
        /*!
            A class for binary contexts. Stores probabilities for 0 and 1 in 
            terms of counts of numbers of occurrences, and also as Triples 
            partitioning the interval [0,1) into two parts [0,p) and [p,1). 
         */
        class Context
        {
        public:
            //! Default Constructor.
            /*!
                Default constructor initialises counts to 1 each of 0 and 1.
             */    
            Context()
            {
                SetCounts( 1 , 1 );
            }

            //! Constructor.
            /*!
                Constructor initialises the counts to those set.
             */    
            Context(int cnt0,int cnt1)
            {
                SetCounts( cnt0 , cnt1 );
            }

            //! Copy constructor
            Context(const Context & cpy)
              : m_num0( cpy.m_num0 ),               
                m_weight( cpy.m_weight ),
                m_prob0( cpy.m_prob0 )
            {}

            //! Assignment=
            Context & operator=(const Context& rhs)
            {
                m_num0 = rhs.m_num0;
                m_weight = rhs.m_weight;
                m_prob0  = rhs.m_prob0;
                return *this;
            }

            //! Destructor
            ~Context() {}

            //! Sets the counts according to the input.
            /*!
                Sets the counts, and then the triples to reflect the counts.
             */    
            void SetCounts(const int cnt0, const int cnt1)
            {
                m_num0 = cnt0;
                m_weight = cnt0 + cnt1;

                SetRanges();
            }   

            //! Returns the count of all symbols.
            calc_t Weight() const { return m_weight; }    

            //! Increment the count by 1
            /*!
                Increment the count of symbol by 1.
                \param    symbol    the symbol whose count is to be incremented (false=0, true=1)
             */    
            inline void IncrCount( const bool symbol )
            {
                if (! symbol ) 
                    m_num0++;

                m_weight++;

                if ( m_weight & 1 )
                    SetRanges();
            }

             //! Divide the counts by 2, making sure neither ends up 0.
            inline void HalveCounts()
            {
                calc_t num1 = m_weight - m_num0;
                m_num0 >>= 1;
                m_num0++;
                num1 >>= 1;
                num1++;

                m_weight = m_num0 + num1;

                SetRanges();
            }

            //! Return the triple associated with Symbol.    
            const calc_t & GetScaledProb0( ) const { return m_prob0; }

            //! Given a number, return the corresponding symbol and triple.
            /*!
                Given a number, return the corresponding symbol and triple.
            */
            bool GetSymbol(const calc_t num, const calc_t factor) const
            {
                return (num >= (m_prob0*factor));
            } 

            inline void SetRanges()
            {
                // Updates the probability ranges
                m_prob0 =( ( m_num0 * m_lookup[m_weight-1] ) >> 21 );
            }

            inline void Update( const bool symbol )
            {
                IncrCount( symbol );

                if ( Weight() >= 1024 )
                    HalveCounts();
            }


        private:
            calc_t m_num0;
            calc_t m_weight;

            calc_t m_prob0;    

            /*!
            Counts m_num0 and m_num1 are scaled to 1024 before being used to
            make probability intervals for use in the arithmetic codec. This
            means calculating (m_num0*1024)/m_weight. m_lookup is a lookup
            table for avoiding doing this division directly. It's defined by

            m_lookup[k]=(1<<31)/m_weight

            So the calculation instead becomes 

            (m_num0*m_lookup[m_weight-1])>>21 

            The 21 comes from 1024/(1<<31).
 
            In principle we're multiplying two ints here so the result should
            be a long int to retain overflow before the bit shift. But it turns
            out that since m_num0<m_weight the overflow bits will always be
            zero so everything can be done with ints. 
            */
            static const unsigned int m_lookup[1024]; 
          

        };

    protected:

        //virtual codec functions (to be overridden)
        ////////////////////////////////////////////

        //! The method by which the contexts are initialised
        virtual void InitContexts()=0;                                        

        //! The method by which the counts are updated.
        void Update( const bool symbol , Context& ctx );    

        //! The method by which _all_ the counts are resized.
        virtual void ResetAll()=0;

        //virtual encode-only functions
        /////////////////////////////// 

        //! Does the work of actually coding the data 
        virtual void DoWorkCode(T & in_data) = 0;    

        //core encode-only functions
        ////////////////////////////

        //! Initialises the Encoder
        void InitEncoder();

        //! encodes a symbol and writes to output
        void EncodeSymbol(const bool symbol, const int context_num);    

        //! flushes the output of the encoder.
        void FlushEncoder(); 

        //! virtual decode-only functions    
        /////////////////////////////// 
        virtual void DoWorkDecode(T & out_data)=0;    

        // core decode-only functions
        ////////////////////////////

        //! Initialise the Decoder
        void InitDecoder();                    

        //! Decodes a symbol given a context number
        bool DecodeSymbol( const int context_num );

    private:
        //! count of the total number of bits input or output
        int m_bit_count;                        

        //! max number of bits to be input
        int m_max_count;                        

        //! Number of underflow bits
        int m_underflow;                        

        //! The present input code
        code_t m_code;                    

        //! Start of the current code range
        code_t m_low_code;                        

        //! End of the current code range
        code_t m_high_code;                    

        // Parameters for controlling coding/decoding
        // codec_params_type cparams;        

        //! Manages interface with file/stream
        BitInputManager* m_bit_input;                

        //! Manages interface with file/stream. Can be header or data
        BasicOutputManager* m_bit_output;

        //! private, bodyless copy constructor: class should not be copied
        ArithCodec(const ArithCodec & cpy);

        //! private, bodyless copy operator=: class should not be assigned
        ArithCodec & operator = (const ArithCodec & rhs);

        // For decoder only (could extend to the encoder later)

        //! A pointer to the data for reading in
        char* m_decode_data_ptr;

        //! A point to the byte currently being read
        char* m_data_ptr;

        //! The index of the bit of the byte being read
        int m_input_bits_left;

    private:

        //! Decoder-only function: read all the data in
        void ReadAllData();

        //! Read in a bit of data
        inline bool InputBit();




    protected:

        //! List of contexts
        std::vector<Context> m_context_list;    
    };

    //Implementation - core functions
    /////////////////////////////////

    template<class T>
    ArithCodec<T>::ArithCodec(BitInputManager* bits_in, size_t number_of_contexts)
      : m_bit_count( 0 ),
        m_bit_input( bits_in ),
        m_decode_data_ptr( 0 ),
        m_context_list( number_of_contexts )
    {
        // nothing needed here
    }    

    //! Constructor for encoding
    template<class T>
    ArithCodec<T>::ArithCodec(BasicOutputManager* bits_out, size_t number_of_contexts)
      : m_bit_count( 0 ),
        m_bit_output( bits_out ),
        m_decode_data_ptr( 0 ),
        m_context_list( number_of_contexts )
    {
        // nothing needed here
    }    

    template<class T>
    ArithCodec<T>::~ArithCodec()
    {
        if ( m_decode_data_ptr )
            delete[] m_decode_data_ptr;
    }

    template<class T>
    int ArithCodec<T>::Compress(T &in_data)
    {
        InitEncoder();                
        DoWorkCode(in_data);
        FlushEncoder();

        int byte_count( m_bit_count/8);
        if ( (byte_count*8)<m_bit_count )
            byte_count++;

        return byte_count;
    }

    template<class T>
    void ArithCodec<T>::Decompress( T &out_data, const int num_bytes )
    {
        m_max_count = num_bytes;
        InitDecoder();
        DoWorkDecode( out_data );
    }

    template<class T>
    void ArithCodec<T>::InitEncoder()
    {
        // Set the m_code word stuff
        m_low_code  = 0;
        m_high_code = CODE_MAX;
        m_underflow = 0;

        InitContexts();
    }

    template<class T>
    inline void ArithCodec<T>::EncodeSymbol(const bool symbol, const int context_num)
    {
        Context& ctx  = m_context_list[context_num];

        const calc_t range_prob( static_cast<calc_t>( (m_high_code - m_low_code ) + 1) * ctx.GetScaledProb0() );

        //formulae given we know we're binary coding    
        if ( !symbol ) // symbol is 0, so m_low_code unchanged 
            m_high_code = m_low_code + static_cast<code_t>( ( ( range_prob  )>>10 ) - 1 );
        else //symbol is 1, so m_high_code unchanged
            m_low_code += static_cast<code_t>(( range_prob ) >>10 );                

        do
        {
            if (( m_high_code & CODE_MSB ) == ( m_low_code & CODE_MSB ))
            {
                m_bit_output->OutputBit( m_high_code & CODE_MSB, m_bit_count);
                for (; m_underflow > 0; m_underflow-- )
                    m_bit_output->OutputBit(~m_high_code & CODE_MSB, m_bit_count);
            }

            else if ( ( m_low_code & CODE_2ND_MSB ) && !( m_high_code & CODE_2ND_MSB ))
            {
                m_underflow ++;
                m_low_code  ^= CODE_2ND_MSB;
                m_high_code ^= CODE_2ND_MSB;
            }
            else break;

            m_low_code  <<= 1;
            m_high_code <<= 1;
            m_high_code ++;
        }
        while ( true );
        
        ctx.Update( symbol );
    }

    template<class T>
    void ArithCodec<T>::FlushEncoder()
    {
        // Flushes the output
        m_bit_output->OutputBit(m_low_code & CODE_2ND_MSB,m_bit_count);
        m_underflow++;

        while ( m_underflow-- > 0 )
            m_bit_output->OutputBit(~m_low_code & CODE_2ND_MSB, m_bit_count);
    }

    template<class T>
    void ArithCodec<T>::InitDecoder()
    {
        InitContexts();

        m_input_bits_left = 8; 

        ReadAllData();

        //Read in a full word of data
        code_t i;
        m_code = 0;

        for ( i = 0; i < (8 * sizeof(code_t)); i++ )
        {
            m_code <<= 1;

            if ( InputBit() )
                m_code++;
        }

        m_low_code  = 0;
        m_high_code = CODE_MAX;
        m_underflow = 0;
    }

 

    template<class T>
    inline bool ArithCodec<T>::DecodeSymbol( const int context_num )
    {
        Context& ctx  = m_context_list[context_num];

        const calc_t count( ( ( static_cast<calc_t>( m_code - m_low_code ) + 1 )<<10 ) - 1 );

        const calc_t range_prob( ( m_high_code - m_low_code  + 1)* ctx.GetScaledProb0() );

        bool symbol( count >= range_prob );

        if( !symbol )//prob_interval.Start()=0, so symbol is 0, so m_low_code unchanged 
            m_high_code = m_low_code + static_cast<code_t>( ( ( range_prob )>>10 ) - 1 );

        else//symbol is 1, so m_high_code unchanged
            m_low_code += static_cast<code_t>(( range_prob )>>10 );        

        do
        {        
            if ( ( m_high_code & CODE_MSB ) == ( m_low_code & CODE_MSB ) )
            {
                // Do nothing
            }        
            else if ( (m_low_code & CODE_2ND_MSB) && !(m_high_code & CODE_2ND_MSB) )
            {
                m_code      ^= CODE_2ND_MSB;
                m_low_code  ^= CODE_2ND_MSB;
                m_high_code ^= CODE_2ND_MSB;
            }        
            else break;

            m_low_code  <<= 1;

            m_high_code <<= 1;
            ++m_high_code;

            m_code      <<= 1;
            m_code += InputBit();

        } while ( true );

        ctx.Update( symbol );

        return symbol;
    }

    template<class T>
    void ArithCodec<T>::ReadAllData()
    {
       if ( m_decode_data_ptr )
           delete[] m_decode_data_ptr;

       m_decode_data_ptr = new char[m_max_count + 2];
       m_bit_input->InputBytes( m_decode_data_ptr , m_max_count );

       m_decode_data_ptr[m_max_count] = 0;
       m_decode_data_ptr[m_max_count+1] = 0;

       m_data_ptr = m_decode_data_ptr;

    }

    template<class T>
    inline bool ArithCodec<T>::InputBit()
    {
        if (m_input_bits_left == 0)
        {
            m_data_ptr++;
            m_input_bits_left = 8;
        }
        m_input_bits_left--;

        return bool( ( (*m_data_ptr) >> m_input_bits_left ) & 1 );
    }
    
    template<class T>
    const unsigned int ArithCodec<T>::Context::m_lookup[1024] = {0x80000000,
0x40000000,0x2AAAAAAA,0x20000000,0x19999999,0x15555555,0x12492492,0x10000000,0xE38E38E,
0xCCCCCCC,0xBA2E8BA,0xAAAAAAA,0x9D89D89,0x9249249,0x8888888,0x8000000,
0x7878787,0x71C71C7,0x6BCA1AF,0x6666666,0x6186186,0x5D1745D,0x590B216,
0x5555555,0x51EB851,0x4EC4EC4,0x4BDA12F,0x4924924,0x469EE58,0x4444444,
0x4210842,0x4000000,0x3E0F83E,0x3C3C3C3,0x3A83A83,0x38E38E3,0x3759F22,
0x35E50D7,0x3483483,0x3333333,0x31F3831,0x30C30C3,0x2FA0BE8,0x2E8BA2E,
0x2D82D82,0x2C8590B,0x2B93105,0x2AAAAAA,0x29CBC14,0x28F5C28,0x2828282,
0x2762762,0x26A439F,0x25ED097,0x253C825,0x2492492,0x23EE08F,0x234F72C,
0x22B63CB,0x2222222,0x2192E29,0x2108421,0x2082082,0x2000000,0x1F81F81,
0x1F07C1F,0x1E9131A,0x1E1E1E1,0x1DAE607,0x1D41D41,0x1CD8568,0x1C71C71,
0x1C0E070,0x1BACF91,0x1B4E81B,0x1AF286B,0x1A98EF6,0x1A41A41,0x19EC8E9,
0x1999999,0x1948B0F,0x18F9C18,0x18ACB90,0x1861861,0x1818181,0x17D05F4,
0x178A4C8,0x1745D17,0x1702E05,0x16C16C1,0x1681681,0x1642C85,0x1605816,
0x15C9882,0x158ED23,0x1555555,0x151D07E,0x14E5E0A,0x14AFD6A,0x147AE14,
0x1446F86,0x1414141,0x13E22CB,0x13B13B1,0x1381381,0x13521CF,0x1323E34,
0x12F684B,0x12C9FB4,0x129E412,0x127350B,0x1249249,0x121FB78,0x11F7047,
0x11CF06A,0x11A7B96,0x1181181,0x115B1E5,0x1135C81,0x1111111,0x10ECF56,
0x10C9714,0x10A6810,0x1084210,0x10624DD,0x1041041,0x1020408,0x1000000,
0xFE03F8,0xFC0FC0,0xFA232C,0xF83E0F,0xF6603D,0xF4898D,0xF2B9D6,
0xF0F0F0,0xEF2EB7,0xED7303,0xEBBDB2,0xEA0EA0,0xE865AC,0xE6C2B4,
0xE52598,0xE38E38,0xE1FC78,0xE07038,0xDEE95C,0xDD67C8,0xDBEB61,
0xDA740D,0xD901B2,0xD79435,0xD62B80,0xD4C77B,0xD3680D,0xD20D20,
0xD0B69F,0xCF6474,0xCE168A,0xCCCCCC,0xCB8727,0xCA4587,0xC907DA,
0xC7CE0C,0xC6980C,0xC565C8,0xC4372F,0xC30C30,0xC1E4BB,0xC0C0C0,
0xBFA02F,0xBE82FA,0xBD6910,0xBC5264,0xBB3EE7,0xBA2E8B,0xB92143,
0xB81702,0xB70FBB,0xB60B60,0xB509E6,0xB40B40,0xB30F63,0xB21642,
0xB11FD3,0xB02C0B,0xAF3ADD,0xAE4C41,0xAD602B,0xAC7691,0xAB8F69,
0xAAAAAA,0xA9C84A,0xA8E83F,0xA80A80,0xA72F05,0xA655C4,0xA57EB5,
0xA4A9CF,0xA3D70A,0xA3065E,0xA237C3,0xA16B31,0xA0A0A0,0x9FD809,
0x9F1165,0x9E4CAD,0x9D89D8,0x9CC8E1,0x9C09C0,0x9B4C6F,0x9A90E7,
0x99D722,0x991F1A,0x9868C8,0x97B425,0x97012E,0x964FDA,0x95A025,
0x94F209,0x944580,0x939A85,0x92F113,0x924924,0x91A2B3,0x90FDBC,
0x905A38,0x8FB823,0x8F1779,0x8E7835,0x8DDA52,0x8D3DCB,0x8CA29C,
0x8C08C0,0x8B7034,0x8AD8F2,0x8A42F8,0x89AE40,0x891AC7,0x888888,
0x87F780,0x8767AB,0x86D905,0x864B8A,0x85BF37,0x853408,0x84A9F9,
0x842108,0x839930,0x83126E,0x828CBF,0x820820,0x81848D,0x810204,
0x808080,0x800000,0x7F807F,0x7F01FC,0x7E8472,0x7E07E0,0x7D8C42,
0x7D1196,0x7C97D9,0x7C1F07,0x7BA71F,0x7B301E,0x7ABA01,0x7A44C6,
0x79D06A,0x795CEB,0x78EA45,0x787878,0x780780,0x77975B,0x772807,
0x76B981,0x764BC8,0x75DED9,0x7572B2,0x750750,0x749CB2,0x7432D6,
0x73C9B9,0x73615A,0x72F9B6,0x7292CC,0x722C99,0x71C71C,0x716253,
0x70FE3C,0x709AD4,0x70381C,0x6FD60F,0x6F74AE,0x6F13F5,0x6EB3E4,
0x6E5478,0x6DF5B0,0x6D978B,0x6D3A06,0x6CDD21,0x6C80D9,0x6C252C,
0x6BCA1A,0x6B6FA1,0x6B15C0,0x6ABC74,0x6A63BD,0x6A0B99,0x69B406,
0x695D04,0x690690,0x68B0AA,0x685B4F,0x680680,0x67B23A,0x675E7C,
0x670B45,0x66B893,0x666666,0x6614BC,0x65C393,0x6572EC,0x6522C3,
0x64D319,0x6483ED,0x64353C,0x63E706,0x639949,0x634C06,0x62FF3A,
0x62B2E4,0x626703,0x621B97,0x61D09E,0x618618,0x613C03,0x60F25D,
0x60A928,0x606060,0x601806,0x5FD017,0x5F8895,0x5F417D,0x5EFACE,
0x5EB488,0x5E6EA9,0x5E2932,0x5DE420,0x5D9F73,0x5D5B2B,0x5D1745,
0x5CD3C3,0x5C90A1,0x5C4DE1,0x5C0B81,0x5BC980,0x5B87DD,0x5B4698,
0x5B05B0,0x5AC524,0x5A84F3,0x5A451C,0x5A05A0,0x59C67C,0x5987B1,
0x59493E,0x590B21,0x58CD5A,0x588FE9,0x5852CD,0x581605,0x57D990,
0x579D6E,0x57619F,0x572620,0x56EAF3,0x56B015,0x567587,0x563B48,
0x560158,0x55C7B4,0x558E5E,0x555555,0x551C97,0x54E425,0x54ABFD,
0x54741F,0x543C8B,0x540540,0x53CE3D,0x539782,0x53610E,0x532AE2,
0x52F4FB,0x52BF5A,0x5289FE,0x5254E7,0x522014,0x51EB85,0x51B738,
0x51832F,0x514F67,0x511BE1,0x50E89C,0x50B598,0x5082D4,0x505050,
0x501E0B,0x4FEC04,0x4FBA3D,0x4F88B2,0x4F5766,0x4F2656,0x4EF583,
0x4EC4EC,0x4E9490,0x4E6470,0x4E348B,0x4E04E0,0x4DD56F,0x4DA637,
0x4D7739,0x4D4873,0x4D19E6,0x4CEB91,0x4CBD73,0x4C8F8D,0x4C61DD,
0x4C3464,0x4C0720,0x4BDA12,0x4BAD3A,0x4B8097,0x4B5428,0x4B27ED,
0x4AFBE6,0x4AD012,0x4AA472,0x4A7904,0x4A4DC9,0x4A22C0,0x49F7E8,
0x49CD42,0x49A2CD,0x497889,0x494E75,0x492492,0x48FADE,0x48D159,
0x48A804,0x487EDE,0x4855E6,0x482D1C,0x480480,0x47DC11,0x47B3D0,
0x478BBC,0x4763D5,0x473C1A,0x47148B,0x46ED29,0x46C5F1,0x469EE5,
0x467804,0x46514E,0x462AC2,0x460460,0x45DE28,0x45B81A,0x459235,
0x456C79,0x4546E6,0x45217C,0x44FC3A,0x44D720,0x44B22E,0x448D63,
0x4468C0,0x444444,0x441FEE,0x43FBC0,0x43D7B7,0x43B3D5,0x439019,
0x436C82,0x434911,0x4325C5,0x43029E,0x42DF9B,0x42BCBD,0x429A04,
0x42776E,0x4254FC,0x4232AE,0x421084,0x41EE7C,0x41CC98,0x41AAD6,
0x418937,0x4167BA,0x41465F,0x412527,0x410410,0x40E31A,0x40C246,
0x40A193,0x408102,0x406090,0x404040,0x402010,0x400000,0x3FE00F,
0x3FC03F,0x3FA08F,0x3F80FE,0x3F618C,0x3F4239,0x3F2305,0x3F03F0,
0x3EE4F9,0x3EC621,0x3EA767,0x3E88CB,0x3E6A4D,0x3E4BEC,0x3E2DA9,
0x3E0F83,0x3DF17B,0x3DD38F,0x3DB5C1,0x3D980F,0x3D7A79,0x3D5D00,
0x3D3FA4,0x3D2263,0x3D053E,0x3CE835,0x3CCB47,0x3CAE75,0x3C91BE,
0x3C7522,0x3C58A2,0x3C3C3C,0x3C1FF0,0x3C03C0,0x3BE7A9,0x3BCBAD,
0x3BAFCB,0x3B9403,0x3B7855,0x3B5CC0,0x3B4145,0x3B25E4,0x3B0A9B,
0x3AEF6C,0x3AD456,0x3AB959,0x3A9E74,0x3A83A8,0x3A68F4,0x3A4E59,
0x3A33D6,0x3A196B,0x39FF18,0x39E4DC,0x39CAB9,0x39B0AD,0x3996B8,
0x397CDB,0x396315,0x394966,0x392FCD,0x39164C,0x38FCE2,0x38E38E,
0x38CA50,0x38B129,0x389818,0x387F1E,0x386639,0x384D6A,0x3834B1,
0x381C0E,0x380380,0x37EB07,0x37D2A4,0x37BA57,0x37A21E,0x3789FA,
0x3771EC,0x3759F2,0x37420C,0x372A3C,0x371280,0x36FAD8,0x36E345,
0x36CBC5,0x36B45A,0x369D03,0x3685C0,0x366E90,0x365774,0x36406C,
0x362977,0x361296,0x35FBC8,0x35E50D,0x35CE65,0x35B7D0,0x35A14F,
0x358AE0,0x357483,0x355E3A,0x354803,0x3531DE,0x351BCC,0x3505CC,
0x34EFDE,0x34DA03,0x34C439,0x34AE82,0x3498DC,0x348348,0x346DC5,
0x345855,0x3442F5,0x342DA7,0x34186B,0x340340,0x33EE26,0x33D91D,
0x33C425,0x33AF3E,0x339A68,0x3385A2,0x3370ED,0x335C49,0x3347B6,
0x333333,0x331EC0,0x330A5E,0x32F60B,0x32E1C9,0x32CD98,0x32B976,
0x32A564,0x329161,0x327D6F,0x32698C,0x3255BA,0x3241F6,0x322E42,
0x321A9E,0x320708,0x31F383,0x31E00C,0x31CCA4,0x31B94C,0x31A603,
0x3192C8,0x317F9D,0x316C80,0x315972,0x314672,0x313381,0x31209F,
0x310DCB,0x30FB06,0x30E84F,0x30D5A6,0x30C30C,0x30B07F,0x309E01,
0x308B91,0x30792E,0x3066DA,0x305494,0x30425B,0x303030,0x301E12,
0x300C03,0x2FFA00,0x2FE80B,0x2FD624,0x2FC44A,0x2FB27D,0x2FA0BE,
0x2F8F0C,0x2F7D67,0x2F6BCF,0x2F5A44,0x2F48C6,0x2F3754,0x2F25F0,
0x2F1499,0x2F034E,0x2EF210,0x2EE0DE,0x2ECFB9,0x2EBEA1,0x2EAD95,
0x2E9C96,0x2E8BA2,0x2E7ABC,0x2E69E1,0x2E5913,0x2E4850,0x2E379A,
0x2E26F0,0x2E1652,0x2E05C0,0x2DF53A,0x2DE4C0,0x2DD451,0x2DC3EE,
0x2DB397,0x2DA34C,0x2D930C,0x2D82D8,0x2D72AF,0x2D6292,0x2D5280,
0x2D4279,0x2D327E,0x2D228E,0x2D12A9,0x2D02D0,0x2CF301,0x2CE33E,
0x2CD386,0x2CC3D8,0x2CB436,0x2CA49F,0x2C9512,0x2C8590,0x2C7619,
0x2C66AD,0x2C574B,0x2C47F4,0x2C38A8,0x2C2966,0x2C1A2F,0x2C0B02,
0x2BFBE0,0x2BECC8,0x2BDDBA,0x2BCEB7,0x2BBFBE,0x2BB0CF,0x2BA1EA,
0x2B9310,0x2B843F,0x2B7579,0x2B66BD,0x2B580A,0x2B4962,0x2B3AC3,
0x2B2C2F,0x2B1DA4,0x2B0F23,0x2B00AC,0x2AF23E,0x2AE3DA,0x2AD580,
0x2AC72F,0x2AB8E8,0x2AAAAA,0x2A9C76,0x2A8E4B,0x2A802A,0x2A7212,
0x2A6403,0x2A55FE,0x2A4802,0x2A3A0F,0x2A2C26,0x2A1E45,0x2A106E,
0x2A02A0,0x29F4DA,0x29E71E,0x29D96B,0x29CBC1,0x29BE1F,0x29B087,
0x29A2F7,0x299571,0x2987F3,0x297A7D,0x296D11,0x295FAD,0x295251,
0x2944FF,0x2937B5,0x292A73,0x291D3A,0x29100A,0x2902E2,0x28F5C2,
0x28E8AB,0x28DB9C,0x28CE95,0x28C197,0x28B4A1,0x28A7B3,0x289ACE,
0x288DF0,0x28811B,0x28744E,0x286789,0x285ACC,0x284E17,0x28416A,
0x2834C5,0x282828,0x281B92,0x280F05,0x280280,0x27F602,0x27E98C,
0x27DD1E,0x27D0B8,0x27C459,0x27B802,0x27ABB3,0x279F6B,0x27932B,
0x2786F2,0x277AC1,0x276E98,0x276276,0x27565B,0x274A48,0x273E3C,
0x273238,0x27263B,0x271A45,0x270E57,0x270270,0x26F690,0x26EAB7,
0x26DEE6,0x26D31B,0x26C758,0x26BB9C,0x26AFE7,0x26A439,0x269893,
0x268CF3,0x26815A,0x2675C8,0x266A3D,0x265EB9,0x26533C,0x2647C6,
0x263C57,0x2630EE,0x26258C,0x261A32,0x260EDD,0x260390,0x25F849,
0x25ED09,0x25E1D0,0x25D69D,0x25CB71,0x25C04B,0x25B52C,0x25AA14,
0x259F02,0x2593F6,0x2588F1,0x257DF3,0x2572FB,0x256809,0x255D1E,
0x255239,0x25475A,0x253C82,0x2531B0,0x2526E4,0x251C1F,0x251160,
0x2506A7,0x24FBF4,0x24F147,0x24E6A1,0x24DC01,0x24D166,0x24C6D2,
0x24BC44,0x24B1BC,0x24A73A,0x249CBF,0x249249,0x2487D9,0x247D6F,
0x24730B,0x2468AC,0x245E54,0x245402,0x2449B5,0x243F6F,0x24352E,
0x242AF3,0x2420BD,0x24168E,0x240C64,0x240240,0x23F821,0x23EE08,
0x23E3F5,0x23D9E8,0x23CFE0,0x23C5DE,0x23BBE1,0x23B1EA,0x23A7F9,
0x239E0D,0x239426,0x238A45,0x23806A,0x237694,0x236CC3,0x2362F8,
0x235933,0x234F72,0x2345B7,0x233C02,0x233251,0x2328A7,0x231F01,
0x231561,0x230BC5,0x230230,0x22F89F,0x22EF14,0x22E58E,0x22DC0D,
0x22D291,0x22C91A,0x22BFA9,0x22B63C,0x22ACD5,0x22A373,0x229A16,
0x2290BE,0x22876B,0x227E1D,0x2274D4,0x226B90,0x226251,0x225917,
0x224FE1,0x2246B1,0x223D86,0x223460,0x222B3E,0x222222,0x22190A,
0x220FF7,0x2206E9,0x21FDE0,0x21F4DB,0x21EBDB,0x21E2E1,0x21D9EA,
0x21D0F9,0x21C80C,0x21BF24,0x21B641,0x21AD62,0x21A488,0x219BB3,
0x2192E2,0x218A16,0x21814F,0x21788C,0x216FCD,0x216714,0x215E5E,
0x2155AE,0x214D02,0x21445A,0x213BB7,0x213318,0x212A7E,0x2121E8,
0x211957,0x2110CA,0x210842,0x20FFBE,0x20F73E,0x20EEC3,0x20E64C,
0x20DDD9,0x20D56B,0x20CD01,0x20C49B,0x20BC3A,0x20B3DD,0x20AB84,
0x20A32F,0x209ADF,0x209293,0x208A4B,0x208208,0x2079C8,0x20718D,
0x206956,0x206123,0x2058F4,0x2050C9,0x2048A3,0x204081,0x203862,
0x203048,0x202832,0x202020,0x201812,0x201008,0x200802,0x200000};


}// end dirac namespace

#endif
