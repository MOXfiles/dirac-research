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
* Contributor(s): Andrew Kennedy (Original Author).
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

#include <libdirac_common/video_format_defaults.h>
#include <libdirac_common/dirac_exception.h>

using namespace dirac;

namespace dirac 
{
void SetDefaultCodecParameters(CodecParams &cparams, FrameType ftype)
{
    // Transform parameters
    cparams.SetZeroTransform(false);
    cparams.SetTransformDepth(4);
    cparams.SetTransformFilter(APPROX97);
    cparams.SetCodeBlockMode(QUANT_SINGLE);
       cparams.SetSpatialPartition(false);
       cparams.SetDefaultSpatialPartition(true);
    switch (cparams.GetVideoFormat())
    {
    case VIDEO_FORMAT_QSIF:
    case VIDEO_FORMAT_QCIF:
        break;

    case VIDEO_FORMAT_CUSTOM:
    case VIDEO_FORMAT_SIF:
    case VIDEO_FORMAT_CIF:
    case VIDEO_FORMAT_SD_PAL:
    case VIDEO_FORMAT_SD_NTSC:
    case VIDEO_FORMAT_SD_525_DIGITAL:
    case VIDEO_FORMAT_SD_625_DIGITAL:
    case VIDEO_FORMAT_HD_720:
    case VIDEO_FORMAT_HD_1080:
        cparams.SetSpatialPartition(true);
        cparams.SetDefaultSpatialPartition(true);
        break;
    default:
        DIRAC_THROW_EXCEPTION(
            ERR_INVALID_VIDEO_FORMAT,
            "Unsupported video format",
            SEVERITY_FRAME_ERROR);
        break;
    }
    cparams.SetDefaultCodeBlocks(ftype);

    if (ftype == INTER_FRAME)
    {
        cparams.SetTransformFilter(FIVETHREE);
        OLBParams bparams;
        SetDefaultBlockParameters(bparams, cparams.GetVideoFormat());
        cparams.SetLumaBlockParams(bparams);
        cparams.SetInterlace(false);
        cparams.SetTopFieldFirst(true);
        cparams.SetMVPrecision(MV_PRECISION_HALF_PIXEL);
        // NOTE: FIXME - need to add global motion params here
        cparams.SetFrameWeightsPrecision(1);
        cparams.SetRef1Weight(1);
        cparams.SetRef2Weight(1);
    }
}

void SetDefaultSourceParameters(const VideoFormat &vf, SourceParams& sparams) 
{
    sparams.SetInterlace(false);
    sparams.SetTopFieldFirst(true);
    sparams.SetSequentialFields(false);
    sparams.SetAspectRatio(ASPECT_RATIO_1_1);
    sparams.SetSignalRange(SIGNAL_RANGE_8BIT_FULL);
    sparams.SetCleanWidth(0);
    sparams.SetCleanHeight(0);
    sparams.SetLeftOffset(0);
    sparams.SetTopOffset(0);
    sparams.SetColourSpecification(1);
    
    switch (vf)
    {
    case VIDEO_FORMAT_CUSTOM:
        sparams.SetFrameRate(FRAMERATE_30_FPS);
        sparams.SetCleanWidth(620);
        sparams.SetCleanHeight(480);
        break;
    case VIDEO_FORMAT_QSIF:
        sparams.SetFrameRate(15, 1);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(176);
        sparams.SetCleanHeight(120);
        break;
    case VIDEO_FORMAT_QCIF:
        sparams.SetFrameRate(25, 2);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(176);
        sparams.SetCleanHeight(144);
        sparams.SetColourSpecification(2);
        break;
    case VIDEO_FORMAT_SIF:
        sparams.SetFrameRate(15, 1);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(352);
        sparams.SetCleanHeight(240);
        break;
    case VIDEO_FORMAT_CIF:
        sparams.SetFrameRate(25, 2);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(352);
        sparams.SetCleanHeight(288);
        sparams.SetColourSpecification(2);
        break;
    case VIDEO_FORMAT_SD_NTSC:
        sparams.SetFrameRate(FRAMERATE_29p97_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(480);
        sparams.SetSignalRange(SIGNAL_RANGE_8BIT_VIDEO);
        sparams.SetInterlace(true);
        break;
    case VIDEO_FORMAT_SD_PAL:
        sparams.SetFrameRate(FRAMERATE_25_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(576);
        sparams.SetSignalRange(SIGNAL_RANGE_8BIT_VIDEO);
        sparams.SetColourSpecification(2);
        sparams.SetInterlace(true);
        break;
    case VIDEO_FORMAT_SD_525_DIGITAL:
        sparams.SetFrameRate(FRAMERATE_29p97_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(480);
        sparams.SetSignalRange(SIGNAL_RANGE_8BIT_VIDEO);
        sparams.SetColourSpecification(1);
        sparams.SetInterlace(true);
        sparams.SetTopOffset(8);
        break;
    case VIDEO_FORMAT_SD_625_DIGITAL:
        sparams.SetFrameRate(FRAMERATE_25_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(702);
        sparams.SetCleanHeight(576);
        sparams.SetSignalRange(SIGNAL_RANGE_8BIT_VIDEO);
        sparams.SetColourSpecification(2);
        sparams.SetInterlace(true);
        sparams.SetTopOffset(9);
        break;
    case VIDEO_FORMAT_HD_720:
        sparams.SetFrameRate(FRAMERATE_50_FPS);
        sparams.SetCleanWidth(1280);
        sparams.SetCleanHeight(720);
        sparams.SetSignalRange(SIGNAL_RANGE_8BIT_VIDEO);
        sparams.SetColourSpecification(3);
        break;
    case VIDEO_FORMAT_HD_1080:
        sparams.SetFrameRate(FRAMERATE_25_FPS);
        sparams.SetCleanWidth(1920);
        sparams.SetCleanHeight(1080);
        sparams.SetSignalRange(SIGNAL_RANGE_8BIT_VIDEO);
        sparams.SetColourSpecification(3);
        sparams.SetInterlace(true);
        break;
    default:
        break;
    }
}

void SetDefaultSequenceParameters(SeqParams& sparams) 
{
   sparams.SetCFormat(format420);
   sparams.SetVideoDepth(8);

    switch(sparams.GetVideoFormat())
    {
    case VIDEO_FORMAT_CUSTOM:
        sparams.SetXl(640);
        sparams.SetYl(480);
        break;
    case VIDEO_FORMAT_QSIF:
        sparams.SetXl(176);
        sparams.SetYl(120);
        break;
    case VIDEO_FORMAT_QCIF:
        sparams.SetXl(176);
        sparams.SetYl(144);
        break;
    case VIDEO_FORMAT_SIF:
        sparams.SetXl(352);
        sparams.SetYl(240);
        break;
    case VIDEO_FORMAT_SD_NTSC:
        sparams.SetXl(704);
        sparams.SetYl(480);
        break;
    case VIDEO_FORMAT_SD_PAL:
        sparams.SetXl(704);
        sparams.SetYl(576);
        break;
    case VIDEO_FORMAT_SD_525_DIGITAL:
        sparams.SetXl(720);
        sparams.SetYl(480);
        break;
    case VIDEO_FORMAT_SD_625_DIGITAL:
        sparams.SetXl(720);
        sparams.SetYl(576);
        break;
    case VIDEO_FORMAT_HD_720:
        sparams.SetXl(1280);
        sparams.SetYl(720);
        break;
    case VIDEO_FORMAT_HD_1080:
        sparams.SetXl(1920);
        sparams.SetYl(1080);
        break;
    case VIDEO_FORMAT_CIF:
    default:
        sparams.SetXl(352);
        sparams.SetYl(288);
        break;
    }

}
    
void SetDefaultEncoderParameters(EncoderParams& encparams)
{
    encparams.SetQf(7.0f);
    encparams.SetLossless(false);
    encparams.SetMVPrecision(MV_PRECISION_HALF_PIXEL);

    switch (encparams.GetVideoFormat())
    {
    case VIDEO_FORMAT_SD_625_DIGITAL:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(3);
        encparams.SetCPD(32.0f);
        break;

    case VIDEO_FORMAT_HD_720:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(7);
        encparams.SetCPD(20.0f);
        break;

    case VIDEO_FORMAT_HD_1080:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(3);
        encparams.SetCPD(32.0f);
        break;

    case VIDEO_FORMAT_CIF:
    default:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(11);
        encparams.SetCPD(20.0f);
        break;
    }

}

void SetDefaultBlockParameters(OLBParams& bparams,
                               const VideoFormat& video_format)
{
    switch (video_format)
    {
    case VIDEO_FORMAT_SD_625_DIGITAL:
        bparams.SetXblen(12);
        bparams.SetYblen(12);
        bparams.SetXbsep(8);
        bparams.SetYbsep(8);
        break;

    case VIDEO_FORMAT_HD_720:
        bparams.SetXblen(16);
        bparams.SetYblen(16);
        bparams.SetXbsep(12);
        bparams.SetYbsep(12);
        break;

    case VIDEO_FORMAT_HD_1080:
        bparams.SetXblen(24);
        bparams.SetYblen(24);
        bparams.SetXbsep(16);
        bparams.SetYbsep(16);
        break;

    case VIDEO_FORMAT_CIF:
    default:
        bparams.SetXblen(12);
        bparams.SetYblen(12);
        bparams.SetXbsep(8);
        bparams.SetYbsep(8);
        break;
    }

}

void SetDefaultBlockParameters(OLBParams& bparams, int pidx)
{
    switch (pidx)
    {
    case 0: // custom - so undefined values
        return;
    case 1: 
        bparams =  OLBParams(8, 8, 4, 4);
        break;
    case 2: 
        bparams =  OLBParams(12, 12, 8, 8);
        break;
    case 3: 
        bparams =  OLBParams(16, 16, 12, 12);
        break;
    case 4: 
        bparams =  OLBParams(24, 24, 16, 16);
        break;
    default:
        DIRAC_THROW_EXCEPTION(
            ERR_UNSUPPORTED_STREAM_DATA,
            "Block params index out of range [0-4]",
            SEVERITY_FRAME_ERROR);
        break;
    }
}

unsigned int BlockParametersIndex (const OLBParams& bparams)
{
    OLBParams bparams_1(8, 8, 4, 4);
    OLBParams bparams_2(12, 12, 8, 8);
    OLBParams bparams_3(16, 16, 12, 12);
    OLBParams bparams_4(24, 24, 16, 16);

    if (bparams == bparams_1)
        return 1;
    else if (bparams == bparams_2)
        return 2;
    else if (bparams == bparams_3)
        return 3;
    else if (bparams == bparams_4)
        return 4;
    else 
        return 0;
}

}
