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
*                 Tim Borer
*                 Andrew Kennedy,
*                 Anuradha Suraparaju
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

#ifndef _COMMON_TYPES_H_
#define _COMMON_TYPES_H_


/*! This file contains common enumerated types used throughout the encoder and 
    the end user interfaces to the encoder and decoder
*/

#ifdef __cplusplus
extern "C" {
#endif
/*
* Some basic enumeration types used throughout the codec and by end user ...//
*/

/*! Types of chroma formatting (formatNK=format not known) */
typedef enum { format444, format422, format420, formatNK } ChromaFormat;

/*! Types of Wavelet filters supported. filterNK -  not known) */
typedef enum 
{
    APPROX97=0,  /* Approximata Daubechies */
    FIVETHREE,   /* Five Three */
    THIRTEENFIVE, /* Thirteen Five */
    HAAR,        /* HAAR */
    RESERVED1,   /* reserved 1 */
    RESERVED2,   /* reserved 1 */
    DAUB97,      /* Daubechies 97 */
    filterNK
} WltFilter;

static const int NUM_WLT_FILTERS = 7;

/*! Types of frame */
typedef enum {
        INTRA_FRAME=0,
        INTER_FRAME
    } FrameType;

/*! Types of referencing */
typedef enum {
        REFERENCE_FRAME=0,
        NON_REFERENCE_FRAME
}   ReferenceType;

/*! Types for video-format */
typedef enum {
        VIDEO_FORMAT_CUSTOM=0,
        VIDEO_FORMAT_QSIF,
        VIDEO_FORMAT_QCIF,
        VIDEO_FORMAT_SIF,
        VIDEO_FORMAT_CIF,
        VIDEO_FORMAT_SD_PAL,
        VIDEO_FORMAT_SD_NTSC,
        VIDEO_FORMAT_SD_525_DIGITAL,
        VIDEO_FORMAT_SD_625_DIGITAL,
        VIDEO_FORMAT_HD_720,
        VIDEO_FORMAT_HD_1080,
        VIDEO_FORMAT_DIGI_CINEMA_2K,
        VIDEO_FORMAT_DIGI_CINEMA_4K,
        VIDEO_FORMAT_UNDEFINED
} VideoFormat;

/*! Types of Colour primaries */
typedef enum {
    CP_ITU_709=0,
    CP_SMPTE_C,
    CP_EBU_3213,
    CP_UNDEF
}ColourPrimaries;

/*! Types of Colour Matrices */
typedef enum {
    CM_HDTV_COMP_INTERNET=0,
    CM_SDTV,
    CM_UNDEF
}ColourMatrix;

/*! Types of Transfer functions */
typedef enum {
    TF_TV=0,
    TF_EXT_GAMUT,
    TF_LINEAR,
    TF_UNDEF
} TransferFunction;

/*! Types of Frame-rate */
typedef enum {
    FRAMERATE_CUSTOM=0,
    FRAMERATE_23p97_FPS,
    FRAMERATE_24_FPS,
    FRAMERATE_25_FPS,
    FRAMERATE_29p97_FPS,
    FRAMERATE_30_FPS,
    FRAMERATE_50_FPS,
    FRAMERATE_59p94_FPS,
    FRAMERATE_60_FPS,
    FRAMERATE_UNDEFINED
} FrameRateType;

/*! Types of Aspect Ratio */
typedef enum {
    ASPECT_RATIO_CUSTOM=0,
    ASPECT_RATIO_1_1,
    ASPECT_RATIO_10_11,
    ASPECT_RATIO_12_11,
    ASPECT_RATIO_UNDEFINED
} AspectRatioType;


/*! Types of signal range */
typedef enum {
    SIGNAL_RANGE_CUSTOM=0,
    SIGNAL_RANGE_8BIT_FULL,
    SIGNAL_RANGE_8BIT_VIDEO,
    SIGNAL_RANGE_10BIT_VIDEO,
    SIGNAL_RANGE_UNDEFINED
} SignalRangeType;

/*! Types of Transfer functions */
typedef enum {
    IT_PROGRESSIVE=0,
    IT_INTERLACED_TFF,
    IT_INTERLACED_BFF,
    IT_UNDEF
} InterlaceType;

#ifdef __cplusplus
}
#endif

#endif 
