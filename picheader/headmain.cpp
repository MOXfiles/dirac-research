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
* Revision 1.4  2004-05-14 17:25:43  stuart_hc
* Replaced binary header files with ASCII text format to achieve cross-platform interoperability.
* Rearranged PicOutput constructor to permit code reuse from picheader/headmain.cpp
*
* Revision 1.3  2004/05/12 16:05:51  tjdwave
*
* Done general code tidy, implementing copy constructors, assignment= and const
* correctness for most classes. Replaced Gop class by FrameBuffer class throughout.
* Added support for frame padding so that arbitrary block sizes and frame
* dimensions can be supported.
*
* Revision 1.2  2004/05/11 14:17:59  tjdwave
* Removed dependency on XParam CLI library for both encoder and decoder.
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
#include <string>
#include "libdirac_common/cmd_line.h"
#include "libdirac_common/common.h"
#include "libdirac_common/pic_io.h"

//This program writes a header file given command-line parameters, for use
//with raw planar YUV data for input to the Dirac encoder.

using namespace std;

void WritePicHeader(SeqParams& sparams,std::ofstream* op_head_ptr);

static void display_help()
{
	cout << "\nDIRAC wavelet video decoder.";
	cout << "\n";
	cout << "\nUsage: progname -<flag1> <flag_val> ... <input1> <intput2> ...";
	cout << "\nIn case of multiple assignment to the same parameter, the last holds.";
	cout << "\n";
	cout << "\nName          Type   I/O Default Value Description";
	cout << "\n====          ====   === ============= ===========                                       ";
	cout << "\noutput        string  I  [ required ]  Output file name";
	cout << "\ncformat       string  I  format420     Chroma format";
	cout << "\nxl            ulong   I  352           Width in pixels";
	cout << "\nyl            ulong   I  288           Height in pixels";
	cout << "\nzl            ulong   I  37            Length in frames";
	cout << "\nframerate     ulong   I  12            Frame rate in Hz";
	cout << "\ninterlace     bool    I  false         Interlace";
	cout << "\ntopfieldfirst bool    I  true          Top Field First (set if interlaced)";
	cout << endl;
}

int main( int argc, char *argv[] )
{
	 /********** create params object to handle command line parameter parsing*********/
	command_line args(argc,argv);

 	//the variables we'll read parameters into
	char output_name[84];
	std::string output;
	SeqParams sparams;

 	//now set up the parameter set with these variables

	//need at least 2 arguments - the program name, and an output
	if (argc < 2)
	{
		display_help();
		exit(1);
	}

	//start with the output file
	if (args.get_inputs().size()==1){
		output=args.get_inputs()[0];
	}

	//check we have real inputs
	if (output.length() ==0)
	{
		display_help();
		exit(1);
	}
	for (size_t i=0;i<output.length();i++)
		output_name[i]=output[i];
	output_name[output.length()] = '\0';

	//now do the options

	//set defaults. To do: set up in constructor
	sparams.cformat=format420;
	sparams.xl=352;
	sparams.yl=288;
	sparams.zl=37;
	sparams.interlace=false;
	sparams.topfieldfirst=true;
	sparams.framerate=13;

	for (vector<command_line::option>::const_iterator opt = args.get_options().begin();
		opt != args.get_options().end();
		++opt)
	{
		if (opt->m_name == "cformat")
		{
			if (opt->m_value=="format420"){
				sparams.cformat=format420;
			}
			else if (opt->m_value=="format422"){
				sparams.cformat=format422;
			}
			else if (opt->m_value=="format411"){
				sparams.cformat=format411;
			}
			else if (opt->m_value=="format444"){
				sparams.cformat=format444;
			}
			else if (opt->m_value=="Yonly"){
				sparams.cformat=Yonly;
			}
		}
		else if (opt->m_name == "xl"){
			sparams.xl=strtoul(opt->m_value.c_str(),NULL,10);
		}
		else if (opt->m_name == "yl"){
			sparams.yl=strtoul(opt->m_value.c_str(),NULL,10);
		}
		else if (opt->m_name == "zl"){
			sparams.zl=strtoul(opt->m_value.c_str(),NULL,10);
		}
		else if (opt->m_name == "interlace" && opt->m_value=="true"){
			sparams.interlace=true;
		}
		else if (opt->m_name == "topfieldfirst" && opt->m_value=="false"){
			sparams.topfieldfirst=false;
		}
		else if (opt->m_name == "framerate"){
			sparams.framerate=strtoul(opt->m_value.c_str(),NULL,10);
		}
	}//opt

	// Open just the header file for output
	PicOutput header(output_name, sparams, true);
	header.WritePicHeader();

	return 0;
}
