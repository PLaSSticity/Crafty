#ifndef TEST_PMEM_H
#define TEST_PMEM_H

#include <cppunit/extensions/HelperMacros.h>

class test_PMEM : public CPPUNIT_NS::TestFixture {
    CPPUNIT_TEST_SUITE(test_PMEM);
    CPPUNIT_TEST(testWriteD);
    CPPUNIT_TEST(testRollback);
    CPPUNIT_TEST(testCheckpoint);
    CPPUNIT_TEST(testMalloc);
    CPPUNIT_TEST_SUITE_END();

public:
    test_PMEM();
    virtual ~test_PMEM();
    void setUp();
    void tearDown();

private:
	void testWriteD();
    void testRollback();
    void testCheckpoint();
	void testMalloc();
};

#endif /* TEST_PMEM_H */

