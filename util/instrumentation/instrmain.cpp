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
* Contributor(s): Chris Bowley (Original Author)
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
#include <sstream>
#include <libdirac_common/common.h>
#include <libdirac_common/motion.h>
#include <libdirac_common/pic_io.h>
#include <libdirac_common/cmd_line.h>
#include <libdirac_instrument/overlay.h>

using namespace std;

struct me_data_entry
{
    MEData * me_data;
    OLBParams block_params;
    FrameParams frame_params;
};

static void DisplayHelp()
{
    cout << "\nInstrumentation display for DIRAC wavelet video coder";
    cout << "\n=====================================================";
    cout << "\n";
    cout << "\nUsage: progname -<flag1> [<flag_val>] ... <input1> <input2> ...";
    cout << "\nIn case of multiple assignment to the same parameter, the last holds.";
    cout << "\n";
    cout << "\nName                 Type   I/O Default Value Description";
    cout << "\n====                 ====   === ============= ===========";
    cout << "\ninput                string  I  [ required ]  Input file name";
    cout << "\noutput               string  I  [ required ]  Output file name";
    cout << "\n";
    cout << "\nmotion_colour        bool    I  true          Display motion vectors using colour wheel";    
    cout << "\nmotion_arrows        bool    I  false         Display motion vectors as arrows";
    cout << "\nmotion_colour_arrows bool    I  false         Display motion vectors as arrows with colour size";
    cout << "\nsplit_mode           bool    I  false         Display macroblock splitting mode";
    cout << "\nsad                  bool    I  false         Display block SAD values";
    cout << "\npred_mode            bool    I  false         Display block prediction mode";
    cout << "\n";
    cout << "\nno_bg                bool    I  false         Display over grey background";
    cout << "\nno_legend            bool    I  false         Do not display colour legend";
    cout << "\n";
    cout << "\nclip                 int     I  25 / 10000    Clip for max value motion vector / SAD overlays";
    cout << "\nref                  int     I  1             Reference frame";
    cout << "\nstart                int     I  0             Frame number at which process starts";
    cout << "\nend                  int     I  end           Frame number at which process stops";
    cout << "\nbuffer               int     I  50            Size of internal buffer for motion data";
    cout << "\n";
    cout << "\nverbose              bool    I  false         Display information during process";
}

// checks motion data array for entry for current frame
//  - if entry exists, frame is processed
//  - if no entry exists, return false
bool ProcessFrame(int fnum, PicInput & inputpic, PicOutput & outputpic, OneDArray<me_data_entry> & array,
                  OverlayParams & oparams)
{
    int index = int(fnum % array.Length());
    
    if (array[index].frame_params.FrameNum() == fnum)
    {
        // read next frame from input sequence
        Frame * frame = new Frame(array[index].frame_params);
        inputpic.ReadNextFrame(*frame);

        Overlay overlay(oparams, *frame);
        
        if (array[index].frame_params.FSort() == I_frame)
            overlay.ProcessFrame();

        else
            overlay.ProcessFrame(*(array[index].me_data), array[index].block_params);
        
        // release me_data
        if (array[index].me_data != 0)
        {
            delete array[index].me_data;
            array[index].me_data = 0;
        }

        // set frame number to -1 to identify it as unallocated
        array[index].frame_params.SetFrameNum(-1);

        //clip the data to keep it within range
        frame->Clip();

        // write the frame to the output file
        outputpic.WriteNextFrame(*frame);

        // de-allocate memory for frame
        delete frame;

        return true;
    }

    return false;
}

// reads motion data file and adds entries into motion data array upto and including frame
// denoted by fnum
void AddFrameEntry(ifstream & in, int fnum, int & mv_frame_num, const SeqParams seqparams,
                   OneDArray<me_data_entry> & me_data_array, bool verbose)
{
    // look for frame number
    in.ignore(100000, ':');
    in >> mv_frame_num;

    // look for frame sort
    in.ignore(10, '>');
    char mv_frame_sort[10];
    in >> mv_frame_sort;

    // position in array where frame data should be placed
    int new_index = mv_frame_num % me_data_array.Length();

    // reading information for an intra frame
    if (strcmp(mv_frame_sort, "intra") == 0)
    {
        if (verbose) cerr << endl << "Reading intra frame " << mv_frame_num << " data";

        me_data_array[new_index].me_data = 0;
        me_data_array[new_index].frame_params = seqparams;
        me_data_array[new_index].frame_params.SetFrameNum(mv_frame_num);
        me_data_array[new_index].frame_params.SetFSort(I_frame);
        me_data_array[new_index].frame_params.SetFrameNum(mv_frame_num);
        
        if (verbose) cerr << endl << "Writing to array position " << mv_frame_num % me_data_array.Length();
    }

    // reading information for a motion-compensated frame
    else
    {
        if (verbose) cerr << endl << "Reading motion-compensated frame " << mv_frame_num << " data";
        int mb_xnum = 0, mb_ynum = 0, mv_xnum = 0, mv_ynum = 0;
        int total_refs = 0;
        int ref = -1;

        // create frame motion data array entry
        me_data_array[new_index].frame_params = seqparams;

        // read reference frame information from top of file
        in >> total_refs;

        for (int i=0; i<total_refs; ++i)
        {
            in >> ref;
            me_data_array[new_index].frame_params.Refs().push_back(ref);
        }

        // read luma motion block dimensions
        in >> me_data_array[new_index].block_params;

        // read array size information from top of file
        in >> mb_ynum; // macroblock array dimensions
        in >> mb_xnum;
        in >> mv_ynum; // motion vector array dimensions
        in >> mv_xnum;
        
        // create motion data object
        // *** remove comments around last argument when encoder uses variable number of references ***
        me_data_array[new_index].me_data = new MEData(mb_xnum, mb_ynum, mv_xnum, mv_ynum /*, total_refs */);

        me_data_array[new_index].frame_params.SetFrameNum(mv_frame_num);

        if (me_data_array[new_index].frame_params.Refs().size() > 1)
            me_data_array[new_index].frame_params.SetFSort(L2_frame);
        else
            me_data_array[new_index].frame_params.SetFSort(L1_frame);

        // read motion vector data
        in >> *me_data_array[new_index].me_data; // overloaded operator>> defined in libdirac_common/motion.cpp

        if (verbose) cerr << endl << "Writing to array position " << mv_frame_num % me_data_array.Length();
    }
}

// manages processing of sequence, operation:
//  - check motion data array for frame entry
//  - if exists, process frame and remove entry
//  - if no entry exists, read motion data file and store frames
//    up to and including current frame for process,
//    retrieve frame motion data from array and process
void ProcessSequence(OverlayParams & oparams, string input, string output, bool verbose, int start, int stop,
                     int buffer)
{
    // Create objects for input and output picture sequences
    PicInput inputpic(input.c_str());
    inputpic.ReadPicHeader();

    PicOutput outputpic(output.c_str(), inputpic.GetSeqParams());
    outputpic.WritePicHeader();

    // read motion data from file
    if (verbose) cerr << endl << "Opening motion data file ";
    char mv_file[100];
    strcpy(mv_file, input.c_str());
    strcat(mv_file, "_mvdata");
    if (verbose) cerr << mv_file;
    ifstream in(mv_file, ios::in);

    if (!in)
    {
        cerr << endl << "Failed to open sequence motion data file. Exiting." << endl;
        exit(EXIT_FAILURE);
    }

    if (verbose) cerr << " ... ok" << endl << "Processing sequence...";

    // fixed size OneDArray to hold frame motion data
    // size default is 50, overridden from command-line option '-buffer'
    OneDArray<me_data_entry> me_data_array(buffer);

    // set all frame numbers to -1 to identify as unallocated
    for (int i=0; i<me_data_array.Length(); ++i)
        me_data_array[i].frame_params.SetFrameNum(-1);

    // read frames until the start frame is found
    // ** is there a better way?? **
    if (start > 0)
    {
        for (int fnum=0; fnum<start; ++fnum)
        {
            Frame * frame = new Frame(inputpic.GetSeqParams());
            inputpic.ReadNextFrame(*frame);
            delete frame;
        }
    }

    // check stop value is less than sequence length
    if (stop >= inputpic.GetSeqParams().Zl() || stop == -1)
        stop = inputpic.GetSeqParams().Zl() - 1;

    // frame by frame processing
    for (int fnum = start; fnum <= stop; ++fnum)
    {
        if (verbose) cerr << endl << endl << "Frame " << fnum;

        // location of frame data in array
        int index = int(fnum % me_data_array.Length());

        if (verbose)
        {
            cerr << "\nArray entry " << index << " is ";
            if (me_data_array[index].frame_params.FrameNum() != -1)
                cerr << "frame number " << me_data_array[index].frame_params.FrameNum();
            else
                cerr << "not allocated";
        }

        // if the frame motion data has not already been read, add the motion data to the vector
        if (!ProcessFrame(fnum, inputpic, outputpic, me_data_array, oparams))
        {
            int mv_frame_num = -1;
            do
            {
                AddFrameEntry(in, fnum, mv_frame_num, inputpic.GetSeqParams(), me_data_array, verbose);

            } while (mv_frame_num != fnum && !in.eof());

            // the frame data should be in the array (provided it is big enough!)
            // if the data is not available, advise and exit
            if (!ProcessFrame(fnum, inputpic, outputpic, me_data_array, oparams))
            {
                cerr << "Cannot find frame " << fnum << " motion data. Check buffer size. Exiting." << endl;
                exit(EXIT_FAILURE);
            }
        }
    }

    // close motion data file
    in.close();
}

int main (int argc, char* argv[])
{
    // read command line options
    string input,output;
    int ref = 1;              // use reference 1
    bool verbose = false;
    int start = 0, stop = -1;
    int buffer = 50;
    
    // set defaults
    OverlayParams oparams;
    oparams.SetOption(motion_colour);   // motion vector colour wheel
    oparams.SetReference(1);            // reference 1
    oparams.SetBackground(true);        // background on
    oparams.SetLegend(true);            // legend on
    oparams.SetMvClip(25);              // motion vector clip = 25
    oparams.SetSADClip(10000);          // SAD clip = 10000

    // create a list of boolean options
    set<string> bool_opts;
    bool_opts.insert("verbose");
    bool_opts.insert("no_bg");
    bool_opts.insert("no_legend");
    bool_opts.insert("motion_colour");
    bool_opts.insert("motion_arrows");
    bool_opts.insert("motion_colour_arrows");    
    bool_opts.insert("split_mode");
    bool_opts.insert("sad");
    bool_opts.insert("pred_mode");    

    // parse command line options
    CommandLine args(argc,argv,bool_opts);

    // need at least 3 arguments - the program name, an input and an output
    if (argc < 3) 
    {
        DisplayHelp();
        exit(1);
    }
    else
    {
        // do required inputs
        if (args.GetInputs().size() == 2)
        {
            input=args.GetInputs()[0];
            output=args.GetInputs()[1];
        }

        // check we have real inputs
        if ((input.length() == 0) || (output.length() == 0))
        {
            DisplayHelp();
            exit(1);
        }

        // now process presets
        for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
            opt != args.GetOptions().end(); ++opt)
        {
            if (opt->m_name == "motion_arrows")
                oparams.SetOption(motion_arrows);
                
            else if (opt->m_name == "motion_colour_arrows")
                oparams.SetOption(motion_colour_arrows);
                
            else if (opt->m_name == "motion_colour")
                oparams.SetOption(motion_colour);

            else if (opt->m_name == "motion_arrows")
                oparams.SetOption(motion_arrows);

            else if (opt->m_name == "split_mode")
                oparams.SetOption(split_mode);

            else if (opt->m_name == "sad")
                oparams.SetOption(SAD);

            else if (opt->m_name == "pred_mode")
                oparams.SetOption(pred_mode);

            if (opt->m_name == "no_bg")
                oparams.SetBackground(false);

            if (opt->m_name == "no_legend")
                oparams.SetLegend(false);

            if (opt->m_name == "verbose")
                verbose = true;
        }

        // parameters
        for (vector<CommandLine::option>::const_iterator opt = args.GetOptions().begin();
            opt != args.GetOptions().end(); ++opt)
        {
            if (opt->m_name == "ref")
            {
                ref = strtoul(opt->m_value.c_str(),NULL,10);

                if (ref==2)
                    oparams.SetReference(2);
                else
                    oparams.SetReference(1);
            } // m_name

            if (opt->m_name == "clip")
            {
                if (oparams.Option() == SAD)
                {
                    oparams.SetSADClip(strtoul(opt->m_value.c_str(),NULL,10));
                    // ensure value is +ve
                    if (oparams.SADClip() <= 0)
                        oparams.SetSADClip(10000);
                }
                else
                {
                    oparams.SetMvClip(strtoul(opt->m_value.c_str(),NULL,10));
                    // ensure value is +ve
                    if (oparams.MvClip() <= 0)
                        oparams.SetMvClip(25);
                }
            } // m_name

            if (opt->m_name == "start")
            {
                start = strtoul(opt->m_value.c_str(),NULL,10);
            } // m_name

            if (opt->m_name == "stop")
            {
                stop = strtoul(opt->m_value.c_str(),NULL,10);
            } // m_name

            if (opt->m_name == "buffer")
            {
                buffer = strtoul(opt->m_value.c_str(),NULL,10);
            } // m_name            
        } // opt
    } // args > 3

    // *** process the sequence ***
    ProcessSequence(oparams, input, output, verbose, start, stop, buffer);
    if (verbose) cerr << endl << "Done sequence." << endl;
	return 0;
}


