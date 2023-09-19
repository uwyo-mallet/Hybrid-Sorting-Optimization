#include "benchmark.h"

#include <stdlib.h>

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
/* Alpha Dev Methods ---------------------------------------------------- */
#ifdef SORT_INTS
    case ALPHA_DEV_SORT3:
      start = get_times();
      alphadev_sort3(data);
      break;
    case ALPHA_DEV_SORT4:
      start = get_times();
      alphadev_sort4(data);
      break;
    case ALPHA_DEV_SORT5:
      start = get_times();
      alphadev_sort5(data);
      break;
    case ALPHA_DEV_SORT6:
      start = get_times();
      alphadev_sort6(data);
      break;
    case ALPHA_DEV_SORT7:
      start = get_times();
      alphadev_sort7(data);
      break;
    case ALPHA_DEV_SORT8:
      start = get_times();
      alphadev_sort8(data);
      break;
    case ALPHA_DEV_SORT3_VAR:
      start = get_times();
      printf("%s:%d: Not implemented\n", __FILE__, __LINE__);
      abort();
      break;
    case ALPHA_DEV_SORT4_VAR:
      start = get_times();
      printf("%s:%d: Not implemented\n", __FILE__, __LINE__);
      abort();
      break;
    case ALPHA_DEV_SORT5_VAR:
      start = get_times();
      printf("%s:%d: Not implemented\n", __FILE__, __LINE__);
      abort();
      break;
#else
    case ALPHA_DEV_SORT3:
    case ALPHA_DEV_SORT4:
    case ALPHA_DEV_SORT5:
    case ALPHA_DEV_SORT6:
    case ALPHA_DEV_SORT7:
    case ALPHA_DEV_SORT8:
    case ALPHA_DEV_SORT3_VAR:
    case ALPHA_DEV_SORT4_VAR:
    case ALPHA_DEV_SORT5_VAR:
      printf("Compiled without support for these methods. Use -DSORT_INTS\n");
      abort();
      break;
#endif  // SORT_INTS
    default:
      // How tf did you end up here?
      break;
  }

  end = get_times();
  return elapsed(&start, &end);
}
