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
#include <cmath>
#include <string>
#include "libdirac_common/cmd_line.h"
#include "libdirac_common/common.h"
#include "libdirac_common/pic_io.h"
using namespace dirac;

//This program writes a header file given command-line parameters, for use
//with raw planar YUV data for input to the Dirac encoder.

using namespace std;

void WritePicHeader(SeqParams& sparams,std::ofstream* op_head_ptr);

static void display_help()
{
    cout << "\nDIRAC wavelet video decoder.";
    cout << "\n";
    cout << "\nUsage: progname -<flag1> [<flag_val>] ... <input1> <intput2> ...";
    cout << "\nIn case of multiple assignment to the same parameter, the last holds.";
    cout << "\n";
    cout << "\nName          Type   I/O Default Value Description";
    cout << "\n====          ====   === ============= ===========                                       ";
    cout << "\noutput        string  I  [ required ]  Output file name";
    cout << "\ncformat       string  I  format420     Chroma format";
    cout << "\nxl            ulong   I  352           Width in pixels";
    cout << "\nyl            ulong   I  288           Height in pixels";
    cout << "\nframerate     ulong   I  12            Frame rate in Hz";
    cout << "\ninterlace     bool    I  false         Interlace";
    cout << "\ntopfieldfirst bool    I  true          Top Field First (set if interlaced)";
    cout << endl;
}

int main( int argc, char *argv[] )
{
     /********** create params object to handle command line parameter parsing*********/
    set<string> bool_opts;
    bool_opts.insert("interlace");
    bool_opts.insert("topfieldfirst");

    CommandLine args(argc,argv,bool_opts);

     //the variables we'll read parameters into
    std::string output;
    SeqParams sparams;

     //now set up the parameter set with these variables

    //need at least 2 arguments - the program name, and an output
    if (argc < 2)
    {
        display_help();
        exit(0);
    }

    //start with the output file
    if (args.GetInputs().size()==1){
        output=args.GetInputs()[0];
    }

    //check we have real inputs
    if (output.length() ==0)
    {
        display_help();
        exit(1);
    }
    //now do the options

    //set defaults. To do: set up in constructor
    sparams.SetCFormat(format420);
    sparams.SetXl(352);
    sparams.SetYl(288);
    sparams.SetInterlace(false);
    sparams.SetTopFieldFirst(true);
    sparams.SetFrameRate(13);

    for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
        opt != args.GetOptions().end();
        ++opt)
    {
        if (opt->m_name == "cformat")
        {
            if (opt->m_value=="format420")
                sparams.SetCFormat( format420 );
            else if (opt->m_value=="format422")
                sparams.SetCFormat( format422 );
            else if (opt->m_value=="format411")
                sparams.SetCFormat( format411 );
            else if (opt->m_value=="format444")
                sparams.SetCFormat( format444 );
            else if (opt->m_value=="Yonly")
                sparams.SetCFormat( Yonly );
        }
        else if (opt->m_name == "xl")
            sparams.SetXl( strtoul(opt->m_value.c_str(),NULL,10) );
        else if (opt->m_name == "yl")
            sparams.SetYl( strtoul(opt->m_value.c_str(),NULL,10) );    
        else if (opt->m_name == "interlace" && opt->m_value=="true")
            sparams.SetInterlace( true );    
        else if (opt->m_name == "topfieldfirst" && opt->m_value=="false")
            sparams.SetTopFieldFirst( false );
        else if (opt->m_name == "framerate")
            sparams.SetFrameRate( strtoul(opt->m_value.c_str(),NULL,10) );

    }//opt

    // Open just the header file for output
    FileStreamOutput header(output.c_str(), sparams, true);
    header.WritePicHeader();

    return 0;
}
