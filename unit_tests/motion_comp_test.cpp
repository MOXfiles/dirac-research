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
* The Original Code is Steve Bearcroft's code.
*
* The Initial Developer of the Original Code is Steve Bearcroft.
* Portions created by the Initial Developer are Copyright (C) 2004.
* All Rights Reserved.
*
* Contributor(s): Steve Bearcroft (Original Author)
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
#include "core_suite.h"
#include "motion_comp_test.h"
#include "frames_test.h"

#include <libdirac_common/frame.h>
#include <libdirac_common/frame_buffer.h>
#include <libdirac_common/mot_comp.h>
using namespace dirac;

#include <memory>

//NOTE: ensure that the suite is added to the default registry in
//cppunit_testsuite.cpp
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION (MotionCompTest, coreSuiteName());

#define X_SIZE  352
#define Y_SIZE  288

MotionCompTest::MotionCompTest()
{
}

MotionCompTest::~MotionCompTest()
{
}

void MotionCompTest::setUp()
{
}

void MotionCompTest::tearDown()
{
}


MvData* setupMV1Data(CodecParams& cp, int mv_x, int mv_y, PredMode mode)
{
    MvData* mv_data = new MvData(cp.XNumMB(),cp.YNumMB(),cp.XNumBlocks(),cp.YNumBlocks());
    MvArray& arr = mv_data->Vectors(1);
    for (int i =arr.FirstY(); i <= arr.LastY(); i++)
    {
        for (int j =arr.FirstX(); j <= arr.LastX(); j++)
        {
            arr[i][j].x = mv_x;
            arr[i][j].y = mv_y;
            mv_data->Mode()[i][j] = mode;
        }
    }
    return mv_data;
}

void setupMV2Data(MvData* mv_data, int mv_x, int mv_y)
{
    MvArray& arr = mv_data->Vectors(2);
    for (int i =arr.FirstY(); i <= arr.LastY(); i++)
    {
        for (int j =arr.FirstX(); j <= arr.LastX(); j++)
        {
            arr[i][j].x = mv_x;
            arr[i][j].y = mv_y;
        }
    }
}

void MotionCompTest::testZeroMotionComp()
{
    for (int i = 0; i < 4; ++i)
    {
        testZeroMotionComp(i);
    }
}

void MotionCompTest::testZeroMotionComp(int precision)
{
    FrameBuffer fbuffer(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);
    CodecParams cp;
    OLBParams bparams(12, 12, 8, 8);

    cp.SetMVPrecision(precision);
    cp.SetBlockSizes(bparams, fbuffer.GetFParams().CFormat());
    cp.SetXNumMB( X_SIZE / cp.LumaBParams(0).Xbsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetOrigXl( X_SIZE );
    cp.SetOrigYl( Y_SIZE );

    cp.SetXNumBlocks( 4*cp.XNumMB() );
    cp.SetYNumBlocks( 4*cp.YNumMB() );

    // MotionCompensator mc( cp );
    
    MvData* mv_data = setupMV1Data(cp, 0, 0, REF1_ONLY);

    FrameParams fp(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(0),0);

    fp.SetFSort(L1_frame);
    fp.SetFrameNum(1);
    fp.Refs().push_back(0);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(1));

    fp.SetFrameNum(2);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(2));

    // mc.CompensateFrame(ADD, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 1, *mv_data);

    // MotionCompensator mc2( cp );

    //too many rounding errors for this to be exactly true;
    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(0), fbuffer.GetFrame(1)));
    // mc2.CompensateFrame(SUBTRACT, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, SUBTRACT, fbuffer, 1, *mv_data);

    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(2), fbuffer.GetFrame(1)));
    delete mv_data;
}

void MotionCompTest::testAddandSubMotionComp()
{
    for (int i = 0; i < 4; ++i)
    {
        testAddandSubMotionComp(i);
    }
}

void MotionCompTest::testAddandSubMotionComp(int precision)
{
    FrameBuffer fbuffer(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);
    CodecParams cp;
    OLBParams bparams(12, 12, 8, 8);
    cp.SetMVPrecision(precision);
    cp.SetBlockSizes(bparams, fbuffer.GetFParams().CFormat());
    cp.SetXNumMB( X_SIZE / cp.LumaBParams(0).Xbsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetOrigXl( X_SIZE );
    cp.SetOrigYl( Y_SIZE );

    cp.SetXNumBlocks( 4*cp.XNumMB() );
    cp.SetYNumBlocks( 4*cp.YNumMB() );

    
    MvData* mv_data = setupMV1Data(cp, 5, 5, REF1_ONLY);

    FrameParams fp(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(0),0);

    fp.SetFSort(L1_frame);
    fp.SetFrameNum(1);
    fp.Refs().push_back(0);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(1));

    fp.SetFrameNum(2);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(2));

    // MotionCompensator mc( cp );
    // mc.CompensateFrame(ADD, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 1, *mv_data);

    // MotionCompensator mc2( cp );
    // mc2.CompensateFrame(SUBTRACT, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, SUBTRACT, fbuffer, 1, *mv_data);

    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(2), fbuffer.GetFrame(1)));
    delete mv_data;
}

void MotionCompTest::testL2_frame()
{
    for (int i = 0; i < 4; ++i)
    {
        testL2_frame(i);
    }
}

void MotionCompTest::testL2_frame(int precision)
{
    FrameBuffer fbuffer(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);
    CodecParams cp;
    OLBParams bparams(12, 12, 8, 8);
    cp.SetMVPrecision(precision);
    cp.SetBlockSizes(bparams, fbuffer.GetFParams().CFormat());
    cp.SetXNumMB( X_SIZE / cp.LumaBParams(0).Xbsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetOrigXl( X_SIZE );
    cp.SetOrigYl( Y_SIZE );

    cp.SetXNumBlocks( 4*cp.XNumMB() );
    cp.SetYNumBlocks( 4*cp.YNumMB() );

    
    MvData* mv_data = setupMV1Data(cp, 5, 5, REF1_ONLY);

    FrameParams fp(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(0),0);

    fp.SetFSort(L2_frame);
    fp.SetFrameNum(1);
    fp.Refs().push_back(0);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(1));

    fp.SetFrameNum(2);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(2));

    // MotionCompensator mc( cp );
    // mc.CompensateFrame(ADD, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 1, *mv_data);

    // MotionCompensator mc2( cp );
    // mc2.CompensateFrame(SUBTRACT, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, SUBTRACT, fbuffer, 1, *mv_data);

    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(2), fbuffer.GetFrame(1)));
    delete mv_data;
}


void MotionCompTest::testI_frame()
{
    FrameBuffer fbuffer(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);
    CodecParams cp;
    OLBParams bparams(12, 12, 8, 8);
    cp.SetBlockSizes(bparams, fbuffer.GetFParams().CFormat());
    cp.SetXNumMB( X_SIZE / cp.LumaBParams(0).Xbsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetOrigXl( X_SIZE );
    cp.SetOrigYl( Y_SIZE );

    cp.SetXNumBlocks( 4*cp.XNumMB() );
    cp.SetYNumBlocks( 4*cp.YNumMB() );


    
    MvData* mv_data = setupMV1Data(cp, 5, 5, REF1_ONLY);

    FrameParams fp(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(0),0);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(1);
    fp.Refs().push_back(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(1),0);


    // MotionCompensator mc( cp );
    // mc.CompensateFrame(ADD, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 1, *mv_data);

    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(0), fbuffer.GetFrame(1)));
    delete mv_data;
}


void MotionCompTest::testRef2()
{
    for (int i = 0; i < 4; ++i)
    {
        testRef2(i);
    }
}

void MotionCompTest::testRef2(int precision)
{
    FrameBuffer fbuffer(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);
    CodecParams cp;
    OLBParams bparams(12, 12, 8, 8);
    cp.SetMVPrecision(precision);
    cp.SetBlockSizes(bparams, fbuffer.GetFParams().CFormat());
    cp.SetXNumMB( X_SIZE / cp.LumaBParams(0).Xbsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetOrigXl( X_SIZE );
    cp.SetOrigYl( Y_SIZE );

    cp.SetXNumBlocks( 4*cp.XNumMB() );
    cp.SetYNumBlocks( 4*cp.YNumMB() );


    
    MvData* mv_data = setupMV1Data(cp, 5, 5, REF2_ONLY);
    setupMV2Data(mv_data, 0, 0);

    FrameParams fp(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(0),0);

    fp.SetFSort(L1_frame);
    fp.SetFrameNum(1);
    fp.Refs().push_back(2);
    fp.Refs().push_back(0);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(1));

    fp.SetFrameNum(2);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(2));

    // MotionCompensator mc( cp );
    // mc.CompensateFrame(ADD, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 1, *mv_data);

    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(0), fbuffer.GetFrame(1)));

    // MotionCompensator mc2( cp );
    // mc2.CompensateFrame(SUBTRACT, fbuffer, 1, *mv_data);
    MotionCompensator::CompensateFrame(cp, SUBTRACT, fbuffer, 1, *mv_data);

    CPPUNIT_ASSERT (FramesTest::equalFrames (fbuffer.GetFrame(2), fbuffer.GetFrame(1)));
    delete mv_data;
}

void MotionCompTest::testRef1and2()
{
    for (int i = 0; i < 4; ++i)
    {
        testRef1and2(i);
    }
}

void MotionCompTest::testRef1and2(int precision)
{
    FrameBuffer fbuffer(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);
    CodecParams cp;
    OLBParams bparams(12, 12, 8, 8);
    cp.SetMVPrecision(precision);
    cp.SetBlockSizes(bparams, fbuffer.GetFParams().CFormat());
    cp.SetXNumMB( X_SIZE / cp.LumaBParams(0).Xbsep() );
    cp.SetYNumMB( Y_SIZE / cp.LumaBParams(0).Ybsep() );
    cp.SetOrigXl( X_SIZE );
    cp.SetOrigYl( Y_SIZE );

    cp.SetXNumBlocks( 4*cp.XNumMB() );
    cp.SetYNumBlocks( 4*cp.YNumMB() );
    
    MvData* mv_data = setupMV1Data(cp, 5, 5, REF1_ONLY);
    setupMV2Data(mv_data, 5, 5);

    MvData* mv_data1 = setupMV1Data(cp, 7, 3, REF2_ONLY);
    setupMV2Data(mv_data1, 7, 3);

    MvData* mv_data2 = setupMV1Data(cp, 5, 5, REF1AND2);
    setupMV2Data(mv_data2, 7, 3);

    FrameParams fp(format420, X_SIZE, Y_SIZE, X_SIZE/2, Y_SIZE/2);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(0);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(0),0);

    fp.SetFSort(I_frame);
    fp.SetFrameNum(1);
    fbuffer.PushFrame(fp);
    FramesTest::setupFrame(fbuffer.GetFrame(1),50);

    fp.SetFSort(L1_frame);
    fp.SetFrameNum(2);
    fp.Refs().push_back(0);
    fp.Refs().push_back(1);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(2));

    fp.SetFrameNum(3);
    fbuffer.PushFrame(fp);
    FramesTest::zeroFrame(fbuffer.GetFrame(3));

    //MotionCompensator mc( cp );

    //mc.CompensateFrame(ADD, fbuffer, 2, *mv_data);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 2, *mv_data);

    //MotionCompensator mc2( cp );

    //mc2.CompensateFrame(ADD, fbuffer, 2, *mv_data1);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 2, *mv_data1);

    // MotionCompensator mc3( cp );

    // mc3.CompensateFrame(ADD, fbuffer, 3, *mv_data2);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 3, *mv_data2);

    //MotionCompensator mc4( cp );

    //mc4.CompensateFrame(ADD, fbuffer, 3, *mv_data2);
    MotionCompensator::CompensateFrame(cp, ADD, fbuffer, 3, *mv_data2);

    CPPUNIT_ASSERT (FramesTest::almostEqualFrames (fbuffer.GetFrame(2), fbuffer.GetFrame(3), 5    ));
    delete mv_data;
    delete mv_data1;
    delete mv_data2;
}
