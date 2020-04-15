#include "test_LOG.h"
#include "log.h"
#include "nvhtm.h"

#include <string>

using namespace std;

CPPUNIT_TEST_SUITE_REGISTRATION(test_LOG);

static GRANULE_TYPE *pool;

test_LOG::test_LOG()
{
}

test_LOG::~test_LOG()
{
}

void test_LOG::setUp()
{
}

void test_LOG::tearDown()
{
}

void test_LOG::testLogSize()
{
  ostringstream msg;

  size_t max_size, new_size;

  max_size = 12345;
  new_size = LOG_base2_before(max_size);

  msg << "Expected " << 8192 << " but obtained " << new_size;
  CPPUNIT_ASSERT_MESSAGE(msg.str(), 8192 == new_size);

  max_size = 67890;
  new_size = LOG_base2_before(max_size);

  CPPUNIT_ASSERT(65536 == new_size);
}

void test_LOG::testLogInit()
{
  void *log_pool = malloc(12345);
  NVLog_s *log;
  size_t expected_size = LOG_base2_before(
    (12345-sizeof(NVLog_s))/sizeof(NVLogEntry_s)
  );

  log = LOG_init_1thread(log_pool, 12345);

  CPPUNIT_ASSERT(log->size_of_log == expected_size);

  free(log_pool);
}

void test_LOG::testLogMod()
{
  int start, end, dist, mod;
  mod = 128; // base 2

  start = 10;
  end   = 20;

  dist = LOG_DISTANCE2(start, end, mod);
  CPPUNIT_ASSERT(dist == 10);

  start = 120;
  end   = 2;

  dist = LOG_DISTANCE2(start, end, mod);
  CPPUNIT_ASSERT(dist == 10);
}

void test_LOG::testLogInsert()
{
}

void test_LOG::testLogCheckpoint()
{
}

void test_LOG::testFixLogs()
{
}

void test_LOG::testFixLogs2()
{
}
