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
* Revision 1.3  2004-04-11 22:50:46  chaoticcoyote
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

#include "arith_codec.h"
#include "wavelet_utils.h"
#include <iostream>

//Subclasses the arithmetic codec to produce a coding/decoding tool for subbands


//! 
/*!

 */
class BandCodec: public ArithCodec<PicArray >{
public:

    //! 
    /*!
        
     */
	BandCodec(BasicOutputManager* bits_out, std::vector<Context>& ctxs,SubbandList& band_list,int band_num): 
	ArithCodec<PicArray >(bits_out,ctxs),bnum(band_num),node(band_list(band_num)),xp(node.xp()),yp(node.yp()),xl(node.xl()),
	yl(node.yl()),vol(node.xl()*node.yl()),reset_coeff_num(DIRAC_MAX(vol/32,50)),cut_off_point(node.scale()>>1)
	{if (node.parent()!=0) pnode=band_list(node.parent());}		


    //! 
    /*!
        
     */
	BandCodec(BitInputManager* bits_in, std::vector<Context>& ctxs,SubbandList& band_list,int band_num): 
	ArithCodec<PicArray >(bits_in,ctxs),bnum(band_num),node(band_list(band_num)),xp(node.xp()),yp(node.yp()),xl(node.xl()),
	yl(node.yl()),vol(node.xl()*node.yl()),reset_coeff_num(DIRAC_MAX(vol/32,50)),cut_off_point(node.scale()>>1)
	{if (node.parent()!=0) pnode=band_list(node.parent());}


    //! 
    /*!
        
     */
	inline void InitContexts();
private:

protected:

	//variables	
	int bnum;
	Subband node;//the subband being coded
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
	int ChooseContext(PicArray& Data, const int BinNumber);
	inline int ChooseContext(PicArray& Data);
	int ChooseSignContext(PicArray& Data);
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
				ContextList[I].set_counts(1,1);
			}
		}
	}
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
	}
}

inline int BandCodec::ChooseContext(PicArray& Data){return NZ_BIN5plus_CTX;
}

//////////////////////////////////////////////////////////////////////////////////
//Now for special class for LF bands (since we don't want/can't refer to parent)//
//////////////////////////////////////////////////////////////////////////////////


    //! 
    /*!
        
     */
class LFBandCodec: public BandCodec{
public:

    //! 
    /*!
        
     */
	LFBandCodec(BasicOutputManager* bits_out, std::vector<Context>& ctxs,SubbandList& band_list,int band_num):
	BandCodec(bits_out,ctxs,band_list,band_num){}


    //! 
    /*!
        
     */
	LFBandCodec(BitInputManager* bits_in, std::vector<Context>& ctxs,SubbandList& band_list,int band_num):
	BandCodec(bits_in,ctxs,band_list,band_num){}

protected:
	virtual void DoWorkCode(PicArray& InData);					//overridden from the base class
	virtual void DoWorkDecode(PicArray& OutData, int num_bits); //ditto

};


//////////////////////////////////////////////////////////////////////////////////
//Finally,special class incorporating prediction for the DC band of intra frames//
//////////////////////////////////////////////////////////////////////////////////


//! 
/*!

 */
class IntraDCBandCodec: public BandCodec{
public:

    //! 
    /*!
        
     */
	IntraDCBandCodec(BasicOutputManager* bits_out, std::vector<Context>& ctxs,SubbandList& band_list):
	BandCodec(bits_out,ctxs,band_list,band_list.length()){}


    //! 
    /*!
        
     */
	IntraDCBandCodec(BitInputManager* bits_in, std::vector<Context>& ctxs,SubbandList& band_list):
	BandCodec(bits_in,ctxs,band_list,band_list.length()){}
private:
	ValueType GetPrediction(PicArray& Data);//prediction of a DC value from its previously coded neighbours

protected:
	virtual void DoWorkCode(PicArray& InData);					//overridden from the base class
	virtual void DoWorkDecode(PicArray& OutData, int num_bits); //ditto

};

#endif
