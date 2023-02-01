#ifndef PLATFORM_H_
#define PLATFORM_H_

#ifdef _WIN32
#error "Windows not yet supported."
#endif  // _WIN32

#ifdef __unix__
#include <argp.h>
#include <inttypes.h>
#include <sys/times.h>
#include <time.h>
#include <unistd.h>

#ifdef UNIT_TESTING
extern void* _test_malloc(const size_t size, const char* file, const int line);
extern void* _test_calloc(const size_t number_of_elements, const size_t size,
                          const char* file, const int line);
extern void _test_free(void* const ptr, const char* file, const int line);

#undef malloc
#undef calloc
#undef free

#define malloc(size) _test_malloc(size, __FILE__, __LINE__)
#define calloc(num, size) _test_calloc(num, size, __FILE__, __LINE__)
#define free(ptr) _test_free(ptr, __FILE__, __LINE__)
#endif  // UNIT_TESTING

struct times
{
  clock_t user;
  clock_t system;
  time_t wall_secs;
  intmax_t wall_nsecs;
};

struct times get_times();
struct times elapsed(struct times* start, struct times* end);
#else
#error "Unsupported system type."
#endif  // linux

#endif  // PLATFORM_H_
