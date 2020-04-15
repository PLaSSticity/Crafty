#ifndef TEST_LOG_H
#define TEST_LOG_H

#include <cppunit/extensions/HelperMacros.h>

class test_LOG : public CPPUNIT_NS::TestFixture {
  CPPUNIT_TEST_SUITE(test_LOG);
  CPPUNIT_TEST(testLogSize);
  CPPUNIT_TEST(testLogInit);
  CPPUNIT_TEST(testLogMod);
  CPPUNIT_TEST_SUITE_END();

public:
  test_LOG();
  virtual ~test_LOG();
  void setUp();
  void tearDown();

private:
  void testLogSize();
  void testLogInit();
  void testLogMod();
  void testLogInsert();
  void testLogCheckpoint();
  void testFixLogs();
  void testFixLogs2();
};

#endif /* TEST_LOG_H */
