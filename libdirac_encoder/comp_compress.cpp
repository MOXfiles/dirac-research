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
* Revision 1.2  2004-03-22 01:04:28  chaoticcoyote
* Added API documentation to encoder library
* Moved large constructors so they are no longer inlined
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

//Compression of an individual component,
//after motion compensation if appropriate
//////////////////////////////////////////

#include "libdirac_encoder/comp_compress.h"
#include "libdirac_common/context.h"
#include "libdirac_common/band_codec.h"
#include "libdirac_common/golomb.h"
#include <ctime>
#include <vector>
#include <iostream>

CompCompressor::CompCompressor(EncoderParams& encp,FrameParams& fp)
  : encparams(encp),
    fparams(fp),
	qflist(60),
    qfinvlist(60),
    offset(60)
{
    Init();
}

void CompCompressor::Init()
{
	fsort=fparams.fsort;
	cformat=fparams.seq_params.cformat;
}

void CompCompressor::Compress(PicArray& pic_data){
	//need to transform, select quantisers for each band, and then compress each component in turn
	csort=pic_data.csort;	
	int depth=4;
	int num_band_bits;
	int est_band_bits;//estimated number of band bits
	BandCodec* bcoder;
	std::vector<Context> ctx_list(24);//could be empty

	Subband node;

	//set up Lagrangian params	
	if (fsort==I_frame) lambda=encparams.I_lambda;
	else if (fsort==L1_frame) lambda=encparams.L1_lambda;
	else lambda=encparams.L2_lambda;

	if (csort==U) lambda*=encparams.UFACTOR;
	if (csort==V) lambda*=encparams.VFACTOR;

	WaveletTransformParams wparams(depth);
	WaveletTransform wtransform(wparams);
	wtransform.Transform(FORWARD,pic_data);
	wtransform.SetBandWeights(encparams,fparams,csort);

	SubbandList& bands=wtransform.BandList();

	unsigned int old_total_bytes=encparams.BIT_OUT->GetTotalBytes();
	unsigned int old_total_head_bytes=encparams.BIT_OUT->GetTotalHeadBytes();	
	unsigned int total_bytes;
	unsigned int total_head;

	GenQuantList();//generate all the quantisation data
	for (int I=bands.length();I>=1;--I){

		est_band_bits=SelectQuant(pic_data,bands,I);

		GolombCode((encparams.BIT_OUT)->header,bands(I).qf(0));
		if (bands(I).qf(0)!=-1){//if not skipped			
			bands(I).set_qf(0,qflist[bands(I).qf(0)]);
			if (I>=bands.length()){
				if (fsort==I_frame && I==bands.length())
					bcoder=new IntraDCBandCodec(&(encparams.BIT_OUT->data),ctx_list,bands);
				else
					bcoder=new LFBandCodec(&(encparams.BIT_OUT->data),ctx_list,bands,I);
			}
			else
				bcoder=new BandCodec(&(encparams.BIT_OUT->data),ctx_list,bands,I);

			bcoder->InitContexts();//may not be necessary
			num_band_bits=bcoder->Compress(pic_data);
			encparams.EntCorrect->Update(I,fsort,csort,est_band_bits,num_band_bits);//update the entropy correction factors
			GolombCode((encparams.BIT_OUT)->header,num_band_bits);
			encparams.BIT_OUT->WriteToFile();
			delete bcoder;
		}
		else{
			encparams.BIT_OUT->WriteToFile();
			SetToZero(pic_data,bands(I));				
		}		
	}

	total_bytes=encparams.BIT_OUT->GetTotalBytes();
	total_head=encparams.BIT_OUT->GetTotalHeadBytes();

	if (encparams.VERBOSE){
		std::cerr<<std::endl<<"Total component bits="<<(total_bytes-old_total_bytes)*8;
		std::cerr<<", of which "<<(total_head-old_total_head_bytes)*8<<" were header.";
	}
	wtransform.Transform(BACKWARD,pic_data);
}

void CompCompressor::GenQuantList(){//generates the list of quantisers and inverse quantisers
	//there is some repetition in this list but at the moment this is easiest from the perspective of SelectQuant
	//Need to remove this repetition later

	qflist[0]=1;		qfinvlist[0]=131072;	offset[0]=0;
	qflist[1]=1;		qfinvlist[1]=131072;	offset[1]=0;
	qflist[2]=1;		qfinvlist[2]=131072;	offset[2]=0;
	qflist[3]=1;		qfinvlist[3]=131072;	offset[3]=0;
	qflist[4]=2;		qfinvlist[4]=65536;		offset[4]=1;
	qflist[5]=2;		qfinvlist[5]=65536;		offset[5]=1;
	qflist[6]=2;		qfinvlist[6]=65536;		offset[6]=1;
	qflist[7]=3;		qfinvlist[7]=43690;		offset[7]=1;
	qflist[8]=4;		qfinvlist[8]=32768;		offset[8]=2;
	qflist[9]=4;		qfinvlist[9]=32768;		offset[9]=2;
	qflist[10]=5;		qfinvlist[10]=26214;	offset[10]=2;
	qflist[11]=6;		qfinvlist[11]=21845;	offset[11]=2;
	qflist[12]=8;		qfinvlist[12]=16384;	offset[12]=3;
	qflist[13]=9;		qfinvlist[13]=14563;	offset[13]=3;
	qflist[14]=11;		qfinvlist[14]=11915;	offset[14]=4;
	qflist[15]=13;		qfinvlist[15]=10082;	offset[15]=5;
	qflist[16]=16;		qfinvlist[16]=8192;		offset[16]=6;
	qflist[17]=19;		qfinvlist[17]=6898;		offset[17]=7;
	qflist[18]=22;		qfinvlist[18]=5957;		offset[18]=8;
	qflist[19]=26;		qfinvlist[19]=5041;		offset[19]=10;
	qflist[20]=32;		qfinvlist[20]=4096;		offset[20]=12;
	qflist[21]=38;		qfinvlist[21]=3449;		offset[21]=14;
	qflist[22]=45;		qfinvlist[22]=2912;		offset[22]=17;
	qflist[23]=53;		qfinvlist[23]=2473;		offset[23]=20;
	qflist[24]=64;		qfinvlist[24]=2048;		offset[24]=24;
	qflist[25]=76;		qfinvlist[25]=1724;		offset[25]=29;
	qflist[26]=90;		qfinvlist[26]=1456;		offset[26]=34;
	qflist[27]=107;		qfinvlist[27]=1224;		offset[27]=40;
	qflist[28]=128;		qfinvlist[28]=1024;		offset[28]=48;
	qflist[29]=152;		qfinvlist[29]=862;		offset[29]=57;
	qflist[30]=181;		qfinvlist[30]=724;		offset[30]=68;
	qflist[31]=215;		qfinvlist[31]=609;		offset[31]=81;
	qflist[32]=256;		qfinvlist[32]=512;		offset[32]=96;
	qflist[33]=304;		qfinvlist[33]=431;		offset[33]=114;
	qflist[34]=362;		qfinvlist[34]=362;		offset[34]=136;
	qflist[35]=430;		qfinvlist[35]=304;		offset[35]=161;
	qflist[36]=512;		qfinvlist[36]=256;		offset[36]=192;
	qflist[37]=608;		qfinvlist[37]=215;		offset[37]=228;
	qflist[38]=724;		qfinvlist[38]=181;		offset[38]=272;
	qflist[39]=861;		qfinvlist[39]=152;		offset[39]=323;
	qflist[40]=1024;	qfinvlist[40]=128;		offset[40]=384;
	qflist[41]=1217;	qfinvlist[41]=107;		offset[41]=456;
	qflist[42]=1448;	qfinvlist[42]=90;		offset[42]=543;
	qflist[43]=1722;	qfinvlist[43]=76;		offset[43]=646;
	qflist[44]=2048;	qfinvlist[44]=64;		offset[44]=768;
	qflist[45]=2435;	qfinvlist[45]=53;		offset[45]=913;
	qflist[46]=2896;	qfinvlist[46]=45;		offset[46]=1086;
	qflist[47]=3444;	qfinvlist[47]=38;		offset[47]=1292;
	qflist[48]=4096;	qfinvlist[48]=32;		offset[48]=1536;
	qflist[49]=4870;	qfinvlist[49]=26;		offset[49]=1826;
	qflist[50]=5792;	qfinvlist[50]=22;		offset[50]=2172;
	qflist[51]=6888;	qfinvlist[51]=19;		offset[51]=2583;
	qflist[52]=8192;	qfinvlist[52]=16;		offset[52]=3072;
	qflist[53]=9741;	qfinvlist[53]=13;		offset[53]=3653;
	qflist[54]=11585;	qfinvlist[54]=11;		offset[54]=4344;
	qflist[55]=13777;	qfinvlist[55]=9;		offset[55]=5166;
	qflist[56]=16384;	qfinvlist[56]=8;		offset[56]=6144;
	qflist[57]=19483;	qfinvlist[57]=6;		offset[57]=7306;
	qflist[58]=23170;	qfinvlist[58]=5;		offset[58]=8689;
	qflist[59]=27554;	qfinvlist[59]=4;		offset[59]=10333;	
}

int CompCompressor::SelectQuant(PicArray& pic_data,SubbandList& bands,int band_num){

	Subband& node=bands(band_num);

	if (band_num==bands.length()){
		AddSubAverage(pic_data,node.xl(),node.yl(),SUBTRACT);
	}

	int min_idx;
	double bandmax=PicAbsMax(pic_data,node.xp(),node.yp(),node.xl(),node.yl());
	if (bandmax>=1)
		node.set_max(int(floor(log(float(bandmax))/log(2.0))));
	else
		node.set_max(0);
	int length=4*node.max()+5;//this is the number of quantisers that are possible

	OneDArray<int> count0(length);
	int count1;	
	OneDArray<int> countPOS(length);
	OneDArray<int> countNEG(length);	
	OneDArray<float> error_total(length);	
	OneDArray<CostType> costs(length);
	int quant_val;	
	ValueType val,abs_val;
	int error;
	float p0,p1;	
	double sign_entropy;	

	int xp=node.xp();
	int yp=node.yp();
	int xl=node.xl();
	int yl=node.yl();
	float vol;

	if (bandmax==0){
		node.set_qf(0,-1);//indicates that the subband is skipped
		if (band_num==bands.length()){
			AddSubAverage(pic_data,node.xl(),node.yl(),ADD);
		}
		return 0;		
	}
	else{
		for (int Q=0;Q<costs.length(0);Q++){
			error_total[Q]=0.0;			
			count0[Q]=0;
			countPOS[Q]=0;
			countNEG[Q]=0;			
		}

		//first, find to nearest integral number of bits using 1/4 of the data
		//////////////////////////////////////////////////////////////////////
		vol=float((yl/2)*(xl/2));//vol is only 1/4 of the coeffs
		count1=int(vol);
		for (int J=yp+1;J<yp+yl;J+=2){
			for (int I=xp+(J%4)/2;I<xp+xl;I+=2){
				//first do no quant at all i.e. divide by 1
				val=pic_data[J][I];
				abs_val=abs(val);
				if (abs_val!=0){
					count0[0]+=abs_val;
					if (val>0.0)
						countPOS[0]++;
					else
						countNEG[0]++;
				}				
				//now do real quantisation
				quant_val=abs_val;				
				for (int Q=4;Q<costs.length(0);Q+=4){
					quant_val>>=(Q/4);								
					if (quant_val){
						count0[Q]+=quant_val;
						quant_val<<=(Q/4);						
						if (val>0){
							countPOS[Q]++;
						}
						else{
							countNEG[Q]++;
						}
					}
					error=abs_val-quant_val;
					if (quant_val!=0)					
						error-=offset[Q];
					error*=error;
					error_total[Q]+=error;
				}//Q
			}//J
		}//I

 		//do entropy calculation etc		
		for (int Q=0;Q<costs.length(0);Q+=4){
			costs[Q].MSE=error_total[Q]/(vol*node.wt()*node.wt());
 		 	//calculate probabilities and entropy
			p0=float(count0[Q])/float(count0[Q]+count1);
			p1=1.0-p0;

			if (p0!=0.0 && p1!=0.0)
				costs[Q].ENTROPY=-(p0*log(p0)+p1*log(p1))/log(2.0);
			else
				costs[Q].ENTROPY=0.0;
			//we want the entropy *per symbol*, not per bit ...			
			costs[Q].ENTROPY*=float(count0[Q]+count1);
			costs[Q].ENTROPY/=vol;

			//now add in the sign entropy
			if (countPOS[Q]+countNEG[Q]!=0){
				p0=float(countNEG[Q])/float(countPOS[Q]+countNEG[Q]);
				p1=1.0-p0;
				if (p0!=0.0 && p1!=0.0)
					sign_entropy=-((p0*log(p0)+p1*log(p1))/log(2.0));
				else
					sign_entropy=0.0;
			}
			else
				sign_entropy=0.0;	

 		 	//we want the entropy *per symbol*, not per bit ...
			sign_entropy*=float(countNEG[Q]+countPOS[Q]);
			sign_entropy/=vol;	

			costs[Q].ENTROPY+=sign_entropy;
			costs[Q].ENTROPY*=encparams.EntCorrect->Factor(band_num,fsort,csort);//sort out correction factors
			costs[Q].TOTAL=costs[Q].MSE+lambda*costs[Q].ENTROPY;
		}
		//find the qf with the lowest cost
		min_idx=0;
		for (int Q=0;Q<costs.length(0);Q+=4) {
			if (costs[Q].TOTAL<costs[min_idx].TOTAL)
				min_idx=Q;
		}

//
// 		cerr<<endl<<"Band num: "<<band_num;
// 		cerr<<endl<<"24 (initial): "<<costs[24].ENTROPY<<" "<<costs[24].MSE<<" "<<costs[24].TOTAL<<" "<<lambda;		

		//now repeat to get to 1/2 bit accuracy
		///////////////////////////////////////
		for (int Q=std::max(0,min_idx-2);Q<=std::min(costs.ubound(0),min_idx+2);Q+=2){
			if (Q!=min_idx){
				error_total[Q]=0.0;			
				count0[Q]=0;
				countPOS[Q]=0;
				countNEG[Q]=0;			
			}
		}
		vol=float((yl/2)*(xl/2));
		count1=int(vol);
		int top_idx=std::min(costs.ubound(0),min_idx+2);
		int bottom_idx=std::max(0,min_idx-2);

		for (int J=yp+1;J<yp+yl;J+=2){
			for (int I=xp+1;I<xp+xl;I+=2){
				val=pic_data[J][I];
				abs_val=abs(val);

				for (int Q=bottom_idx;Q<=top_idx;Q+=2){
					if (Q!=min_idx){
						quant_val=int(abs_val);					
						quant_val*=qfinvlist[Q];
						quant_val>>=17;
						if (quant_val){
							count0[Q]+=quant_val;
							quant_val*=qflist[Q];						
							if (val>0.0){
								countPOS[Q]++;
							}
							else{
								countNEG[Q]++;
							}
						}
						error=abs_val-quant_val;
						if (quant_val!=0)					
							error-=offset[Q];
						error*=error;
						error_total[Q]+=float(error);
					}//end of if
				}//Q
			}//J
		}//I

 		//do entropy calculation		
		for (int Q=bottom_idx;Q<=top_idx;Q+=2){
			if (Q!=min_idx){
				costs[Q].MSE=error_total[Q]/(vol*node.wt()*node.wt());
	 		 	//calculate probabilities and entropy
				p0=float(count0[Q])/float(count0[Q]+count1);
				p1=1.0-p0;

				if (p0!=0.0 && p1!=0.0)
					costs[Q].ENTROPY=-(p0*log(p0)+p1*log(p1))/log(2.0);
				else
					costs[Q].ENTROPY=0.0;
				//we want the entropy *per symbol*, not per bit ...			
				costs[Q].ENTROPY*=count0[Q]+count1;
				costs[Q].ENTROPY/=vol;

 			//now add in the sign entropy
				if (countPOS[Q]+countNEG[Q]!=0){
					p0=float(countNEG[Q])/float(countPOS[Q]+countNEG[Q]);
					p1=1.0-p0;
					if (p0!=0.0 && p1!=0.0)
						sign_entropy=-((p0*log(p0)+p1*log(p1))/log(2.0));
					else
						sign_entropy=0.0;
				}
				else
					sign_entropy=0.0;	

 		 	//we want the entropy *per symbol*, not per bit ...
				sign_entropy*=float(countNEG[Q]+countPOS[Q]);
				sign_entropy/=vol;	

				costs[Q].ENTROPY+=sign_entropy;
				costs[Q].ENTROPY*=encparams.EntCorrect->Factor(band_num,fsort,csort);//sort out correction factors
				costs[Q].TOTAL=costs[Q].MSE+lambda*costs[Q].ENTROPY;
			}
		}//Q

 		//find the qf with the lowest cost
		for (int Q=bottom_idx;Q<=top_idx;Q+=2){
			if (costs[Q].TOTAL<costs[min_idx].TOTAL)
				min_idx=Q;
		}

 		//finally use 1/2 the values to get 1/4 bit accuracy
		////////////////////////////////////////////////////		

		bottom_idx=std::max(0,min_idx-1);
		top_idx=std::min(costs.length(0)-1,min_idx+1);
		for (int Q=bottom_idx;Q<=top_idx;Q++){
			error_total[Q]=0.0;			
			count0[Q]=0;
			countPOS[Q]=0;
			countNEG[Q]=0;			
		}
		vol=float((yl/2)*xl);
		count1=int(vol);		
		for (int J=yp;J<yp+yl;++J){				
			for (int I=xp+1;I<xp+xl;I+=2){				
				val=pic_data[J][I];
				abs_val=abs(val);
				for (int Q=bottom_idx;Q<=top_idx;Q++){
					quant_val=int(abs_val);					
					quant_val*=qfinvlist[Q];
					quant_val>>=17;
					if (quant_val){
						count0[Q]+=quant_val;
						quant_val*=qflist[Q];						
						if (val>0){
							countPOS[Q]++;
						}
						else{
							countNEG[Q]++;
						}
					}
					error=abs_val-quant_val;
					if (quant_val!=0)					
						error-=offset[Q];
					error*=error;
					error_total[Q]+=float(error);
				}//Q
			}//J
		}//I

 		//do entropy calculation		
		for (int Q=bottom_idx;Q<=top_idx;Q++){
			costs[Q].MSE=error_total[Q]/(vol*node.wt()*node.wt());

		 	//calculate probabilities and entropy
			p0=float(count0[Q])/float(count0[Q]+count1);
			p1=1.0-p0;
			if (p0!=0.0 && p1!=0.0)
				costs[Q].ENTROPY=-(p0*log(p0)+p1*log(p1))/log(2.0);
			else
				costs[Q].ENTROPY=0.0;
 		 	//we want the entropy *per symbol*, not per bit ...			
			costs[Q].ENTROPY*=float(count0[Q]+count1);
			costs[Q].ENTROPY/=vol;

 			//now add in the sign entropy
			if (countPOS[Q]+countNEG[Q]!=0){
				p0=float(countNEG[Q])/float(countPOS[Q]+countNEG[Q]);
				p1=1.0-p0;
				if (p0!=0.0 && p1!=0.0)
					sign_entropy=-((p0*log(p0)+p1*log(p1))/log(2.0));
				else
					sign_entropy=0.0;
			}
			else
				sign_entropy=0.0;	

 		 	//we want the entropy *per symbol*, not per bit ...
			sign_entropy*=float(countNEG[Q]+countPOS[Q]);
			sign_entropy/=vol;	

			costs[Q].ENTROPY+=sign_entropy;
			costs[Q].ENTROPY*=encparams.EntCorrect->Factor(band_num,fsort,csort);//sort out correction factors
			costs[Q].TOTAL=costs[Q].MSE+lambda*costs[Q].ENTROPY;
		}//Q

 		//find the qf with the lowest cost
		for (int Q=bottom_idx;Q<=top_idx;Q++){
			if (costs[Q].TOTAL<costs[min_idx].TOTAL)
				min_idx=Q;
		}
//
// 		cerr<<endl<<"Band num: "<<band_num;
// 		cerr<<endl<<"23: "<<costs[23].ENTROPY<<" "<<costs[23].MSE<<" "<<costs[23].TOTAL<<" "<<lambda;
// 		cerr<<endl<<"24: "<<costs[24].ENTROPY<<" "<<costs[24].MSE<<" "<<costs[24].TOTAL<<" "<<lambda;

		if (costs[min_idx].ENTROPY==0.0)//then can skip after all
			node.set_qf(0,-1);
		else
			node.set_qf(0,min_idx);

		if (band_num==bands.length()){
			AddSubAverage(pic_data,node.xl(),node.yl(),ADD);
		}
		return int(costs[min_idx].ENTROPY*float(xl*yl));
	}

}

ValueType CompCompressor::PicAbsMax(PicArray& pic_data){
	//finds the maximum absolute value of the picture array
	return PicAbsMax(pic_data,pic_data.lbound(0),pic_data.lbound(1),pic_data.length(0),pic_data.length(1));
}

ValueType CompCompressor::PicAbsMax(PicArray& pic_data,int xp, int yp ,int xl ,int yl){
	int lbound0=std::max(pic_data.lbound(0),xp);	
	int lbound1=std::max(pic_data.lbound(1),yp);	
	int ubound0=std::min(pic_data.ubound(0),xp+xl-1);	
	int ubound1=std::min(pic_data.ubound(1),yp+yl-1);		
	ValueType val=0;

	for (int J=lbound1;J<=ubound1;++J){
		for (int I=lbound0;I<=ubound0;++I){	
			val=std::max(val,pic_data[J][I]);	
		}
	}
	return val;
}

void CompCompressor::SetToZero(PicArray& pic_data,Subband& node){
	for (int J=node.yp();J<node.yp()+node.yl();++J){	
		for (int I=node.xp();I<node.xp()+node.xl();++I){
			pic_data[J][I]=0;
		}
	}
}

void CompCompressor::AddSubAverage(PicArray& pic_data,int xl,int yl,AddOrSub dirn){

	ValueType last_val=8187;//corresponds to mid-grey in this band with these filters
	ValueType last_val2;
	if (dirn==SUBTRACT){
		for (int J=0;J<yl;J++){
			for (int I=0;I<xl;I++){
				last_val2=pic_data[J][I];		
				pic_data[J][I]-=last_val;
				last_val=last_val2;
			}
		}
	}
	else{
		for (int J=0;J<yl;J++){
			for (int I=0;I<xl;I++){
				pic_data[J][I]+=last_val;
				last_val=pic_data[J][I];
			}
		}

	}
}
