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
*
*/

#ifndef _COMMON_H_
#define _COMMON_H_

#include "bit_manager.h"
#include "arrays.h"
#include "context.h"

//common header for the encoder and decoder
//basic types used throughout the codec

enum ChromaFormat {Yonly,format422, format444, format420, format411,formatNK};
enum FrameSort{I_frame,L1_frame,L2_frame};
enum PredMode{INTRA,REF1_ONLY,REF2_ONLY,REF1AND2};
enum CompSort{Y,U,V,R,G,B};
enum AddOrSub{ADD,SUBTRACT};
enum Direction {FORWARD,BACKWARD};
enum WltFilter {DAUB,HAAR};

class PicArray: public TwoDArray<ValueType>{
public:
	PicArray(): TwoDArray<ValueType>(){}
	PicArray(int xl,int yl): TwoDArray<ValueType>(xl,yl),csort(Y){}
	~PicArray(){}
	CompSort csort;
};

class CostType{
public:
	double MSE;
	double ENTROPY;
	double TOTAL;
};

//contexts used in the codecs
enum CtxAliases{//used for residual coding
	SIGN0_CTX,		//0		-sign, previous symbol is 0
	SIGN_POS_CTX,	//1		-sign, previous symbol is +ve
	SIGN_NEG_CTX,	//2		-sign, previous symbol is -ve


	Z_BIN1z_CTX,	//3		-bin 1, parent is zero, neighbours zero
	Z_BIN1nz_CTX,	//4		-bin 1, parent is zero, neighbours non-zero
	Z_BIN2_CTX,		//5		-bin 2, parent is zero
	Z_BIN3_CTX,		//6		-bin 3, parent is zero
	Z_BIN4_CTX,		//7		-bin 4, parent is zero
	Z_BIN5plus_CTX,	//8		-bins 5 plus, parent is zero

	NZ_BIN1z_CTX,	//9		-bin 1, parent is non-zero, neighbours zero
	NZ_BIN1a_CTX,	//10	-bin 1, parent is non-zero, neighbours small
	NZ_BIN1b_CTX,	//11	-bin 1, parent is non-zero, neighbours large
	NZ_BIN2_CTX,	//12	-bin 2, parent is non-zero
	NZ_BIN3_CTX,	//13	-bin 3, parent is non-zero
	NZ_BIN4_CTX,	//14	-bin 4, parent is non-zero
	NZ_BIN5plus_CTX,//15	-bins 5 plus, parent is non-zero

	ZTz_CTX,		//16	-zerotree, neighbouring symbols are zerotree elements
	ZTnz_CTX,		//17	-zerotree, neighbouring symbols are not zerotree elements
	ZTzb_CTX,		//16	-zerotree, neighbouring symbols are zerotree elements
	ZTnzb_CTX		//17	-zerotree, neighbouring symbols are not zerotree elements
};

enum MvCtxAliases{//used for MV data coding

	YDC_BIN1_CTX,		//0 	-1st bin of DC value for Y
	YDC_BIN2plus_CTX,	//1 	-remaining DC bins
	YDC_SIGN0_CTX,		//2		-sign of Y DC value, previous value 0
	UDC_BIN1_CTX,		//3 	--ditto
	UDC_BIN2plus_CTX,	//4 	--for
	UDC_SIGN0_CTX,		//5		--U
	VDC_BIN1_CTX,		//6 	--and
	VDC_BIN2plus_CTX,	//7 	--V
	VDC_SIGN0_CTX,		//8		--components

	REF1x_BIN1_CTX,		//9		-bin 1, REF1 x vals
	REF1x_BIN2_CTX,		//10	-bin 2, REF1 x vals
	REF1x_BIN3_CTX,		//11	-bin 3, REF1 x vals
	REF1x_BIN4_CTX,		//12	-bin 4, REF1 x vals
	REF1x_BIN5plus_CTX,	//13	-bin 5, REF1 x vals
	REF1x_SIGN0_CTX,	//14	-sign, REF1 x vals, previous value 0
	REF1x_SIGNP_CTX,	//15	-sign, REF1 x vals, previous value +ve
	REF1x_SIGNN_CTX,	//16	-sign, REF1 x vals, previous value -ve

	REF1y_BIN1_CTX,		//17		-bin 1, REF1 y vals
	REF1y_BIN2_CTX,		//18	-bin 2, REF1 y vals
	REF1y_BIN3_CTX,		//19	-bin 3, REF1 y vals
	REF1y_BIN4_CTX,		//20	-bin 4, REF1 y vals
	REF1y_BIN5plus_CTX,	//21	-bin 5, REF1 y vals
	REF1y_SIGN0_CTX,	//22	-sign, REF1 y vals, previous value 0
	REF1y_SIGNP_CTX,	//23	-sign, REF1 y vals, previous value +ve
	REF1y_SIGNN_CTX,	//24	-sign, REF1 y vals, previous value -ve

	REF2x_BIN1_CTX,		//25	-bin 1, REF2 x vals
	REF2x_BIN2_CTX,		//26	-bin 2, REF2 x vals
	REF2x_BIN3_CTX,		//27	-bin 3, REF2 x vals
	REF2x_BIN4_CTX,		//28	-bin 4, REF2 x vals
	REF2x_BIN5plus_CTX,	//29	-bin 5, REF2 x vals
	REF2x_SIGN0_CTX,	//30	-sign, REF2 x vals, previous value 0
	REF2x_SIGNP_CTX,	//31	-sign, REF1 y vals, previous value +ve
	REF2x_SIGNN_CTX,	//32	-sign, REF1 y vals, previous value -ve

	REF2y_BIN1_CTX,		//33	-bin 1, REF2 y vals
	REF2y_BIN2_CTX,		//34	-bin 2, REF2 y vals
	REF2y_BIN3_CTX,		//35	-bin 3, REF2 y vals
	REF2y_BIN4_CTX,		//36	-bin 4, REF2 y vals
	REF2y_BIN5plus_CTX,	//37	-bin 5, REF2 y vals
	REF2y_SIGN0_CTX,	//38	-sign, REF2 y vals, previous value 0
	REF2y_SIGNP_CTX,	//39	-sign, REF2 y vals, previous value +ve
	REF2y_SIGNN_CTX,	//40	-sign, REF2 y vals, previous value -ve

	PMODE_BIN1_CTX,		//41	-bin 1, prediction mode value
	PMODE_BIN2_CTX,		//42	-bin 2, prediction mode value
	PMODE_BIN3_CTX,		//43	-bin 3, prediction mode value. Bin 4 not required

	MB_CMODE_CTX,		//44	-context for MB common block mode
	MB_SPLIT_BIN1_CTX,	//45	-bin1, MB split mode vals
	MB_SPLIT_BIN2_CTX	//46	-bin2, MB split mode vals. Bin 3 not required
};

class EntropyCorrector{//factors for correcting entropy estimates
public:
	EntropyCorrector(int depth): Yfctrs(3*depth+1,3),Ufctrs(3*depth+1,3),Vfctrs(3*depth+1,3){Init();}
	float Factor(int bandnum, FrameSort fsort,CompSort c);
	void Update(int bandnum, FrameSort fsort, CompSort c,int est_bits,int actual_bits);//updates the factors
private:
	void Init();//initialises the correction factors
	TwoDArray<float> Yfctrs;
	TwoDArray<float> Ufctrs;
	TwoDArray<float> Vfctrs;
};

class ContextInitialiser{	//class to provide initial values for contexts for arithmetic coding
							//of subbands.Place in band_codec?
public:
	ContextInitialiser(int num_bands) {
		init_list=new OneDArray<std::vector<Context>*>(3*3*num_bands);
		for (int I=init_list->first();I<=init_list->last();++I)
			(*init_list)[I]=new std::vector<Context>(24);}
	~ContextInitialiser(){
		for (int I=init_list->first();I<=init_list->last();++I)
			delete (*init_list)[I];
		delete init_list;
	}

	std::vector<Context>& GetInitCtx(FrameSort& fsort,CompSort& csort,int band_num);
	void SetInitCtx(FrameSort& fsort,CompSort& csort,int band_num,std::vector<Context>& inits);

private:
	OneDArray<std::vector<Context>*>* init_list;
};


struct OLBParams{//params for overlapped blocks
	int XBLEN,YBLEN,XBSEP,YBSEP,XOFFSET,YOFFSET;
};

struct SeqParams{//parameters relating to the video sequence

	SeqParams(): xl(0),yl(0),zl(0),cformat(format422),
	interlace(false),topfieldfirst(true),framerate(12){}
	int xl;
	int yl;
	int zl;	
	ChromaFormat cformat;
	bool interlace;		//is the sequence interlaced?
	bool topfieldfirst;	//if interlaced, is the top field first
	int framerate;		//frame rate
};

struct CodecParams{//parameters relating to the operation of the coder and decoder

	//parameters used throughout the codec
	//for coding and decoding
	CodecParams(): X_NUM_MB(0),Y_NUM_MB(0),X_NUMBLOCKS(0),Y_NUMBLOCKS(0),
	GOP_LEN(1),NUM_L1(0),L1_SEP(0),VERBOSE(false),sparams(),lbparams(3),cbparams(3){}

	CodecParams(SeqParams& sp): X_NUM_MB(0),Y_NUM_MB(0),X_NUMBLOCKS(0),Y_NUMBLOCKS(0),
	GOP_LEN(1),NUM_L1(0),L1_SEP(0),VERBOSE(false),sparams(sp),lbparams(3),cbparams(3){}

	OLBParams& LumaBParams(int n){return lbparams[n];}
	OLBParams& ChromaBParams(int n){return cbparams[n];}
	void SetBlockSizes(OLBParams& olbparams);

	int X_NUM_MB,Y_NUM_MB;
	int X_NUMBLOCKS, Y_NUMBLOCKS;
	int GOP_LEN;		//overall (target)
	int NUM_L1;			//gop params
	int L1_SEP;			//-can be overridden for particular GOPs
	bool VERBOSE;
	SeqParams sparams;
	ContextInitialiser* ctx_inits;

private:
	OneDArray<OLBParams> lbparams;
	OneDArray<OLBParams> cbparams;
};

class EncoderParams: public CodecParams{//codec params plus parameters relating solely to the operation of the encoder
public:
	EncoderParams(): CodecParams(),UFACTOR(1.0),VFACTOR(1.0),CPD(20.0),I_lambda(0.f),L1_lambda(0.0f),
	L2_lambda(0.0f),L1I_lambda(0.0f),L1_ME_lambda(0.0f),L2_ME_lambda(0.0f),L1I_ME_lambda(0.0f),
	EntCorrect(0),BIT_OUT(0){}
	EncoderParams(SeqParams& sp): CodecParams(sp),UFACTOR(1.0),VFACTOR(1.0),CPD(20.0),I_lambda(0.f),L1_lambda(0.0f),
	L2_lambda(0.0f),L1I_lambda(0.0f),L1_ME_lambda(0.0f),L2_ME_lambda(0.0f),L1I_ME_lambda(0.0f),
	EntCorrect(0),BIT_OUT(0){}

	float UFACTOR;		//factors for weighting U
	float VFACTOR;		//and V quantisation noise
	float CPD;			//cycles per degree
	float I_lambda,L1_lambda,L2_lambda,L1I_lambda;//Lagrangian parameters for coding
	float L1_ME_lambda,L2_ME_lambda,L1I_ME_lambda;//Lagrangian params for motion estimation
	EntropyCorrector* EntCorrect;
	BitOutputManager* BIT_OUT;	//ptr to object for managing bitstream output
};


class DecoderParams: public CodecParams{//codec params plus parameters relating solely to the operation of the encoder
public:
	DecoderParams(): CodecParams(),BIT_IN(0){}
	DecoderParams(SeqParams& sp): CodecParams(sp),BIT_IN(0){}
	BitInputManager* BIT_IN;	//ptr to object for managing bitstream input
};

struct FrameParams {
	//parameters for setting up a frame object

	FrameParams(): fsort(I_frame){}	
	FrameParams(const SeqParams& sp): seq_params(sp),fsort(I_frame){}
	FrameParams(const SeqParams& sp, FrameSort& fs): seq_params(sp),fsort(fs){}	

	SeqParams seq_params;	
	FrameSort fsort;	
};

//A simple bounds checking function
inline ValueType BChk(const ValueType &num, const ValueType &max){
	if(num < 0) return 0;
	else if(num >= max) return max-1;
	else return num;
}

#endif
