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
 * Contributor(s): Thomas Davies (Original Author), 
 *                 Scott R Ladd,
 *                 Anuradha Suraparaju 
 *                 Andrew Kennedy
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
#include <limits>
#include <fstream>
#include <set>
#include <cmath>
#include <ctime>
#include <cassert>
#include <string>
#include <libdirac_encoder/dirac_encoder.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

using namespace std;

const int VIDEO_BUFFER_SIZE = 1024*1024;
unsigned char video_buf[VIDEO_BUFFER_SIZE];

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
    cout << "\nCIF     bool    I  true          Use CIF compression presets";
    cout << "\nHD720   bool    I  false         Use HD-720 compression presets";
    cout << "\nHD1080  bool    I  false         Use HD-1080 compression presets";
    cout << "\nSD576   bool    I  false         Use SD-576 compression presets";
    cout << "\nwidth   ulong   I  Preset        Width of frame";
    cout << "\nheight  ulong   I  Preset        Length of frame";
    cout << "\ncformat ulong   I  Yonly         Chroma format 0=Yonly 1=422 2=444 3=420 4=411";
    cout << "\nfr      ulong   I  Preset        Frame rate(s) (e.n or e/n format)";
    cout << "\nstart   ulong   I  0UL           Frame number to start encoding from";
    cout << "\nstop    ulong   I  EOF           Frame number after which encoding finishes";
    cout << "\nL1_sep  ulong   I  0UL           Separation of L1 frames";
    cout << "\nnum_L1  ulong   I  0UL           Number of L1 frames";
    cout << "\nxblen   ulong   I  0UL           Overlapping block horizontal length";
    cout << "\nyblen   ulong   I  0UL           Overlapping block vertical length";
    cout << "\nxbsep   ulong   I  0UL           Overlapping block horizontal separation";
    cout << "\nybsep   ulong   I  0UL           Overlapping block vertical separation";
    cout << "\ncpd     ulong   I  0UL           Perceptual weighting - vertical cycles per deg.";
    cout << "\nqf      float   I  0.0F          Overall quality factor (>0, typically: 7=medium, 9=high)";
    cout << "\nverbose bool    I  false         Verbose mode";
    cout << "\nnolocal bool    I  false         Don't write diagnostics & locally decoded video";
    cout << endl;
}



bool WritePicData (std::ofstream &fdata, dirac_encoder_t *encoder)
{
    dirac_seqparams_t &sparams = encoder->enc_ctx.seq_params;
    dirac_framebuf_t &fbuf = encoder->dec_buf;
    bool ret_stat = true;

    if (encoder->decoded_frame_avail)
    {
        ios::iostate oldExceptions = fdata.exceptions();
        fdata.exceptions (ios::failbit | ios::badbit);
        try
        {
            assert (fbuf.buf[0] != 0);
            fdata.write ((char *)fbuf.buf[0], sparams.width*sparams.height);
            if (sparams.chroma !=Yonly)
            {
                assert (fbuf.buf[1] != 0);
                assert (fbuf.buf[2] != 0);
                fdata.write ((char *)fbuf.buf[1], 
                sparams.chroma_width*sparams.chroma_height);
                fdata.write ((char *)fbuf.buf[2], 
                sparams.chroma_width*sparams.chroma_height);
            }
        }
        catch (...)
        {
            std::cout << "Incomplete frame " << std::endl;
            ret_stat = false;
        }
        fdata.exceptions (oldExceptions);
    }
    return ret_stat;
}

bool WriteSequenceHeader (std::ofstream &fdata, dirac_encoder_t *encoder)
{
    bool ret_stat = true;
    dirac_seqparams_t &sparams = encoder->enc_ctx.seq_params;
    ios::iostate oldExceptions = fdata.exceptions();
    fdata.exceptions (ios::failbit | ios::badbit);

    try
    {
        fdata << sparams.chroma << std::endl;
        fdata << sparams.width << std::endl;
        fdata << sparams.height << std::endl;
        fdata << sparams.interlace << std::endl;
        fdata << sparams.topfieldfirst << std::endl;
        fdata << sparams.frame_rate.numerator << std::endl;
    }

    catch (...)
    {
        std::cerr << "Error writing sequence info in diagnostics file." << std::endl;
        ret_stat =  false;
    }
    fdata.exceptions (oldExceptions);
    return ret_stat;
}

bool WriteDiagnosticsData (std::ofstream &fdata, dirac_encoder_t *encoder)
{
    //dirac_seqparams_t &sparams = encoder->enc_ctx.seq_params;
    dirac_instr_t &instr = encoder->instr;
    bool ret_stat = true;

    if (encoder->instr_data_avail)
    {
        ios::iostate oldExceptions = fdata.exceptions();
        fdata.exceptions (ios::failbit | ios::badbit);
        try
        {
            fdata << std::endl << "[frame:" << instr.fnum << "]";
            if (instr.ftype == I_frame)
            {
                fdata << ">intra" << std::endl;
                return true;
            }

            fdata << ">mo_comp";
            fdata << std::endl << std::endl << instr.num_refs << " ";
            for (int i=0; i<instr.num_refs; ++i)
            {
                fdata << instr.refs[i] << " ";
            }
            fdata << instr.ybsep << " " << instr.xbsep << " ";
            fdata << instr.mb_ylen << " " << instr.mb_xlen << " ";
            fdata << instr.mv_ylen << " " << instr.mv_xlen 
                    << std::endl << std::endl ;
            for (int j=0; j<instr.mb_ylen; j++)
            {
                for (int i=0; i<instr.mb_xlen; i++)
                    fdata << instr.mb_split_mode[j*instr.mb_xlen + i] << " ";
                fdata << std::endl;
            }
            fdata << std::endl;

            for (int j=0; j<instr.mb_ylen; j++)
            {
                for (int i=0; i<instr.mb_xlen; i++)
                    fdata << instr.mb_common_mode[j*instr.mb_xlen + i] << " ";
                fdata << std::endl;
            }
            fdata << std::endl;

            for (int j=0; j<instr.mb_ylen; j++)
            {
                for (int i=0; i<instr.mb_xlen; i++)
                    fdata << instr.mb_costs[j*instr.mb_xlen + i] << " ";
                fdata << std::endl;
            }
            fdata << std::endl;

            for (int j=0; j<instr.mv_ylen; j++)
            {
                for (int i=0; i<instr.mv_xlen; i++)
                    fdata << instr.pred_mode[j*instr.mv_xlen + i] << " ";
                fdata << std::endl;
            }
            fdata << std::endl;

            for (int j=0; j<instr.mv_ylen; j++)
            {
                for (int i=0; i<instr.mv_xlen; i++)
                    fdata << instr.intra_costs[j*instr.mv_xlen + i] << " ";
                fdata << std::endl;
            }
            fdata << std::endl;

            // FIXME: MEData is always created with num_refs set to 2
            // if (instr.num_refs > 1)
            {
                for (int j=0; j<instr.mv_ylen; j++)
                {
                    for (int i=0; i<instr.mv_xlen; i++)
                    {
                        fdata << instr.bipred_costs[j*instr.mv_xlen + i].SAD 
                        <<" " << instr.bipred_costs[j*instr.mv_xlen + i].mvcost
                        << " ";;
                    }
                    fdata << std::endl;
                }
            }
            fdata << std::endl;
            
            for (int j=0; j<instr.mv_ylen; j++)
            {
                for (int i=0; i<instr.mv_xlen; i++)
                    fdata << instr.dc_ycomp[j*instr.mv_xlen + i] << " ";
                fdata << std::endl;
            }
            
            // FIXME: always expects 3 components
            // if (sparams.chroma != Yonly)
            {
                fdata << std::endl;
                for (int j=0; j<instr.mv_ylen; j++)
                {
                    for (int i=0; i<instr.mv_xlen; i++)
                        fdata << instr.dc_ucomp[j*instr.mv_xlen + i] << " ";
                    fdata << std::endl;
                }
                fdata << std::endl;
                for (int j=0; j<instr.mv_ylen; j++)
                {
                    for (int i=0; i<instr.mv_xlen; i++)
                        fdata << instr.dc_vcomp[j*instr.mv_xlen + i] << " ";
                    fdata << std::endl;
                }
            }

            // FIXME: MEData is always created with num_refs set to 2
            for (int k = 0; k < 2; k++)
            {
                fdata << std::endl;
                for (int j=0; j<instr.mv_ylen; j++)
                {
                    for (int i=0; i<instr.mv_xlen; i++)
                    {
                        fdata << instr.mv[k][j*instr.mv_xlen + i].x 
                        <<" " << instr.mv[k][j*instr.mv_xlen + i].y
                        << " ";;
                    }
                    fdata << std::endl;
                }
                fdata << std::endl;
                for (int j=0; j<instr.mv_ylen; j++)
                {
                    for (int i=0; i<instr.mv_xlen; i++)
                    {
                        fdata << instr.pred_costs[k][j*instr.mv_xlen + i].SAD 
                        <<" " << instr.pred_costs[k][j*instr.mv_xlen + i].mvcost
                        << " ";;
                    }
                    fdata << std::endl;
                }
                fdata << std::endl;
            }
        }
        catch (...)
        {
            std::cout << "Error writing diagnostics data" << std::endl;
            ret_stat = false;
        }
        fdata.exceptions (oldExceptions);
    }
    return ret_stat;
}

bool ReadPicData (std::ifstream &fdata, unsigned char *buffer, int frame_size)
{
    bool ret_stat = true;

    ios::iostate oldExceptions = fdata.exceptions();
    fdata.exceptions (ios::failbit | ios::badbit);
    try
    {
        fdata.read ((char *)buffer, frame_size);
    }
    catch (...)
    {
        ret_stat = false;
    }
    fdata.exceptions (oldExceptions);
    return ret_stat;
}

bool Skip (std::ifstream &fdata, int start_frame, int frame_size)
{
    bool ret_stat = true;
    ios::iostate oldExceptions = fdata.exceptions();
    fdata.exceptions (ios::failbit | ios::badbit);
    try
    {
        fdata.seekg(frame_size*start_frame, std::ios::cur );
    }
    catch (...)
    {
        std::cerr << "Skipping of first "<< start_frame << "frames failed" 
                  << std::endl;
        ret_stat = false;
    }
    fdata.exceptions (oldExceptions);
    return ret_stat;
}

int GetFrameBufferSize (const dirac_encoder_context_t &enc_ctx)
{
    int xl = enc_ctx.seq_params.width;
    int yl = enc_ctx.seq_params.height;

    int size;

    switch (enc_ctx.seq_params.chroma)
    {
    case format411:
    case format420:
        size = (xl*yl*3)/2;
        break;
    case format422:
        size = (xl*yl)*2;
        break;
    case format444:
        size = (xl*yl)*3;
        break;
    case Yonly:
    default:
        size = xl * yl;
        break;
    }
    return size;
}
const char *chroma2string (dirac_chroma_t chroma)
{
    switch (chroma)
    {
    case Yonly:
        return "Y_ONLY";

    case format422:
        return "4:2:2";

    case format444:
        return "4:4:4";

    case format420:
        return "4:2:0";

    case format411:
        return "4:1:1";

    default:
        break;
    }

    return "Unknown";
}

void display_codec_params(dirac_encoder_context_t &enc_ctx)
{
    std::cout << "Sequence parameters : " << std::endl;
    std::cout << "\theight=" << enc_ctx.seq_params.height;
    std::cout << " width=" << enc_ctx.seq_params.width << std::endl;
    std::cout << "\tchroma=" << chroma2string(enc_ctx.seq_params.chroma) << std::endl;
    std::cout << "\tframe rate=" << enc_ctx.seq_params.frame_rate.numerator;
    std::cout << "/" << enc_ctx.seq_params.frame_rate.denominator << std::endl;
    std::cout << "Encoder parameters : " << std::endl;
    std::cout << "\tquality factor=" << enc_ctx.enc_params.qf << std::endl;
    std::cout << "\tGOP parameters : num_L1="  << enc_ctx.enc_params.num_L1;
    std::cout << " L1_sep=" << enc_ctx.enc_params.L1_sep << std::endl;
    std::cout << "\tBlock parameters : xblen="  << enc_ctx.enc_params.xblen;
    std::cout << " yblen=" << enc_ctx.enc_params.yblen;
    std::cout << " xbsep=" << enc_ctx.enc_params.xbsep;
    std::cout << " ybsep=" << enc_ctx.enc_params.ybsep << std::endl;
}

int main (int argc, char* argv[])
{
    /*********************************************************************************/
            /**********  command line parameter parsing*********/

    // An array indicating whether a parameter has been parsed
    bool* parsed = new bool[argc];

    // Program name has been parsed
    parsed[0] = true;    

    // No other parameters 
    for (int i=1 ; i<argc ; ++i )
        parsed[i] = false;


    // the variables we'll read parameters into
    dirac_encoder_context_t enc_ctx;

    //output name for the bitstream
    string bit_name;

    string input,output;

    // The start and end-points of the parts of the file to be coded
    // (end_pos set to -1 means code to the end)
    int start_pos = 0;
    int end_pos = -1;
    bool verbose = false;
    bool nolocal = false;

    memset (&enc_ctx, 0, sizeof(dirac_encoder_context_t));
    if (argc<3)//need at least 3 arguments - the program name, an input and 
               //an output
    {
        display_help();
        return (EXIT_SUCCESS);
    }

   //Now do the options
   // Checking for presets. Assume CIF by default
    dirac_encoder_presets_t preset = CIF;
    for (int i = 1; i < argc; i++)
    {
        if ( strcmp (argv[i], "-CIF") == 0 )
        {
            preset = CIF;
            parsed[i] = true;
        }
        else if ( strcmp (argv[i], "-HD720") == 0 )
        {
            preset = HD720;
            parsed[i] = true;
        }
        else if ( strcmp (argv[i], "-HD1080") == 0 )
        {
            preset = HD1080;
            parsed[i] = true;
        }
        else if ( strcmp (argv[i], "-SD576") == 0 )
        {
            preset = SD576;
            parsed[i] = true;
        }
    }

    // initialise the encoder context
    dirac_encoder_context_init (&enc_ctx, preset);

    //now go over again and override presets with other values
    for (int i = 1; i < argc; )
    {
        if ( strcmp(argv[i], "-width") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.seq_params.width =  
                strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }

        if ( strcmp(argv[i], "-height") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.seq_params.height =  
                strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        
        if ( strcmp(argv[i], "-cformat") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.seq_params.chroma =  
                (ChromaFormat)strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }

        if ( strcmp(argv[i], "-fr") == 0 )
        {
            parsed[i] = true;
            i++;
            if(strncmp(argv[i], "59.94", 5)==0)
            {
                parsed[i] = true;
                enc_ctx.seq_params.frame_rate.numerator=60000;
                enc_ctx.seq_params.frame_rate.denominator=1001;
            }
            else if(strncmp(argv[i], "23.98", 5)==0)
            {
                parsed[i] = true;
                enc_ctx.seq_params.frame_rate.numerator=24000; 
                enc_ctx.seq_params.frame_rate.denominator=1001;
            }
            else if(strncmp(argv[i], "29.97", 5)==0)
            {
                parsed[i] = true;
                enc_ctx.seq_params.frame_rate.numerator=30000;
                enc_ctx.seq_params.frame_rate.denominator=1001;
            }
            //test for decimal format
            else if(strcspn(argv[i], ".")!=strlen(argv[i]))
            {
                parsed[i] = true;
                // look for whole number
                char* num_token = strtok(argv[i], ".");
                int whole = strtoul(num_token,NULL,10);
                int decimal=0;
                int decimal_length=0;

                // look for decimal part
                num_token = strtok(NULL, "");
                if(num_token)
                {
                    decimal_length=strlen(num_token);
                    decimal=strtoul(num_token, NULL, 10);
                }
                // calculate amount to raise to whole number
                int multiply = (int)std::pow(10.0, decimal_length);
                enc_ctx.seq_params.frame_rate.numerator =  
                    decimal == 0 ? whole : (multiply*whole)+decimal;
                enc_ctx.seq_params.frame_rate.denominator = 
                    decimal == 0 ? 1 : multiply;
                 
            }
            else 
            {
                parsed[i] = true;
                // assume e/d format
                char* token = strtok(argv[i], "/");
                enc_ctx.seq_params.frame_rate.numerator =  
                strtoul(token,NULL,10);
                token = strtok(NULL, "");
                if(token)
                    enc_ctx.seq_params.frame_rate.denominator = 
                    strtoul(token, NULL, 10);
             }
        }

        if ( strcmp(argv[i], "-qf") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.enc_params.qf =  atof(argv[i]);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-L1_sep") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.enc_params.L1_sep =  
                strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-num_L1") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.enc_params.num_L1 = 
                strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-xblen") == 0 )
        {
            parsed[i] = true;
            i++;
            enc_ctx.enc_params.xblen = 
                strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-yblen") == 0 )
        {
            parsed[i] = true;
            i++;
             enc_ctx.enc_params.yblen = 
                 strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-xbsep") == 0 )
        {
            parsed[i] = true;
            i++;
             enc_ctx.enc_params.xbsep = 
                 strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-ybsep") == 0 )
        {
            parsed[i] = true;
            i++;
             enc_ctx.enc_params.ybsep = 
                 strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-cpd") == 0 )
        {
            parsed[i] = true;
            i++;
             enc_ctx.enc_params.cpd = 
                 strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-verbose") == 0 )
        {
            parsed[i] = true;
            verbose = true;
        }
        else if ( strcmp(argv[i], "-nolocal") == 0 )
        {
            parsed[i] = true;
            nolocal = true;
        }
        else if ( strcmp(argv[i], "-start") == 0 )
        {
            parsed[i] = true;
            i++;
            start_pos = strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        else if ( strcmp(argv[i], "-stop") == 0 )
        {
            parsed[i] = true;
            i++;
            end_pos = strtoul(argv[i],NULL,10);
            parsed[i] = true;
        }
        i++;
    }//opt

    // last two arguments must be file names
    if (argv[argc-2][0] == '-' || argv[argc-1][0] == '-')
    {
        display_help();
        exit(1);
    }

    input=argv[argc-2];
    output=argv[argc-1];
    parsed[argc-2] = true;
    parsed[argc-1] = true;

    //check we have real inputs
    if ((input.length() == 0) || (output.length() ==0))
    {
        display_help();
        exit(1);
    }

    // check we have parsed everything
    bool all_parsed = true;
    for (int i=0 ; i<argc ; ++i)
    {
        if ( !parsed[i] )
        {
            all_parsed = false;
            std::cerr<<std::endl<<"Unknown option "<<argv[i];
        }
    }
    if ( !all_parsed )
    {
        display_help();
        exit(1);
    }

    if (strcmp (input.c_str(), output.c_str()) == 0)
    {
        std::cerr << "Input and output file names must be different" << std::endl;
        exit(1);
    }

    display_codec_params(enc_ctx);
    bit_name = output + ".drc";
        


  /********************************************************************/
    //next do picture file stuff

    // Open uncompressed data file
    std::string input_name_yuv = input + ".yuv";
    std::ifstream 
       ip_pic_ptr (input_name_yuv.c_str(), std::ios::in | std::ios::binary);
    if (!ip_pic_ptr)
    {
        std::cerr << std::endl <<
            "Can't open input data file: " << input_name_yuv << std::endl;
        return EXIT_FAILURE;
    }


   
   /********************************************************************/
    //open the bitstream file
     std::ofstream outfile(bit_name.c_str(),std::ios::out | std::ios::binary);    //bitstream output

    // open the decoded ouput file
    std::string output_name_yuv = output + ".yuv";
    std::ofstream outyuv(output_name_yuv.c_str(),std::ios::out | std::ios::binary);

      // open the diagnostics ouput file
    std::string output_name_imt = output + ".imt";
    std::ofstream outimt(output_name_imt.c_str(),std::ios::out | std::ios::binary);

   /********************************************************************/
    //do the work!!

    // Determine buffer size
    int frame_size = GetFrameBufferSize (enc_ctx);
    unsigned char *frame_buf = new unsigned char [frame_size];

    if ( end_pos == -1 )
        end_pos = INT_MAX;

    if (!Skip( ip_pic_ptr, start_pos, frame_size ))
    {
        return EXIT_FAILURE;
    };

    dirac_encoder_t  *encoder;

    if (nolocal)
    {
        enc_ctx.decode_flag = 0;
        enc_ctx.instr_flag = 0;
    }
    else
    {
        enc_ctx.decode_flag = 1;
        enc_ctx.instr_flag = 1;
    }
        
    encoder = dirac_encoder_init( &enc_ctx, verbose );

    if (!encoder)
    {
        std::cerr << "Unrecoverable Error: dirac_encoder_init failed. "
                  << std::endl;
        return EXIT_FAILURE;
    }

    WriteSequenceHeader ( outimt, encoder );

    int frames_written = 0;
    dirac_encoder_state_t state;
    do 
    {
        if (ReadPicData( ip_pic_ptr, frame_buf, frame_size ) == true)
        {
            if (dirac_encoder_load( encoder, frame_buf, frame_size ) < 0)
            {
                std::cerr << "dirac_encoder_load failed: Unrecoverable Encoder Error. Quitting..." 
                          << std::endl;
                return EXIT_FAILURE;
            }
        }
        else
           break; //eof

        do
        {
            encoder->enc_buf.buffer = video_buf;
            encoder->enc_buf.size = VIDEO_BUFFER_SIZE;
            state = dirac_encoder_output ( encoder );
            switch (state)
            {
            case ENC_STATE_AVAIL:
                assert (encoder->enc_buf.size > 0);
               
                outfile.write((char *)encoder->enc_buf.buffer, 
                              encoder->enc_buf.size);
                              frames_written++;
                break;

            case ENC_STATE_BUFFER:
                break;

            case ENC_STATE_INVALID:
                std::cerr << "Invalid state. Unrecoverable Encoder Error. Quitting..." 
                          << std::endl;
                return EXIT_FAILURE;
            default:
                std::cerr << "Unknown Encoder state" << endl;
                break;
            }

            WritePicData (outyuv, encoder);
            WriteDiagnosticsData (outimt, encoder);

        } while (state == ENC_STATE_AVAIL);

    } while (frames_written <= (end_pos - start_pos));

    encoder->enc_buf.buffer = video_buf;
    encoder->enc_buf.size = VIDEO_BUFFER_SIZE;
    if (dirac_encoder_end_sequence( encoder ) > 0)
    {
        outfile.write((char *)encoder->enc_buf.buffer, 
                      encoder->enc_buf.size);

        if ( verbose )           
            std::cout << "The resulting bit-rate at "
                      << (double)encoder->enc_ctx.seq_params.frame_rate.numerator/
                          encoder->enc_ctx.seq_params.frame_rate.denominator
                      << "Hz is " << encoder->enc_seqstats.bit_rate 
                      << " bits/sec." << std::endl;
    }

   
   /********************************************************************/

     // close the encoder
    dirac_encoder_close( encoder );
     // close the bitstream file
    outfile.close();
     // close the decoded output file
    outyuv.close();
   
     // close the decoded output header file
    outimt.close();

    // close the pic data file
    ip_pic_ptr.close();


    // delete frame buffer
    delete [] frame_buf;
        return EXIT_SUCCESS;

    delete[] parsed;

}
