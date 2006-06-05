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
#include "frames_test.h"
#include "arrays_test.h"

#include <libdirac_common/frame.h>
using namespace dirac;

#include <memory>

//NOTE: ensure that the suite is added to the default registry in
//cppunit_testsuite.cpp
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION (FramesTest, coreSuiteName());

void FramesTest::setupFrame (Frame& frame, int start_val)
{
    setupPicArray(frame.Ydata(), start_val);
    setupPicArray(frame.Udata(), start_val);
    setupPicArray(frame.Vdata(), start_val);
}

bool FramesTest::setupPicArray (PicArray &arr, int start_val)
{
    char value =start_val; // use char to limit values to 8 bits
    int err_count = 0;
    int i, j;
    for ( i =arr.FirstY(); i <= arr.LastY(); i++)
    {
        for ( j =arr.FirstX(); j <= arr.LastX(); j++)
        {
            arr[i][j] = ++value;
        }
    }
    value = start_val;
    for ( i =arr.FirstY(); i <= arr.LastY(); i++)
    {
        for ( j =arr.FirstX(); j <= arr.LastX(); j++)
        {
            ++value;
            if (arr[i][j] != value)
                err_count++;
        }
    }
    CPPUNIT_ASSERT_EQUAL (err_count, 0);
    return true;
}

void FramesTest::zeroFrame (Frame& frame)
{
    zeroPicArray(frame.Ydata());
    zeroPicArray(frame.Udata());
    zeroPicArray(frame.Vdata());
}

bool FramesTest::zeroPicArray (PicArray &arr)
{
    short value =0;
    int err_count = 0;
    int i, j;
    for ( i =arr.FirstY(); i <= arr.LastY(); i++)
    {
        for ( j =arr.FirstX(); j <= arr.LastX(); j++)
        {
            arr[i][j] = value;
        }
    }
    value = 0;
    for ( i =arr.FirstY(); i <= arr.LastY(); i++)
    {
        for ( j =arr.FirstX(); j <= arr.LastX(); j++)
        {
            if (arr[i][j] != value)
                err_count++;
        }
    }
    CPPUNIT_ASSERT_EQUAL (err_count, 0);
    return true;
}


bool FramesTest::equalPicArrays (const PicArray &lhs, const PicArray &rhs)
{
    CPPUNIT_ASSERT_EQUAL (lhs.CSort(), rhs.CSort());
    CPPUNIT_ASSERT_EQUAL (lhs.LengthX(), rhs.LengthX());
    CPPUNIT_ASSERT_EQUAL (lhs.LengthY(), rhs.LengthY());
    CPPUNIT_ASSERT_EQUAL (lhs.FirstX(), rhs.FirstX());
    CPPUNIT_ASSERT_EQUAL (lhs.FirstY(), rhs.FirstY());
    CPPUNIT_ASSERT_EQUAL (lhs.LastX(), rhs.LastX() );
    CPPUNIT_ASSERT_EQUAL (lhs.LastY(), rhs.LastY() );

    for (int i =lhs.FirstY(); i <= lhs.LastY(); i++)
    {
        ValueType * lshRow = lhs[i];
        ValueType * rshRow = rhs[i];
        for (int j =lhs.FirstX(); j <= lhs.LastX(); j++)
        {
            if (!( lshRow[j] == rshRow[j]))
            {
                return false;
            }
        }
    }

    return true;
}


bool FramesTest::almostEqualPicArrays (const PicArray &lhs, const PicArray &rhs, int allowedError)
{
    CPPUNIT_ASSERT_EQUAL (lhs.CSort(), rhs.CSort());
    CPPUNIT_ASSERT_EQUAL (lhs.LengthX(), rhs.LengthX());
    CPPUNIT_ASSERT_EQUAL (lhs.LengthY(), rhs.LengthY());
    CPPUNIT_ASSERT_EQUAL (lhs.FirstX(), rhs.FirstX());
    CPPUNIT_ASSERT_EQUAL (lhs.FirstY(), rhs.FirstY());
    CPPUNIT_ASSERT_EQUAL (lhs.LastX(), rhs.LastX() );
    CPPUNIT_ASSERT_EQUAL (lhs.LastY(), rhs.LastY() );

    for (int i =lhs.FirstY(); i <= lhs.LastY(); i++)
    {
        ValueType * lshRow = lhs[i];
        ValueType * rshRow = rhs[i];
        for (int j =lhs.FirstX(); j <= lhs.LastX(); j++)
        {
            if ( allowedError < std::abs(lshRow[j] - rshRow[j]))
            {
                return false;
            }
        }
    }

    return true;
}

bool FramesTest::equalFrames (const Frame &lhs, const Frame &rhs)
{
    CPPUNIT_ASSERT_EQUAL (lhs.GetFparams().CFormat(), rhs.GetFparams().CFormat() );
    CPPUNIT_ASSERT (equalPicArrays(lhs.Ydata(), rhs.Ydata()));
    CPPUNIT_ASSERT (equalPicArrays(lhs.Udata(), rhs.Udata()));
    CPPUNIT_ASSERT (equalPicArrays(lhs.Vdata(), rhs.Vdata()));
    CPPUNIT_ASSERT_EQUAL (lhs.GetFparams().GetVideoDepth(), rhs.GetFparams().GetVideoDepth() );

    return true;
}

bool FramesTest::almostEqualFrames (const Frame &lhs, const Frame &rhs, int allowedError)
{
    CPPUNIT_ASSERT_EQUAL (lhs.GetFparams().CFormat(), rhs.GetFparams().CFormat() );
    CPPUNIT_ASSERT (almostEqualPicArrays(lhs.Ydata(), rhs.Ydata(), allowedError));
    CPPUNIT_ASSERT (almostEqualPicArrays(lhs.Udata(), rhs.Udata(), allowedError));
    CPPUNIT_ASSERT (almostEqualPicArrays(lhs.Vdata(), rhs.Vdata(), allowedError));

    return true;
}

FramesTest::FramesTest()
{
}

FramesTest::~FramesTest()
{
}

void FramesTest::setUp()
{
}

void FramesTest::tearDown()
{
}

void FramesTest::testConstructor()
{
    FrameParams f_params(format444, 20, 30, 20, 30, 8);
    Frame frame(f_params);

    CPPUNIT_ASSERT_EQUAL (20, frame.Ydata().LengthX());
    CPPUNIT_ASSERT_EQUAL (30, frame.Ydata().LengthY());
    CPPUNIT_ASSERT_EQUAL (20, frame.Ydata().LastX() - frame.Ydata().FirstX() + 1);
    CPPUNIT_ASSERT_EQUAL (30, frame.Ydata().LastY() - frame.Ydata().FirstY() + 1);
}

void FramesTest::testDefaultFParam()
{
    FrameParams f_params;
    Frame frame(f_params);

    CPPUNIT_ASSERT_EQUAL (0, frame.Ydata().LengthX());
    CPPUNIT_ASSERT_EQUAL (0, frame.Ydata().LengthY());
    CPPUNIT_ASSERT_EQUAL (0, frame.Ydata().FirstX());
    CPPUNIT_ASSERT_EQUAL (0, frame.Ydata().FirstY());
    CPPUNIT_ASSERT_EQUAL (-1, frame.Ydata().LastX());
    CPPUNIT_ASSERT_EQUAL (-1, frame.Ydata().LastY());
}

void FramesTest::testCopyConstructor()
{
    FrameParams f_params(format444, 20, 30, 20, 30, 8);
    Frame frame(f_params);
    setupFrame(frame, 0);
    
    Frame frame_copy(frame);
    CPPUNIT_ASSERT (equalFrames (frame, frame_copy));
}

void FramesTest::testAssignment()
{
    FrameParams f_params(format444, 20, 30, 20, 30, 8);
    Frame frame(f_params);
    setupFrame(frame, 0);

    FrameParams f_params_copy(format444,10,10, 10, 10, 8);
    Frame frame_copy(f_params_copy);

    frame_copy = frame;

    CPPUNIT_ASSERT (equalFrames (frame, frame_copy));
}

