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
* The Original Code is BBC Research and Development code.
*
* The Initial Developer of the Original Code is the British Broadcasting
* Corporation.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Thomas Davies (Original Author), Scott R Ladd
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


//Compression of an individual component,
//after motion compensation if appropriate
//////////////////////////////////////////

#include <libdirac_encoder/comp_compress.h>
#include <libdirac_common/band_codec.h>
#include <libdirac_common/golomb.h>
#include <ctime>
#include <vector>
#include <iostream>

CompCompressor::CompCompressor( EncoderParams& encp,const FrameParams& fp): 
m_encparams(encp),
m_fparams(fp),
m_fsort( m_fparams.FSort() ),
m_cformat( m_fparams.CFormat() ),
m_qflist(60),
m_qfinvlist(60),
m_offset(60)
{
}


void CompCompressor::Compress(PicArray& pic_data){
    //need to transform, select quantisers for each band, and then compress each component in turn
    m_csort=pic_data.CSort();	
    const int depth=4;
    unsigned int num_band_bits;
    unsigned int est_band_bits;//estimated number of band bits
    BandCodec* bcoder;
    const size_t CONTEXTS_REQUIRED = 24;

	Subband node;

	//set up Lagrangian params	
	if (m_fsort == I_frame) 
		m_lambda= m_encparams.ILambda();
	else if (m_fsort == L1_frame) 
		m_lambda= m_encparams.L1Lambda();
	else 
		m_lambda= m_encparams.L2Lambda();

	if (m_csort == U) 
		m_lambda*= m_encparams.UFactor();
	if (m_csort == V) 
		m_lambda*= m_encparams.VFactor();

	WaveletTransform wtransform(depth);

	wtransform.Transform( FORWARD , pic_data );
	wtransform.SetBandWeights( m_encparams.CPD() , m_fparams , m_csort);

	SubbandList& bands=wtransform.BandList();

	unsigned int old_total_bytes=m_encparams.BitsOut().GetTotalBytes();
	unsigned int old_total_head_bytes=m_encparams.BitsOut().GetTotalHeadBytes();	
	unsigned int total_bytes;
	unsigned int total_head;

	GenQuantList();//generate all the quantisation data
	for (int I=bands.Length() ; I>=1 ; --I )
	{

		est_band_bits=SelectQuant(pic_data , bands , I);

		GolombCode( m_encparams.BitsOut().Header() , bands(I).Qf(0) );
		if (bands(I).Qf(0)!=-1)
		{//if not skipped			

			bands(I).SetQf(0,m_qflist[bands(I).Qf(0)]);

 			//pick the right codec according to the frame type and subband
			if (I>=bands.Length()){
				if ( m_fsort==I_frame && I==bands.Length() )
					bcoder=new IntraDCBandCodec( &(m_encparams.BitsOut().Data() ) , CONTEXTS_REQUIRED , bands);
				else
					bcoder=new LFBandCodec( &(m_encparams.BitsOut().Data() ) ,CONTEXTS_REQUIRED, bands , I);
			}
			else
				bcoder=new BandCodec( &(m_encparams.BitsOut().Data() ) , CONTEXTS_REQUIRED , bands , I);

			num_band_bits=bcoder->Compress(pic_data);

 			//update the entropy correction factors
			m_encparams.EntropyFactors().Update(I,m_fsort , m_csort , est_band_bits , num_band_bits);

			//Write the length of the data chunk into the header, and flush everything out to file
			UnsignedGolombCode( m_encparams.BitsOut().Header() , num_band_bits);
			m_encparams.BitsOut().WriteToFile();

			delete bcoder;			
		}
		else
		{
			m_encparams.BitsOut().WriteToFile();
			if (I == bands.Length() && m_fsort == I_frame)
				SetToVal(pic_data,bands(I),2692);
			else
				SetToVal(pic_data,bands(I),0);
		}		
	}

	total_bytes=m_encparams.BitsOut().GetTotalBytes();
	total_head=m_encparams.BitsOut().GetTotalHeadBytes();

	if (m_encparams.Verbose())
	{
		std::cerr<<std::endl<<"Total component bits="<<(total_bytes-old_total_bytes)*8;
		std::cerr<<", of which "<<(total_head-old_total_head_bytes)*8<<" were header.";
	}
	wtransform.Transform( BACKWARD , pic_data );
}

void CompCompressor::GenQuantList()
{	//generates the list of quantisers and inverse quantisers
	//there is some repetition in this list but at the moment this is easiest from the perspective of SelectQuant
	//Need to remove this repetition later

	m_qflist[0]=1;		m_qfinvlist[0]=131072;	m_offset[0]=0;
	m_qflist[1]=1;		m_qfinvlist[1]=131072;	m_offset[1]=0;
	m_qflist[2]=1;		m_qfinvlist[2]=131072;	m_offset[2]=0;
	m_qflist[3]=1;		m_qfinvlist[3]=131072;	m_offset[3]=0;
	m_qflist[4]=2;		m_qfinvlist[4]=65536;		m_offset[4]=1;
	m_qflist[5]=2;		m_qfinvlist[5]=65536;		m_offset[5]=1;
	m_qflist[6]=2;		m_qfinvlist[6]=65536;		m_offset[6]=1;
	m_qflist[7]=3;		m_qfinvlist[7]=43690;		m_offset[7]=1;
	m_qflist[8]=4;		m_qfinvlist[8]=32768;		m_offset[8]=2;
	m_qflist[9]=4;		m_qfinvlist[9]=32768;		m_offset[9]=2;
	m_qflist[10]=5;		m_qfinvlist[10]=26214;	m_offset[10]=2;
	m_qflist[11]=6;		m_qfinvlist[11]=21845;	m_offset[11]=2;
	m_qflist[12]=8;		m_qfinvlist[12]=16384;	m_offset[12]=3;
	m_qflist[13]=9;		m_qfinvlist[13]=14563;	m_offset[13]=3;
	m_qflist[14]=11;		m_qfinvlist[14]=11915;	m_offset[14]=4;
	m_qflist[15]=13;		m_qfinvlist[15]=10082;	m_offset[15]=5;
	m_qflist[16]=16;		m_qfinvlist[16]=8192;		m_offset[16]=6;
	m_qflist[17]=19;		m_qfinvlist[17]=6898;		m_offset[17]=7;
	m_qflist[18]=22;		m_qfinvlist[18]=5957;		m_offset[18]=8;
	m_qflist[19]=26;		m_qfinvlist[19]=5041;		m_offset[19]=10;
	m_qflist[20]=32;		m_qfinvlist[20]=4096;		m_offset[20]=12;
	m_qflist[21]=38;		m_qfinvlist[21]=3449;		m_offset[21]=14;
	m_qflist[22]=45;		m_qfinvlist[22]=2912;		m_offset[22]=17;
	m_qflist[23]=53;		m_qfinvlist[23]=2473;		m_offset[23]=20;
	m_qflist[24]=64;		m_qfinvlist[24]=2048;		m_offset[24]=24;
	m_qflist[25]=76;		m_qfinvlist[25]=1724;		m_offset[25]=29;
	m_qflist[26]=90;		m_qfinvlist[26]=1456;		m_offset[26]=34;
	m_qflist[27]=107;		m_qfinvlist[27]=1224;		m_offset[27]=40;
	m_qflist[28]=128;		m_qfinvlist[28]=1024;		m_offset[28]=48;
	m_qflist[29]=152;		m_qfinvlist[29]=862;		m_offset[29]=57;
	m_qflist[30]=181;		m_qfinvlist[30]=724;		m_offset[30]=68;
	m_qflist[31]=215;		m_qfinvlist[31]=609;		m_offset[31]=81;
	m_qflist[32]=256;		m_qfinvlist[32]=512;		m_offset[32]=96;
	m_qflist[33]=304;		m_qfinvlist[33]=431;		m_offset[33]=114;
	m_qflist[34]=362;		m_qfinvlist[34]=362;		m_offset[34]=136;
	m_qflist[35]=430;		m_qfinvlist[35]=304;		m_offset[35]=161;
	m_qflist[36]=512;		m_qfinvlist[36]=256;		m_offset[36]=192;
	m_qflist[37]=608;		m_qfinvlist[37]=215;		m_offset[37]=228;
	m_qflist[38]=724;		m_qfinvlist[38]=181;		m_offset[38]=272;
	m_qflist[39]=861;		m_qfinvlist[39]=152;		m_offset[39]=323;
	m_qflist[40]=1024;	m_qfinvlist[40]=128;		m_offset[40]=384;
	m_qflist[41]=1217;	m_qfinvlist[41]=107;		m_offset[41]=456;
	m_qflist[42]=1448;	m_qfinvlist[42]=90;		m_offset[42]=543;
	m_qflist[43]=1722;	m_qfinvlist[43]=76;		m_offset[43]=646;
	m_qflist[44]=2048;	m_qfinvlist[44]=64;		m_offset[44]=768;
	m_qflist[45]=2435;	m_qfinvlist[45]=53;		m_offset[45]=913;
	m_qflist[46]=2896;	m_qfinvlist[46]=45;		m_offset[46]=1086;
	m_qflist[47]=3444;	m_qfinvlist[47]=38;		m_offset[47]=1292;
	m_qflist[48]=4096;	m_qfinvlist[48]=32;		m_offset[48]=1536;
	m_qflist[49]=4870;	m_qfinvlist[49]=26;		m_offset[49]=1826;
	m_qflist[50]=5792;	m_qfinvlist[50]=22;		m_offset[50]=2172;
	m_qflist[51]=6888;	m_qfinvlist[51]=19;		m_offset[51]=2583;
	m_qflist[52]=8192;	m_qfinvlist[52]=16;		m_offset[52]=3072;
	m_qflist[53]=9741;	m_qfinvlist[53]=13;		m_offset[53]=3653;
	m_qflist[54]=11585;	m_qfinvlist[54]=11;		m_offset[54]=4344;
	m_qflist[55]=13777;	m_qfinvlist[55]=9;		m_offset[55]=5166;
	m_qflist[56]=16384;	m_qfinvlist[56]=8;		m_offset[56]=6144;
	m_qflist[57]=19483;	m_qfinvlist[57]=6;		m_offset[57]=7306;
	m_qflist[58]=23170;	m_qfinvlist[58]=5;		m_offset[58]=8689;
	m_qflist[59]=27554;	m_qfinvlist[59]=4;		m_offset[59]=10333;	
}

int CompCompressor::SelectQuant(PicArray& pic_data,SubbandList& bands,const int band_num)
{

	Subband& node=bands(band_num);

	//Point to start looking for quantisation factors in the qf list.
	//May be able to short-circuit searching in future by setting this
	int qf_start_idx=12;

	if (band_num==bands.Length()){
		AddSubAverage(pic_data,node.Xl(),node.Yl(),SUBTRACT);
	}

	int min_idx;
	double bandmax=PicAbsMax(pic_data,node.Xp(),node.Yp(),node.Xl(),node.Yl());

	if (bandmax>=1)
		node.SetMax(int(floor(log(float(bandmax))/log(2.0))));
	else
		node.SetMax(0);
	int length=4*node.Max()+5;//this is the number of quantisers that are possible

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

	int xp=node.Xp();
	int yp=node.Yp();
	int xl=node.Xl();
	int yl=node.Yl();
	float vol;

	if (bandmax<m_qflist[qf_start_idx-3]){
		//coefficients are small so the subband can be skipped
		node.SetQf(0,-1);//indicates that the subband is skipped
		if (band_num==bands.Length()){
			AddSubAverage(pic_data,node.Xl(),node.Yl(),ADD);
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
			for (int I=xp+((J-yp)%4)/2;I<xp+xl;I+=2){

				val=pic_data[J][I];
				quant_val=abs(val);
				abs_val=quant_val;

				for (int Q=qf_start_idx;Q<costs.length(0);Q+=4){
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
						error-=m_offset[Q];
					error*=error;
					error_total[Q]+=error;
				}//Q
			}//J
		}//I

 		//do entropy calculation etc		
		for (int Q=qf_start_idx;Q<costs.length(0);Q+=4){
			costs[Q].MSE=error_total[Q]/(vol*node.Wt()*node.Wt());
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
			//sort out correction factors
			costs[Q].ENTROPY*=m_encparams.EntropyFactors().Factor(band_num,m_fsort,m_csort);
			costs[Q].TOTAL=costs[Q].MSE+m_lambda*costs[Q].ENTROPY;
		}
		//find the qf with the lowest cost
		min_idx=qf_start_idx;
		for (int Q=qf_start_idx;Q<costs.length(0);Q+=4) {
			if (costs[Q].TOTAL<costs[min_idx].TOTAL)
				min_idx=Q;
		}

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
						quant_val*=m_qfinvlist[Q];
						quant_val>>=17;
						if (quant_val){
							count0[Q]+=quant_val;
							quant_val*=m_qflist[Q];						
							if (val>0.0){
								countPOS[Q]++;
							}
							else{
								countNEG[Q]++;
							}
						}
						error=abs_val-quant_val;
						if (quant_val!=0)					
							error-=m_offset[Q];
						error*=error;
						error_total[Q]+=float(error);
					}//end of if
				}//Q
			}//J
		}//I

 		//do entropy calculation		
		for (int Q=bottom_idx;Q<=top_idx;Q+=2){
			if (Q!=min_idx){
				costs[Q].MSE=error_total[Q]/(vol*node.Wt()*node.Wt());
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
				//sort out correction factors
				costs[Q].ENTROPY*=m_encparams.EntropyFactors().Factor(band_num,m_fsort,m_csort);
				costs[Q].TOTAL=costs[Q].MSE+m_lambda*costs[Q].ENTROPY;
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
					quant_val*=m_qfinvlist[Q];
					quant_val>>=17;
					if (quant_val){
						count0[Q]+=quant_val;
						quant_val*=m_qflist[Q];						
						if (val>0){
							countPOS[Q]++;
						}
						else{
							countNEG[Q]++;
						}
					}
					error=abs_val-quant_val;
					if (quant_val!=0)					
						error-=m_offset[Q];
					error*=error;
					error_total[Q]+=float(error);
				}//Q
			}//J
		}//I

 		//do entropy calculation		
		for (int Q=bottom_idx;Q<=top_idx;Q++){
			costs[Q].MSE=error_total[Q]/(vol*node.Wt()*node.Wt());

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
			//sort out correction factors
			costs[Q].ENTROPY*=m_encparams.EntropyFactors().Factor(band_num,m_fsort,m_csort);
			costs[Q].TOTAL=costs[Q].MSE+m_lambda*costs[Q].ENTROPY;
		}//Q

 		//find the qf with the lowest cost
		for (int Q=bottom_idx;Q<=top_idx;Q++){
			if (costs[Q].TOTAL<costs[min_idx].TOTAL)
				min_idx=Q;
		}

		if (costs[min_idx].ENTROPY==0.0)//then can skip after all
			node.SetQf(0,-1);
		else
			node.SetQf(0,min_idx);

		if (band_num==bands.Length()){
			AddSubAverage(pic_data,node.Xl(),node.Yl(),ADD);
		}
		return int(costs[min_idx].ENTROPY*float(xl*yl));
	}

}

ValueType CompCompressor::PicAbsMax(const PicArray& pic_data) const{
	//finds the maximum absolute value of the picture array
	return PicAbsMax(pic_data,pic_data.lbound(0),pic_data.lbound(1),pic_data.length(0),pic_data.length(1));
}

ValueType CompCompressor::PicAbsMax(const PicArray& pic_data,int xp, int yp ,int xl ,int yl) const{

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

void CompCompressor::SetToVal(PicArray& pic_data,const Subband& node,ValueType val){
	for (int J=node.Yp();J<node.Yp()+node.Yl();++J){	
		for (int I=node.Xp();I<node.Xp()+node.Xl();++I){
			pic_data[J][I]=val;
		}
	}
}

void CompCompressor::AddSubAverage(PicArray& pic_data,int xl,int yl,AddOrSub dirn){

	ValueType last_val=2692;//corresponds to mid-grey in this DC band with these filters
							//NB this is hard-wired for a level 4 transform
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
