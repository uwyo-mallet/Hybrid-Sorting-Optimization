#include <setjmp.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#define UNIT_TESTING
#include <cmocka.h>

#include "platform.h"
#include "sort.h"

typedef unsigned long ul_t;
typedef struct
{
  volatile char c[4 * 1024];
  int64_t val;
} large_struct;

#define N 512U
#define MIN_THRESH 0U
#define MAX_THRESH 256U
#define ARRAY_SIZE(x) (sizeof(x)) / (sizeof(*x))
static const int random_data_int[N] = {
    -182, -229, -86,  243,  -125, -230, -154, 167,  163,  98,   -4,   201,
    122,  8,    85,   236,  67,   -13,  161,  -204, 252,  139,  199,  -196,
    236,  192,  175,  204,  187,  -242, -146, 142,  172,  40,   165,  63,
    77,   -10,  -9,   19,   -177, 249,  -141, 125,  -81,  21,   -185, 228,
    253,  49,   -191, -162, 116,  -22,  -229, 82,   -182, 194,  -131, 72,
    -252, 218,  -90,  203,  10,   174,  -166, 135,  120,  78,   -249, 47,
    123,  66,   -12,  196,  -243, -66,  144,  -63,  218,  90,   63,   -1,
    107,  -222, 33,   180,  67,   230,  -106, -242, 247,  -206, -41,  -247,
    -139, -205, 233,  41,   45,   208,  -229, -199, -115, 34,   7,    108,
    -243, -194, 73,   71,   -252, 111,  -193, -50,  97,   2,    106,  59,
    -58,  -182, 225,  -181, 83,   -207, -121, 207,  255,  -183, -189, -157,
    -83,  -159, 148,  -174, -9,   -141, -180, 249,  206,  33,   -146, -97,
    -177, -94,  -126, -182, 131,  199,  15,   91,   -25,  119,  125,  42,
    26,   -236, 2,    -155, 140,  -155, 115,  248,  -206, 97,   -180, 160,
    154,  -106, -246, -137, -103, -136, -98,  70,   240,  -26,  -55,  -3,
    114,  -159, -136, -51,  -105, -47,  -96,  181,  110,  -125, 40,   244,
    93,   -183, -118, 31,   -242, -124, -140, 145,  46,   34,   19,   60,
    228,  128,  -133, -149, -246, -203, -78,  -188, 224,  -104, 223,  -145,
    215,  -144, 81,   228,  239,  -21,  -64,  138,  117,  83,   -17,  -175,
    31,   -112, -11,  51,   109,  -137, 5,    248,  68,   79,   70,   17,
    115,  222,  136,  30,   -242, 20,   -111, 233,  -70,  -122, -92,  214,
    -252, -181, -202, -199, -235, -44,  -162, -134, -106, 9,    -160, 183,
    238,  -165, 35,   62,   -148, 70,   -223, -185, 148,  -83,  -243, 87,
    -246, 123,  -237, 55,   -186, 193,  -108, 117,  38,   183,  94,   176,
    222,  45,   -43,  -121, 245,  -38,  -92,  176,  -231, 69,   -153, 16,
    -133, 75,   -101, 190,  109,  -241, 1,    -162, -213, -171, 34,   49,
    233,  -35,  -255, 161,  31,   -59,  -11,  -62,  -80,  43,   83,   219,
    247,  -27,  -207, 137,  45,   -151, -5,   213,  176,  183,  -211, 11,
    -34,  239,  46,   -238, 80,   -109, -197, -180, -102, -227, -23,  -70,
    250,  169,  -96,  256,  -19,  -239, 16,   122,  174,  84,   -238, -56,
    152,  115,  -149, -205, 208,  -121, 201,  173,  6,    -209, -11,  -215,
    -204, 144,  -161, 26,   207,  15,   -80,  182,  -225, -231, 125,  208,
    2,    -212, 129,  206,  -232, -72,  -8,   -198, -184, -232, -104, 29,
    17,   -155, 127,  -220, -4,   -48,  60,   84,   235,  193,  18,   156,
    -123, 143,  -238, -32,  -33,  124,  -70,  -56,  37,   -30,  36,   -26,
    234,  231,  -55,  186,  -41,  50,   -201, -95,  192,  183,  106,  214,
    -162, -120, -222, 37,   -212, 117,  -246, 21,   24,   152,  224,  62,
    -88,  -154, -33,  -93,  -172, -101, 162,  -243, -219, -250, -12,  120,
    186,  247,  196,  -218, 51,   118,  -59,  33,   -57,  83,   72,   -13,
    -187, 51,   -40,  0,    -88,  -87,  -24,  238,  105,  250,  -176, 28,
    -79,  -170, -31,  119,  -44,  152,  -110, -235, 206,  -229, 141,  -169,
    225,  228,  -210, 158,  -39,  -199, 73,   -193, 41,   -220, 229,  51,
    36,   4,    -82,  58,   200,  233,  -214, -250};

static const int32_t random_data_int32_t[N] = {
    -182, -229, -86,  243,  -125, -230, -154, 167,  163,  98,   -4,   201,
    122,  8,    85,   236,  67,   -13,  161,  -204, 252,  139,  199,  -196,
    236,  192,  175,  204,  187,  -242, -146, 142,  172,  40,   165,  63,
    77,   -10,  -9,   19,   -177, 249,  -141, 125,  -81,  21,   -185, 228,
    253,  49,   -191, -162, 116,  -22,  -229, 82,   -182, 194,  -131, 72,
    -252, 218,  -90,  203,  10,   174,  -166, 135,  120,  78,   -249, 47,
    123,  66,   -12,  196,  -243, -66,  144,  -63,  218,  90,   63,   -1,
    107,  -222, 33,   180,  67,   230,  -106, -242, 247,  -206, -41,  -247,
    -139, -205, 233,  41,   45,   208,  -229, -199, -115, 34,   7,    108,
    -243, -194, 73,   71,   -252, 111,  -193, -50,  97,   2,    106,  59,
    -58,  -182, 225,  -181, 83,   -207, -121, 207,  255,  -183, -189, -157,
    -83,  -159, 148,  -174, -9,   -141, -180, 249,  206,  33,   -146, -97,
    -177, -94,  -126, -182, 131,  199,  15,   91,   -25,  119,  125,  42,
    26,   -236, 2,    -155, 140,  -155, 115,  248,  -206, 97,   -180, 160,
    154,  -106, -246, -137, -103, -136, -98,  70,   240,  -26,  -55,  -3,
    114,  -159, -136, -51,  -105, -47,  -96,  181,  110,  -125, 40,   244,
    93,   -183, -118, 31,   -242, -124, -140, 145,  46,   34,   19,   60,
    228,  128,  -133, -149, -246, -203, -78,  -188, 224,  -104, 223,  -145,
    215,  -144, 81,   228,  239,  -21,  -64,  138,  117,  83,   -17,  -175,
    31,   -112, -11,  51,   109,  -137, 5,    248,  68,   79,   70,   17,
    115,  222,  136,  30,   -242, 20,   -111, 233,  -70,  -122, -92,  214,
    -252, -181, -202, -199, -235, -44,  -162, -134, -106, 9,    -160, 183,
    238,  -165, 35,   62,   -148, 70,   -223, -185, 148,  -83,  -243, 87,
    -246, 123,  -237, 55,   -186, 193,  -108, 117,  38,   183,  94,   176,
    222,  45,   -43,  -121, 245,  -38,  -92,  176,  -231, 69,   -153, 16,
    -133, 75,   -101, 190,  109,  -241, 1,    -162, -213, -171, 34,   49,
    233,  -35,  -255, 161,  31,   -59,  -11,  -62,  -80,  43,   83,   219,
    247,  -27,  -207, 137,  45,   -151, -5,   213,  176,  183,  -211, 11,
    -34,  239,  46,   -238, 80,   -109, -197, -180, -102, -227, -23,  -70,
    250,  169,  -96,  256,  -19,  -239, 16,   122,  174,  84,   -238, -56,
    152,  115,  -149, -205, 208,  -121, 201,  173,  6,    -209, -11,  -215,
    -204, 144,  -161, 26,   207,  15,   -80,  182,  -225, -231, 125,  208,
    2,    -212, 129,  206,  -232, -72,  -8,   -198, -184, -232, -104, 29,
    17,   -155, 127,  -220, -4,   -48,  60,   84,   235,  193,  18,   156,
    -123, 143,  -238, -32,  -33,  124,  -70,  -56,  37,   -30,  36,   -26,
    234,  231,  -55,  186,  -41,  50,   -201, -95,  192,  183,  106,  214,
    -162, -120, -222, 37,   -212, 117,  -246, 21,   24,   152,  224,  62,
    -88,  -154, -33,  -93,  -172, -101, 162,  -243, -219, -250, -12,  120,
    186,  247,  196,  -218, 51,   118,  -59,  33,   -57,  83,   72,   -13,
    -187, 51,   -40,  0,    -88,  -87,  -24,  238,  105,  250,  -176, 28,
    -79,  -170, -31,  119,  -44,  152,  -110, -235, 206,  -229, 141,  -169,
    225,  228,  -210, 158,  -39,  -199, 73,   -193, 41,   -220, 229,  51,
    36,   4,    -82,  58,   200,  233,  -214, -250};

static const int64_t random_data_int64_t[N] = {
    -182, -229, -86,  243,  -125, -230, -154, 167,  163,  98,   -4,   201,
    122,  8,    85,   236,  67,   -13,  161,  -204, 252,  139,  199,  -196,
    236,  192,  175,  204,  187,  -242, -146, 142,  172,  40,   165,  63,
    77,   -10,  -9,   19,   -177, 249,  -141, 125,  -81,  21,   -185, 228,
    253,  49,   -191, -162, 116,  -22,  -229, 82,   -182, 194,  -131, 72,
    -252, 218,  -90,  203,  10,   174,  -166, 135,  120,  78,   -249, 47,
    123,  66,   -12,  196,  -243, -66,  144,  -63,  218,  90,   63,   -1,
    107,  -222, 33,   180,  67,   230,  -106, -242, 247,  -206, -41,  -247,
    -139, -205, 233,  41,   45,   208,  -229, -199, -115, 34,   7,    108,
    -243, -194, 73,   71,   -252, 111,  -193, -50,  97,   2,    106,  59,
    -58,  -182, 225,  -181, 83,   -207, -121, 207,  255,  -183, -189, -157,
    -83,  -159, 148,  -174, -9,   -141, -180, 249,  206,  33,   -146, -97,
    -177, -94,  -126, -182, 131,  199,  15,   91,   -25,  119,  125,  42,
    26,   -236, 2,    -155, 140,  -155, 115,  248,  -206, 97,   -180, 160,
    154,  -106, -246, -137, -103, -136, -98,  70,   240,  -26,  -55,  -3,
    114,  -159, -136, -51,  -105, -47,  -96,  181,  110,  -125, 40,   244,
    93,   -183, -118, 31,   -242, -124, -140, 145,  46,   34,   19,   60,
    228,  128,  -133, -149, -246, -203, -78,  -188, 224,  -104, 223,  -145,
    215,  -144, 81,   228,  239,  -21,  -64,  138,  117,  83,   -17,  -175,
    31,   -112, -11,  51,   109,  -137, 5,    248,  68,   79,   70,   17,
    115,  222,  136,  30,   -242, 20,   -111, 233,  -70,  -122, -92,  214,
    -252, -181, -202, -199, -235, -44,  -162, -134, -106, 9,    -160, 183,
    238,  -165, 35,   62,   -148, 70,   -223, -185, 148,  -83,  -243, 87,
    -246, 123,  -237, 55,   -186, 193,  -108, 117,  38,   183,  94,   176,
    222,  45,   -43,  -121, 245,  -38,  -92,  176,  -231, 69,   -153, 16,
    -133, 75,   -101, 190,  109,  -241, 1,    -162, -213, -171, 34,   49,
    233,  -35,  -255, 161,  31,   -59,  -11,  -62,  -80,  43,   83,   219,
    247,  -27,  -207, 137,  45,   -151, -5,   213,  176,  183,  -211, 11,
    -34,  239,  46,   -238, 80,   -109, -197, -180, -102, -227, -23,  -70,
    250,  169,  -96,  256,  -19,  -239, 16,   122,  174,  84,   -238, -56,
    152,  115,  -149, -205, 208,  -121, 201,  173,  6,    -209, -11,  -215,
    -204, 144,  -161, 26,   207,  15,   -80,  182,  -225, -231, 125,  208,
    2,    -212, 129,  206,  -232, -72,  -8,   -198, -184, -232, -104, 29,
    17,   -155, 127,  -220, -4,   -48,  60,   84,   235,  193,  18,   156,
    -123, 143,  -238, -32,  -33,  124,  -70,  -56,  37,   -30,  36,   -26,
    234,  231,  -55,  186,  -41,  50,   -201, -95,  192,  183,  106,  214,
    -162, -120, -222, 37,   -212, 117,  -246, 21,   24,   152,  224,  62,
    -88,  -154, -33,  -93,  -172, -101, 162,  -243, -219, -250, -12,  120,
    186,  247,  196,  -218, 51,   118,  -59,  33,   -57,  83,   72,   -13,
    -187, 51,   -40,  0,    -88,  -87,  -24,  238,  105,  250,  -176, 28,
    -79,  -170, -31,  119,  -44,  152,  -110, -235, 206,  -229, 141,  -169,
    225,  228,  -210, 158,  -39,  -199, 73,   -193, 41,   -220, 229,  51,
    36,   4,    -82,  58,   200,  233,  -214, -250};

static int compare_large_struct(const void* a, const void* b)
{
  large_struct* A = (large_struct*)a;
  large_struct* B = (large_struct*)b;
  if (A->val < B->val) return -1;
  if (A->val > B->val) return 1;
  return 0;
}

#define COMPARE(TYPE)                                     \
  static int compare_##TYPE(const void* a, const void* b) \
  {                                                       \
    TYPE A = *(TYPE*)a;                                   \
    TYPE B = *(TYPE*)b;                                   \
    if (A < B) return -1;                                 \
    if (A > B) return 1;                                  \
    return 0;                                             \
  }
COMPARE(char)
COMPARE(int)
COMPARE(int32_t)
COMPARE(int64_t)
COMPARE(unsigned)
COMPARE(ul_t)
COMPARE(float)
COMPARE(double)

#define TEST_EMPTY(FUNC)                            \
  static void test_empty_##FUNC(void** state)       \
  {                                                 \
    (void)state;                                    \
    char expected_mem[] = {'a', 'b', 'c', 'd'};     \
    const size_t n = ARRAY_SIZE(expected_mem);      \
    char* test_mem[n];                              \
    memcpy(test_mem, expected_mem, n);              \
    FUNC(test_mem, 1, sizeof(char), compare_char);  \
    assert_memory_equal(test_mem, expected_mem, n); \
  }

#define TEST_THRESH_EMPTY(FUNC)                        \
  static void test_thresh_empty_##FUNC(void** state)   \
  {                                                    \
    (void)state;                                       \
    char expected_mem[] = {'a', 'b', 'c', 'd'};        \
    const size_t n = ARRAY_SIZE(expected_mem);         \
    char* test_mem[n];                                 \
    memcpy(test_mem, expected_mem, n);                 \
    FUNC(test_mem, 1, sizeof(char), compare_char, 42); \
    assert_memory_equal(test_mem, expected_mem, n);    \
  }

#define TEST_SIGNED_ASCENDING(FUNC, TYPE)                          \
  static void test_signed_ascending_##TYPE##_##FUNC(void** state)  \
  {                                                                \
    (void)state;                                                   \
    TYPE expected_mem[N];                                          \
    TYPE test_mem[N];                                              \
    for (int i = -255; i < 256; ++i)                               \
    {                                                              \
      const size_t index = i + 255;                                \
      test_mem[index] = i;                                         \
    }                                                              \
    memcpy(expected_mem, test_mem, N * sizeof(TYPE));              \
    qsort(expected_mem, N, sizeof(TYPE), compare_##TYPE);          \
    FUNC(test_mem, N, sizeof(TYPE), compare_##TYPE);               \
    assert_memory_equal(test_mem, expected_mem, N * sizeof(TYPE)); \
  }

#define TEST_SIGNED_DESCENDING(FUNC, TYPE)                         \
  static void test_signed_descending_##TYPE##_##FUNC(void** state) \
  {                                                                \
    (void)state;                                                   \
    TYPE expected_mem[N];                                          \
    TYPE test_mem[N];                                              \
    for (int i = 256; i > -255; --i)                               \
    {                                                              \
      const size_t index = (-i) + 256;                             \
      test_mem[index] = i;                                         \
    }                                                              \
    memcpy(expected_mem, test_mem, N * sizeof(TYPE));              \
    qsort(expected_mem, N, sizeof(TYPE), compare_##TYPE);          \
    FUNC(test_mem, N, sizeof(TYPE), compare_##TYPE);               \
    assert_memory_equal(test_mem, expected_mem, N * sizeof(TYPE)); \
  }

#define TEST_SIGNED_RANDOM(FUNC, TYPE)                             \
  static void test_signed_random_##TYPE##_##FUNC(void** state)     \
  {                                                                \
    (void)state;                                                   \
    const TYPE* data = random_data_##TYPE;                         \
    TYPE expected_mem[N];                                          \
    TYPE test_mem[N];                                              \
    memcpy(test_mem, data, N * sizeof(TYPE));                      \
    memcpy(expected_mem, data, N * sizeof(TYPE));                  \
    qsort(expected_mem, N, sizeof(TYPE), compare_##TYPE);          \
    FUNC(test_mem, N, sizeof(TYPE), compare_##TYPE);               \
    assert_memory_equal(test_mem, expected_mem, N * sizeof(TYPE)); \
  }

#define TEST_THRESH_SIGNED_ASCENDING(FUNC, TYPE)                         \
  static void test_thresh_signed_ascending_##TYPE##_##FUNC(void** state) \
  {                                                                      \
    (void)state;                                                         \
    TYPE expected_mem[N];                                                \
    TYPE test_mem[N];                                                    \
    TYPE working_mem[N];                                                 \
    for (int i = -255; i < 256; ++i)                                     \
    {                                                                    \
      const size_t index = i + 255;                                      \
      test_mem[index] = i;                                               \
    }                                                                    \
    memcpy(expected_mem, test_mem, N * sizeof(TYPE));                    \
    qsort(expected_mem, N, sizeof(TYPE), compare_##TYPE);                \
    for (size_t i = MIN_THRESH; i < MAX_THRESH; ++i)                     \
    {                                                                    \
      memcpy(working_mem, test_mem, N * sizeof(TYPE));                   \
      FUNC(working_mem, N, sizeof(TYPE), compare_##TYPE, i);             \
      assert_memory_equal(working_mem, expected_mem, N * sizeof(TYPE));  \
    }                                                                    \
  }

#define TEST_THRESH_SIGNED_DESCENDING(FUNC, TYPE)                         \
  static void test_thresh_signed_descending_##TYPE##_##FUNC(void** state) \
  {                                                                       \
    (void)state;                                                          \
    TYPE expected_mem[N];                                                 \
    TYPE test_mem[N];                                                     \
    TYPE working_mem[N];                                                  \
    for (int i = 256; i > -255; --i)                                      \
    {                                                                     \
      const size_t index = (-i) + 256;                                    \
      test_mem[index] = i;                                                \
    }                                                                     \
    memcpy(expected_mem, test_mem, N * sizeof(TYPE));                     \
    qsort(expected_mem, N, sizeof(TYPE), compare_##TYPE);                 \
    for (size_t i = MIN_THRESH; i < MAX_THRESH; ++i)                      \
    {                                                                     \
      memcpy(working_mem, test_mem, N * sizeof(TYPE));                    \
      FUNC(working_mem, N, sizeof(TYPE), compare_##TYPE, i);              \
      assert_memory_equal(working_mem, expected_mem, N * sizeof(TYPE));   \
    }                                                                     \
  }

#define TEST_THRESH_SIGNED_RANDOM(FUNC, TYPE)                           \
  static void test_thresh_signed_random_##TYPE##_##FUNC(void** state)   \
  {                                                                     \
    (void)state;                                                        \
    const TYPE* data = random_data_##TYPE;                              \
    TYPE expected_mem[N];                                               \
    TYPE working_mem[N];                                                \
    memcpy(expected_mem, data, N * sizeof(TYPE));                       \
    qsort(expected_mem, N, sizeof(TYPE), compare_##TYPE);               \
    for (size_t i = MIN_THRESH; i < MAX_THRESH; ++i)                    \
    {                                                                   \
      memcpy(working_mem, data, N * sizeof(TYPE));                      \
      FUNC(working_mem, N, sizeof(TYPE), compare_##TYPE, i);            \
      assert_memory_equal(working_mem, expected_mem, N * sizeof(TYPE)); \
    }                                                                   \
  }

#define TEST_LARGE_STRUCT_ASCENDING(FUNC)                                  \
  static void test_large_struct_ascending_##FUNC(void** state)             \
  {                                                                        \
    (void)state;                                                           \
    large_struct expected_mem[N];                                          \
    large_struct test_mem[N];                                              \
    for (int i = -255; i < 256; ++i)                                       \
    {                                                                      \
      const size_t index = i + 255;                                        \
      test_mem[index].val = i;                                             \
    }                                                                      \
    memcpy(expected_mem, test_mem, N * sizeof(large_struct));              \
    qsort(expected_mem, N, sizeof(large_struct), compare_large_struct);    \
    FUNC(test_mem, N, sizeof(large_struct), compare_large_struct);         \
    assert_memory_equal(test_mem, expected_mem, N * sizeof(large_struct)); \
  }

#define TEST_LARGE_STRUCT_DESCENDING(FUNC)                                 \
  static void test_large_struct_descending_##FUNC(void** state)            \
  {                                                                        \
    (void)state;                                                           \
    large_struct expected_mem[N];                                          \
    large_struct test_mem[N];                                              \
    for (int i = 256; i > -255; --i)                                       \
    {                                                                      \
      const size_t index = (-i) + 256;                                     \
      test_mem[index].val = i;                                             \
    }                                                                      \
    memcpy(expected_mem, test_mem, N * sizeof(large_struct));              \
    qsort(expected_mem, N, sizeof(large_struct), compare_large_struct);    \
    FUNC(test_mem, N, sizeof(large_struct), compare_large_struct);         \
    assert_memory_equal(test_mem, expected_mem, N * sizeof(large_struct)); \
  }

#define TEST_LARGE_STRUCT_RANDOM(FUNC)                                     \
  static void test_large_struct_random_##FUNC(void** state)                \
  {                                                                        \
    (void)state;                                                           \
    const int64_t* data = random_data_int64_t;                             \
    large_struct expected_mem[N];                                          \
    large_struct test_mem[N];                                              \
    for (size_t i = 0; i < N; ++i)                                         \
    {                                                                      \
      test_mem[i].val = data[i];                                           \
    }                                                                      \
    memcpy(expected_mem, test_mem, N * sizeof(large_struct));              \
    qsort(expected_mem, N, sizeof(large_struct), compare_large_struct);    \
    FUNC(test_mem, N, sizeof(large_struct), compare_large_struct);         \
    assert_memory_equal(test_mem, expected_mem, N * sizeof(large_struct)); \
  }

#define TEST_THRESH_LARGE_STRUCT_ASCENDING(FUNC)                           \
  static void test_thresh_large_struct_ascending_##FUNC(void** state)      \
  {                                                                        \
    (void)state;                                                           \
    large_struct expected_mem[N];                                          \
    large_struct test_mem[N];                                              \
    large_struct working_mem[N];                                           \
    for (int i = -255; i < 256; ++i)                                       \
    {                                                                      \
      const size_t index = i + 255;                                        \
      test_mem[index].val = i;                                             \
    }                                                                      \
    memcpy(expected_mem, test_mem, N * sizeof(large_struct));              \
    qsort(expected_mem, N, sizeof(large_struct), compare_large_struct);    \
    for (size_t i = MIN_THRESH; i < MAX_THRESH; ++i)                       \
    {                                                                      \
      memcpy(working_mem, test_mem, N * sizeof(large_struct));             \
      FUNC(working_mem, N, sizeof(large_struct), compare_large_struct, i); \
      assert_memory_equal(                                                 \
          working_mem, expected_mem, N * sizeof(large_struct));            \
    }                                                                      \
  }

#define TEST_THRESH_LARGE_STRUCT_DESCENDING(FUNC)                          \
  static void test_thresh_large_struct_descending_##FUNC(void** state)     \
  {                                                                        \
    (void)state;                                                           \
    large_struct expected_mem[N];                                          \
    large_struct test_mem[N];                                              \
    large_struct working_mem[N];                                           \
    for (int i = 256; i > -255; --i)                                       \
    {                                                                      \
      const size_t index = (-i) + 256;                                     \
      test_mem[index].val = i;                                             \
    }                                                                      \
    memcpy(expected_mem, test_mem, N * sizeof(large_struct));              \
    qsort(expected_mem, N, sizeof(large_struct), compare_large_struct);    \
    for (size_t i = MIN_THRESH; i < MAX_THRESH; ++i)                       \
    {                                                                      \
      memcpy(working_mem, test_mem, N * sizeof(large_struct));             \
      FUNC(working_mem, N, sizeof(large_struct), compare_large_struct, i); \
      assert_memory_equal(                                                 \
          working_mem, expected_mem, N * sizeof(large_struct));            \
    }                                                                      \
  }

#define TEST_THRESH_LARGE_STRUCT_RANDOM(FUNC)                              \
  static void test_thresh_large_struct_random_##FUNC(void** state)         \
  {                                                                        \
    (void)state;                                                           \
    large_struct expected_mem[N];                                          \
    large_struct test_mem[N];                                              \
    large_struct working_mem[N];                                           \
    const int64_t* data = random_data_int64_t;                             \
    for (size_t i = 0; i < N; ++i)                                         \
    {                                                                      \
      test_mem[i].val = data[i];                                           \
    }                                                                      \
    memcpy(expected_mem, test_mem, N * sizeof(large_struct));              \
    qsort(expected_mem, N, sizeof(large_struct), compare_large_struct);    \
    for (size_t i = MIN_THRESH; i < MAX_THRESH; ++i)                       \
    {                                                                      \
      memcpy(working_mem, test_mem, N * sizeof(large_struct));             \
      FUNC(working_mem, N, sizeof(large_struct), compare_large_struct, i); \
      assert_memory_equal(                                                 \
          working_mem, expected_mem, N * sizeof(large_struct));            \
    }                                                                      \
  }

/* Empty (n <= 1) */
TEST_EMPTY(qsort)
TEST_EMPTY(msort_heap)
TEST_EMPTY(basic_ins_sort)
TEST_EMPTY(fast_ins_sort)
TEST_EMPTY(shell_sort)
TEST_THRESH_EMPTY(msort_heap_with_old_ins)
TEST_THRESH_EMPTY(msort_heap_with_basic_ins)
TEST_THRESH_EMPTY(msort_heap_with_shell)
TEST_THRESH_EMPTY(msort_heap_with_fast_ins)
TEST_THRESH_EMPTY(msort_heap_with_network)
TEST_THRESH_EMPTY(msort_with_network)
TEST_THRESH_EMPTY(quicksort_with_ins)
TEST_THRESH_EMPTY(quicksort_with_fast_ins)

/* int ascending */
TEST_SIGNED_ASCENDING(qsort, int)
TEST_SIGNED_ASCENDING(msort_heap, int)
TEST_SIGNED_ASCENDING(basic_ins_sort, int)
TEST_SIGNED_ASCENDING(fast_ins_sort, int)
TEST_SIGNED_ASCENDING(shell_sort, int)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_old_ins, int)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_basic_ins, int)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_shell, int)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_fast_ins, int)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_network, int)
TEST_THRESH_SIGNED_ASCENDING(msort_with_network, int)
TEST_THRESH_SIGNED_ASCENDING(quicksort_with_ins, int)
TEST_THRESH_SIGNED_ASCENDING(quicksort_with_fast_ins, int)

/* int descending */
TEST_SIGNED_DESCENDING(qsort, int)
TEST_SIGNED_DESCENDING(msort_heap, int)
TEST_SIGNED_DESCENDING(basic_ins_sort, int)
TEST_SIGNED_DESCENDING(fast_ins_sort, int)
TEST_SIGNED_DESCENDING(shell_sort, int)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_old_ins, int)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_basic_ins, int)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_shell, int)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_fast_ins, int)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_network, int)
TEST_THRESH_SIGNED_DESCENDING(msort_with_network, int)
TEST_THRESH_SIGNED_DESCENDING(quicksort_with_ins, int)
TEST_THRESH_SIGNED_DESCENDING(quicksort_with_fast_ins, int)

/* int random */
TEST_SIGNED_RANDOM(qsort, int)
TEST_SIGNED_RANDOM(msort_heap, int)
TEST_SIGNED_RANDOM(basic_ins_sort, int)
TEST_SIGNED_RANDOM(fast_ins_sort, int)
TEST_SIGNED_RANDOM(shell_sort, int)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_old_ins, int)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_basic_ins, int)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_shell, int)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_fast_ins, int)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_network, int)
TEST_THRESH_SIGNED_RANDOM(msort_with_network, int)
TEST_THRESH_SIGNED_RANDOM(quicksort_with_ins, int)
TEST_THRESH_SIGNED_RANDOM(quicksort_with_fast_ins, int)

/* int32_t ascending */
TEST_SIGNED_ASCENDING(qsort, int32_t)
TEST_SIGNED_ASCENDING(msort_heap, int32_t)
TEST_SIGNED_ASCENDING(basic_ins_sort, int32_t)
TEST_SIGNED_ASCENDING(fast_ins_sort, int32_t)
TEST_SIGNED_ASCENDING(shell_sort, int32_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_old_ins, int32_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_basic_ins, int32_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_shell, int32_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_fast_ins, int32_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_network, int32_t)
TEST_THRESH_SIGNED_ASCENDING(msort_with_network, int32_t)
TEST_THRESH_SIGNED_ASCENDING(quicksort_with_ins, int32_t)
TEST_THRESH_SIGNED_ASCENDING(quicksort_with_fast_ins, int32_t)

/* int32_t descending */
TEST_SIGNED_DESCENDING(qsort, int32_t)
TEST_SIGNED_DESCENDING(msort_heap, int32_t)
TEST_SIGNED_DESCENDING(basic_ins_sort, int32_t)
TEST_SIGNED_DESCENDING(fast_ins_sort, int32_t)
TEST_SIGNED_DESCENDING(shell_sort, int32_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_old_ins, int32_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_basic_ins, int32_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_shell, int32_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_fast_ins, int32_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_network, int32_t)
TEST_THRESH_SIGNED_DESCENDING(msort_with_network, int32_t)
TEST_THRESH_SIGNED_DESCENDING(quicksort_with_ins, int32_t)
TEST_THRESH_SIGNED_DESCENDING(quicksort_with_fast_ins, int32_t)

/* int32_t random */
TEST_SIGNED_RANDOM(qsort, int32_t)
TEST_SIGNED_RANDOM(msort_heap, int32_t)
TEST_SIGNED_RANDOM(basic_ins_sort, int32_t)
TEST_SIGNED_RANDOM(fast_ins_sort, int32_t)
TEST_SIGNED_RANDOM(shell_sort, int32_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_old_ins, int32_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_basic_ins, int32_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_shell, int32_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_fast_ins, int32_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_network, int32_t)
TEST_THRESH_SIGNED_RANDOM(msort_with_network, int32_t)
TEST_THRESH_SIGNED_RANDOM(quicksort_with_ins, int32_t)
TEST_THRESH_SIGNED_RANDOM(quicksort_with_fast_ins, int32_t)

/* int64_t ascending */
TEST_SIGNED_ASCENDING(qsort, int64_t)
TEST_SIGNED_ASCENDING(msort_heap, int64_t)
TEST_SIGNED_ASCENDING(basic_ins_sort, int64_t)
TEST_SIGNED_ASCENDING(fast_ins_sort, int64_t)
TEST_SIGNED_ASCENDING(shell_sort, int64_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_old_ins, int64_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_basic_ins, int64_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_shell, int64_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_fast_ins, int64_t)
TEST_THRESH_SIGNED_ASCENDING(msort_heap_with_network, int64_t)
TEST_THRESH_SIGNED_ASCENDING(msort_with_network, int64_t)
TEST_THRESH_SIGNED_ASCENDING(quicksort_with_ins, int64_t)
TEST_THRESH_SIGNED_ASCENDING(quicksort_with_fast_ins, int64_t)

/* int64_t descending */
TEST_SIGNED_DESCENDING(qsort, int64_t)
TEST_SIGNED_DESCENDING(msort_heap, int64_t)
TEST_SIGNED_DESCENDING(basic_ins_sort, int64_t)
TEST_SIGNED_DESCENDING(fast_ins_sort, int64_t)
TEST_SIGNED_DESCENDING(shell_sort, int64_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_old_ins, int64_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_basic_ins, int64_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_shell, int64_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_fast_ins, int64_t)
TEST_THRESH_SIGNED_DESCENDING(msort_heap_with_network, int64_t)
TEST_THRESH_SIGNED_DESCENDING(msort_with_network, int64_t)
TEST_THRESH_SIGNED_DESCENDING(quicksort_with_ins, int64_t)
TEST_THRESH_SIGNED_DESCENDING(quicksort_with_fast_ins, int64_t)

/* int64_t random */
TEST_SIGNED_RANDOM(qsort, int64_t)
TEST_SIGNED_RANDOM(msort_heap, int64_t)
TEST_SIGNED_RANDOM(basic_ins_sort, int64_t)
TEST_SIGNED_RANDOM(fast_ins_sort, int64_t)
TEST_SIGNED_RANDOM(shell_sort, int64_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_old_ins, int64_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_basic_ins, int64_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_shell, int64_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_fast_ins, int64_t)
TEST_THRESH_SIGNED_RANDOM(msort_heap_with_network, int64_t)
TEST_THRESH_SIGNED_RANDOM(msort_with_network, int64_t)
TEST_THRESH_SIGNED_RANDOM(quicksort_with_ins, int64_t)
TEST_THRESH_SIGNED_RANDOM(quicksort_with_fast_ins, int64_t)

/* large_struct ascending */
TEST_LARGE_STRUCT_ASCENDING(qsort)
TEST_LARGE_STRUCT_ASCENDING(msort_heap)
TEST_LARGE_STRUCT_ASCENDING(basic_ins_sort)
TEST_LARGE_STRUCT_ASCENDING(fast_ins_sort)
TEST_LARGE_STRUCT_ASCENDING(shell_sort)
TEST_THRESH_LARGE_STRUCT_ASCENDING(msort_heap_with_old_ins)
TEST_THRESH_LARGE_STRUCT_ASCENDING(msort_heap_with_basic_ins)
TEST_THRESH_LARGE_STRUCT_ASCENDING(msort_heap_with_shell)
TEST_THRESH_LARGE_STRUCT_ASCENDING(msort_heap_with_fast_ins)
TEST_THRESH_LARGE_STRUCT_ASCENDING(msort_heap_with_network)
TEST_THRESH_LARGE_STRUCT_ASCENDING(msort_with_network)
TEST_THRESH_LARGE_STRUCT_ASCENDING(quicksort_with_ins)
TEST_THRESH_LARGE_STRUCT_ASCENDING(quicksort_with_fast_ins)

/* large_struct descending */
TEST_LARGE_STRUCT_DESCENDING(qsort)
TEST_LARGE_STRUCT_DESCENDING(msort_heap)
TEST_LARGE_STRUCT_DESCENDING(basic_ins_sort)
TEST_LARGE_STRUCT_DESCENDING(fast_ins_sort)
TEST_LARGE_STRUCT_DESCENDING(shell_sort)
TEST_THRESH_LARGE_STRUCT_DESCENDING(msort_heap_with_old_ins)
TEST_THRESH_LARGE_STRUCT_DESCENDING(msort_heap_with_basic_ins)
TEST_THRESH_LARGE_STRUCT_DESCENDING(msort_heap_with_shell)
TEST_THRESH_LARGE_STRUCT_DESCENDING(msort_heap_with_fast_ins)
TEST_THRESH_LARGE_STRUCT_DESCENDING(msort_heap_with_network)
TEST_THRESH_LARGE_STRUCT_DESCENDING(msort_with_network)
TEST_THRESH_LARGE_STRUCT_DESCENDING(quicksort_with_ins)
TEST_THRESH_LARGE_STRUCT_DESCENDING(quicksort_with_fast_ins)

/* large_struct random */
TEST_LARGE_STRUCT_RANDOM(qsort)
TEST_LARGE_STRUCT_RANDOM(msort_heap)
TEST_LARGE_STRUCT_RANDOM(basic_ins_sort)
TEST_LARGE_STRUCT_RANDOM(fast_ins_sort)
TEST_LARGE_STRUCT_RANDOM(shell_sort)
TEST_THRESH_LARGE_STRUCT_RANDOM(msort_heap_with_old_ins)
TEST_THRESH_LARGE_STRUCT_RANDOM(msort_heap_with_basic_ins)
TEST_THRESH_LARGE_STRUCT_RANDOM(msort_heap_with_shell)
TEST_THRESH_LARGE_STRUCT_RANDOM(msort_heap_with_fast_ins)
TEST_THRESH_LARGE_STRUCT_RANDOM(msort_heap_with_network)
TEST_THRESH_LARGE_STRUCT_RANDOM(msort_with_network)
TEST_THRESH_LARGE_STRUCT_RANDOM(quicksort_with_ins)
TEST_THRESH_LARGE_STRUCT_RANDOM(quicksort_with_fast_ins)

int main()
{
  // clang-format off
  const struct CMUnitTest tests[] = {
      /* Empty (n <= 1) */
      cmocka_unit_test(test_empty_qsort),
      cmocka_unit_test(test_empty_msort_heap),
      cmocka_unit_test(test_empty_basic_ins_sort),
      cmocka_unit_test(test_empty_fast_ins_sort),
      cmocka_unit_test(test_empty_shell_sort),
      cmocka_unit_test(test_thresh_empty_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_empty_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_empty_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_empty_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_empty_msort_heap_with_network),
      cmocka_unit_test(test_thresh_empty_msort_with_network),
      cmocka_unit_test(test_thresh_empty_quicksort_with_ins),
      cmocka_unit_test(test_thresh_empty_quicksort_with_fast_ins),

      /* int ascending */
      cmocka_unit_test(test_signed_ascending_int_qsort),
      cmocka_unit_test(test_signed_ascending_int_msort_heap),
      cmocka_unit_test(test_signed_ascending_int_basic_ins_sort),
      cmocka_unit_test(test_signed_ascending_int_fast_ins_sort),
      cmocka_unit_test(test_signed_ascending_int_shell_sort),
      cmocka_unit_test(test_thresh_signed_ascending_int_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_ascending_int_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_ascending_int_msort_with_network),
      cmocka_unit_test(test_thresh_signed_ascending_int_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int_quicksort_with_fast_ins),

      /* int descending */
      cmocka_unit_test(test_signed_descending_int_qsort),
      cmocka_unit_test(test_signed_descending_int_msort_heap),
      cmocka_unit_test(test_signed_descending_int_basic_ins_sort),
      cmocka_unit_test(test_signed_descending_int_fast_ins_sort),
      cmocka_unit_test(test_signed_descending_int_shell_sort),
      cmocka_unit_test(test_thresh_signed_descending_int_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_descending_int_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_descending_int_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_descending_int_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_descending_int_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_descending_int_msort_with_network),
      cmocka_unit_test(test_thresh_signed_descending_int_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_descending_int_quicksort_with_fast_ins),

      /* int random */
      cmocka_unit_test(test_signed_random_int_qsort),
      cmocka_unit_test(test_signed_random_int_msort_heap),
      cmocka_unit_test(test_signed_random_int_basic_ins_sort),
      cmocka_unit_test(test_signed_random_int_fast_ins_sort),
      cmocka_unit_test(test_signed_random_int_shell_sort),
      cmocka_unit_test(test_thresh_signed_random_int_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_random_int_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_random_int_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_random_int_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_random_int_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_random_int_msort_with_network),
      cmocka_unit_test(test_thresh_signed_random_int_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_random_int_quicksort_with_fast_ins),

      /* int32_t ascending */
      cmocka_unit_test(test_signed_ascending_int32_t_qsort),
      cmocka_unit_test(test_signed_ascending_int32_t_msort_heap),
      cmocka_unit_test(test_signed_ascending_int32_t_basic_ins_sort),
      cmocka_unit_test(test_signed_ascending_int32_t_fast_ins_sort),
      cmocka_unit_test(test_signed_ascending_int32_t_shell_sort),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_msort_with_network),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int32_t_quicksort_with_fast_ins),

      /* int32_t descending */
      cmocka_unit_test(test_signed_descending_int32_t_qsort),
      cmocka_unit_test(test_signed_descending_int32_t_msort_heap),
      cmocka_unit_test(test_signed_descending_int32_t_basic_ins_sort),
      cmocka_unit_test(test_signed_descending_int32_t_fast_ins_sort),
      cmocka_unit_test(test_signed_descending_int32_t_shell_sort),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_msort_with_network),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_descending_int32_t_quicksort_with_fast_ins),

      /* int32_t random */
      cmocka_unit_test(test_signed_random_int32_t_qsort),
      cmocka_unit_test(test_signed_random_int32_t_msort_heap),
      cmocka_unit_test(test_signed_random_int32_t_basic_ins_sort),
      cmocka_unit_test(test_signed_random_int32_t_fast_ins_sort),
      cmocka_unit_test(test_signed_random_int32_t_shell_sort),
      cmocka_unit_test(test_thresh_signed_random_int32_t_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_random_int32_t_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_random_int32_t_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_random_int32_t_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_random_int32_t_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_random_int32_t_msort_with_network),
      cmocka_unit_test(test_thresh_signed_random_int32_t_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_random_int32_t_quicksort_with_fast_ins),

      /* int64_t ascending */
      cmocka_unit_test(test_signed_ascending_int64_t_qsort),
      cmocka_unit_test(test_signed_ascending_int64_t_msort_heap),
      cmocka_unit_test(test_signed_ascending_int64_t_basic_ins_sort),
      cmocka_unit_test(test_signed_ascending_int64_t_fast_ins_sort),
      cmocka_unit_test(test_signed_ascending_int64_t_shell_sort),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_msort_with_network),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_ascending_int64_t_quicksort_with_fast_ins),

      /* int64_t descending */
      cmocka_unit_test(test_signed_descending_int64_t_qsort),
      cmocka_unit_test(test_signed_descending_int64_t_msort_heap),
      cmocka_unit_test(test_signed_descending_int64_t_basic_ins_sort),
      cmocka_unit_test(test_signed_descending_int64_t_fast_ins_sort),
      cmocka_unit_test(test_signed_descending_int64_t_shell_sort),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_msort_with_network),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_descending_int64_t_quicksort_with_fast_ins),

      /* int64_t random */
      cmocka_unit_test(test_signed_random_int64_t_qsort),
      cmocka_unit_test(test_signed_random_int64_t_msort_heap),
      cmocka_unit_test(test_signed_random_int64_t_basic_ins_sort),
      cmocka_unit_test(test_signed_random_int64_t_fast_ins_sort),
      cmocka_unit_test(test_signed_random_int64_t_shell_sort),
      cmocka_unit_test(test_thresh_signed_random_int64_t_msort_heap_with_old_ins),
      cmocka_unit_test(test_thresh_signed_random_int64_t_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_signed_random_int64_t_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_signed_random_int64_t_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_signed_random_int64_t_msort_heap_with_network),
      cmocka_unit_test(test_thresh_signed_random_int64_t_msort_with_network),
      cmocka_unit_test(test_thresh_signed_random_int64_t_quicksort_with_ins),
      cmocka_unit_test(test_thresh_signed_random_int64_t_quicksort_with_fast_ins),

      /* large_struct ascending */
      cmocka_unit_test(test_large_struct_ascending_qsort),
      cmocka_unit_test(test_large_struct_ascending_msort_heap),
      cmocka_unit_test(test_large_struct_ascending_basic_ins_sort),
      cmocka_unit_test(test_large_struct_ascending_fast_ins_sort),
      cmocka_unit_test(test_large_struct_ascending_shell_sort),
      /* cmocka_unit_test(test_thresh_large_struct_ascending_msort_heap_with_old_ins), */
      cmocka_unit_test(test_thresh_large_struct_ascending_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_large_struct_ascending_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_large_struct_ascending_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_large_struct_ascending_msort_heap_with_network),
      cmocka_unit_test(test_thresh_large_struct_ascending_msort_with_network),
      cmocka_unit_test(test_thresh_large_struct_ascending_quicksort_with_ins),
      cmocka_unit_test(test_thresh_large_struct_ascending_quicksort_with_fast_ins),

      /* large_struct descending */
      cmocka_unit_test(test_large_struct_descending_qsort),
      cmocka_unit_test(test_large_struct_descending_msort_heap),
      cmocka_unit_test(test_large_struct_descending_basic_ins_sort),
      cmocka_unit_test(test_large_struct_descending_fast_ins_sort),
      cmocka_unit_test(test_large_struct_descending_shell_sort),
      /* cmocka_unit_test(test_thresh_large_struct_descending_msort_heap_with_old_ins), */
      cmocka_unit_test(test_thresh_large_struct_descending_msort_heap_with_basic_ins),
      cmocka_unit_test(test_thresh_large_struct_descending_msort_heap_with_shell),
      cmocka_unit_test(test_thresh_large_struct_descending_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_large_struct_descending_msort_heap_with_network),
      cmocka_unit_test(test_thresh_large_struct_descending_msort_with_network),
      cmocka_unit_test(test_thresh_large_struct_descending_quicksort_with_ins),
      cmocka_unit_test(test_thresh_large_struct_descending_quicksort_with_fast_ins),

      /* large_struct random */
      cmocka_unit_test(test_large_struct_random_qsort),
      cmocka_unit_test(test_large_struct_random_msort_heap),
      cmocka_unit_test(test_large_struct_random_basic_ins_sort),
      cmocka_unit_test(test_large_struct_random_fast_ins_sort),
      /* cmocka_unit_test(test_large_struct_random_shell_sort), */
      /* cmocka_unit_test(test_thresh_large_struct_random_msort_heap_with_old_ins), */
      cmocka_unit_test(test_thresh_large_struct_random_msort_heap_with_basic_ins),
      /* cmocka_unit_test(test_thresh_large_struct_random_msort_heap_with_shell), */
      cmocka_unit_test(test_thresh_large_struct_random_msort_heap_with_fast_ins),
      cmocka_unit_test(test_thresh_large_struct_random_msort_heap_with_network),
      cmocka_unit_test(test_thresh_large_struct_random_msort_with_network),
      /* cmocka_unit_test(test_thresh_large_struct_random_quicksort_with_ins), */
      /* cmocka_unit_test(test_thresh_large_struct_random_quicksort_with_fast_ins), */
  };
  // clang-format on

  return cmocka_run_group_tests(tests, NULL, NULL);
}
