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

#include "common.h"
#include <algorithm>

float EntropyCorrector::Factor(int bandnum, FrameSort fsort,CompSort c){
	if (c==U)
		return Ufctrs[fsort][bandnum-1];
	else if (c==V)
		return Vfctrs[fsort][bandnum-1];
	else
		return Yfctrs[fsort][bandnum-1];
}

void EntropyCorrector::Init(){//depth is the depth of the wavelet transform

	//do I-frames
	for (int I=0;I<Yfctrs.length(0);++I){
		if (I==Yfctrs.last(0)){		
			Yfctrs[I_frame][I]=1.0;
			Ufctrs[I_frame][I]=1.0;
			Vfctrs[I_frame][I]=1.0;
			Yfctrs[L1_frame][I]=0.85;
			Ufctrs[L1_frame][I]=0.85;
			Vfctrs[L1_frame][I]=0.85;
			Yfctrs[L2_frame][I]=0.85;
			Ufctrs[L2_frame][I]=0.85;
			Vfctrs[L2_frame][I]=0.85;
		}
		else if (I>=Yfctrs.last(0)-3){
			Yfctrs[I_frame][I]=0.85;
			Ufctrs[I_frame][I]=0.85;
			Vfctrs[I_frame][I]=0.85;
			Yfctrs[L1_frame][I]=0.75;
			Ufctrs[L1_frame][I]=0.75;
			Vfctrs[L1_frame][I]=0.75;
			Yfctrs[L2_frame][I]=0.75;
			Ufctrs[L2_frame][I]=0.75;
			Vfctrs[L2_frame][I]=0.75;			
		}
		else{
			Yfctrs[I_frame][I]=0.75;
			Ufctrs[I_frame][I]=0.75;
			Vfctrs[I_frame][I]=0.75;
			Yfctrs[L1_frame][I]=0.75;
			Ufctrs[L1_frame][I]=0.75;
			Vfctrs[L1_frame][I]=0.75;
			Yfctrs[L2_frame][I]=0.75;
			Ufctrs[L2_frame][I]=0.75;
			Vfctrs[L2_frame][I]=0.75;			
		}
	}
}

void EntropyCorrector::Update(int bandnum, FrameSort fsort, CompSort c,int est_bits,int actual_bits){
	//updates the factors - note that the estimated bits are assumed to already include the correction factor
	float multiplier;
	if (actual_bits!=0 && est_bits!=0)
		multiplier=float(actual_bits)/float(est_bits);
	else
		multiplier=1.0;
	if (c==U)
		Ufctrs[fsort][bandnum-1]*=multiplier;
	else if (c==V)
		Vfctrs[fsort][bandnum-1]*=multiplier;
	else
		Yfctrs[fsort][bandnum-1]*=multiplier;
}

void CodecParams::SetBlockSizes(OLBParams& olbparams){
	//given the raw overlapped block parameters, set the modified internal parameters to
	//take account of the chroma sampling format and overlapping requirements, as well
	//as the equivalent parameters for sub-MBs and MBs

	lbparams[2]=olbparams;
	lbparams[2].XBSEP=std::max(lbparams[2].XBSEP,4);
	lbparams[2].XBLEN=std::max(lbparams[2].XBSEP+2,lbparams[2].XBLEN);
	lbparams[2].YBSEP=std::max(lbparams[2].YBSEP,4);
	lbparams[2].YBLEN=std::max(lbparams[2].YBSEP+2,lbparams[2].YBLEN);	
	lbparams[2].XOFFSET=(lbparams[2].XBLEN-lbparams[2].XBSEP)/2;	
	lbparams[2].YOFFSET=(lbparams[2].YBLEN-lbparams[2].YBSEP)/2;	
	if ((lbparams[2].XBLEN-lbparams[2].XBSEP)%2!=0)
		lbparams[2].XBLEN++;
	if ((lbparams[2].YBLEN-lbparams[2].YBSEP)%2!=0)
		lbparams[2].YBLEN++;

	X_NUMBLOCKS=sparams.xl/lbparams[2].XBSEP;
	if (sparams.xl%X_NUMBLOCKS!=0)
		X_NUMBLOCKS++;
	Y_NUMBLOCKS=sparams.yl/lbparams[2].YBSEP;
	if (sparams.yl%Y_NUMBLOCKS!=0)
		Y_NUMBLOCKS++;
	X_NUM_MB=X_NUMBLOCKS/4;
	if (X_NUMBLOCKS%4!=0)
		X_NUM_MB++;
	Y_NUM_MB=Y_NUMBLOCKS/4;
	if (Y_NUMBLOCKS%4!=0)
		Y_NUM_MB++;

	//the chroma block params
	if (sparams.cformat==format420){
		cbparams[2].XBSEP=lbparams[2].XBSEP/2;
		cbparams[2].YBSEP=lbparams[2].YBSEP/2;	
		cbparams[2].XBLEN=std::max(lbparams[2].XBLEN/2,cbparams[2].XBSEP+2);
		cbparams[2].YBLEN=std::max(lbparams[2].YBLEN/2,cbparams[2].YBSEP+2);
	}
	else if (sparams.cformat==format422){
		cbparams[2].XBSEP=lbparams[2].XBSEP/2;
		cbparams[2].YBSEP=lbparams[2].YBSEP;	
		cbparams[2].XBLEN=std::max(lbparams[2].XBLEN/2,cbparams[2].XBSEP+2);
		cbparams[2].YBLEN=std::max(lbparams[2].YBLEN,cbparams[2].YBSEP+2);
	}
	else if (sparams.cformat==format411){
		cbparams[2].XBSEP=lbparams[2].XBSEP/4;
		cbparams[2].YBSEP=lbparams[2].YBSEP;	
		cbparams[2].XBLEN=std::max(lbparams[2].XBLEN/4,cbparams[2].XBSEP+2);
		cbparams[2].YBLEN=std::max(lbparams[2].YBLEN,cbparams[2].YBSEP+2);
	}
	else{
		cbparams[2].XBSEP=lbparams[2].XBSEP;
		cbparams[2].YBSEP=lbparams[2].YBSEP;	
		cbparams[2].XBLEN=std::max(lbparams[2].XBLEN,cbparams[2].XBSEP+2);
		cbparams[2].YBLEN=std::max(lbparams[2].YBLEN,cbparams[2].YBSEP+2);
	}

	if ((cbparams[2].XBLEN-cbparams[2].XBSEP)%2!=0)
		cbparams[2].XBLEN++;
	if ((cbparams[2].YBLEN-cbparams[2].YBSEP)%2!=0)
		cbparams[2].YBLEN++;

	cbparams[2].XOFFSET=(cbparams[2].XBLEN-cbparams[2].XBSEP)/2;	
	cbparams[2].YOFFSET=(cbparams[2].YBLEN-cbparams[2].YBSEP)/2;	

	lbparams[1].XBSEP=lbparams[2].XBSEP<<1;
	lbparams[1].XBLEN=lbparams[2].XBLEN+lbparams[2].XBSEP;
	lbparams[1].YBSEP=lbparams[2].YBSEP<<1;
	lbparams[1].YBLEN=lbparams[2].YBLEN+lbparams[2].XBSEP;	
	lbparams[1].XOFFSET=lbparams[2].XOFFSET;
	lbparams[1].YOFFSET=lbparams[2].YOFFSET;
	lbparams[0].XBSEP=lbparams[1].XBSEP<<1;
	lbparams[0].XBLEN=lbparams[1].XBLEN+lbparams[1].XBSEP;
	lbparams[0].YBSEP=lbparams[1].YBSEP<<1;
	lbparams[0].YBLEN=lbparams[1].YBLEN+lbparams[1].XBSEP;		
	lbparams[0].XOFFSET=lbparams[1].XOFFSET;
	lbparams[0].YOFFSET=lbparams[1].YOFFSET;

	cbparams[1].XBSEP=cbparams[2].XBSEP<<1;
	cbparams[1].XBLEN=cbparams[2].XBLEN+cbparams[2].XBSEP;
	cbparams[1].YBSEP=cbparams[2].YBSEP<<1;
	cbparams[1].YBLEN=cbparams[2].YBLEN+cbparams[2].XBSEP;	
	cbparams[1].XOFFSET=cbparams[2].XOFFSET;
	cbparams[1].YOFFSET=cbparams[2].YOFFSET;
	cbparams[0].XBSEP=cbparams[1].XBSEP<<1;
	cbparams[0].XBLEN=cbparams[1].XBLEN+cbparams[1].XBSEP;
	cbparams[0].YBSEP=cbparams[1].YBSEP<<1;
	cbparams[0].YBLEN=cbparams[1].YBLEN+cbparams[1].XBSEP;		
	cbparams[0].XOFFSET=cbparams[1].XOFFSET;
	cbparams[0].YOFFSET=cbparams[1].YOFFSET;

}

std::vector<Context>& ContextInitialiser::GetInitCtx(FrameSort& fsort,CompSort& csort,int band_num){
	int idx=int(fsort)*39+int(csort)*13+band_num;
	return *(*init_list)[idx];
}
void ContextInitialiser::SetInitCtx(FrameSort& fsort,CompSort& csort,int band_num,std::vector<Context>& inits){
	int idx=int(fsort)*39+int(csort)*13+band_num;
	*(*init_list)[idx]=inits;
}
