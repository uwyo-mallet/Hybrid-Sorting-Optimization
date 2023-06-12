#include "benchmark.h"

#include <stdlib.h>

#include "alphadev.h"
#include "platform.h"
#include "sort.h"

struct times measure_sort_time(int method, sort_t* data, const size_t n,
                               const int threshold)
{
  struct times start;
  struct times end;

  switch (method)
  {
    /* Standard Methods ----------------------------------------------------- */
    case QSORT:
      start = get_times();
      qsort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case MSORT_HEAP:
      start = get_times();
      msort_heap(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case BASIC_INS:
      start = get_times();
      basic_ins_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case FAST_INS:
      start = get_times();
      fast_ins_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case SHELL:
      start = get_times();
      shell_sort(data, n, sizeof(sort_t), sort_t_compare);
      break;
    case SORT3_ALPHADEV:
      if (n != 3)
      {
        fprintf(stderr, "Sort3AlphaDev only supports input size of 3");
        abort();
      }
      start = get_times();
      Sort3AlphaDev((int*)data);
      break;
    case SORT4_ALPHADEV:
      if (n != 4)
      {
        fprintf(stderr, "Sort4AlphaDev only supports input size of 4");
        abort();
      }
      start = get_times();
      Sort4AlphaDev((int*)data);
      break;
    case SORT5_ALPHADEV:
      if (n != 5)
      {
        fprintf(stderr, "Sort5AlphaDev only supports input size of 5");
        abort();
      }
      start = get_times();
      Sort5AlphaDev((int*)data);
      break;
    case SORT6_ALPHADEV:
      if (n != 6)
      {
        fprintf(stderr, "Sort6AlphaDev only supports input size of 6");
        abort();
      }
      start = get_times();
      Sort6AlphaDev((int*)data);
      break;
    case SORT7_ALPHADEV:
      if (n != 7)
      {
        fprintf(stderr, "Sort7AlphaDev only supports input size of 7");
        abort();
      }
      start = get_times();
      Sort7AlphaDev((int*)data);
      break;
    case SORT8_ALPHADEV:
      if (n != 8)
      {
        fprintf(stderr, "Sort8AlphaDev only supports input size of 8");
        abort();
      }
      start = get_times();
      Sort8AlphaDev((int*)data);
      break;
    case VARSORT3_ALPHADEV:
      if (n > 3)
      {
        fprintf(stderr, "VarSort3AlphaDev only supports input size <= 3");
        abort();
      }
      start = get_times();
      VarSort3AlphaDev((int*)data);
      break;
    case VARSORT4_ALPHADEV:
      if (n > 4)
      {
        fprintf(stderr, "VarSort4AlphaDev only supports input size <= 4");
        abort();
      }
      start = get_times();
      VarSort4AlphaDev((int*)data);
      break;
    case VARSORT5_ALPHADEV:
      if (n > 5)
      {
        fprintf(stderr, "VarSort5AlphaDev only supports input size <= 5");
        abort();
      }
      start = get_times();
      VarSort5AlphaDev((int*)data);
      break;

    /* Hybrid Methods ------------------------------------------------------- */
    case MSORT_HEAP_WITH_OLD_INS:
      start = get_times();
      msort_heap_with_old_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_BASIC_INS:
      start = get_times();
      msort_heap_with_basic_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_SHELL:
      start = get_times();
      msort_heap_with_shell(data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_FAST_INS:
      start = get_times();
      msort_heap_with_fast_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_HEAP_WITH_NETWORK:
      start = get_times();
      msort_heap_with_network(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case MSORT_WITH_NETWORK:
      start = get_times();
      msort_with_network(data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case QUICKSORT_WITH_INS:
      start = get_times();
      quicksort_with_ins(data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    case QUICKSORT_WITH_FAST_INS:
      start = get_times();
      quicksort_with_fast_ins(
          data, n, sizeof(sort_t), sort_t_compare, threshold);
      break;
    default:
      // How tf did you end up here?
      break;
  }

  end = get_times();
  return elapsed(&start, &end);
}
