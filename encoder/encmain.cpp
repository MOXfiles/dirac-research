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

#include <iostream>
#include <fstream>
#include <cmath>
#include <ctime>
#include <string>
#include <libdirac_encoder/seq_compress.h>
#include <libdirac_common/pic_io.h>
#include <libdirac_common/cmd_line.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;


static void display_help()
{
	cout << "\nDIRAC wavelet video coder.";
	cout << "\n";
	cout << "\nUsage: progname -<flag1> [<flag_val>] ... <input1> <input2> ...";
	cout << "\nIn case of multiple assignment to the same parameter, the last holds.";
	cout << "\n";
	cout << "\nName    Type   I/O Default Value Description";
	cout << "\n====    ====   === ============= ===========                                       ";
	cout << "\ninput   string  I  [ required ]  Input file name";
	cout << "\noutput  string  I  [ required ]  Output file name";
	cout << "\nstream  bool    I  true          Use streaming compression presets";
	cout << "\nHD720p  bool    I  false         Use HD-720p compression presets";
	cout << "\nHD1080  bool    I  false         Use HD-1080 compression presets";
	cout << "\nSD576   bool    I  false         Use SD-576 compression presets";
	cout << "\nL1_sep  ulong   I  0UL           Separation of L1 frames";
	cout << "\nnum_L1  ulong   I  0UL           Number of L1 frames";
	cout << "\nxblen   ulong   I  0UL           Overlapping block horizontal length";
	cout << "\nyblen   ulong   I  0UL           Overlapping block vertical length";
	cout << "\nxbsep   ulong   I  0UL           Overlapping block horizontal separation";
	cout << "\nybsep   ulong   I  0UL           Overlapping block vertical separation";
	cout << "\ncpd     ulong   I  0UL           Perceptual weighting - vertical cycles per degree";
	cout << "\nqf      float   I  0.0F          Overall quality factor (0.0 - 10.0)";
	cout << "\nverbose bool    I  false         Verbose mode";
	cout << endl;
}

int main (int argc, char* argv[]){
	/*********************************************************************************/
			/**********  command line parameter parsing*********/

		 /********** create params object to handle command line parameter parsing*********/
	//To do: put parsing in a different function/constructor.

    // create a list of boolean options
	set<string> bool_opts;
	bool_opts.insert("verbose");
	bool_opts.insert("stream");
	bool_opts.insert("HD720p");
	bool_opts.insert("HD1080");
	bool_opts.insert("SD576");

	CommandLine args(argc,argv,bool_opts);

	//the variables we'll read parameters into
	EncoderParams encparams;
	OLBParams bparams(12, 12, 8, 8);
	char input_name[84];//input name for pictures
	char output_name[84];//output name for pictures
	char bit_name[84];	//output name for the bitstream
	string input,output;

	//Set default values. To do: these should really be set in the constructor for the encoder parameters
	//These default values assume a streaming preset.
    encparams.SetQf(5.0);
	encparams.SetL1Sep(3);
	encparams.SetNumL1(11);
	encparams.SetUFactor(3.0f);
	encparams.SetVFactor(1.75f);
	encparams.SetCPD(20.0f);

	encparams.SetVerbose( false );

	if (argc<3)//need at least 3 arguments - the program name, an input and an output
	{
		display_help();
	}
	else//carry on!
	{
		// parse command-line arguments
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

		for (size_t i=0;i<input.length();i++)
			input_name[i]=input[i];

		input_name[input.length()] = '\0';

		for (size_t i=0;i<output.length();i++)
			output_name[i]=output[i];

		output_name[output.length()] = '\0';

		strncpy(bit_name,output_name,84);
		strcat(bit_name,".drc");

        // attempt to create a directory for MvData
        char dir_path[120];
        strcpy(dir_path, output.c_str());
        strcat(dir_path, "_mvdata");
        encparams.SetOutputPath(dir_path);
        std::cerr<<std::endl<<"Creating file "<<dir_path;
        std::ofstream out(dir_path, std::ios::out);
        out.close();

		//Now do the options
		//Start with quantisation factors
		for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
			opt != args.GetOptions().end(); ++opt){
			if (opt->m_name == "qf")
			{
				encparams.SetQf( atof(opt->m_value.c_str()) );
			}
		}//opt


		//Now do checking for presets
		for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
			opt != args.GetOptions().end(); ++opt)
		{
			if (opt->m_name == "stream")
			{
				encparams.SetL1Sep(3);
				encparams.SetNumL1(11);
				bparams.SetXblen( 12 );
				bparams.SetYblen( 12 );
				bparams.SetXbsep( 8 );
				bparams.SetXbsep( 8 );
				encparams.SetUFactor(3.0f);
				encparams.SetVFactor(1.75f);
				encparams.SetCPD(20.0f);

			}
			else if (opt->m_name == "HD720p")
			{
				encparams.SetL1Sep(6);
				encparams.SetNumL1(3);
				bparams.SetXblen( 16 );
				bparams.SetYblen( 16 );
				bparams.SetXbsep( 10 );
				bparams.SetXbsep( 12 );
				encparams.SetUFactor(3.0f);
				encparams.SetVFactor(1.75f);
				encparams.SetCPD(20.0f);
            }
			else if (opt->m_name == "HD1080")
			{
				encparams.SetL1Sep(3);
				encparams.SetNumL1(3);
				bparams.SetXblen( 20 );
				bparams.SetYblen( 20 );
				bparams.SetXbsep( 16 );
				bparams.SetXbsep( 16 );
				encparams.SetUFactor(3.0f);
				encparams.SetVFactor(1.75f);
				encparams.SetCPD(32.0f);
			}
			else if (opt->m_name == "SD576")
			{
				encparams.SetL1Sep(3);
				encparams.SetNumL1(3);
                bparams.SetXblen( 12 );
				bparams.SetYblen( 12 );
				bparams.SetXbsep( 8 );
				bparams.SetXbsep( 8 );
				encparams.SetUFactor(3.0f);
				encparams.SetVFactor(1.75f);
				encparams.SetCPD(32.0f);
			}
		}//opt

		//now go over again and override presets with other values
		for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
			opt != args.GetOptions().end(); ++opt)
		{
			if (opt->m_name == "L1_sep")
			{
				encparams.SetL1Sep( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "num_L1")
			{
				encparams.SetNumL1( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "xblen")
			{
				bparams.SetXblen( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "yblen")
			{
				bparams.SetYblen( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "xbsep")
			{
				bparams.SetXbsep( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "ybsep")
			{
				bparams.SetYbsep( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "cpd")
			{
				encparams.SetCPD( strtoul(opt->m_value.c_str(),NULL,10) );
			}
			else if (opt->m_name == "verbose")
			{
				encparams.SetVerbose( true );
			}
		}//opt

		//Now rationalise the GOP options
		//this stuff should really be done in a constructor!
		if (encparams.NumL1()<0){//don't have a proper GOP
			encparams.SetL1Sep( std::max(1 , encparams.L1Sep()) );
		}
		else if (encparams.NumL1() == 0){//have I-frame only coding
			encparams.SetL1Sep(0);
		}


  /********************************************************************/
	     //next do picture file stuff

   		/* ------ open input files & get params -------- */

		PicInput myinputpic(input_name);
		myinputpic.ReadPicHeader();

  		/* ------- open output files and write the header -------- */

		PicOutput myoutputpic(output_name,myinputpic.GetSeqParams());
		myoutputpic.WritePicHeader();

	/********************************************************************/


   		//set up all the block parameters so we have a self-consistent set

		encparams.SetBlockSizes( bparams , myinputpic.GetSeqParams().CFormat() );

   /********************************************************************/
      //open the bitstream file
		std::ofstream outfile(bit_name,std::ios::out | std::ios::binary);	//bitstream output

   /********************************************************************/
    	//do the work!!
		SequenceCompressor seq_compressor(&myinputpic,&outfile,encparams);
		seq_compressor.CompressNextFrame();
		for (int I=0 ; I<myinputpic.GetSeqParams().Zl() ; ++I)
		{
			if (!seq_compressor.Finished())
				myoutputpic.WriteNextFrame( seq_compressor.CompressNextFrame() );
		}//I

   /********************************************************************/
 	//close the bitstream file
		outfile.close();

		if ( encparams.Verbose() )
			std::cerr<<std::endl<<"Finished encoding\n";
		return EXIT_SUCCESS;


	}//?sufficient args?
}
