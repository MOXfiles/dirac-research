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

#include <sstream>
#include <libdirac_common/video_format_defaults.h>
#include <libdirac_common/dirac_exception.h>

using namespace dirac;

namespace dirac 
{
void SetDefaultCodecParameters(CodecParams &cparams, 
                               FrameType ftype,
                               unsigned int num_refs)
{
    std::ostringstream errstr;
    // Transform parameters
    cparams.SetZeroTransform(false);
    cparams.SetTransformDepth(4);
    WltFilter wf;
    SetDefaultTransformFilter(ftype, wf);
    cparams.SetTransformFilter(wf);
    cparams.SetCodeBlockMode(QUANT_SINGLE);
    cparams.SetSpatialPartition(false);

    // Default is set to progressive specified irrespective
    // of whether the source material is interlaced or progressive.
    // Overridden from command line of encoder or in bytestream for decoder.
    cparams.SetInterlacedCoding(false);
    cparams.SetTopFieldFirst(true);
    switch (cparams.GetVideoFormat())
    {
    case VIDEO_FORMAT_QSIF525:
    case VIDEO_FORMAT_QCIF:
    case VIDEO_FORMAT_CUSTOM:
    case VIDEO_FORMAT_SIF525:
    case VIDEO_FORMAT_CIF:
    case VIDEO_FORMAT_4CIF:
    case VIDEO_FORMAT_4SIF525:
    case VIDEO_FORMAT_SD_480I60:
    case VIDEO_FORMAT_SD_576I50:
    case VIDEO_FORMAT_HD_720P60:
    case VIDEO_FORMAT_HD_720P50:
    case VIDEO_FORMAT_HD_1080I60:
    case VIDEO_FORMAT_HD_1080I50:
    case VIDEO_FORMAT_HD_1080P60:
    case VIDEO_FORMAT_HD_1080P50:
    case VIDEO_FORMAT_DIGI_CINEMA_2K24:
    case VIDEO_FORMAT_DIGI_CINEMA_4K24:
        cparams.SetSpatialPartition(true);
        break;
    default:
        errstr << "Unsupported video format " << cparams.GetVideoFormat()
               << std::endl;
        DIRAC_THROW_EXCEPTION(
            ERR_INVALID_VIDEO_FORMAT,
            errstr.str(),
            SEVERITY_FRAME_ERROR);
        break;
    }

    if (ftype == INTER_FRAME)
    {
        ASSERTM (num_refs > 0 && num_refs < 3, "Number of reference frames should be 1 or 2 fo INTER frames" );
        OLBParams bparams;
        cparams.SetUsingGlobalMotion(false);
        SetDefaultBlockParameters(bparams, cparams.GetVideoFormat());
        cparams.SetLumaBlockParams(bparams);
        cparams.SetInterlacedCoding(false);
        cparams.SetMVPrecision(MV_PRECISION_QUARTER_PIXEL);
        // NOTE: FIXME - need to add global motion params here
        if (num_refs == 1)
        {
            cparams.SetFrameWeightsPrecision(0);
            cparams.SetRef1Weight(1);
            cparams.SetRef2Weight(0);
        }
        else
        {
            cparams.SetFrameWeightsPrecision(1);
            cparams.SetRef1Weight(1);
            cparams.SetRef2Weight(1);
        }
    }
}

void SetDefaultSourceParameters(const VideoFormat &vf, SourceParams& sparams) 
{
    std::ostringstream errstr;
    sparams.SetVideoFormat(vf);
    sparams.SetCFormat(format420);
    sparams.SetInterlace(false);
    sparams.SetTopFieldFirst(true);
    sparams.SetAspectRatio(ASPECT_RATIO_1_1);
    sparams.SetSignalRange(SIGNAL_RANGE_8BIT_FULL);
    sparams.SetLeftOffset(0);
    sparams.SetTopOffset(0);
    sparams.SetColourSpecification(1);
    
    switch (vf)
    {
    case VIDEO_FORMAT_CUSTOM:
        sparams.SetXl(640);
        sparams.SetYl(480);
        sparams.SetTopFieldFirst(false);
        sparams.SetFrameRate(FRAMERATE_23p97_FPS);
        sparams.SetCleanWidth(640);
        sparams.SetCleanHeight(480);
        sparams.SetColourSpecification(0);
        break;
    case VIDEO_FORMAT_QSIF525:
        sparams.SetXl(176);
        sparams.SetYl(120);
        sparams.SetTopFieldFirst(false);
        sparams.SetFrameRate(FRAMERATE_14p98_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(176);
        sparams.SetCleanHeight(120);
        sparams.SetColourSpecification(1);
        break;
    case VIDEO_FORMAT_QCIF:
        sparams.SetXl(176);
        sparams.SetYl(144);
        sparams.SetFrameRate(FRAMERATE_12p5_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(176);
        sparams.SetCleanHeight(144);
        sparams.SetColourSpecification(2);
        break;
    case VIDEO_FORMAT_SIF525:
        sparams.SetXl(352);
        sparams.SetYl(240);
        sparams.SetTopFieldFirst(false);
        sparams.SetFrameRate(FRAMERATE_14p98_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(352);
        sparams.SetCleanHeight(240);
        sparams.SetColourSpecification(1);
        break;
    case VIDEO_FORMAT_CIF:
        sparams.SetXl(352);
        sparams.SetYl(288);
        sparams.SetFrameRate(FRAMERATE_12p5_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(352);
        sparams.SetCleanHeight(288);
        sparams.SetColourSpecification(2);
        break;
    case VIDEO_FORMAT_4SIF525:
        sparams.SetXl(704);
        sparams.SetYl(480);
        sparams.SetTopFieldFirst(false);
        sparams.SetFrameRate(FRAMERATE_14p98_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(480);
        sparams.SetColourSpecification(1);
        break;
    case VIDEO_FORMAT_4CIF:
        sparams.SetXl(704);
        sparams.SetYl(576);
        sparams.SetFrameRate(FRAMERATE_12p5_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(576);
        sparams.SetColourSpecification(2);
        break;
    case VIDEO_FORMAT_SD_480I60:
        sparams.SetXl(720);
        sparams.SetYl(480);
        sparams.SetCFormat(format422);
        sparams.SetInterlace(true);
        sparams.SetTopFieldFirst(false);
        sparams.SetFrameRate(FRAMERATE_29p97_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_10_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(480);
        sparams.SetLeftOffset(8);
        sparams.SetSignalRange(SIGNAL_RANGE_10BIT_VIDEO);
        sparams.SetColourSpecification(1);
        break;
    case VIDEO_FORMAT_SD_576I50:
        sparams.SetXl(720);
        sparams.SetYl(576);
        sparams.SetCFormat(format422);
        sparams.SetInterlace(true);
        sparams.SetFrameRate(FRAMERATE_25_FPS);
        sparams.SetAspectRatio(ASPECT_RATIO_12_11);
        sparams.SetCleanWidth(704);
        sparams.SetCleanHeight(576);
        sparams.SetLeftOffset(8);
        sparams.SetSignalRange(SIGNAL_RANGE_10BIT_VIDEO);
        sparams.SetColourSpecification(2);
        break;
    case VIDEO_FORMAT_HD_720P50:
    case VIDEO_FORMAT_HD_720P60:
        sparams.SetXl(1280);
        sparams.SetYl(720);
        sparams.SetCFormat(format422);
        if (vf == VIDEO_FORMAT_HD_720P50)
            sparams.SetFrameRate(FRAMERATE_50_FPS);
        else
            sparams.SetFrameRate(FRAMERATE_59p94_FPS);
        sparams.SetCleanWidth(1280);
        sparams.SetCleanHeight(720);
        sparams.SetSignalRange(SIGNAL_RANGE_10BIT_VIDEO);
        sparams.SetColourSpecification(3);
        break;
    case VIDEO_FORMAT_HD_1080I60:
    case VIDEO_FORMAT_HD_1080I50:
    case VIDEO_FORMAT_HD_1080P60:
    case VIDEO_FORMAT_HD_1080P50:
        sparams.SetXl(1920);
        sparams.SetYl(1080);
        sparams.SetCFormat(format422);
        switch (vf)
        {
        case VIDEO_FORMAT_HD_1080I60:
            sparams.SetInterlace(true);
            sparams.SetFrameRate(FRAMERATE_29p97_FPS);
            break;
        case VIDEO_FORMAT_HD_1080I50:
            sparams.SetInterlace(true);
            sparams.SetFrameRate(FRAMERATE_25_FPS);
            break;
        case VIDEO_FORMAT_HD_1080P60:
            sparams.SetFrameRate(FRAMERATE_59p94_FPS);
            break;
        case VIDEO_FORMAT_HD_1080P50:
            sparams.SetFrameRate(FRAMERATE_50_FPS);
            break;
        default:
            break;
        }
        sparams.SetSignalRange(SIGNAL_RANGE_10BIT_VIDEO);
        sparams.SetCleanWidth(1920);
        sparams.SetCleanHeight(1080);
        sparams.SetColourSpecification(3);
        break;
    case VIDEO_FORMAT_DIGI_CINEMA_2K24:
        sparams.SetXl(2048);
        sparams.SetYl(1080);
        sparams.SetCFormat(format444);
        sparams.SetFrameRate(FRAMERATE_24_FPS);
        sparams.SetCleanWidth(2048);
        sparams.SetCleanHeight(1080);
        sparams.SetSignalRange(SIGNAL_RANGE_12BIT_VIDEO);
        sparams.SetColourSpecification(4);
        break;
    case VIDEO_FORMAT_DIGI_CINEMA_4K24:
        sparams.SetXl(4096);
        sparams.SetYl(2160);
        sparams.SetCFormat(format444);
        sparams.SetFrameRate(FRAMERATE_24_FPS);
        sparams.SetCleanWidth(4096);
        sparams.SetCleanHeight(2160);
        sparams.SetSignalRange(SIGNAL_RANGE_12BIT_VIDEO);
        sparams.SetColourSpecification(4);
        break;
    default:
        errstr << "Unsupported video format " << sparams.GetVideoFormat()
               << std::endl;
        DIRAC_THROW_EXCEPTION(
            ERR_INVALID_VIDEO_FORMAT,
            errstr.str(),
            SEVERITY_FRAME_ERROR);
        break;
    }
}

void SetDefaultEncoderParameters(EncoderParams& encparams)
{
    encparams.SetLossless(false);
    encparams.SetQf(7.0f);
    encparams.SetMVPrecision(MV_PRECISION_QUARTER_PIXEL);

    switch (encparams.GetVideoFormat())
    {
    case VIDEO_FORMAT_4SIF525:
    case VIDEO_FORMAT_4CIF:
    case VIDEO_FORMAT_SD_480I60:
    case VIDEO_FORMAT_SD_576I50:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(7);
        encparams.SetCPD(32.0f);
        break;

    case VIDEO_FORMAT_HD_720P60:
    case VIDEO_FORMAT_HD_720P50:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(15);
        encparams.SetCPD(20.0f);
        break;

    case VIDEO_FORMAT_HD_1080I60:
    case VIDEO_FORMAT_HD_1080I50:
    case VIDEO_FORMAT_HD_1080P60:
    case VIDEO_FORMAT_HD_1080P50:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(7);
        encparams.SetCPD(32.0f);
        break;

    case VIDEO_FORMAT_CIF:
    default:
        encparams.SetL1Sep(3);
        encparams.SetNumL1(19);
        encparams.SetCPD(20.0f);
        break;
    }

}

void SetDefaultBlockParameters(OLBParams& bparams,
                               const VideoFormat& video_format)
{
    switch (video_format)
    {
    case VIDEO_FORMAT_QCIF:
    case VIDEO_FORMAT_QSIF525:
        bparams.SetXblen(8);
        bparams.SetYblen(8);
        bparams.SetXbsep(4);
        bparams.SetYbsep(4);
        break;

    case VIDEO_FORMAT_CUSTOM:
    case VIDEO_FORMAT_SIF525:
    case VIDEO_FORMAT_CIF:
    case VIDEO_FORMAT_4SIF525:
    case VIDEO_FORMAT_4CIF:
    case VIDEO_FORMAT_SD_480I60:
    case VIDEO_FORMAT_SD_576I50:
        bparams.SetXblen(12);
        bparams.SetYblen(12);
        bparams.SetXbsep(8);
        bparams.SetYbsep(8);
        break;

    case VIDEO_FORMAT_HD_720P60:
    case VIDEO_FORMAT_HD_720P50:
        bparams.SetXblen(16);
        bparams.SetYblen(16);
        bparams.SetXbsep(12);
        bparams.SetYbsep(12);
        break;

    case VIDEO_FORMAT_HD_1080I60:
    case VIDEO_FORMAT_HD_1080I50:
    case VIDEO_FORMAT_HD_1080P60:
    case VIDEO_FORMAT_HD_1080P50:
    case VIDEO_FORMAT_DIGI_CINEMA_2K24:
    case VIDEO_FORMAT_DIGI_CINEMA_4K24:
        bparams.SetXblen(24);
        bparams.SetYblen(24);
        bparams.SetXbsep(16);
        bparams.SetYbsep(16);
        break;

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

void SetDefaultTransformFilter(FrameType ftype, WltFilter &wf)
{
    if (ftype == INTRA_FRAME)
        wf = DD9_7;
    else
        wf = LEGALL5_3;
}
}
