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
* Contributor(s): Anuradha Suraparaju (Original Author)
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <libdirac_common/dirac_assertions.h>
#include <libdirac_decoder/dirac_parser.h>

int verbose = 0;
int skip = 0;

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

const char *ftype2string (dirac_frame_type_t ftype)
{
    switch (ftype)
    {
    case I_frame:
        return "I_frame";
    
    case L1_frame:
        return "L1_frame";
    
    case L2_frame:
        return "L2_frame";

    default:
        break;
    }
    return "Unknown";
}

static void WritePicData (dirac_decoder_t *decoder, FILE *fp)
{
    ASSERT (decoder != NULL);
    ASSERT (fp);
    
    ASSERT(decoder->fbuf);

    ASSERT(decoder->fbuf->buf[0]);
    fwrite (decoder->fbuf->buf[0], decoder->seq_params.width*decoder->seq_params.height, 1, fp);

    if (decoder->seq_params.chroma != Yonly)
    {
        ASSERT(decoder->fbuf->buf[1]);
        fwrite (decoder->fbuf->buf[1], decoder->seq_params.chroma_width*decoder->seq_params.chroma_height, 1, fp);

        ASSERT(decoder->fbuf->buf[2]);
        fwrite (decoder->fbuf->buf[2], decoder->seq_params.chroma_width*decoder->seq_params.chroma_height, 1, fp);
    }
}

static void WritePicHeader (dirac_decoder_t *decoder, FILE *fp)
{
    ASSERT (decoder != NULL);
    ASSERT (fp);

    fprintf (fp, "%d\n", decoder->seq_params.chroma);
    fprintf (fp, "%d\n", decoder->seq_params.width);
    fprintf (fp, "%d\n", decoder->seq_params.height);
    fprintf (fp, "%d\n", decoder->seq_params.num_frames);
    fprintf (fp, "%d\n", decoder->seq_params.interlace);
    fprintf (fp, "%d\n", decoder->seq_params.topfieldfirst);
    fprintf (fp, "%d\n", decoder->seq_params.frame_rate);
}

static void FreeFrameBuffer (dirac_decoder_t *decoder)
{
    ASSERT (decoder != NULL);
    if (decoder->fbuf)
    {
        for (int i = 0; i < 3; i++)
        {
            if (decoder->fbuf->buf[i])
                free(decoder->fbuf->buf[i]);
            decoder->fbuf->buf[i] = 0;
        }
    }
}

static void DecodeDirac (const char *iname, const char *oname)
{
    clock_t start_t, stop_t;
    dirac_decoder_t *decoder = NULL;
    FILE *ifp;
    FILE *fpdata, *fphdr;
    unsigned char buffer[4096];
    int bytes;
    int num_frames = 0;
    char infile_name[FILENAME_MAX];
    char outfile_hdr[FILENAME_MAX];
    char outfile_data[FILENAME_MAX];
    DecoderState state = STATE_BUFFER;

    strncpy(infile_name, iname, sizeof(infile_name));
    strcat(infile_name, ".drc");

    strncpy(outfile_hdr, oname, sizeof(outfile_hdr));
    strcat(outfile_hdr, ".hdr");

    strncpy(outfile_data, oname, sizeof(outfile_data));
    strcat(outfile_data, ".yuv");

    if ((ifp = fopen (infile_name, "rb")) ==NULL)
    {
        perror(iname);
        return;
    }

    if ((fphdr = fopen (outfile_hdr, "w")) ==NULL)
    {
        perror(outfile_hdr);
        fclose(ifp);
        return;
    }

    if ((fpdata = fopen (outfile_data, "wb")) ==NULL)
    {
        perror(outfile_hdr);
        fclose(ifp);
        fclose(fphdr);
        return;
    }

    /* initialise the decoder */
    decoder = dirac_decoder_init(verbose);

    ASSERT (decoder != NULL);


    start_t=clock();
    do 
    {
        /* parse the input data */
        state = dirac_parse(decoder);
        
        switch (state)
        {
        case STATE_BUFFER:
            /*
            * parser is out of data. Read data from input stream and pass it
            * on to the parser
            */
            bytes = fread (buffer, 1, sizeof(buffer), ifp);
            if (bytes)
                dirac_buffer (decoder, buffer, buffer + bytes);
            break;

        case STATE_SEQUENCE:
            {
            /*
            * Start of sequence detected. Allocate for the frame buffers and
            * pass this buffer to the parser
            */
            unsigned char *buf[3];

            if (verbose)
            {
                fprintf (stderr, "SEQUENCE : width=%d height=%d chroma=%s chroma_width=%d chroma_height=%d num_frames=%d frame_rate=%d, interlace=%s topfieldfirst=%s\n", 
                decoder->seq_params.width,
                decoder->seq_params.height,
                chroma2string(decoder->seq_params.chroma),
                decoder->seq_params.chroma_width,
                decoder->seq_params.chroma_height,
                decoder->seq_params.num_frames,
                decoder->seq_params.frame_rate,
                decoder->seq_params.interlace ? "yes" : "no",
                decoder->seq_params.interlace ? "yes" : "no");
            }

            FreeFrameBuffer(decoder);

            buf[0] = buf[1] = buf[2] = 0;

            buf[0] = (unsigned char *)malloc (decoder->seq_params.width * decoder->seq_params.height);
            if (decoder->seq_params.chroma != Yonly)
            {
                buf[1] = (unsigned char *)malloc (decoder->seq_params.chroma_width * decoder->seq_params.chroma_height);
                buf[2] = (unsigned char *)malloc (decoder->seq_params.chroma_width * decoder->seq_params.chroma_height);
            }
            dirac_set_buf (decoder, buf, NULL);

            /* write the header file */
            WritePicHeader(decoder, fphdr);
            }
            break;

        case STATE_SEQUENCE_END:
            /*
            * End of Sequence detected. Free the frame buffers
            */
            if (verbose)
                fprintf (stderr, "SEQUENCE_END\n");
            
            FreeFrameBuffer(decoder);
            break;
        
        case STATE_PICTURE_START:
            /*
            * Start of frame detected. If decoder is too slow and frame can be
            * skipped, inform the parser to skip decoding the frame
            */
            num_frames++;
            if (verbose)
            {
                fprintf (stderr, "PICTURE_START : frame_type=%s frame_num=%d\n",
                    ftype2string(decoder->frame_params.ftype),
                    decoder->frame_params.fnum);
            }
            /* Just for testing skip every L2_frame */
            if (skip && decoder->frame_params.ftype == L2_frame)
            {
                if (verbose)
                    fprintf (stderr, "              : Skipping frame\n");

                dirac_skip (decoder, 1);
            }
            else
                dirac_skip (decoder, 0);
            break;

        case STATE_PICTURE_AVAIL:
            if (verbose)
            {
                fprintf (stderr, "PICTURE_AVAIL : frame_type=%s frame_num=%d\n",
                    ftype2string(decoder->frame_params.ftype),
                    decoder->frame_params.fnum);
            }
            /* picture available for display */
            WritePicData(decoder, fpdata);
            break;

        case STATE_INVALID:
            /* Invalid state. Stop all processing */
            fprintf (stderr, "Error processing file %s\n", iname);
            break;

        default:
            continue;
        }
    } while (bytes > 0 && state != STATE_INVALID);
    stop_t=clock();

    fprintf (stderr, "Time per frame: %g\n",
            (double)(stop_t-start_t)/(double)(CLOCKS_PER_SEC*num_frames));

    fclose(fpdata);
    fclose(fphdr);
    fclose(ifp);

    /* free all resources */
    dirac_decoder_close(decoder);
}

static void printUsage(const char *str)
{
    fprintf (stderr, "DIRAC wavelet video decoder.\n");
    fprintf (stderr, "Usage: %s [-h|-help] [-v|-verbose] [-s|-skip] input-file output-file \\\n"
                   "\t-h|-help     Display help message\n"
                   "\t-v|-verbose  Verbose mode\n"
                   "\t-s|-skip     Skip decoding L2 frames\n"
                   "\tinput-file   dirac file name excluding extension\n"
                   "\touput-file   decoded output file excluding extension\n",
                   str);
}

extern int main (int argc, char **argv)
{
    int i;

    int offset = 1;

    if (argc == 1)
    {
        printUsage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    for (i=1; i<argc; i++)
    {
        if (*argv[i] == '-')
        {
            if (strcmp (argv[i], "-v") == 0 ||
                strcmp (argv[i], "-verbose")== 0)
            {
                verbose = 1;
            }
            else if (strcmp (argv[i], "-s") == 0 ||
                strcmp (argv[i], "-skip")== 0)
            {
                skip = 1;
            }
            else if (strcmp (argv[i], "-h") == 0 ||
                strcmp (argv[i], "-help")== 0)
            {
                printUsage(argv[0]);
                exit(EXIT_SUCCESS);
            }
            offset++;
        }
    }

    if ((argc - offset) != 2)
    {
        printUsage(argv[0]);
        exit(EXIT_SUCCESS);
    }

    /* call decode routine */
    DecodeDirac (argv[argc-2], argv[argc-1]);
    return EXIT_SUCCESS;
}
