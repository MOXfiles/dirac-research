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
* Revision 1.3  2004-05-12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/04/06 18:06:53  chaoticcoyote
* Boilerplate for Doxygen comments; testing ability to commit into SF CVS
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#ifndef _MV_CODEC_H_
#define _MV_CODEC_H_

/////////////////////////////////////////////////
//Class to do motion vector coding and decoding//
//------using adaptive arithmetic coding-------//
/////////////////////////////////////////////////

#include "libdirac_common/arith_codec.h"
#include "libdirac_common/common.h"
#include "libdirac_common/motion.h"
#include "libdirac_common/wavelet_utils.h"
#include <vector>
#include <iostream>

//! Codes and decodes all the Motion Vector data
/*!
	Derived from the ArithCodec class, this codes and decodes all the motion vector data.
 */
class MvDataCodec: public ArithCodec<MvData >{

public:
    //! Constructor for encoding
	    /*!
		Creates a MvDataCodec object to encode MV data, based on parameters
		/param	bits_out	the output for the encoded bits
		/param	ctxs		the contexts used in the encoding process
		/param	cp			the coding/decoding parameter set
		/param 	cf			the chroma format
     */	
	MvDataCodec(BasicOutputManager* bits_out, std::vector<Context>& ctxs,CodecParams& cp,ChromaFormat& cf): 
	ArithCodec<MvData >(bits_out,ctxs),
	cparams(cp),
	cformat(cf)
	{}		

    //! Constructor for decoding
	    /*!
		Creates a MvDataCodec object to encode MV data, based on parameters
		/param	bits_in		the input for the encoded bits
		/param	ctxs		the contexts used in the encoding process
		/param	cp			the coding/decoding parameter set
		/param 	cf			the chroma format
     */		
	MvDataCodec(BitInputManager* bits_in, std::vector<Context>& ctxs,CodecParams& cp,ChromaFormat& cf): 
	ArithCodec<MvData >(bits_in,ctxs),
	cparams(cp),
	cformat(cf)
	{}		

    //! Initialises the contexts	
	void InitContexts();
private:
	int MB_count;
	CodecParams cparams;
	ChromaFormat cformat;

	int b_xp,b_yp;		//position of current block
	int mb_xp,mb_yp;	//position of current MB
	int mb_tlb_x,mb_tlb_y;	//position of top-left block of current MB

	//functions	
	MvDataCodec(const MvDataCodec& cpy);			//private, bodyless copy constructor: class should not be copied
	MvDataCodec& operator=(const MvDataCodec& rhs);//private, bodyless copy operator=: class should not be assigned

	//coding functions	
	void CodeMBSplit(MvData& in_data);		//code the MB splitting mode
	void CodeMBCom(MvData& in_data);	//code the MB common ref mode
	void CodePredmode(MvData& in_data);	//code the block prediction mode
	void CodeMv1(MvData& in_data);		//code the first motion vector
	void CodeMv2(MvData& in_data);		//code the second motion vector
	void CodeDC(MvData& in_data);		//code the dc value of intra blocks

	//decoding functions
	void DecodeMBSplit(MvData& out_data);	//decode the MB splitting mode
	void DecodeMBCom(MvData& out_data);//decode the MB common ref mode
	void DecodePredmode(MvData& out_data);//decode the block prediction mode
	void DecodeMv1(MvData& out_data);	//decode the first motion vector
	void DecodeMv2(MvData& out_data);	//decode the second motion vector
	void DecodeDC(MvData& out_data);	//decode the dc value of intra blocks	

protected:

	void DoWorkCode(MvData& in_data);
	void DoWorkDecode(MvData& out_data, int num_bits);

	//Context stuff	
	void Update(const int& context_num, const bool& Symbol);
	void Resize(const int& context_num);
	void Reset_all();

	int ChooseContext(const MvData& data, const int BinNumber) const;
	int ChooseContext(const MvData& data) const;
	int ChooseSignContext(const MvData& data) const;

	int ChooseMBSContext(const MvData& data, const int BinNumber) const;
	int ChooseMBCContext(const MvData& data) const;
	int ChoosePredContext(const MvData& data, const int BinNumber) const;
	int ChooseREF1xContext(const MvData& data, const int BinNumber) const;
	int ChooseREF1xSignContext(const MvData& data) const;
	int ChooseREF1yContext(const MvData& data, const int BinNumber) const;
	int ChooseREF1ySignContext(const MvData& data) const;
	int ChooseREF2xContext(const MvData& data, const int BinNumber) const;
	int ChooseREF2xSignContext(const MvData& data) const;
	int ChooseREF2yContext(const MvData& data, const int BinNumber) const;
	int ChooseREF2ySignContext(const MvData& data) const;
	int ChooseYDCContext(const MvData& data, const int BinNumber) const;
	int ChooseUDCContext(const MvData& data, const int BinNumber) const;
	int ChooseVDCContext(const MvData& data, const int BinNumber) const;
	int ChooseYDCSignContext(const MvData& data) const;
	int ChooseUDCSignContext(const MvData& data) const;
	int ChooseVDCSignContext(const MvData& data) const;

	//prediction stuff

	unsigned int MBSplitPrediction(const TwoDArray<MBData>& mbdata) const;
	bool MBCBModePrediction(const TwoDArray<MBData>& mbdata) const;
	unsigned int BlockModePrediction(const TwoDArray<PredMode>& preddata) const;
	MVector Mv1Prediction(const MvArray& mvarray,const TwoDArray<PredMode>& preddata) const;
	MVector Mv2Prediction(const MvArray& mvarray,const TwoDArray<PredMode>& preddata) const;
	ValueType DCPrediction(const TwoDArray<ValueType>& dcdata,const TwoDArray<PredMode>& preddata) const;
};

//public functions//
////////////////////
inline void MvDataCodec::InitContexts(){
	for (uint I=0; I<ContextList.size();++I)
		ContextList[I].SetCounts(1,1);	
}

//protected functions//
///////////////////////

inline void MvDataCodec::Update(const int& context_num, const bool& Symbol){	
	ContextList[context_num].IncrCount(Symbol,1);
	if (ContextList[context_num].Weight()>=1024){
		Resize(context_num);
	}	
}

inline void MvDataCodec::Resize(const int& context_num){
	ContextList[context_num].HalveCounts();	
}

inline void MvDataCodec::Reset_all(){}

//basic context functions

inline int MvDataCodec::ChooseContext(const MvData& data, const int BinNumber) const {return 0;}

inline int MvDataCodec::ChooseContext(const MvData& data) const {return 0;}

inline int MvDataCodec::ChooseSignContext(const MvData& data) const {return 0;}

//proper context functions

inline int MvDataCodec::ChooseMBSContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1) return MB_SPLIT_BIN1_CTX;
	else return MB_SPLIT_BIN2_CTX;
}

inline int MvDataCodec::ChooseMBCContext(const MvData& data) const {return MB_CMODE_CTX;}

inline int MvDataCodec::ChoosePredContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1)
		return PMODE_BIN1_CTX;
	else if (BinNumber==2)
		return PMODE_BIN2_CTX;
	else
		return PMODE_BIN3_CTX;
}
inline int MvDataCodec::ChooseREF1xContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1)
		return REF1x_BIN1_CTX;
	else if (BinNumber==2)
		return REF1x_BIN2_CTX;
	else if (BinNumber==3)
		return REF1x_BIN3_CTX;
	else if (BinNumber==4)
		return REF1x_BIN4_CTX;
	else
		return REF1x_BIN5plus_CTX;
}


inline int MvDataCodec::ChooseREF1xSignContext(const MvData& data) const { return REF1x_SIGN0_CTX;}

inline int MvDataCodec::ChooseREF1yContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1)
		return REF1y_BIN1_CTX;
	else if (BinNumber==2)
		return REF1y_BIN2_CTX;
	else if (BinNumber==3)
		return REF1y_BIN3_CTX;
	else if (BinNumber==4)
		return REF1y_BIN4_CTX;
	else
		return REF1y_BIN5plus_CTX;
}

inline int MvDataCodec::ChooseREF1ySignContext(const MvData& data) const {return REF1y_SIGN0_CTX;}

inline int MvDataCodec::ChooseREF2xContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1)
		return REF2x_BIN1_CTX;
	else if (BinNumber==2)
		return REF2x_BIN2_CTX;
	else if (BinNumber==3)
		return REF2x_BIN3_CTX;
	else if (BinNumber==4)
		return REF2x_BIN4_CTX;
	else
		return REF2x_BIN5plus_CTX;
}

inline int MvDataCodec::ChooseREF2xSignContext(const MvData& data) const {return REF2x_SIGN0_CTX;}

inline int MvDataCodec::ChooseREF2yContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1)
		return REF2y_BIN1_CTX;
	else if (BinNumber==2)
		return REF2y_BIN2_CTX;
	else if (BinNumber==3)
		return REF2y_BIN3_CTX;
	else if (BinNumber==4)
		return REF2y_BIN4_CTX;
	else
		return REF2y_BIN5plus_CTX;
}

inline int MvDataCodec::ChooseREF2ySignContext(const MvData& data) const {return REF2y_SIGN0_CTX;}

inline int MvDataCodec::ChooseYDCContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1) return YDC_BIN1_CTX;
	else return YDC_BIN2plus_CTX;
}

inline int MvDataCodec::ChooseUDCContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1) return UDC_BIN1_CTX;
	else return UDC_BIN2plus_CTX;
}

inline int MvDataCodec::ChooseVDCContext(const MvData& data, const int BinNumber) const {
	if (BinNumber==1) return VDC_BIN1_CTX;
	else return VDC_BIN2plus_CTX;
}

inline int MvDataCodec::ChooseYDCSignContext(const MvData& data) const {return YDC_SIGN0_CTX;}

inline int MvDataCodec::ChooseUDCSignContext(const MvData& data) const {return UDC_SIGN0_CTX;}

inline int MvDataCodec::ChooseVDCSignContext(const MvData& data) const {return VDC_SIGN0_CTX;}

//prediction functions

inline unsigned int MvDataCodec::MBSplitPrediction(const TwoDArray<MBData>& mbdata) const {	
	std::vector<unsigned int> nbrs;
	if (mb_xp>0 && mb_yp>0){
		nbrs.push_back(mbdata[mb_yp-1][mb_xp].split_mode);
		nbrs.push_back(mbdata[mb_yp-1][mb_xp-1].split_mode);
		nbrs.push_back(mbdata[mb_yp][mb_xp-1].split_mode);
		return GetMean(nbrs);	
	}
	else if (mb_xp>0 && mb_yp==0) return mbdata[mb_yp][mb_xp-1].split_mode;
	else if (mb_xp==0 && mb_yp>0) return mbdata[mb_yp-1][mb_xp].split_mode;
	else return 0;
}

inline bool MvDataCodec::MBCBModePrediction(const TwoDArray<MBData>& mbdata) const {	
	std::vector<unsigned int> nbrs;
	if (mb_xp>0 && mb_yp>0){
		nbrs.push_back(uint(mbdata[mb_yp-1][mb_xp].common_ref));
		nbrs.push_back(uint(mbdata[mb_yp-1][mb_xp-1].common_ref));
		nbrs.push_back(uint(mbdata[mb_yp][mb_xp-1].common_ref));
		return bool(GetMean(nbrs));	
	}
	else if (mb_xp>0 && mb_yp==0) return mbdata[mb_yp][mb_xp-1].common_ref;
	else if (mb_xp==0 && mb_yp>0) return mbdata[mb_yp-1][mb_xp].common_ref;
	else return true;
}

inline unsigned int MvDataCodec::BlockModePrediction(const TwoDArray<PredMode>& preddata) const {
	std::vector<unsigned int> nbrs;
	if (b_xp>0 && b_yp>0){
		nbrs.push_back(uint(preddata[b_yp-1][b_xp]));
		nbrs.push_back(uint(preddata[b_yp-1][b_xp-1]));
		nbrs.push_back(uint(preddata[b_yp][b_xp-1]));
		return GetMean(nbrs);	
	}
	else if (b_xp>0 && b_yp==0) return preddata[b_yp][b_xp-1];
	else if (b_xp==0 && b_yp>0) return preddata[b_yp-1][b_xp];
	else return uint(REF1_ONLY);
}

inline MVector MvDataCodec::Mv1Prediction(const MvArray& mvarray,const TwoDArray<PredMode>& preddata) const {
	std::vector<MVector> nbrs;
	PredMode pmode;	
	MVector zero;
	if (b_xp>0 && b_yp>0){
		pmode=preddata[b_yp-1][b_xp];
		if (pmode==REF1_ONLY || pmode==REF1AND2) 
			nbrs.push_back(mvarray[b_yp-1][b_xp]);
		pmode=preddata[b_yp-1][b_xp-1];
		if (pmode==REF1_ONLY || pmode==REF1AND2)
			nbrs.push_back(mvarray[b_yp-1][b_xp-1]);
		pmode=preddata[b_yp][b_xp-1];
		if (pmode==REF1_ONLY || pmode==REF1AND2)		
			nbrs.push_back(mvarray[b_yp][b_xp-1]);
		if (nbrs.size()>0) return MvMedian(nbrs);	
		else return zero;
	}
	else if (b_xp>0 && b_yp==0){
		pmode=preddata[0][b_xp-1];
		if (pmode==REF1_ONLY || pmode==REF1AND2) return mvarray[0][b_xp-1];
		else return zero;
	}
	else if (b_xp==0 && b_yp>0){
		pmode=preddata[b_yp-1][0];
		if (pmode==REF1_ONLY || pmode==REF1AND2) return mvarray[b_yp-1][0];
		else return zero;
	}

	return zero;
}

inline MVector MvDataCodec::Mv2Prediction(const MvArray& mvarray,const TwoDArray<PredMode>& preddata) const {
	std::vector<MVector> nbrs;
	PredMode pmode;
	MVector zero;
	if (b_xp>0 && b_yp>0){
		pmode=preddata[b_yp-1][b_xp];
		if (pmode==REF2_ONLY || pmode==REF1AND2)
			nbrs.push_back(mvarray[b_yp-1][b_xp]);
		pmode=preddata[b_yp-1][b_xp-1];
		if (pmode==REF2_ONLY || pmode==REF1AND2)
			nbrs.push_back(mvarray[b_yp-1][b_xp-1]);
		pmode=preddata[b_yp][b_xp-1];
		if (pmode==REF2_ONLY || pmode==REF1AND2)
			nbrs.push_back(mvarray[b_yp][b_xp-1]);
		if (nbrs.size()>0) return MvMedian(nbrs);
		else return zero;
	}
	else if (b_xp>0 && b_yp==0){
		pmode=preddata[0][b_xp-1];
		if(pmode==REF2_ONLY || pmode==REF1AND2)	return mvarray[0][b_xp-1];
		else return zero;
	}
	else if (b_xp==0 && b_yp>0){
		pmode=preddata[b_yp-1][0];
		if(pmode==REF2_ONLY || pmode==REF1AND2)	return mvarray[b_yp-1][0];
		else return zero;
	}

	return zero;
}

inline ValueType MvDataCodec::DCPrediction(const TwoDArray<ValueType>& dcdata,const TwoDArray<PredMode>& preddata) const {
	std::vector<int> nbrs;
	PredMode pmode;
	if (b_xp>0 && b_yp>0){
		pmode=preddata[b_yp-1][b_xp];
		if (pmode==INTRA) 
			nbrs.push_back(int(dcdata[b_yp-1][b_xp]));
		pmode=preddata[b_yp-1][b_xp-1];
		if (pmode==INTRA)
			nbrs.push_back(int(dcdata[b_yp-1][b_xp-1]));
		pmode=preddata[b_yp][b_xp-1];
		if (pmode==INTRA)		
			nbrs.push_back(int(dcdata[b_yp][b_xp-1]));
		if (nbrs.size()>0) return ValueType(GetMean(nbrs));	
		else return 128;
	}
	else if (b_xp>0 && b_yp==0){
		pmode=preddata[0][b_xp-1];
		if (pmode==INTRA) return dcdata[0][b_xp-1];
		else return 128;
	}
	else if (b_xp==0 && b_yp>0){
		pmode=preddata[b_yp-1][0];
		if (pmode==INTRA) return dcdata[b_yp-1][0];
		else return 128;
	}
	else return 128;
}
#endif
