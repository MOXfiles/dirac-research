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
* Revision 1.4  2004-05-12 08:35:33  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/04/11 22:50:46  chaoticcoyote
* Modifications to allow compilation by Visual C++ 6.0
* Changed local for loop declarations into function-wide definitions
* Replaced variable array declarations with new/delete of dynamic array
* Added second argument to allocator::alloc calls, since MS has no default
* Fixed missing and namespace problems with min, max, cos, and abs
* Added typedef unsigned int uint (MS does not have this)
* Added a few missing std:: qualifiers that GCC didn't require
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

#ifndef _BAND_CODEC_H_
#define _BAND_CODEC_H_

#include "libdirac_common/arith_codec.h"
#include "libdirac_common/wavelet_utils.h"
#include <iostream>

//Subclasses the arithmetic codec to produce a coding/decoding tool for subbands


//! A general class for coding and decoding wavelet subband data.
/*!
	A general class for coding and decoding wavelet subband data, deriving from the abstract ArithCodec class.
 */
class BandCodec: public ArithCodec<PicArray >{
public:

	//! Constructor for encoding.
    /*!
		Creates a BandCodec object to encode subband data
		/param	bits_out	the output for the encoded bits
		/param	ctxs		the contexts used in the encoding process
		/param	band_list	the set of all the subbands
		/param 	band_num	the number of the subband being coded 
     */
	BandCodec(BasicOutputManager* bits_out, const std::vector<Context>& ctxs,const SubbandList& band_list,int band_num):
	ArithCodec<PicArray >(bits_out,ctxs),
	bnum(band_num),
	node(band_list(band_num)),
	xp(node.Xp()),
	yp(node.Yp()),
	xl(node.Xl()),
	yl(node.Yl()),
	vol(node.Xl()*node.Yl()),
	reset_coeff_num(std::max(vol/32,50)),
	cut_off_point(node.Scale()>>1)
	{
		if (node.Parent()!=0) 
			pnode=band_list(node.Parent());
	}		

    //! Constructor for decoding.
    /*!
		Creates a BandCodec object to decode subband data.
		/param	bits_in		the input for the encoded bits
		/param	ctxs		the contexts used in the decoding process
		/param	band_list	the set of all the subbands
		/param 	band_num	the number of the subband being decoded 
     */
	BandCodec(BitInputManager* bits_in, const std::vector<Context>& ctxs,const SubbandList& band_list,int band_num):
	ArithCodec<PicArray >(bits_in,ctxs),
	bnum(band_num),
	node(band_list(band_num)),
	xp(node.Xp()),
	yp(node.Yp()),
	xl(node.Xl()),
	yl(node.Yl()),
	vol(node.Xl()*node.Yl()),
	reset_coeff_num(std::max(vol/32,50)),
	cut_off_point(node.Scale()>>1)
	{
		if (node.Parent()!=0) pnode=band_list(node.Parent());
	}

	//! Initialise the contexts according to predefined counts.
	inline void InitContexts();

protected:

	//variables	
	int bnum;
	const Subband node;//the subband being coded
	int xp,yp,xl,yl;//dimensions of the subband
	int xpos,ypos;//position within the subband
	int vol;
	int reset_coeff_num;//the number of coefficients after which contexts are reset
	int coeff_count;
	int qf,qfinv;//quantisation and inverse quantisation values
	ValueType offset;//reconstruction point
	ValueType nhood_sum;
	Subband pnode;//the parent subband
	int pxp,pyp,pxl,pyl;//coords of the parent subband
	int pxpos,pypos;//position of the parent coefficient
	bool parent_zero;
	ValueType cut_off_point;//used in selecting a context

	//functions
	virtual void DoWorkCode(PicArray& InData);					//overridden from the base class
	virtual void DoWorkDecode(PicArray& OutData, int num_bits); //ditto

	void CodeVal(PicArray& InData, ValueType& val);//code an individual value
	void DecodeVal(PicArray& OutData);//decode an individual value

	inline void Update(const int& context_num, const bool& Symbol);
	inline void Resize(const int& context_num);
	inline void Reset_all();
	int ChooseContext(const PicArray& Data, const int BinNumber) const;
	int ChooseContext(const PicArray& Data) const;
	int ChooseSignContext(const PicArray& Data) const;

private:

	BandCodec(const BandCodec& cpy);			//private, bodyless copy constructor: class should not be copied
	BandCodec& operator=(const BandCodec& rhs);	//private, bodyless copy operator=: class should not be assigned
};

//Overridden common codec functions
///////////////////////////////////////
inline void BandCodec::InitContexts(){
	//initialises the contexts. 
	//If ContextList does not already have values, then they're set to default values. 
	//This way, the constructor can override default initialisation.
	Context tmp_ctx;
	for (unsigned int I=0;I<ContextList.size();++I){
		if (I>=ContextList.size()){
			ContextList.push_back(tmp_ctx);
		}
		else {
			if (ContextList[I].Weight()==0){
				ContextList[I].SetCounts(1,1);
			}
		}
	}//I
}

inline void BandCodec::Update(const int& context_num, const bool& Symbol){
	ContextList[context_num].IncrCount(Symbol,1);
	if (ContextList[context_num].Weight()>=1024){
		Resize(context_num);
	}

}

inline void BandCodec::Resize(const int& context_num){
	ContextList[context_num].HalveCounts();
}

inline void BandCodec::Reset_all(){

	for (unsigned int C=0;C<ContextList.size();++C){
		if (ContextList[C].Weight()>16)
			ContextList[C].HalveCounts();
	}//C
}

inline int BandCodec::ChooseContext(const PicArray& Data) const{return NZ_BIN5plus_CTX;}

//////////////////////////////////////////////////////////////////////////////////
//Now for special class for LF bands (since we don't want/can't refer to parent)//
//////////////////////////////////////////////////////////////////////////////////

//! A class specially for coding the LF subbands 
/*!
    A class specially for coding the LF subbands, where we don't want to/can't refer to the 
	parent subband.
*/
class LFBandCodec: public BandCodec{
public:
    //! Constructor for encoding
    /*!
		Creates a LFBandCodec object to encode subband data.
		/param	bits_out	the output for the encoded bits
		/param	ctxs		the contexts used in the encoding process
		/param	band_list	the set of all the subbands
		/param 	band_num	the number of the subband being coded 
     */		
	LFBandCodec(BasicOutputManager* bits_out, const std::vector<Context>& ctxs,const SubbandList& band_list,int band_num):
	BandCodec(bits_out,ctxs,band_list,band_num){}

	//! Constructor for decoding
    /*!
		Creates a LFBandCodec object to decode subband data.
		/param	bits_in		the input for the encoded bits
		/param	ctxs		the contexts used in the decoding process
		/param	band_list	the set of all the subbands
		/param 	band_num	the number of the subband being decoded 
     */
	LFBandCodec(BitInputManager* bits_in, const std::vector<Context>& ctxs,const SubbandList& band_list,int band_num):
	BandCodec(bits_in,ctxs,band_list,band_num){}

protected:
	virtual void DoWorkCode(PicArray& InData);					//overridden from the base class
	virtual void DoWorkDecode(PicArray& OutData, int num_bits); //ditto

private:
	LFBandCodec(const LFBandCodec& cpy);			//private, bodyless copy constructor: class should not be copied
	LFBandCodec& operator=(const LFBandCodec& rhs);	//private, bodyless copy operator=: class should not be assigned

};


//////////////////////////////////////////////////////////////////////////////////
//Finally,special class incorporating prediction for the DC band of intra frames//
//////////////////////////////////////////////////////////////////////////////////

//! A class specially for coding the DC subband of Intra frames 
/*!
	A class specially for coding the DC subband of Intra frames, using intra-band prediction 
	of coefficients.
*/
class IntraDCBandCodec: public BandCodec{
public:
    //! Constructor for encoding
    /*!
		Creates a IntraDCBandCodec object to encode subband data, based on parameters
		/param	bits_out	the output for the encoded bits
		/param	ctxs		the contexts used in the encoding process
		/param	band_list	the set of all the subbands
		/param 	band_num	the number of the subband being coded 
     */
	IntraDCBandCodec(BasicOutputManager* bits_out, const std::vector<Context>& ctxs,const SubbandList& band_list):
	BandCodec(bits_out,ctxs,band_list,band_list.Length()){}

	//! Constructor for decoding
    /*!
		Creates a LFBandCodec object to decode subband data, based on parameters
		/param	bits_in		the input for the encoded bits
		/param	ctxs		the contexts used in the decoding process
		/param	band_list	the set of all the subbands
		/param 	band_num	the number of the subband being decoded 
     */	
	IntraDCBandCodec(BitInputManager* bits_in, const std::vector<Context>& ctxs,const SubbandList& band_list):
	BandCodec(bits_in,ctxs,band_list,band_list.Length()){}

protected:
	virtual void DoWorkCode(PicArray& InData);					//overridden from the base class
	virtual void DoWorkDecode(PicArray& OutData, int num_bits); //ditto

private:
	IntraDCBandCodec(const IntraDCBandCodec& cpy);				//private, bodyless copy constructor: class should not be copied
	IntraDCBandCodec& operator=(const IntraDCBandCodec& rhs);	//private, bodyless copy operator=: class should not be assigned

	ValueType GetPrediction(PicArray& Data);//prediction of a DC value from its previously coded neighbours
};

#endif
