#ifndef ARRAYS_TEST_H
#define ARRAYS_TEST_H
#include <cppunit/extensions/HelperMacros.h>

class TwoDArraysTest : public CPPUNIT_NS::TestFixture
{
  CPPUNIT_TEST_SUITE( TwoDArraysTest );
  CPPUNIT_TEST( testConstructor );
  CPPUNIT_TEST( testDefaultConstructor );
  CPPUNIT_TEST( testCopyConstructor );
  CPPUNIT_TEST( testAssignment );
  CPPUNIT_TEST( testResize );
  CPPUNIT_TEST_SUITE_END();

public:
  TwoDArraysTest();
  virtual ~TwoDArraysTest();

  virtual void setUp();
  virtual void tearDown();

  void testConstructor();
  void testDefaultConstructor();
  void testCopyConstructor();
  void testAssignment();
  void testResize();

private:
  TwoDArraysTest( const TwoDArraysTest &copy );
  void operator =( const TwoDArraysTest &copy );
private:
};
#endif
