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
* Revision 1.2  2004-04-06 18:06:53  chaoticcoyote
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

#include "arith_codec.h"
#include "common.h"
#include "motion.h"
#include "wavelet_utils.h"
#include <vector>
#include <iostream>

        
//! 
/*!

 */
class MvDataCodec: public ArithCodec<MvData >{
public:
        
    //! 
    /*!
        
     */
	MvDataCodec(BasicOutputManager* bits_out, std::vector<Context>& ctxs,CodecParams& cp,ChromaFormat& cf): 
        
    //! 
    /*!
        
     */
	ArithCodec<MvData >(bits_out,ctxs),cparams(cp),cformat(cf){}		

        
    //! 
    /*!
        
     */
	MvDataCodec(BitInputManager* bits_in, std::vector<Context>& ctxs,CodecParams& cp,ChromaFormat& cf): 
        
    //! 
    /*!
        
     */
	ArithCodec<MvData >(bits_in,ctxs),cparams(cp),cformat(cf){}		

        
    //! 
    /*!
        
     */
	void InitContexts();
private:
	int MB_count;
	CodecParams cparams;
	ChromaFormat cformat;

	int b_xp,b_yp;		//position of current block
	int mb_xp,mb_yp;	//position of current MB
	int mb_tlb_x,mb_tlb_y;	//position of top-left block of current MB

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

	int ChooseContext(MvData& data, const int BinNumber);
	int ChooseContext(MvData& data);
	int ChooseSignContext(MvData& data);

	int ChooseMBSContext(MvData& data, const int BinNumber);
	int ChooseMBCContext(MvData& data);
	int ChoosePredContext(MvData& data, const int BinNumber);
	int ChooseREF1xContext(MvData& data, const int BinNumber);
	int ChooseREF1xSignContext(MvData& data);
	int ChooseREF1yContext(MvData& data, const int BinNumber);
	int ChooseREF1ySignContext(MvData& data);
	int ChooseREF2xContext(MvData& data, const int BinNumber);
	int ChooseREF2xSignContext(MvData& data);
	int ChooseREF2yContext(MvData& data, const int BinNumber);
	int ChooseREF2ySignContext(MvData& data);
	int ChooseYDCContext(MvData& data, const int BinNumber);
	int ChooseUDCContext(MvData& data, const int BinNumber);
	int ChooseVDCContext(MvData& data, const int BinNumber);
	int ChooseYDCSignContext(MvData& data);
	int ChooseUDCSignContext(MvData& data);
	int ChooseVDCSignContext(MvData& data);

	//prediction stuff

	unsigned int MBSplitPrediction(TwoDArray<MBData>& mbdata);
	bool MBCBModePrediction(TwoDArray<MBData>& mbdata);
	unsigned int BlockModePrediction(TwoDArray<PredMode>& preddata);
	MVector Mv1Prediction(MvArray& mvarray,TwoDArray<PredMode>& preddata);
	MVector Mv2Prediction(MvArray& mvarray,TwoDArray<PredMode>& preddata);
	ValueType DCPrediction(TwoDArray<ValueType>& dcdata,TwoDArray<PredMode>& preddata);
};

//public functions//
////////////////////
inline void MvDataCodec::InitContexts(){
	for (uint I=0; I<ContextList.size();++I)
		ContextList[I].set_counts(1,1);	
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

inline int MvDataCodec::ChooseContext(MvData& data, const int BinNumber){return 0;}

inline int MvDataCodec::ChooseContext(MvData& data){return 0;}

inline int MvDataCodec::ChooseSignContext(MvData& data){return 0;}

//proper context functions

inline int MvDataCodec::ChooseMBSContext(MvData& data, const int BinNumber){
	if (BinNumber==1) return MB_SPLIT_BIN1_CTX;
	else return MB_SPLIT_BIN2_CTX;
}

inline int MvDataCodec::ChooseMBCContext(MvData& data){return MB_CMODE_CTX;}

inline int MvDataCodec::ChoosePredContext(MvData& data, const int BinNumber){
	if (BinNumber==1)
		return PMODE_BIN1_CTX;
	else if (BinNumber==2)
		return PMODE_BIN2_CTX;
	else
		return PMODE_BIN3_CTX;
}
inline int MvDataCodec::ChooseREF1xContext(MvData& data, const int BinNumber){
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


inline int MvDataCodec::ChooseREF1xSignContext(MvData& data){ return REF1x_SIGN0_CTX;}

inline int MvDataCodec::ChooseREF1yContext(MvData& data, const int BinNumber){
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

inline int MvDataCodec::ChooseREF1ySignContext(MvData& data){return REF1y_SIGN0_CTX;}

inline int MvDataCodec::ChooseREF2xContext(MvData& data, const int BinNumber){
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

inline int MvDataCodec::ChooseREF2xSignContext(MvData& data){return REF2x_SIGN0_CTX;}

inline int MvDataCodec::ChooseREF2yContext(MvData& data, const int BinNumber){
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

inline int MvDataCodec::ChooseREF2ySignContext(MvData& data){return REF2y_SIGN0_CTX;}

inline int MvDataCodec::ChooseYDCContext(MvData& data, const int BinNumber){
	if (BinNumber==1) return YDC_BIN1_CTX;
	else return YDC_BIN2plus_CTX;
}

inline int MvDataCodec::ChooseUDCContext(MvData& data, const int BinNumber){
	if (BinNumber==1) return UDC_BIN1_CTX;
	else return UDC_BIN2plus_CTX;
}

inline int MvDataCodec::ChooseVDCContext(MvData& data, const int BinNumber){
	if (BinNumber==1) return VDC_BIN1_CTX;
	else return VDC_BIN2plus_CTX;
}

inline int MvDataCodec::ChooseYDCSignContext(MvData& data){return YDC_SIGN0_CTX;}

inline int MvDataCodec::ChooseUDCSignContext(MvData& data){return UDC_SIGN0_CTX;}

inline int MvDataCodec::ChooseVDCSignContext(MvData& data){return VDC_SIGN0_CTX;}

//prediction functions

inline unsigned int MvDataCodec::MBSplitPrediction(TwoDArray<MBData>& mbdata){	
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

inline bool MvDataCodec::MBCBModePrediction(TwoDArray<MBData>& mbdata){	
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

inline unsigned int MvDataCodec::BlockModePrediction(TwoDArray<PredMode>& preddata){
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

inline MVector MvDataCodec::Mv1Prediction(MvArray& mvarray,TwoDArray<PredMode>& preddata){
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

inline MVector MvDataCodec::Mv2Prediction(MvArray& mvarray,TwoDArray<PredMode>& preddata){
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

inline ValueType MvDataCodec::DCPrediction(TwoDArray<ValueType>& dcdata,TwoDArray<PredMode>& preddata){
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
