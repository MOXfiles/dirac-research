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
* Revision 0.1.0  2004/02/20 09:36:09  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "mv_codec.h"
#include <iostream>

//coding functions//
////////////////////

void MvDataCodec::DoWorkCode(MvData& in_data){
	int step,max;
	int pstep,pmax;
	int split_depth;
	bool common_ref;

	MB_count=0;
	for (mb_yp=0,mb_tlb_y=0;mb_yp<in_data.mb.length(1);++mb_yp,mb_tlb_y+=4){
		for (mb_xp=0,mb_tlb_x=0;mb_xp<in_data.mb.length(0);++mb_xp,mb_tlb_x+=4){
 			//start with split mode
			CodeMBSplit(in_data);
			split_depth=in_data.mb[mb_yp][mb_xp].split_mode;

			step=4>>(split_depth);
			max=(1<<split_depth);

			//next do common_ref
			if(split_depth!=0){
				CodeMBCom(in_data);
				pstep=step;
				pmax=max;
			}
			else{
				pstep=4;
				pmax=1;
			}
			common_ref=in_data.mb[mb_yp][mb_xp].common_ref;


			//do prediction modes			
			for (b_yp=mb_tlb_y;b_yp<mb_tlb_y+4;b_yp+=pstep){				
				for (b_xp=mb_tlb_x;b_xp<mb_tlb_x+4;b_xp+=pstep){
					CodePredmode(in_data);
				}//b_xp
			}//b_yp
			step=4>>(split_depth);			
   			//now do all the block mvs in the mb			
			for (b_yp=mb_tlb_y;b_yp<mb_tlb_y+4;b_yp+=step){
				for (b_xp=mb_tlb_x;b_xp<mb_tlb_x+4;b_xp+=step){
					if (in_data.mode[b_yp][b_xp]==REF1_ONLY || in_data.mode[b_yp][b_xp]==REF1AND2 )
						CodeMv1(in_data);
					if (in_data.mode[b_yp][b_xp]==REF2_ONLY || in_data.mode[b_yp][b_xp]==REF1AND2 )
						CodeMv2(in_data);
					if(in_data.mode[b_yp][b_xp]==INTRA)
						CodeDC(in_data);					
				}//b_xp
			}//b_yp	
			//Update all contexts here?
		}//mb_xp
	}//mb_yp

}


void MvDataCodec::CodeMBSplit(MvData& in_data){
	int val=in_data.mb[mb_yp][mb_xp].split_mode-MBSplitPrediction(in_data.mb);
	if (val<0)
		val+=3;//produce prediction mod 3	
	int ctx;
	for (int bin=1;bin<=val;++bin){
		ctx=ChooseMBSContext(in_data,bin);
		EncodeSymbol(0,ctx);
	}
	if (val!=2)//if we've had two zeroes, know we must have value 2
		EncodeSymbol(1,ChooseMBSContext(in_data,val+1));
}
void MvDataCodec::CodeMBCom(MvData& in_data){
	bool val=in_data.mb[mb_yp][mb_xp].common_ref;
	if (val!=MBCBModePrediction(in_data.mb)) EncodeSymbol(1,ChooseMBCContext(in_data));
	else EncodeSymbol(0,ChooseMBCContext(in_data));
}

void MvDataCodec::CodePredmode(MvData& in_data){
	int val=in_data.mode[b_yp][b_xp]-BlockModePrediction(in_data.mode);
	if (val<0)
		val+=4;//produce value mod 4	
	for (int bin=1;bin<=val;++bin){
		EncodeSymbol(0,ChoosePredContext(in_data,bin));
	}
	if (val!=3)//if we've had three zeroes, know we must have value 3
		EncodeSymbol(1,ChoosePredContext(in_data,val+1));
}
void MvDataCodec::CodeMv1(MvData& in_data){
	MVector pred=Mv1Prediction(in_data.mv1,in_data.mode);

	int val=in_data.mv1[b_yp][b_xp].x-pred.x;
	int abs_val=abs(val);
	for (int bin=1;bin<=abs_val;++bin){
		EncodeSymbol(0,ChooseREF1xContext(in_data,bin));
	}
	EncodeSymbol(1,ChooseREF1xContext(in_data,abs_val+1));
	if (val!=0){
		EncodeSymbol(((val>0)? 1 : 0),ChooseREF1xSignContext(in_data));
	}

	val=in_data.mv1[b_yp][b_xp].y-pred.y;		
	abs_val=abs(val);	
	for (int bin=1;bin<=abs_val;++bin){		
		EncodeSymbol(0,ChooseREF1yContext(in_data,bin));		
	}
	EncodeSymbol(1,ChooseREF1yContext(in_data,abs_val+1));
	if (val!=0){
		EncodeSymbol(((val>0)? 1 : 0),ChooseREF1ySignContext(in_data));
	}
}
void MvDataCodec::CodeMv2(MvData& in_data){
	MVector pred=Mv2Prediction(in_data.mv2,in_data.mode);	

	int val=in_data.mv2[b_yp][b_xp].x-pred.x;
	int abs_val=abs(val);
	for (int bin=1;bin<=abs_val;++bin){
		EncodeSymbol(0,ChooseREF2xContext(in_data,bin));
	}
	EncodeSymbol(1,ChooseREF2xContext(in_data,abs_val+1));
	if (val!=0){
		EncodeSymbol(((val>0)? 1 : 0),ChooseREF2xSignContext(in_data));
	}

	val=in_data.mv2[b_yp][b_xp].y-pred.y;
	abs_val=abs(val);	
	for (int bin=1;bin<=abs_val;++bin){
		EncodeSymbol(0,ChooseREF2yContext(in_data,bin));
	}
	EncodeSymbol(1,ChooseREF2yContext(in_data,abs_val+1));
	if (val!=0){
		EncodeSymbol(((val>0)? 1 : 0),ChooseREF2ySignContext(in_data));
	}
}
void MvDataCodec::CodeDC(MvData& in_data){

	//begin with Y DC value	
	ValueType val=in_data.dcY[b_yp][b_xp]-DCPrediction(in_data.dcY,in_data.mode);
	ValueType abs_val=abs(val);

	for (ValueType bin=1;bin<=abs_val;++bin){
		EncodeSymbol(0,ChooseYDCContext(in_data,bin));
	}
	EncodeSymbol(1,ChooseYDCContext(in_data,abs_val+1));
	if (val!=0){
		EncodeSymbol(((val>0)? 1 : 0),ChooseYDCSignContext(in_data));
	}
	//now do U and V if necessary
	if (cformat!=Yonly){//continue with U and V DC values

		val=in_data.dcU[b_yp][b_xp]-DCPrediction(in_data.dcU,in_data.mode);
		abs_val=abs(val);

		for (ValueType bin=1;bin<=abs_val;++bin){
			EncodeSymbol(0,ChooseUDCContext(in_data,bin));
		}
		EncodeSymbol(1,ChooseUDCContext(in_data,abs_val+1));
		if (val!=0){
			EncodeSymbol(((val>0)? 1 : 0),ChooseUDCSignContext(in_data));
		}
		val=in_data.dcV[b_yp][b_xp]-DCPrediction(in_data.dcV,in_data.mode);
		abs_val=abs(val);

		for (ValueType bin=1;bin<=abs_val;++bin){
			EncodeSymbol(0,ChooseVDCContext(in_data,bin));
		}
		EncodeSymbol(1,ChooseVDCContext(in_data,abs_val+1));
		if (val!=0){
			EncodeSymbol(((val>0)? 1 : 0),ChooseVDCSignContext(in_data));
		}
	}
}

//decoding functions//
//////////////////////

void MvDataCodec::DoWorkDecode(MvData& out_data, int num_bits){
	int step,max;
	int pstep,pmax;	
	int split_depth;
	bool common_ref;
	int xstart,ystart;	

	for (mb_yp=0,mb_tlb_y=0;mb_yp<out_data.mb.length(1);++mb_yp,mb_tlb_y+=4){
		for (mb_xp=0,mb_tlb_x=0;mb_xp<out_data.mb.length(0);++mb_xp,mb_tlb_x+=4){
 			//start with split mode
			DecodeMBSplit(out_data);
			split_depth=out_data.mb[mb_yp][mb_xp].split_mode;
			step=4>>(split_depth);
			max=(1<<split_depth);

			//next do common_ref
			if(split_depth!=0){
				DecodeMBCom(out_data);
				pstep=step;
				pmax=max;
			}
			else{
				out_data.mb[mb_yp][mb_xp].common_ref=true;
				pstep=4;
				pmax=1;
			}
			common_ref=out_data.mb[mb_yp][mb_xp].common_ref;

			//do prediction modes
			for (b_yp=mb_tlb_y;b_yp<mb_tlb_y+4;b_yp+=pstep){				
				for (b_xp=mb_tlb_x;b_xp<mb_tlb_x+4;b_xp+=pstep){
					DecodePredmode(out_data);
   					//propagate throughout MB				
					for (int y=b_yp;y<b_yp+pstep;y++){
						for (int x=b_xp;x<b_xp+pstep;x++){
							out_data.mode[y][x]=out_data.mode[b_yp][b_xp];														
						}//b_xp
					}//b_yp
				}//I
			}//J

   			//now do all the block mvs in the mb
			for (int J=0;J<max;++J){				
				for (int I=0;I<max;++I){
					xstart=b_xp=mb_tlb_x+I*step;
					ystart=b_yp=mb_tlb_y+J*step;											
					if (out_data.mode[b_yp][b_xp]==REF1_ONLY || out_data.mode[b_yp][b_xp]==REF1AND2 )
						DecodeMv1(out_data);
					if (out_data.mode[b_yp][b_xp]==REF2_ONLY || out_data.mode[b_yp][b_xp]==REF1AND2 )
						DecodeMv2(out_data);
					if(out_data.mode[b_yp][b_xp]==INTRA)
						DecodeDC(out_data);
  					//propagate throughout MB	
					for (b_yp=ystart;b_yp<ystart+step;b_yp++){
						for (b_xp=xstart;b_xp<xstart+step;b_xp++){					
							out_data.mv1[b_yp][b_xp].x=out_data.mv1[ystart][xstart].x;
							out_data.mv1[b_yp][b_xp].y=out_data.mv1[ystart][xstart].y;
							out_data.mv2[b_yp][b_xp].x=out_data.mv2[ystart][xstart].x;
							out_data.mv2[b_yp][b_xp].y=out_data.mv2[ystart][xstart].y;
							out_data.dcY[b_yp][b_xp]=out_data.dcY[ystart][xstart];
							out_data.dcU[b_yp][b_xp]=out_data.dcU[ystart][xstart];
							out_data.dcV[b_yp][b_xp]=out_data.dcV[ystart][xstart];
						}//b_xp
					}//b_yp
				}//I					
			}//J

		}//mb_xp
	}//mb_yp

}

void MvDataCodec::DecodeMBSplit(MvData& out_data){
	int val=0;
	int bin=1;
	bool bit;

	do{
		DecodeSymbol(bit,ChooseMBSContext(out_data,bin));
		if (!bit)
			val++;
		bin++;
	}while (!bit && val!=2); 
	out_data.mb[mb_yp][mb_xp].split_mode=(val+MBSplitPrediction(out_data.mb))%3;	
}

void MvDataCodec::DecodeMBCom(MvData& out_data){
	bool bit;
	DecodeSymbol(bit,ChooseMBCContext(out_data));
	if (bit)
		out_data.mb[mb_yp][mb_xp].common_ref=!MBCBModePrediction(out_data.mb);
	else
		out_data.mb[mb_yp][mb_xp].common_ref=MBCBModePrediction(out_data.mb);
}

void MvDataCodec::DecodePredmode(MvData& out_data){
	int val=0;
	int bin=1;
	bool bit;
	do{
		DecodeSymbol(bit,ChoosePredContext(out_data,bin));				
		if (!bit)
			val++;
		bin++;
	}while (!bit && val!=3); 
	out_data.mode[b_yp][b_xp]=PredMode((val+BlockModePrediction(out_data.mode))%4);
}

void MvDataCodec::DecodeMv1(MvData& out_data){
	MVector pred=Mv1Prediction(out_data.mv1,out_data.mode);	
	int val=0;
	int bin=1;
	bool bit;
	do{
		DecodeSymbol(bit,ChooseREF1xContext(out_data,bin));
		if (!bit)
			val++;
		bin++;
	}while (!bit);
	if (val!=0){
		DecodeSymbol(bit,ChooseREF1xSignContext(out_data));
		if (!bit)
			val=-val;
	}
	out_data.mv1[b_yp][b_xp].x=val+pred.x;

	val=0;
	bin=1;
	do{
		DecodeSymbol(bit,ChooseREF1yContext(out_data,bin));
		if (!bit)
			val++;
		bin++;
	}while (!bit);
	if (val!=0){
		DecodeSymbol(bit,ChooseREF1ySignContext(out_data));
		if (!bit)
			val=-val;
	}
	out_data.mv1[b_yp][b_xp].y=val+pred.y;
}

void MvDataCodec::DecodeMv2(MvData& out_data){
	MVector pred=Mv2Prediction(out_data.mv2,out_data.mode);

	int val=0;
	int bin=1;
	bool bit;
	do{
		DecodeSymbol(bit,ChooseREF2xContext(out_data,bin));
		if (!bit)
			val++;
		bin++;
	}while (!bit);
	if (val!=0){
		DecodeSymbol(bit,ChooseREF2xSignContext(out_data));
		if (!bit)
			val=-val;
	}
	out_data.mv2[b_yp][b_xp].x=val+pred.x;

	val=0;
	bin=1;
	do{
		DecodeSymbol(bit,ChooseREF2yContext(out_data,bin));
		if (!bit)
			val++;
		bin++;
	}while (!bit);
	if (val!=0){
		DecodeSymbol(bit,ChooseREF2ySignContext(out_data));
		if (!bit)
			val=-val;
	}
	out_data.mv2[b_yp][b_xp].y=val+pred.y;

}

void MvDataCodec::DecodeDC(MvData& out_data){

	//begin with Y DC value	
	ValueType val=0;
	int bin=1;
	bool bit;
	do{
		DecodeSymbol(bit,ChooseYDCContext(out_data,bin));
		if (!bit)
			val++;
		bin++;
	}while (!bit);
	if (val!=0){
		DecodeSymbol(bit,ChooseYDCSignContext(out_data));
		if (!bit)
			val=-val;
	}
	out_data.dcY[b_yp][b_xp]=val+DCPrediction(out_data.dcY,out_data.mode);

	if (cformat!=Yonly){//move onto U and V DC values
		val=0;
		bin=1;
		do{
			DecodeSymbol(bit,ChooseUDCContext(out_data,bin));
			if (!bit)
				val++;
			bin++;
		}while (!bit);
		if (val!=0){
			DecodeSymbol(bit,ChooseUDCSignContext(out_data));
			if (!bit)
				val=-val;
		}
		out_data.dcU[b_yp][b_xp]=val+DCPrediction(out_data.dcU,out_data.mode);

		val=0;
		bin=1;
		do{
			DecodeSymbol(bit,ChooseVDCContext(out_data,bin));
			if (!bit)
				val++;
			bin++;
		}while (!bit);
		if (val!=0){
			DecodeSymbol(bit,ChooseVDCSignContext(out_data));
			if (!bit)
				val=-val;
		}
		out_data.dcV[b_yp][b_xp]=val+DCPrediction(out_data.dcV,out_data.mode);
	}
}

//context selection functions//
///////////////////////////////
