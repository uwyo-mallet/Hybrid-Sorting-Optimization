#include "sort.h"

#include <inttypes.h>
#include <string.h>

int int64_t_compare(const void *a, const void *b)
{
  int64_t *A = (int64_t *)a;
  int64_t *B = (int64_t *)b;
  return (*A < *B);
}

#define SWAP(a, b, size)         \
  do                             \
  {                              \
    size_t __size = (size);      \
    char *__a = (a), *__b = (b); \
    do                           \
    {                            \
      char __tmp = *__a;         \
      *__a++ = *__b;             \
      *__b++ = __tmp;            \
    } while (--__size > 0);      \
  } while (0)

struct msort_param
{
  size_t s;          /* Size of an individual element. */
  size_t var;        /* Variance */
  compar_d_fn_t cmp; /* Comparator function. */
  char *t;           /* Temporary storage space. */
};

static void msort_with_tmp(const struct msort_param *p, void *b, size_t n)
{
  char *b1, *b2;
  size_t n1, n2;

  if (n <= 1) return;

  n1 = n / 2;
  n2 = n - n1;
  b1 = b;
  b2 = (char *)b + (n1 * p->s);

  msort_with_tmp(p, b1, n1);
  msort_with_tmp(p, b2, n2);

  char *tmp = p->t;
  const size_t s = p->s;
  compar_d_fn_t cmp = p->cmp;
  switch (p->var)
  {
    case 0:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          *(uint32_t *)tmp = *(uint32_t *)b1;
          b1 += sizeof(uint32_t);
          --n1;
        }
        else
        {
          *(uint32_t *)tmp = *(uint32_t *)b2;
          b2 += sizeof(uint32_t);
          --n2;
        }
        tmp += sizeof(uint32_t);
      }
      break;
    case 1:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          *(uint64_t *)tmp = *(uint64_t *)b1;
          b1 += sizeof(uint64_t);
          --n1;
        }
        else
        {
          *(uint64_t *)tmp = *(uint64_t *)b2;
          b2 += sizeof(uint64_t);
          --n2;
        }
        tmp += sizeof(uint64_t);
      }
      break;
    case 2:
      while (n1 > 0 && n2 > 0)
      {
        unsigned long *tmpl = (unsigned long *)tmp;
        unsigned long *bl;

        tmp += s;
        if ((*cmp)(b1, b2) <= 0)
        {
          bl = (unsigned long *)b1;
          b1 += s;
          --n1;
        }
        else
        {
          bl = (unsigned long *)b2;
          b2 += s;
          --n2;
        }
        while (tmpl < (unsigned long *)tmp) *tmpl++ = *bl++;
      }
      break;
    case 3:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(*(const void **)b1, *(const void **)b2) <= 0)
        {
          *(void **)tmp = *(void **)b1;
          b1 += sizeof(void *);
          --n1;
        }
        else
        {
          *(void **)tmp = *(void **)b2;
          b2 += sizeof(void *);
          --n2;
        }
        tmp += sizeof(void *);
      }
      break;
    default:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          tmp = (char *)memcpy(tmp, b1, s);
          b1 += s;
          --n1;
        }
        else
        {
          tmp = (char *)memcpy(tmp, b2, s);
          b2 += s;
          --n2;
        }
      }
      break;
  }

  if (n1 > 0) memcpy(tmp, b1, n1 * s);
  memcpy(b, p->t, (n - n2) * s);
}

static void msort_with_tmp_hybrid_ins(const struct msort_param *p, void *b,
                                      size_t n, const size_t threshold)
{
  char *b1, *b2;
  size_t n1, n2;

  if (n <= 1) return;

  if (n < threshold)
  {
#define min(x, y) ((x) < (y) ? (x) : (y))
    const compar_d_fn_t cmp = p->cmp;
    const size_t size = p->s;
    char *base_ptr = (char *)b;
    char *const end_ptr = &base_ptr[size * (n - 1)];
    char *tmp_ptr = b;
    const size_t max_thresh = threshold * size;
    char *thresh = min(end_ptr, base_ptr + max_thresh);
    char *run_ptr;

    /*
      Find smallest element in first threshold and place it at the
      array's beginning.  This is the smallest array element,
      and the operation speeds up insertion sort's inner loop.
    */

    for (run_ptr = tmp_ptr + size; run_ptr <= thresh; run_ptr += size)
      if ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
      {
        tmp_ptr = run_ptr;
      }

    if (tmp_ptr != base_ptr)
    {
      SWAP(tmp_ptr, base_ptr, size);
    }

    /* Insertion sort, running from left-hand-side up to right-hand-side.  */
    run_ptr = base_ptr + size;
    while ((run_ptr += size) <= end_ptr)
    {
      tmp_ptr = run_ptr - size;
      while ((*cmp)((void *)run_ptr, (void *)tmp_ptr) < 0)
      {
        tmp_ptr -= size;
      }

      tmp_ptr += size;
      if (tmp_ptr != run_ptr)
      {
        char *trav;

        trav = run_ptr + size;
        while (--trav >= run_ptr)
        {
          char c = *trav;
          char *hi, *lo;

          for (hi = lo = trav; (lo -= size) >= tmp_ptr; hi = lo)
          {
            *hi = *lo;
          }
          *hi = c;
        }
      }
    }

    return;
  }

  n1 = n / 2;
  n2 = n - n1;
  b1 = b;
  b2 = (char *)b + (n1 * p->s);

  msort_with_tmp_hybrid_ins(p, b1, n1, threshold);
  msort_with_tmp_hybrid_ins(p, b2, n2, threshold);

  char *tmp = p->t;
  const size_t s = p->s;
  compar_d_fn_t cmp = p->cmp;
  switch (p->var)
  {
    case 0:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          *(uint32_t *)tmp = *(uint32_t *)b1;
          b1 += sizeof(uint32_t);
          --n1;
        }
        else
        {
          *(uint32_t *)tmp = *(uint32_t *)b2;
          b2 += sizeof(uint32_t);
          --n2;
        }
        tmp += sizeof(uint32_t);
      }
      break;
    case 1:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          *(uint64_t *)tmp = *(uint64_t *)b1;
          b1 += sizeof(uint64_t);
          --n1;
        }
        else
        {
          *(uint64_t *)tmp = *(uint64_t *)b2;
          b2 += sizeof(uint64_t);
          --n2;
        }
        tmp += sizeof(uint64_t);
      }
      break;
    case 2:
      while (n1 > 0 && n2 > 0)
      {
        unsigned long *tmpl = (unsigned long *)tmp;
        unsigned long *bl;

        tmp += s;
        if ((*cmp)(b1, b2) <= 0)
        {
          bl = (unsigned long *)b1;
          b1 += s;
          --n1;
        }
        else
        {
          bl = (unsigned long *)b2;
          b2 += s;
          --n2;
        }
        while (tmpl < (unsigned long *)tmp) *tmpl++ = *bl++;
      }
      break;
    case 3:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(*(const void **)b1, *(const void **)b2) <= 0)
        {
          *(void **)tmp = *(void **)b1;
          b1 += sizeof(void *);
          --n1;
        }
        else
        {
          *(void **)tmp = *(void **)b2;
          b2 += sizeof(void *);
          --n2;
        }
        tmp += sizeof(void *);
      }
      break;
    default:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          tmp = (char *)memcpy(tmp, b1, s);
          b1 += s;
          --n1;
        }
        else
        {
          tmp = (char *)memcpy(tmp, b2, s);
          b2 += s;
          --n2;
        }
      }
      break;
  }

  if (n1 > 0) memcpy(tmp, b1, n1 * s);
  memcpy(b, p->t, (n - n2) * s);
}

void msort_heap(void *b, size_t n, size_t s, compar_d_fn_t cmp)
{
  // Assume the array is of sufficient size to require a heap allocation, and
  // that we are sorting primitives.
  const size_t size = n * s;
  char *tmp = NULL;

  tmp = malloc(size);
  if (tmp == NULL)
  {
    // Kill the program with an error
    perror("malloc");
    exit(1);
  }

  struct msort_param p;
  p.s = s;
  p.var = 4;
  p.cmp = cmp;
  p.t = tmp;

  // TODO: Guess the type?
  if ((s & (sizeof(uint32_t) - 1)) == 0 &&
      ((char *)b - (char *)0) % __alignof__(uint32_t) == 0)
  {
    if (s == sizeof(uint32_t))
      p.var = 0;
    else if (s == sizeof(uint64_t) &&
             ((char *)b - (char *)0) % __alignof__(uint64_t) == 0)
      p.var = 1;
    else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
             ((char *)b - (char *)0) % __alignof__(unsigned long) == 0)
      p.var = 2;
  }

  msort_with_tmp(&p, b, n);

  free(tmp);
}

void msort_heap_hybrid(void *b, size_t n, size_t s, compar_d_fn_t cmp,
                       const size_t threshold)
{
  // Assume the array is of sufficient size to require a heap allocation, and
  // that we are sorting primitives.
  const size_t size = n * s;
  char *tmp = NULL;

  tmp = malloc(size);
  if (tmp == NULL)
  {
    // Kill the program with an error
    perror("malloc");
    exit(1);
  }

  struct msort_param p;
  p.s = s;
  p.var = 4;
  p.cmp = cmp;
  p.t = tmp;

  // TODO: Guess the type?
  if ((s & (sizeof(uint32_t) - 1)) == 0 &&
      ((char *)b - (char *)0) % __alignof__(uint32_t) == 0)
  {
    if (s == sizeof(uint32_t))
      p.var = 0;
    else if (s == sizeof(uint64_t) &&
             ((char *)b - (char *)0) % __alignof__(uint64_t) == 0)
      p.var = 1;
    else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
             ((char *)b - (char *)0) % __alignof__(unsigned long) == 0)
      p.var = 2;
  }

  msort_with_tmp_hybrid_ins(&p, b, n, threshold);

  free(tmp);
}