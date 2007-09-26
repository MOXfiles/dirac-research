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
* Contributor(s): Chris Bowley (Original Author),
*                 Tim Borer
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

#include <limits>
#include <util/instrumentation/process_sequence.h>
#include <libdirac_common/frame.h>
using namespace dirac;

ProcessSequence::ProcessSequence(OverlayParams & oparams,
                                 FileStreamInput & inputpic,
                                 FileStreamOutput & outputpic,
                                 std::ifstream & in, bool verbose,
                                 int buffer, SourceParams & srcparams) :
    m_oparams(oparams),
    m_inputpic(inputpic),
    m_outputpic(outputpic),
    m_verbose(verbose),
    m_data_in(in),
    m_data_array(buffer),
    m_srcparams(srcparams)
{

}

// checks motion data array for entry for current frame
//  - if entry exists, frame is processed
//  - if no entry exists, return false
bool ProcessSequence::DoFrame()
{
    int index = int(m_process_fnum % m_data_array.Length());

    if (m_data_array[index].frame_params.FrameNum() == m_process_fnum)
    {
        // read next frame from input sequence
        Frame * frame = new Frame(m_data_array[index].frame_params);
        m_inputpic.GetStream()->ReadNextPicture(*frame);

        Overlay overlay(m_oparams, *frame);

        if (m_data_array[index].frame_params.FSort().IsIntra())
            overlay.ProcessFrame();

        else
            overlay.ProcessFrame(*(m_data_array[index].me_data), m_data_array[index].block_params);

        // release me_data
        if (m_data_array[index].me_data != 0)
        {
            delete m_data_array[index].me_data;
            m_data_array[index].me_data = 0;
        }

        // set frame number to -1 to identify it as unallocated
        m_data_array[index].frame_params.SetFrameNum(-1);

        //clip the data to keep it within range
        frame->Clip();

        // write the frame to the output file
        m_outputpic.GetStream()->WriteNextFrame(*frame);

        // de-allocate memory for frame
        delete frame;

        return true;
    }

    return false;
}

// reads motion data file and adds entries into motion data array upto and including frame
// denoted by fnum
void ProcessSequence::AddFrameEntry()
{
    // look for frame sort
    m_data_in.ignore(10, '>');
    char mv_frame_sort[10];
    m_data_in >> mv_frame_sort;

    // position in array where frame data should be placed
    int new_index = m_data_fnum % m_data_array.Length();

    // reading information for an intra frame
    if (strcmp(mv_frame_sort, "intra") == 0)
    {
        if (m_verbose) std::cout << std::endl << "Reading intra frame " << m_data_fnum << " data";

        m_data_array[new_index].me_data = 0;
        m_data_array[new_index].frame_params = m_srcparams;
        m_data_array[new_index].frame_params.SetFrameNum(m_data_fnum);
        m_data_array[new_index].frame_params.SetFSort(FrameSort::IntraRefFrameSort());

        if (m_verbose)
        {
            std::cout << std::endl << "Writing to array position ";
            std::cout << m_data_fnum % m_data_array.Length();
        }
    }

    // reading information for a motion-compensated frame
    else
    {
        if (m_verbose)
        {
            std::cout << std::endl << "Reading motion-compensated frame ";
            std::cout << m_data_fnum << " data";
        }

        int mb_xnum = 0, mb_ynum = 0, mv_xnum = 0, mv_ynum = 0;
        int total_refs = 0;
        int ref = -1;

        // create frame motion data array entry
        m_data_array[new_index].frame_params = m_srcparams;

        // read reference frame information from top of file
        m_data_in >> total_refs;

        // clear reference vector
        m_data_array[new_index].frame_params.Refs().clear();

        for (int i=0; i<total_refs; ++i)
        {
            m_data_in >> ref;
            m_data_array[new_index].frame_params.Refs().push_back(ref);
        }

        // add NO_REF reference if there is no reference 2
        if (total_refs == 1)
            m_data_array[new_index].frame_params.Refs().push_back(NO_REF);

        // read luma motion block dimensions
        m_data_in >> m_data_array[new_index].block_params;

        // read array size information from top of file
        m_data_in >> mb_ynum; // macroblock array dimensions
        m_data_in >> mb_xnum;
        m_data_in >> mv_ynum; // motion vector array dimensions
        m_data_in >> mv_xnum;

        // create motion data object
        m_data_array[new_index].me_data = new MEData(mb_xnum, mb_ynum, mv_xnum, mv_ynum , total_refs );

        m_data_array[new_index].frame_params.SetFrameNum(m_data_fnum);

        if (m_data_array[new_index].frame_params.Refs().size() > 1)
            m_data_array[new_index].frame_params.SetFSort(FrameSort::InterNonRefFrameSort());
        else
            m_data_array[new_index].frame_params.SetFSort(FrameSort::InterRefFrameSort());

        // read motion vector data
        m_data_in >> *m_data_array[new_index].me_data; // overloaded operator>> defined in libdirac_common/motion.cpp

        if (m_verbose)
        {
            std::cout << std::endl << "Writing to array position ";
            std::cout << m_data_fnum % m_data_array.Length();
        }
    }
}

// manages processing of sequence, operation:
//  - check motion data array for frame entry
//  - if exists, process frame and remove entry
//  - if no entry exists, read motion data file and store frames
//    up to and including current frame for process,
//    retrieve frame motion data from array and process
void ProcessSequence::DoSequence(int start, int stop)
{
    // set all frame numbers to -1 to identify as unallocated
    for (int i=0; i<m_data_array.Length(); ++i)
        m_data_array[i].frame_params.SetFrameNum(-1);

    // read frames until the start frame is found
    // ** is there a better way?? **
    if (start > 0)
    {
        for (int fnum=0; fnum<start; ++fnum)
        {
            FrameParams fparams(m_inputpic.GetSourceParams());
            Frame * frame = new Frame(fparams);
            m_inputpic.GetStream()->ReadNextPicture(*frame);
            delete frame;
        }
    }

    if ( stop == -1)
        stop = INT_MAX;

    bool read_data_fnum;
    int data_next_fnum = -1;
    m_data_fnum = -1;

    // look for frame number
    m_data_in.ignore(100000, ':');
    m_data_in >> m_data_fnum;

    // frame by frame processing
    for (m_process_fnum = start; m_process_fnum <= stop; ++m_process_fnum)
    {
        if (m_verbose) std::cout << std::endl << std::endl << "Frame " << m_process_fnum;

        // location of frame data in array
        int index = int(m_process_fnum % m_data_array.Length());

        if (m_verbose)
        {
            std::cout << "\nArray entry " << index << " is ";
            if (m_data_array[index].frame_params.FrameNum() != -1)
                std::cout << "frame number " << m_data_array[index].frame_params.FrameNum();
            else
                std::cout << "not allocated";
        }

        // if the frame motion data has not already been read, add the motion data to the vector
        if (!DoFrame())
        {
            read_data_fnum = false;
            do
            {
                if (read_data_fnum)
                {
                    // look for frame number of next data
                    m_data_in.ignore(100000, ':');
                    m_data_in >> m_data_fnum;
                }
                AddFrameEntry();
                read_data_fnum = true;

            } while (m_data_fnum != m_process_fnum && !m_data_in.eof());

            // now check the next set of data is not for the same frame
            do
            {
                // look for frame number
                m_data_in.ignore(100000, ':');
                data_next_fnum = -1;
                m_data_in >> data_next_fnum;

                if (m_data_fnum == data_next_fnum && !m_data_in.eof())
                {
                    m_data_fnum = data_next_fnum;
                    if (m_verbose) std::cout << std::endl << "Updating frame data";
                    AddFrameEntry();
                }

            } while (m_data_fnum == data_next_fnum && !m_data_in.eof());


            // update data frame number
            m_data_fnum = data_next_fnum;

            // the frame data should be in the array (provided it is big enough!)
            // if the data is not available, advise and exit
            if (!DoFrame())
            {
                if (m_data_in.eof())
                    break;
                std::cerr << "Cannot find frame " << m_process_fnum << " motion data. ";
                std::cerr << "Check buffer size. Exiting." << std::endl;
                exit(EXIT_FAILURE);
            }
        }
        else
        {
            if (data_next_fnum == -1)
                break;
        }
    }

    // close motion data file
    m_data_in.close();
}

