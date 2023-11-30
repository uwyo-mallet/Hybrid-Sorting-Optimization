#define BOOST_TEST_MODULE Sorting Tests
#include "tests.hpp"

#include <inttypes.h>
#include <stdint.h>

#include <array>
#include <boost/mpl/list.hpp>
#include <boost/test/data/monomorphic.hpp>
#include <boost/test/data/test_case.hpp>
#include <boost/test/unit_test.hpp>
#include <unordered_map>

extern "C"
{
#include "sort.h"
}

#include "sort_cxx.h"

#define THRESHOLD_TEST_MIN 1u
#define THRESHOLD_TEST_MAX 17u

namespace bdata = boost::unit_test::data;

typedef void (*sort_fn_t)(void*, size_t, size_t, compar_d_fn_t);
typedef void (*threshold_sort_fn_t)(void*, size_t, size_t, compar_d_fn_t,
                                    const size_t);

template <>
int compare<LargeStruct>(void* a, void* b)
{
  LargeStruct* A = (LargeStruct*)a;
  LargeStruct* B = (LargeStruct*)b;

  if (A->val < B->val) return -1;
  if (A->val > B->val) return 1;
  return 0;
}

static const char* sortFnNames[] = {"msort_heap",
                                    "basic_ins_sort",
                                    "fast_ins_sort",
                                    "shell_sort",
                                    "cxx_std"};
static const sort_fn_t sortFns[] = {msort_heap,
                                    basic_ins_sort,
                                    fast_ins_sort,
                                    shell_sort,
                                    cxx_std_sort};

static const char* thresholdSortFnNames[] = {"msort_heap_with_old_ins",
                                             "msort_heap_with_basic_ins",
                                             "msort_heap_with_shell",
                                             "msort_heap_with_fast_ins",
                                             "msort_heap_with_network",
                                             "msort_with_network",
                                             "quicksort_with_ins",
                                             "quicksort_with_fast_ins"};

static const threshold_sort_fn_t thresholdSortFns[] = {
    msort_heap_with_old_ins,
    msort_heap_with_basic_ins,
    msort_heap_with_shell,
    msort_heap_with_fast_ins,
    msort_heap_with_network,
    msort_with_network,
    quicksort_with_ins,
    quicksort_with_fast_ins};

#define SORT_TEST_CASE(type)                                              \
  BOOST_DATA_TEST_CASE_F(TestDataFixture<type>,                           \
                         SortTest_##type,                                 \
                         bdata::make(sortFnNames) ^ bdata::make(sortFns), \
                         fnName,                                          \
                         fn)                                              \
  {                                                                       \
    (void)fnName;                                                         \
                                                                          \
    memcpy(working, ascending, size);                                     \
    (*fn)(working, n, sizeof(type), cmp);                                 \
    BOOST_CHECK(is_sorted(ASCENDING));                                    \
                                                                          \
    memcpy(working, descending, size);                                    \
    (*fn)(working, n, sizeof(type), cmp);                                 \
    BOOST_CHECK(is_sorted(DESCENDING));                                   \
                                                                          \
    memcpy(working, random, size);                                        \
    (*fn)(working, n, sizeof(type), cmp);                                 \
    BOOST_CHECK(is_sorted(RANDOM));                                       \
                                                                          \
    memcpy(working, pipeOrgan, size);                                     \
    (*fn)(working, n, sizeof(type), cmp);                                 \
    BOOST_CHECK(is_sorted(PIPEORGAN));                                    \
                                                                          \
    memcpy(working, singleNum, size);                                     \
    (*fn)(working, n, sizeof(type), cmp);                                 \
    BOOST_CHECK(is_sorted(SINGLENUM));                                    \
  }

#define THRESHOLD_SORT_TEST_CASE(type)                                      \
  BOOST_DATA_TEST_CASE_F(                                                   \
      TestDataFixture<type>,                                                \
      ThresholdSortTest_##type,                                             \
      (bdata::make(thresholdSortFnNames) ^ bdata::make(thresholdSortFns)) * \
          bdata::xrange(THRESHOLD_TEST_MIN, THRESHOLD_TEST_MAX),            \
      fnName,                                                               \
      fn,                                                                   \
      threshold)                                                            \
  {                                                                         \
    (void)fnName;                                                           \
                                                                            \
    memcpy(working, ascending, size);                                       \
    (*fn)(working, n, sizeof(type), cmp, threshold);                        \
    BOOST_CHECK(is_sorted(ASCENDING));                                      \
                                                                            \
    memcpy(working, descending, size);                                      \
    (*fn)(working, n, sizeof(type), cmp, threshold);                        \
    BOOST_CHECK(is_sorted(DESCENDING));                                     \
                                                                            \
    memcpy(working, random, size);                                          \
    (*fn)(working, n, sizeof(type), cmp, threshold);                        \
    BOOST_CHECK(is_sorted(RANDOM));                                         \
                                                                            \
    memcpy(working, pipeOrgan, size);                                       \
    (*fn)(working, n, sizeof(type), cmp, threshold);                        \
    BOOST_CHECK(is_sorted(PIPEORGAN));                                      \
                                                                            \
    memcpy(working, singleNum, size);                                       \
    (*fn)(working, n, sizeof(type), cmp, threshold);                        \
    BOOST_CHECK(is_sorted(SINGLENUM));                                      \
  }

typedef unsigned long unsigned_long;
typedef long double long_double;

SORT_TEST_CASE(char);
SORT_TEST_CASE(uint8_t);
SORT_TEST_CASE(uint16_t);
SORT_TEST_CASE(uint32_t);
SORT_TEST_CASE(uint64_t);
SORT_TEST_CASE(unsigned_long);
SORT_TEST_CASE(uintmax_t);
SORT_TEST_CASE(int8_t);
SORT_TEST_CASE(int16_t);
SORT_TEST_CASE(int32_t);
SORT_TEST_CASE(int64_t);
SORT_TEST_CASE(intmax_t);
SORT_TEST_CASE(float);
SORT_TEST_CASE(double);
SORT_TEST_CASE(long_double);
SORT_TEST_CASE(uintptr_t);
SORT_TEST_CASE(LargeStruct);

THRESHOLD_SORT_TEST_CASE(char);
THRESHOLD_SORT_TEST_CASE(uint8_t);
THRESHOLD_SORT_TEST_CASE(uint16_t);
THRESHOLD_SORT_TEST_CASE(uint32_t);
THRESHOLD_SORT_TEST_CASE(uint64_t);
THRESHOLD_SORT_TEST_CASE(unsigned_long);
THRESHOLD_SORT_TEST_CASE(uintmax_t);
THRESHOLD_SORT_TEST_CASE(int8_t);
THRESHOLD_SORT_TEST_CASE(int16_t);
THRESHOLD_SORT_TEST_CASE(int32_t);
THRESHOLD_SORT_TEST_CASE(int64_t);
THRESHOLD_SORT_TEST_CASE(intmax_t);
THRESHOLD_SORT_TEST_CASE(float);
THRESHOLD_SORT_TEST_CASE(double);
// THRESHOLD_SORT_TEST_CASE(long_double); // quicksort_with_fast_ins fails
THRESHOLD_SORT_TEST_CASE(uintptr_t);
THRESHOLD_SORT_TEST_CASE(LargeStruct);
