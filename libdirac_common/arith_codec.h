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
                    Scott R Ladd
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

        //! A class for encapsulating interval fractions for use in arithmetic coding.
        /*!
             A class for encapsulating a subinterval of the unit interval [0,1) 
             (0<=x<1) as a start value, a stop value (numerators) and a weight 
             value (the denominator). The interval is the to be interpreted as
             [m_start/m_weight,m_stop/m_weight).
         */
        class Triple
        {
        public:
            //! Constructor.
            Triple()
              : m_start(0),
                m_stop(0),
                m_weight(0) {}

            //! Value constructor
            Triple(code_t start, code_t stop, code_t weight)
            {
                m_start  = start;
                m_stop   = stop;
                m_weight = weight;
            }

            //! Copy constructor
            Triple(const Triple& rhs)
              : m_start(rhs.m_start),
                m_stop(rhs.m_stop),
                m_weight(rhs.m_weight) { }

            //! Assignment 
            Triple & operator = (const Triple& rhs)
            {
                m_start  = rhs.m_start;
                m_stop   = rhs.m_stop;
                m_weight = rhs.m_weight;
                return *this;
            }

            //! Get the start value    
            code_t Start() const { return m_start; }

            //! Get the stop value    
            code_t Stop() const  { return m_stop; }

            //! Get the weight value    
            code_t Weight() const { return m_weight; }

            //! Sets the values    
            void SetValues(const code_t start , const code_t stop , const code_t wt)
            { 
                m_start = start;
                m_stop = stop; 
                m_weight = wt;
            }

        private:
            //! The m_start value. 
            code_t m_start;    

            //! The m_stop value.Should be >=m_start. 
            code_t m_stop;

            //! The denominator for interpreting m_start, m_stop. Should be >= m_start,m_stop. 
            code_t m_weight;    
        };

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
                SetCounts(1,1);
            }

            //! Constructor.
            /*!
                Constructor initialises the counts to those set.
             */    
            Context(int cnt0,int cnt1)
            {
                SetCounts(cnt0,cnt1);
            }

            //! Copy constructor
            Context(const Context & cpy)
              : count0( cpy.count0 ),
                count1( cpy.count1 ),
                trip0( cpy.trip0 ),
                trip1( cpy.trip1 )
            {}

            //! Assignment=
            Context & operator=(const Context& rhs)
            {
                count0 = rhs.count0;
                count1 = rhs.count1;
                trip0  = rhs.trip0;
                trip1  = rhs.trip1;
                return *this;
            }

            //! Destructor
            ~Context() {}

            //! Sets the counts according to the input.
            /*!
                Sets the counts, and then the triples to reflect the counts.
             */    
            void SetCounts(int cnt0, int cnt1)
            {
                count0 = cnt0;
                count1 = cnt1;
                SetTriples();
            }

            //! Returns the count of zeroes.
            code_t GetCount0() const { return count0; }    

            //! Returns the count of ones.
            code_t GetCount1() const { return count1; }    

            //! Increment the count.
            /*!
                Increment the count of Symbol by amnt.
                \param    symbol    the symbol whose count is to be incremented (false=0, true=1)
                \param    amnt    the amount to increment by
             */    
            void IncrCount( const bool symbol , const int amnt )
            {
                if ( symbol ) 
                    count1 += amnt; 
                else 
                    count0 += amnt;

                SetTriples();
            }

            //! Increment the count by 1
            /*!
                Increment the count of symbol by 1.
                \param    symbol    the symbol whose count is to be incremented (false=0, true=1)
             */    
            void IncrCount( const bool symbol )
            {
                if ( symbol ) 
                    count1++; 
                else 
                    count0++;

                SetTriples();
            }

             //! Divide the counts by 2, making sure neither ends up 0.
            void HalveCounts()
            {
                count0 >>= 1;
                count0++;
                count1 >>= 1;
                count1++;

                SetTriples();
            }

            //! Return the weight, equal to the count of 0 plus the count of 1.    
            code_t Weight() const { return trip0.Weight(); }

            //! Return the triple associated with Symbol.    
            const Triple & GetTriple( const bool symbol ) const { return (symbol ? trip1 : trip0); }

            //! Given a number, return the corresponding symbol and triple.
            /*!
                Given a number, which should be in the range [0,m_weight)
                return the corresponding symbol.  The range [0,m_weight) is
                partitioned into portions [0,count0), [count0,m_weight)
                corresponding to 0 and 1.
            */
            bool GetSymbol(const calc_t num, const calc_t factor , Triple & trip_val) const
            {
                if (num < trip0.Stop()*factor)
                {
                    trip_val = trip0;
                    return false; //ie zero
                }
                else
                {
                    trip_val = trip1;
                    return true; //ie 1
                }
            } 


        private:
            code_t count0;
            code_t count1;

            Triple trip0;
            Triple trip1;

            void SetTriples()
            {
                // updates triples given counts
                code_t wt( count0 + count1 );

                trip0.SetValues( 0 , count0 , wt );
                trip1.SetValues( count0 , wt , wt );    
            }
        };

    protected:

        //virtual codec functions (to be overridden)
        ////////////////////////////////////////////

        //! The method by which the contexts are initialised
        virtual void InitContexts()=0;                                        

        //! The method by which the counts are updated.
        virtual void Update( const bool symbol , const int context_num )=0;    

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

        //! encodes a triple and writes to output
        void EncodeTriple(const Triple & c , const calc_t range );

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

        //! Remove the symbol from the coded input stream
        void RemFromStream(const Triple & trip , const calc_t range);

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
    void ArithCodec<T>::EncodeTriple( const Triple &trip , const calc_t range)
    {
        //formulae given we know we're binary coding    
        if ( !trip.Start() ) // trip.Start()=0, so symbol is 0, so m_low_code unchanged 
            m_high_code = m_low_code + static_cast<code_t>(( range * trip.Stop() ) / trip.Weight() - 1 );
        else //symbol is 1, so m_high_code unchanged
            m_low_code += static_cast<code_t>(( range * trip.Start() ) / trip.Weight() );                

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
            else return ;

            m_low_code  <<= 1;
            m_high_code <<= 1;
            m_high_code ++;
        }
        while ( true );
    }

    template<class T>
    inline void ArithCodec<T>::EncodeSymbol(const bool symbol, const int context_num)
    {
        const calc_t range( static_cast<calc_t>( m_high_code - m_low_code ) + 1 );
        EncodeTriple( m_context_list[context_num].GetTriple(symbol) , range );
        Update( symbol , context_num );
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
    void ArithCodec<T>::RemFromStream( const Triple &trip , const calc_t range )
    {
        if( !trip.Start() )//trip.Start()=0, so symbol is 0, so m_low_code unchanged 
            m_high_code = m_low_code + static_cast<code_t>(( range * trip.Stop() ) / trip.Weight() - 1 );

        else//symbol is 1, so m_high_code unchanged
            m_low_code += static_cast<code_t>(( range * trip.Start() ) / trip.Weight() );        

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
            else return;

            m_low_code  <<= 1;
            m_high_code <<= 1;
            m_high_code++;
            m_code      <<= 1;

            m_code += InputBit();

        } while ( true );

    }

    template<class T>
    inline bool ArithCodec<T>::DecodeSymbol( const int context_num )
    {
        Triple limits;

        const Context& c( m_context_list[context_num] );
        const calc_t count( ( static_cast<calc_t>( m_code - m_low_code ) + 1 ) * c.Weight() - 1 );

        const calc_t range( static_cast<calc_t>( m_high_code - m_low_code ) + 1 );

        bool symbol( m_context_list[context_num].GetSymbol( count , range , limits ) );

        RemFromStream( limits , range );
        Update(  symbol , context_num );

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

}// end dirac namespace

#endif
