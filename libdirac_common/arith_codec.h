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
* Revision 1.1  2004-03-11 17:45:43  timborer
* Initial revision
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
* Arith coding engine by Richard Felton, with mods by Thomas Davies
*/

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

#include "common.h"
#include "context.h"
#include "bit_manager.h"
#include <vector>

template<class T>//T is container/array type
class ArithCodec{
	//abstract binary arithmetic coding class
public:
	// Constructors	
	ArithCodec(BitInputManager* bits_in, std::vector<Context>& ctxs):
	bit_count(0),BitIP(bits_in),ContextList(ctxs){}	
	ArithCodec(BasicOutputManager* bits_out, std::vector<Context>& ctxs):
	bit_count(0),BitOP(bits_out),ContextList(ctxs){}	


	// Destructors
	virtual ~ArithCodec(){}
	//General encode and decode functions: these call virtual functions which should be overridden
	int Compress(T &InData){
		InitEncoder();				
		DoWorkCode(InData);
		FlushEncoder();
		return bit_count;
	}

	void Decompress(T &OutData, int num_bits){
		max_count=num_bits;
		InitDecoder();
		DoWorkDecode(OutData,num_bits);
		FlushDecoder();		
		BitIP->FlushInput();
	}

protected :

	//Variables
	int bit_count;						//count of the total number of bits input or output
	int max_count;						//max number of bits to be input
	long Underflow;						// Number of underflow bits
	MyCodeType code;					// The present input code
	MyCodeType count;					// Scaled midpoint of the decoding interval	
	MyCodeType low;						// Start of the current code range
	MyCodeType high;					// End of the current code range
	//	codec_params_type cparams;			// Parameters for controlling coding/decoding
	BitInputManager* BitIP;				//Manages interface with file/stream
	BasicOutputManager* BitOP;			//Manages interface with file/stream. Can be header or data	
	std::vector<Context > ContextList;		// List of contexts.


	//virtual codec functions (to be overridden)
	////////////////////////////////////////////
	virtual void InitContexts()=0;										// The method by which the contexts are initialised
	virtual void Update(const int& context_num, const bool& Symbol)=0;	// The method by which the counts are updated.
	virtual void Resize(const int& context_num)=0;						// The method by which the counts are resized
	virtual void Reset_all()=0;											// The method by which _all_ the counts are resized.
	virtual int ChooseContext(T &Data, const int BinNumber)=0;			// Choose the context based on previous data.
	virtual int ChooseContext(T &Data)=0;								// Choose the context based on previous data.

	//virtual encode-only functions
	///////////////////////////////	
	virtual void DoWorkCode(T &InData)=0;	

	//core encode-only functions
	////////////////////////////	
	inline void InitEncoder();											// Initialises the Encoder
	inline void EncodeTriple(const Triple &c);							// encodes a triple and writes to output
	inline void EncodeSymbol(const bool& symbol, const int& context_num);	// encodes a symbol and writes to output
	inline void FlushEncoder();//flushes the output of the encoder.	
	inline void FlushDecoder();//flushes the input of the encoder by reading all the remaining bits	

	//virtual decode-only functions	
	///////////////////////////////	
	virtual void DoWorkDecode(T &OutData, int num_bits)=0;	

	//core decode-only functions
	////////////////////////////	
	inline void InitDecoder();					// Initialise the Decoder
	inline void RemFromStream(const Triple& c);// Remove the symbol from the coded input stream
	inline void SetCurrentCount(const int context_num);	
	inline void DecodeSymbol(bool& symbol,int context_num);	// Decodes a symbol given a context number
};

//Implementation - core functions
/////////////////////////////////

template<class T>
inline void ArithCodec<T>::InitEncoder(){
	//Set the code word stuff
	low = 0;
	high = MyCodeTypeMax;
	Underflow = 0;
}
template<class T>
inline void ArithCodec<T>::EncodeTriple(const Triple &c){
	MyCalcType range;

	// Rescale high and low for the new symbol
	range=(MyCalcType)(high-low) + 1;
	// 	high=low + (MyCodeType)(( range * c.Stop ) / c.Weight - 1 );//general formulae
	// 	low+=(MyCodeType)(( range * c.Start ) / c.Weight );

	//formulae given we know we're binary coding	
	if(!c.Start)//c.Start=0, so symbol is 0, so low unchanged 
		high = low + (MyCodeType)(( range * c.Stop ) / c.Weight - 1 );
	else//symbol is 1, so high unchanged
		low+=(MyCodeType)(( range * c.Start ) / c.Weight );				

	do// This loop turns out new bits until high and low are far enough apart to have stabilized.
	{
		// If this test passes, it means that the MSDigits match, and can be sent to the output stream.
		if (( high & MyCodeTypeMSB ) == ( low & MyCodeTypeMSB ))
		{
			BitOP->OutputBit( high & MyCodeTypeMSB, bit_count);
			while ( Underflow > 0 )
			{
				BitOP->OutputBit(~high & MyCodeTypeMSB, bit_count);
				Underflow--;
			}
		}
		// If this test passes, we're in danger of underflow
		else if ( ( low & MyCodeTypeSecondMSB ) && !( high & MyCodeTypeSecondMSB ))
		{
			Underflow += 1;
			low &= (MyCodeTypeSecondMSB) - 1;
			high |= MyCodeTypeSecondMSB;
		}
		else return ;

		low <<= 1;
		high <<= 1;
		high |= 1;
	}
	while(1);
}
template<class T>
inline void ArithCodec<T>::EncodeSymbol(const bool& symbol, const int& context_num){
	Triple trip;
	trip=ContextList[context_num].GetTriple(symbol);
	EncodeTriple(trip);
	Update(context_num, symbol);
}
template<class T>
inline void ArithCodec<T>::FlushEncoder(){
	// Flushes the output
	BitOP->OutputBit(low & MyCodeTypeSecondMSB,bit_count);
	Underflow++;
	while ( Underflow-- > 0 ){
		BitOP->OutputBit(~low & MyCodeTypeSecondMSB, bit_count);
	}	
}

template<class T>
void ArithCodec<T>::InitDecoder(){

	//Read in a full word of data
	MyCodeType i;
	code = 0;
	for ( i = 0 ; i < (8*sizeof(MyCodeType));i++ ){
		code <<= 1;
		if (BitIP->InputBit(bit_count,max_count))
			code++;
	}
	low = 0;
	high = MyCodeTypeMax;
	Underflow = 0;
}
template<class T>
void ArithCodec<T>::RemFromStream(const Triple &c)
{
	MyCalcType range;

	//First, the range is expanded to account for the symbol removal.	
	range=(MyCalcType)( high - low ) + 1;
	if(!c.Start){//c.Start=0, so symbol is 0, so low unchanged 
		high = low + (MyCodeType)(( range * c.Stop ) / c.Weight - 1 );
	}
	else{//symbol is 1, so high unchanged
		low+=(MyCodeType)(( range * c.Start ) / c.Weight );			
	}	

	do
	{		
		//If the MSDigits match, the bits will be shifted out.
		if ( ( high & MyCodeTypeMSB ) == ( low & MyCodeTypeMSB ) ){
		}		
  		//Else, if underflow is threatening, shift out the 2nd MSDigit.
		else if ((low & MyCodeTypeSecondMSB) == MyCodeTypeSecondMSB  && (high & MyCodeTypeSecondMSB) == 0 ){
			code ^= MyCodeTypeSecondMSB;
			low   &= (MyCodeTypeSecondMSB) - 1;
			high  |= MyCodeTypeSecondMSB;
		}		
		//Otherwise, nothing can be shifted out, so return.
		else return;

		low <<= 1;
		high <<= 1;
		high |= 1;
		code <<= 1;
		if (BitIP->InputBit(bit_count,max_count))
			code++; 

	} while (1);
}
template<class T>
void ArithCodec<T>::SetCurrentCount(const int context_num){
	MyCalcType range;
	range = (long) ( high - low ) + 1;
	count=(MyCodeType)((((long) ( code - low ) + 1 ) * ContextList[context_num].Weight() - 1 ) / range );
}
template<class T>
void ArithCodec<T>::DecodeSymbol(bool& symbol, const int context_num){
	Triple Limits;	
	SetCurrentCount(context_num);
	symbol=ContextList[context_num].GetSymbol(count, Limits);		
	RemFromStream(Limits);
	Update(context_num, symbol);
}
template<class T>
void ArithCodec<T>::FlushDecoder(){

	// Flushes the input
	while(bit_count<max_count){
		BitIP->InputBit(bit_count,max_count);
	}
}

#endif
