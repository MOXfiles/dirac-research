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
* Revision 1.4  2004-05-12 16:04:20  tjdwave
*
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.3  2004/05/11 14:17:58  tjdwave
* Removed dependency on XParam CLI library for both encoder and decoder.
*
* Revision 1.2  2004/04/12 01:57:46  chaoticcoyote
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
#include "libdirac_common/common.h"
#include "libdirac_decoder/seq_decompress.h"
#include "libdirac_common/pic_io.h"
#include "libdirac_common/cmd_line.h"
#include <string>
#include <ctime>

using namespace std;

static void display_help()
{
	cout << "\nDIRAC wavelet video decoder.";
	cout << "\n";
	cout << "\nUsage: progname -<flag1> <flag_val> ... <input1> <intput2> ...";
	cout << "\nIn case of multiple assignment to the same parameter, the last holds.";
	cout << "\n";
	cout << "\nName    Type   I/O Default Value Description";
	cout << "\n====    ====   === ============= ===========                                       ";
	cout << "\ninput   string  I  [ required ]  Input file name";
	cout << "\noutput  string  I  [ required ]  Output file name";
	cout << "\nverbose bool    I  false         Verbose mode";
	cout << endl;
}

int main(int argc, char* argv[]) {

	/******************************************************************/

	 /********** create params object to handle command line parameter parsing*********/
	command_line args(argc,argv);
	char input_name[84];							// char arrays used for file names
	char output_name[84];
	char bit_name[84];								//output name for the bitstream
	string input,output;
	bool verbose=false;

	if (argc<3)//need at least 3 arguments - the program name, an input and an output
	{
		display_help();
	}
	else//carry on!
	{
		//now set up the parameter set with these variables
		//Do required inputs
		if (args.get_inputs().size()==2){
			input=args.get_inputs()[0];
			output=args.get_inputs()[1];
		}
		//check we have real inputs
		if ((input.length() == 0) || (output.length() ==0))
		{
			display_help();
			exit(1);
		}

		for (size_t i=0;i<input.length();i++) input_name[i]=input[i];
		input_name[input.length()] = '\0';
		for (size_t i=0;i<output.length();i++) output_name[i]=output[i];
		output_name[output.length()] = '\0';

		strncpy(bit_name,input_name,84);
		strcat(bit_name,".drc");

		//next check for options
		for (vector<command_line::option>::const_iterator opt = args.get_options().begin();
			opt != args.get_options().end(); ++opt)
		{
			if (opt->m_name == "verbose" && opt->m_value=="true")
			{
				verbose=true;
			}

		}//opt

	 /******************************************************************/
	 	//read the stream data in and get the sequence data out

		std::ifstream infile(bit_name,std::ios::in | std::ios::binary);
		SequenceDecompressor mydecompress(&infile,verbose);
		SeqParams& sparams=mydecompress.GetSeqParams();

	 /******************************************************************/
	 	//set up the ouput pictures

		PicOutput myoutputpic(output_name,sparams);
		myoutputpic.WritePicHeader();

 /******************************************************************/
	 	//do the decoding loop
		clock_t start_t, stop_t;
		start_t=clock();
		mydecompress.DecompressNextFrame();
		for (int I=0;I<sparams.zl;++I)
			myoutputpic.WriteNextFrame(mydecompress.DecompressNextFrame());
		stop_t=clock();

		double diff=double(stop_t-start_t);
		infile.close();

		if (verbose){
			std::cerr<<"Time per frame: "<<diff/double(CLOCKS_PER_SEC*sparams.zl);
			std::cerr<<std::endl<<"Finished decoding";
		}

		return EXIT_SUCCESS;
	}//?sufficient arguments
}

