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
	cout << "\nUsage: progname -<flag1> [<flag_val>] ... <input1> <intput2> ...";
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
    // create a list of boolean options
	set<string> bool_opts;
	bool_opts.insert("verbose");

	CommandLine args(argc,argv,bool_opts);

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
		if (args.GetInputs().size()==2){
			input=args.GetInputs()[0];
			output=args.GetInputs()[1];
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
		for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
			opt != args.GetOptions().end(); ++opt)
		{
			if (opt->m_name == "verbose")
			{
				verbose=true;
			}

		}//opt

	 /******************************************************************/
	 	//read the stream data in and get the sequence data out

		std::ifstream infile(bit_name,std::ios::in | std::ios::binary);
		if (! infile)
		{
			std::cerr << "Can't open " << bit_name << std::endl;
			exit(1);
		}
		SequenceDecompressor mydecompress( &infile , verbose );
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
		for (int I=0;I<sparams.Zl();++I)
			myoutputpic.WriteNextFrame(mydecompress.DecompressNextFrame());
		stop_t=clock();

		double diff=double(stop_t-start_t);
		infile.close();

		if (verbose){
			std::cerr<<"Time per frame: "<<diff/double(CLOCKS_PER_SEC*sparams.Zl());
			std::cerr<<std::endl<<"Finished decoding\n";
		}

		return EXIT_SUCCESS;
	}//?sufficient arguments
}
