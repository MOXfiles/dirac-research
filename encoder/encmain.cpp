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
* Revision 1.2  2004-04-12 01:57:46  chaoticcoyote
* Fixed problem Intel C++ had in finding xparam headers on Linux
* Solved Segmentation Fault bug in pic_io.cpp
*
* Revision 1.1.1.1  2004/03/11 17:45:43  timborer
* Initial import (well nearly!)
*
* Revision 0.1.0  2004/02/20 09:36:08  thomasd
* Dirac Open Source Video Codec. Originally devised by Thomas Davies,
* BBC Research and Development
*
*/
#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <string>
#include "libdirac_encoder/seq_compress.h"
#include "libdirac_common/pic_io.h"
#include "xparam.h"
#include "xparam_extend.h"

using xParam::iParamVar;
using xParam::Val;

int main (int argc, char* argv[]){

	 /********** create params object to handle command line parameter parsing*********/
	xParam::ParamSet ps;

 	//the variables we'll read parameters into
	char input_name[84];							// char arrays used for file names
	char output_name[84];
	char bit_name[84];								//output name for the bitstream

	bool stream;									// compression presets
	bool HD720p;									//
	bool HD1080;									//
	bool SD576;										//

	bool verbose;									// verbose mode

	size_t L1_sep;									// compression paramters
	size_t num_L1;
	size_t xblen;
	size_t yblen;
	size_t xbsep;
	size_t ybsep;
	size_t cpd;

	float qf;										//quality/quatisation factors. The higher the factor, the lower the quality
	float Iqf;										//and the lower the bitrate
	float L1qf;
	float L2qf;	

	std::string input;								// string variables used by xparams to hold parsed file names
	std::string output;

 	//now set up the parameter set with these variables
	try 
	{	
		ps<< "DIRAC wavelet video coder."
		<< iParamVar(input,		"input			!Input file name")
		<< iParamVar(output,	"output			!Output file name")
		<< iParamVar(stream,	"stream			!Use streaming compression presets", 					Val(false))
		<< iParamVar(HD720p,	"HD720p			!Use HD-720p compression presets", 						Val(false))
		<< iParamVar(HD1080,	"HD1080			!Use HD-1080 compression presets", 						Val(false))
		<< iParamVar(SD576,		"SD576			!Use SD-576 compression presets", 						Val(false))
		<< iParamVar(L1_sep,	"L1_sep			!Separation of L1 frames",								Val(0))
		<< iParamVar(num_L1,	"num_L1			!Number of L1 frames",									Val(0))
		<< iParamVar(xblen,		"xblen			!Overlapping block horizontal length",					Val(0))
		<< iParamVar(yblen,		"yblen			!Overlapping block vertical length",					Val(0))
		<< iParamVar(xbsep,		"xbsep			!Overlapping block horizontal separation",				Val(0))
		<< iParamVar(ybsep,		"ybsep			!Overlapping block vertical separation",				Val(0))
		<< iParamVar(cpd,		"cpd			!Perceptual weighting - vertical cycles per degree",	Val(0))
		<< iParamVar(qf,		"qf				!Overall quality factor",								Val(0.0))
		<< iParamVar(Iqf,		"Iqf			!I-frame quality factor",								Val(20.0))
		<< iParamVar(L1qf,		"L1qf			!L1-frame quality factor",								Val(22.0))
		<< iParamVar(L2qf,		"L2qf			!L2-frame quality factor",								Val(24.0))
		<< iParamVar(verbose,	"verbose		!Verbose mode",											Val(false));

		ps.input(argc,argv);
	}
	catch(xParam::Error e) 
	{
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Aborting." << std::endl;
		exit(EXIT_FAILURE); 
	}

	for (size_t i=0;i<input.length();i++) input_name[i]=input[i];
	input_name[input.length()] = '\0';
	for (size_t i=0;i<output.length();i++) output_name[i]=output[i];
	output_name[output.length()] = '\0';

	strncpy(bit_name,output_name,84);
	strcat(bit_name,".drc");

	EncoderParams encparams;
	OLBParams bparams;

 /********************************************************************/	
    //next do picture file stuff

  	/* ------ open input files & get params -------- */

	PicInput myinputpic(input_name);
	myinputpic.ReadPicHeader();
	encparams.sparams=myinputpic.GetSeqParams();

 	/* ------- open output files and write the header -------- */

	PicOutput myoutputpic(output_name,encparams.sparams);
	myoutputpic.WritePicHeader();

 /********************************************************************/
	 //option parsing

	try{
		if(ps["verbose"].was_assigned_to() && verbose)
			encparams.VERBOSE=true;
		else
			encparams.VERBOSE=false;


		if (ps["qf"].was_assigned_to()){
			Iqf=qf;
			L1qf=Iqf+2.0f;
			L2qf=Iqf+5.0f;
		}	
		//presets
		if (ps["HD720p"].was_assigned_to()){
			encparams.L1_SEP=6;
			encparams.NUM_L1=3;
			bparams.XBLEN=16;		
			bparams.YBLEN=16;		
			bparams.XBSEP=10;		
			bparams.YBSEP=12;
			encparams.UFACTOR=3.0f;	
			encparams.VFACTOR=1.75f;		
			encparams.CPD=20.0f;

			encparams.I_lambda=pow(10.0,((Iqf/13.34)+0.12));
			encparams.L1_lambda=pow(10.0,((L1qf/11.11)+0.14));
			encparams.L2_lambda=pow(10.0,((L2qf/11.11)+0.14));
		}
		else if (ps["HD1080"].was_assigned_to()){		
			encparams.L1_SEP=3;
			encparams.NUM_L1=3;
			bparams.XBLEN=20;		
			bparams.YBLEN=20;		
			bparams.XBSEP=16;		
			bparams.YBSEP=16;		
			encparams.UFACTOR=3.0f;		
			encparams.VFACTOR=1.75f;
			encparams.CPD=32.0f;

			//TBC - not yet tuned
			encparams.I_lambda=pow(10.0,((Iqf/8.9)-0.58));
			encparams.L1_lambda=pow(10.0,((L1qf/9.7)+0.05));
			encparams.L2_lambda=pow(10.0,((L2qf/9.7)+0.05));
		}
		else if (ps["SD576"].was_assigned_to()){
			encparams.L1_SEP=3;
			encparams.NUM_L1=3;
			bparams.XBLEN=12;		
			bparams.YBLEN=12;		
			bparams.XBSEP=8;		
			bparams.YBSEP=8;		
			encparams.UFACTOR=3.0f;		
			encparams.VFACTOR=1.75f;
			encparams.CPD=32.0f;

			encparams.I_lambda=pow(10.0,((Iqf/8.9)-0.58));		
			encparams.L1_lambda=pow(10.0,((L1qf/9.7)+0.05));
			encparams.L2_lambda=pow(10.0,((L2qf/9.7)+0.05));
		}
		else{//assume streaming presets
			encparams.L1_SEP=3;
			encparams.NUM_L1=11;
			bparams.XBLEN=12;		
			bparams.YBLEN=12;		
			bparams.XBSEP=8;		
			bparams.YBSEP=8;		
			encparams.UFACTOR=3.0f;		
			encparams.VFACTOR=1.75f;		
			encparams.CPD=20.0f;

			encparams.I_lambda=pow(10.0,(Iqf/12.0)-0.3);
			encparams.L1_lambda=pow(10.0,((L1qf/9.0)-0.81));
			encparams.L2_lambda=pow(10.0,((L2qf/9.0)-0.81));
		}
 	//next mod in the presence of additional flags
		if (ps["num_L1"].was_assigned_to())
			encparams.NUM_L1 = num_L1;
		if (ps["L1_sep"].was_assigned_to())
			encparams.L1_SEP = L1_sep;
		if (ps["xbsep"].was_assigned_to())
			bparams.XBSEP = xbsep;
		if (ps["ybsep"].was_assigned_to())
			bparams.YBSEP = ybsep;
		if (ps["xblen"].was_assigned_to())
			bparams.XBLEN = xblen;
		if (ps["yblen"].was_assigned_to())
			bparams.YBLEN = yblen;
		if (ps["cpd"].was_assigned_to())
			encparams.CPD = cpd;	


	}
	catch(xParam::Error e) {
		std::cerr << "Parameter error: " << e.what() << std::endl;
		std::cerr << "Aborting." << std::endl;
		exit(EXIT_FAILURE); 
	}
	if (encparams.NUM_L1>0 && encparams.L1_SEP>0)
		encparams.GOP_LEN=(encparams.NUM_L1+1)*encparams.L1_SEP;
	else{
		encparams.NUM_L1=0;
		encparams.L1_SEP=0;
		encparams.GOP_LEN=1;
	}

 /********************************************************************/	

     //set up the remaining codec parameters

	encparams.SetBlockSizes(bparams);			//set up the block parameters

   	//Do the motion estimation Lagrangian parameters
	//factor1 normalises the Lagrangian ME factors to take into account different overlaps
	float factor1=float(bparams.XBLEN*bparams.YBLEN)/
		float(bparams.XBSEP*bparams.YBSEP);
   	//factor2 normalises the Lagrangian ME factors to take into account the number of
   	//blocks in the picture. The more blocks there are, the more the MV field must be
   	//smoothed and hence the higher the ME lambda must be
	float factor2=sqrt(float(encparams.X_NUMBLOCKS*encparams.Y_NUMBLOCKS));
   	//factor3 is an heuristic factor taking into account the different CPD values and picture sizes, since residues
   	//after motion compensation will have a different impact depending upon the perceptual weighting
   	//in the subsequent wavelet transform. This has to be tuned by hand. Probably varies with bit-rate too.
	float factor3;
	if (ps["HD720p"].was_assigned_to()) factor3=2000;
	else if (ps["HD1080"].was_assigned_to()) factor3=100;//TBC - not yet tuned
	else if (ps["SD576"].was_assigned_to()) factor3=100;
	else factor3=250.0;//assumed streaming presets
	float ratio=factor1*factor2/factor3;
	encparams.L1_ME_lambda=encparams.L1_lambda*ratio;
	encparams.L2_ME_lambda=encparams.L2_lambda*ratio;
	encparams.L1I_ME_lambda=encparams.L1I_lambda*ratio;
	encparams.EntCorrect=new EntropyCorrector(4);	

  /********************************************************************/	
     //open the bitstream file	
	std::ofstream outfile(bit_name,std::ios::out | std::ios::binary);	//bitstream output
	encparams.BIT_OUT=new BitOutputManager(&outfile);	

  /********************************************************************/	
   	//do the work!!

	SequenceCompressor seq_compressor(&myinputpic,encparams);
	seq_compressor.CompressNextFrame();
	for (int I=0;I<encparams.sparams.zl;++I){
		myoutputpic.WriteNextFrame(seq_compressor.CompressNextFrame());	
	}//I

  /********************************************************************/		

	outfile.close(); 	//close file
	int total_bits=encparams.BIT_OUT->GetTotalBytes()*8;
	if (encparams.VERBOSE){
		std::cerr<<std::endl<<std::endl<<"Finished encoding.";		
		std::cerr<<"Total bits for sequence="<<total_bits;
		std::cerr<<", of which "<<encparams.BIT_OUT->GetTotalHeadBytes()*8<<" were header.";
		std::cerr<<std::endl<<"Resulting bit-rate at "<<encparams.sparams.framerate<<"Hz is ";
		std::cerr<<total_bits*encparams.sparams.framerate/encparams.sparams.zl<<" bits/sec.";
	}	

	delete encparams.BIT_OUT;
	delete encparams.EntCorrect;

}
