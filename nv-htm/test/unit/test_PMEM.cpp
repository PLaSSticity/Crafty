#include "test_PMEM.h"
#include "log.h"
#include "nvhtm_helper.h"

CPPUNIT_TEST_SUITE_REGISTRATION(test_PMEM);

static GRANULE_TYPE *pool;
static GRANULE_D_TYPE *pool2;

// TODO: broken with avni

#define SIZE_POOL (NVMHTM_LOG_SIZE * sizeof (GRANULE_TYPE))

test_PMEM::test_PMEM()
{
}

test_PMEM::~test_PMEM()
{
}

void test_PMEM::setUp()
{
}

void test_PMEM::tearDown()
{
}

void test_PMEM::testWriteD()
{
}

void test_PMEM::testRollback()
{
}

void test_PMEM::testCheckpoint()
{
}

void test_PMEM::testMalloc()
{
}
