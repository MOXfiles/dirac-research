#include "core_suite.h"
#include "arrays_test.h"
#include <libdirac_common/arrays.h>
#include <memory>

//NOTE: ensure that the suite is added to the default registry in
//cppunit_testsuite.cpp
CPPUNIT_TEST_SUITE_NAMED_REGISTRATION (TwoDArraysTest, coreSuiteName());

bool equalArrays (const TwoDArray<int> &lhs, const TwoDArray<int> &rhs)
{
	CPPUNIT_ASSERT_EQUAL (lhs.LengthX(), rhs.LengthX());
	CPPUNIT_ASSERT_EQUAL (lhs.LengthY(), rhs.LengthY());
	CPPUNIT_ASSERT_EQUAL (lhs.FirstX(), rhs.FirstX());
	CPPUNIT_ASSERT_EQUAL (lhs.FirstY(), rhs.FirstY());
	CPPUNIT_ASSERT_EQUAL (lhs.LastX(), rhs.LastX() );
	CPPUNIT_ASSERT_EQUAL (lhs.LastY(), rhs.LastY() );

	int value = 0;
	for (int i =lhs.FirstY(); i <= lhs.LastY(); i++)
	{
		for (int j =lhs.FirstX(); j <= lhs.LastX(); j++)
		{
			++value;
			if (!((lhs[i][j] == value) && lhs[i][j] == rhs[i][j]))
				return false;
		}
	}

	return true;
}

void setup2DArray (TwoDArray<int> &arr, int dimx, int dimy, int start_val)
{
	arr.Resize(dimx, dimy);
	int value =start_val;
	int err_count = 0;
	for (int i =arr.FirstY(); i <= arr.LastY(); i++)
	{
		for (int j =arr.FirstX(); j <= arr.LastX(); j++)
		{
			arr[i][j] = ++value;
		}
	}
	value = start_val;
	for (int i =arr.FirstY(); i <= arr.LastY(); i++)
	{
		for (int j =arr.FirstX(); j <= arr.LastX(); j++)
		{
			++value;
			if (arr[i][j] != value)
				err_count++;
		}
	}
	CPPUNIT_ASSERT_EQUAL (err_count, 0);
}

TwoDArraysTest::TwoDArraysTest()
{
}

TwoDArraysTest::~TwoDArraysTest()
{
}

void TwoDArraysTest::setUp()
{
}

void TwoDArraysTest::tearDown()
{
}

void TwoDArraysTest::testConstructor()
{
	TwoDArray<int> work_data(20, 30);

	CPPUNIT_ASSERT_EQUAL (work_data.LengthY(), 20);
	CPPUNIT_ASSERT_EQUAL (work_data.LengthX(), 30);
	CPPUNIT_ASSERT_EQUAL (work_data.LastX() - work_data.FirstX() + 1, 30);
	CPPUNIT_ASSERT_EQUAL (work_data.LastY() - work_data.FirstY() + 1, 20);
}

void TwoDArraysTest::testDefaultConstructor()
{
	TwoDArray<int> work_data;

	CPPUNIT_ASSERT_EQUAL (work_data.LengthX(), 0);
	CPPUNIT_ASSERT_EQUAL (work_data.LengthY(), 0);
	CPPUNIT_ASSERT_EQUAL (work_data.FirstX(), 0);
	CPPUNIT_ASSERT_EQUAL (work_data.FirstY(), 0);
	CPPUNIT_ASSERT_EQUAL (work_data.LastX(), -1);
	CPPUNIT_ASSERT_EQUAL (work_data.LastY(), -1);
}

void TwoDArraysTest::testCopyConstructor()
{
	TwoDArray<int> work_data;
	setup2DArray (work_data, 20, 30, 0);
	
	TwoDArray<int> work_copy(work_data);
	bool ret_val = equalArrays (work_data, work_copy);
	CPPUNIT_ASSERT (ret_val == true);
}

void TwoDArraysTest::testAssignment()
{
	TwoDArray<int> work_data;
	setup2DArray (work_data, 20, 30, 0);

	TwoDArray<int> work_copy;

	work_copy = work_data;
	bool ret_val = equalArrays (work_data, work_copy);
	CPPUNIT_ASSERT (ret_val == true);
}

void TwoDArraysTest::testResize()
{
	TwoDArray<int> work_data(20, 30);

	CPPUNIT_ASSERT_EQUAL (work_data.LengthX(), 30);
	CPPUNIT_ASSERT_EQUAL (work_data.LengthY(), 20);
	CPPUNIT_ASSERT_EQUAL (work_data.LastX() - work_data.FirstX() + 1, 30);
	CPPUNIT_ASSERT_EQUAL (work_data.LastY() - work_data.FirstY() + 1, 20);
	work_data.Resize(30, 20);
	CPPUNIT_ASSERT_EQUAL (work_data.LengthX(), 20);
	CPPUNIT_ASSERT_EQUAL (work_data.LengthY(), 30);
	CPPUNIT_ASSERT_EQUAL (work_data.LastX() - work_data.FirstX() + 1, 20);
	CPPUNIT_ASSERT_EQUAL (work_data.LastY() - work_data.FirstY() + 1, 30);
}
