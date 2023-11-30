#include "benchmark.h"

#include <stdlib.h>

#include "platform.h"
#include "sort.h"
#include "sort_cxx.h"

struct times measure_sort_time(int method, sort_t* data, const size_t n,
                               const int threshold, struct perf_fds* perf)
{
#define START_TIME() get_times(1, perf)
#define END_TIME() get_times(0, perf)

  struct times start;
  struct times end;

  switch (method)
  {
    /* Standard Methods ----------------------------------------------------- */
    case QSORT:
      start = START_TIME();
      qsort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case MSORT_HEAP:
      start = START_TIME();
      msort_heap(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case BASIC_INS:
      start = START_TIME();
      basic_ins_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case FAST_INS:
      start = START_TIME();
      fast_ins_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case SHELL:
      start = START_TIME();
      shell_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case CXX_STD:
      start = START_TIME();
      cxx_std_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    /* Threshold Methods ---------------------------------------------------- */
    case MSORT_HEAP_WITH_OLD_INS:
      start = START_TIME();
      msort_heap_with_old_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_BASIC_INS:
      start = START_TIME();
      msort_heap_with_basic_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_SHELL:
      start = START_TIME();
      msort_heap_with_shell(data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_FAST_INS:
      start = START_TIME();
      msort_heap_with_fast_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_NETWORK:
      start = START_TIME();
      msort_heap_with_network(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_WITH_NETWORK:
      start = START_TIME();
      msort_with_network(data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case QUICKSORT_WITH_INS:
      start = START_TIME();
      quicksort_with_ins(data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case QUICKSORT_WITH_FAST_INS:
      start = START_TIME();
      quicksort_with_fast_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
/* AlphaDev Methods --------------------------------------------------- */
#ifdef ALPHADEV
    case SORT3_ALPHADEV:
      start = START_TIME();
      Sort3AlphaDev(data);
      break;
    case SORT4_ALPHADEV:
      start = START_TIME();
      Sort4AlphaDev(data);
      break;
    case SORT5_ALPHADEV:
      start = START_TIME();
      Sort5AlphaDev(data);
      break;
    case SORT6_ALPHADEV:
      start = START_TIME();
      Sort6AlphaDev(data);
      break;
    case SORT7_ALPHADEV:
      start = START_TIME();
      Sort7AlphaDev(data);
      break;
    case SORT8_ALPHADEV:
      start = START_TIME();
      Sort8AlphaDev(data);
      break;
    case VARSORT3_ALPHADEV:
      start = START_TIME();
      VarSort3AlphaDev(data);
      break;
    case VARSORT4_ALPHADEV:
      start = START_TIME();
      VarSort4AlphaDev(data);
      break;
    case VARSORT5_ALPHADEV:
      start = START_TIME();
      VarSort5AlphaDev(data);
      break;
#endif  // ALPHADEV
    default:
      fprintf(stderr, "[FATAL]: defaulted from method selection.\n");
      exit(EXIT_FAILURE);
      break;
  }
  end = END_TIME();
  return elapsed(&start, &end, perf);
}
