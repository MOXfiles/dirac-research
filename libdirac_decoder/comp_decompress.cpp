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
* Revision 1.4  2004-05-26 14:33:46  tjdwave
* Updated default DC prediction value to take into account the removal of
* scaling from the wavelet transform.
*
* Revision 1.3  2004/05/18 07:46:15  tjdwave
* Added support for I-frame only coding by setting num_L1 equal 0; num_L1 negative gives a single initial I-frame ('infinitely' many L1 frames). Revised quantiser selection to cope with rounding error noise.
*
* Revision 1.2  2004/05/12 08:35:34  tjdwave
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/

#include "libdirac_decoder/comp_decompress.h"
#include "libdirac_common/wavelet_utils.h"
#include "libdirac_common/band_codec.h"
#include "libdirac_common/golomb.h"
#include <vector>

#include <ctime>

using std::vector;

void CompDecompressor::Decompress(PicArray& pic_data){
	const FrameSort& fsort=fparams.fsort;
	int depth=4;
	BandCodec* bdecoder;
	vector<Context> ctx_list(24);
	Subband node;
	unsigned int max_bits;
	int qf_idx;

	WaveletTransformParams wparams(depth);
	WaveletTransform wtransform(wparams);
	SubbandList& bands=wtransform.BandList();
	bands.Init(depth,pic_data.length(0),pic_data.length(1));

	GenQuantList();

	for (int I=bands.Length();I>=1;--I){

		//read the header data first
		qf_idx=GolombDecode(*(decparams.BIT_IN));
		if (qf_idx!=-1){
			bands(I).SetQf(0,qflist[qf_idx]);
			max_bits=UnsignedGolombDecode(*(decparams.BIT_IN));
			(decparams.BIT_IN)->FlushInput();

			if (I>=bands.Length()){
				if (fsort==I_frame && I==bands.Length())
					bdecoder=new IntraDCBandCodec(decparams.BIT_IN,ctx_list,bands);
				else
					bdecoder=new LFBandCodec(decparams.BIT_IN,ctx_list,bands,I);
			}
			else
				bdecoder=new BandCodec(decparams.BIT_IN,ctx_list,bands,I);

			bdecoder->InitContexts();
			bdecoder->Decompress(pic_data,max_bits);
			delete bdecoder;
		}
		else{
			(decparams.BIT_IN)->FlushInput();
			if (I==bands.Length() && fsort==I_frame)
				SetToVal(pic_data,bands(I),2692);
			else
				SetToVal(pic_data,bands(I),0);
		}
	}
	wtransform.Transform(BACKWARD,pic_data);
}

void CompDecompressor::SetToVal(PicArray& pic_data,const Subband& node,ValueType val){
	for (int J=node.Yp();J<node.Yp()+node.Yl();++J){	
		for (int I=node.Xp();I<node.Xp()+node.Xl();++I){
			pic_data[J][I]=val;
		}
	}
}

void CompDecompressor::GenQuantList(){//generates the list of quantisers and inverse quantisers
	//there is some repetition in this list but at the moment this is easiest from the perspective of SelectQuant
	//Need to remove this repetition later TJD 29 March 04.

	qflist[0]=1;		
	qflist[1]=1;		
	qflist[2]=1;		
	qflist[3]=1;		
	qflist[4]=2;		
	qflist[5]=2;		
	qflist[6]=2;		
	qflist[7]=3;		
	qflist[8]=4;		
	qflist[9]=4;		
	qflist[10]=5;		
	qflist[11]=6;		
	qflist[12]=8;		
	qflist[13]=9;		
	qflist[14]=11;		
	qflist[15]=13;		
	qflist[16]=16;		
	qflist[17]=19;		
	qflist[18]=22;		
	qflist[19]=26;		
	qflist[20]=32;		
	qflist[21]=38;		
	qflist[22]=45;		
	qflist[23]=53;		
	qflist[24]=64;		
	qflist[25]=76;		
	qflist[26]=90;		
	qflist[27]=107;		
	qflist[28]=128;		
	qflist[29]=152;		
	qflist[30]=181;		
	qflist[31]=215;		
	qflist[32]=256;		
	qflist[33]=304;		
	qflist[34]=362;		
	qflist[35]=430;		
	qflist[36]=512;		
	qflist[37]=608;		
	qflist[38]=724;		
	qflist[39]=861;		
	qflist[40]=1024;	
	qflist[41]=1217;	
	qflist[42]=1448;	
	qflist[43]=1722;	
	qflist[44]=2048;	
	qflist[45]=2435;	
	qflist[46]=2896;	
	qflist[47]=3444;	
	qflist[48]=4096;	
	qflist[49]=4870;	
	qflist[50]=5792;	
	qflist[51]=6888;	
	qflist[52]=8192;	
	qflist[53]=9741;	
	qflist[54]=11585;	
	qflist[55]=13777;	
	qflist[56]=16384;	
	qflist[57]=19483;	
	qflist[58]=23170;	
	qflist[59]=27554;		
}
