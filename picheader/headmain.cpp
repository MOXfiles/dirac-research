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

#include <iostream>
#include <fstream>
#include <cmath>
#include <string>
#include <xparam.h>
#include <xparam_extend.h>
#include "libdirac_common/common.h"

//This program writes a header file given command-line parameters, for use
//with raw planar YUV data for input to the Dirac encoder.

using namespace xParam;

void WritePicHeader(SeqParams& sparams,std::ofstream* op_head_ptr);

PARAM_BEGIN_REG
	PARAM_ENUM(ChromaFormat);
PARAM_ENUM_VAL(Yonly);
PARAM_ENUM_VAL(format422);
PARAM_ENUM_VAL(format444);
PARAM_ENUM_VAL(format420);
PARAM_ENUM_VAL(format411);
PARAM_ENUM_VAL(formatNK);
PARAM_END_REG


	int main( int argc, char *argv[] )
{
	 /********** create params object to handle command line parameter parsing*********/
	xParam::ParamSet ps;

 	//the variables we'll read parameters into
	char output_name[84];
	std::string output;
	SeqParams sparams;

 	//now set up the parameter set with these variables
	try
	{
		ps<< "make_header"
		<< iParamVar(output,				"output			!Output file name")
		<< iParamVar(sparams.cformat,		"cformat		!Chroma format",			Val(format420))
		<< iParamVar(sparams.xl,			"xl				!Width in pixels",			Val(352))
		<< iParamVar(sparams.yl,			"yl				!Height in pixels", 		Val(288))
		<< iParamVar(sparams.zl,			"zl				!Length", 					Val(37))
		<< iParamVar(sparams.framerate,		"framerate		!Framerate(Hz)",			Val(12))
		<< iParamVar(sparams.interlace,		"interlace		!Separation of L1 frames",	Val(false))
		<< iParamVar(sparams.topfieldfirst,	"topfieldfirst	!Number of L1 frames",		Val(true));

		ps.input(argc,argv);
	}
	catch(xParam::Error e)
	{
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Aborting." << std::endl;
		exit(EXIT_FAILURE);
	}

	for (size_t i=0;i<output.length();i++) output_name[i]=output[i];
	output_name[output.length()] = '\0';

	strcat(output_name,".hdr");

	std::ofstream* op_head_ptr=new std::ofstream(output_name,std::ios::out | std::ios::binary);	//header output
	WritePicHeader(sparams,op_head_ptr);
	op_head_ptr->close();

	return 0;
}

void WritePicHeader(SeqParams& sparams,std::ofstream* op_head_ptr){//write a human-readable picture header as separate file

	int head_data[7];

	//Write the chroma format
	head_data[0]=sparams.cformat;

	//Y component breadth and height
	head_data[1]=sparams.xl;
	head_data[2]=sparams.yl;

	//Number of frames
	head_data[3]=sparams.zl;

	//interlaced or not
	head_data[4]=sparams.interlace;

	//top-field first or not (only relevant if interlaced)
	head_data[5]=sparams.topfieldfirst;

	//frame-rate code (needed for display)
	head_data[6]=sparams.framerate;

	if (*op_head_ptr){
		op_head_ptr->write((char*) &head_data,sizeof head_data);
	}
	else
		std::cerr<<std::endl<<"Can't open picture header file for writing";

}
