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

#ifndef DIRAC_PARSER_H
#define DIRAC_PARSER_H

#include <libdirac_common/common_types.h>
#include <libdirac_decoder/decoder_types.h>

/*! 
    C interface to Dirac decoder
    A set of 'C' functions that define the interface to the Dirac decoder.
*/
#ifdef __cplusplus
extern "C" {
#endif

typedef ChromaFormat dirac_chroma_t;
typedef FrameSort dirac_frame_type_t;

/*! Structure that holds the sequence parameters */
typedef struct
{
    /*! numper of pixels per line */
    int width;
    /*! number of lines per frame */
    int height;
    /*! total number of frame in sequence */
    int num_frames;
    /*! chroma type */
    dirac_chroma_t chroma;
    /*! numper of pixels of chroma per line */
    int chroma_width;
    /*! number of lines of chroma per frame */
    int chroma_height;
    /*! frame rate */
    int frame_rate;
    /*! interlace flag: 0 - progressive; 1 - interlaced */
    int interlace;
    /*! top field comes first : 0 - false; 1 - true */
    int topfieldfirst;
} dirac_seqparams_t;

/*! Structure that holds the frame parameters */
typedef struct
{
    /*! frame type */
    dirac_frame_type_t ftype;
    /*! frame number in decoded order */
    int fnum;
} dirac_frameparams_t;

/*! Structure that holds the frame buffers into which data is written */
typedef struct
{
    /*! buffers to hold the luma and chroma data */
    unsigned char  *buf[3];
    /*! user data */
    void  *id;
} dirac_framebuf_t;

/*! Structure that holds the information returned by the parser */
typedef struct 
{
    /*! parser state */
    DecoderState state;
    /*! sequence parameters */
    dirac_seqparams_t seq_params;
    /*! frame parameters */
    dirac_frameparams_t frame_params;
    /*! void pointer to internal parser */
    void *parser;
    /*! frame buffer to hold luma and chroma data */
    dirac_framebuf_t *fbuf;
    /*! boolean flag that indicates if a decoded frame is available */
    int frame_avail;
    /*! verbose output */
    int verbose;

} dirac_decoder_t;

/*! 
    Decoder Init
    Initialise the decoder. It returns a dirac_decoder_t object
    \param verbose boolean flag to set verbose output
*/
dirac_decoder_t *dirac_decoder_init(int verbose);

/*!
    Close the decoder
    \param decoder  Decoder object
*/
void dirac_decoder_close(dirac_decoder_t *decoder);

/*!
    Parses the data in the input buffer. This function returns the 
    following values
    STATE_BUFFER         Not enough data in internal buffer to process 
    STATE_SEQUENCE       Start of sequence detected
    STATE_PICTURE_START  Start of picture detected
    STATE_PICTURE_AVAIL  Decoded picture available
    STATE_SEQUENCE_END   End of sequence detected
    STATE_INVALID        Invalid stream. Stop further processing

    \param decoder  Decoder object

*/
DecoderState dirac_parse (dirac_decoder_t *decoder);

/*!
    Copy data into internal buffer
    \param decoder  Decoder object
    \param start    Start of data
    \param end      End of data
*/
void dirac_buffer (dirac_decoder_t *decoder, unsigned char *start, unsigned char *end);

/*!
    Set the output buffer into which the decoder copies the decoded data
    \param decoder  Decoder object
    \param buf      Array of char buffers to hold luma and chroma data
    \param id       User data
*/
void dirac_set_buf (dirac_decoder_t *decoder, unsigned char *buf[3], void *id);

/*!
    Skip the next frame to be decoded
    \param decoder  Decoder object
    \param skip     Value 0 - decode next frame; 1 - skip next frame
*/
void dirac_skip(dirac_decoder_t *decoder, int skip);

#ifdef __cplusplus
}
#endif
#endif
