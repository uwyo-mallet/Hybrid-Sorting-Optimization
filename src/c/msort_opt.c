#define _GNU_SOURCE

#include <alloca.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>

#include "sort.h"

enum dtype
{
  UINT32,
  UINT64,
  ULONG,
  DEFAULT
};

#define min_cmp(a, b, cmp) (((*cmp))((a), (b)) < 0 ? (a) : (b))
#define max_cmp(a, b, cmp) (((*cmp))((a), (b)) > 0 ? (a) : (b))

inline static void sort2(void *a, void *b, const size_t s, void *tmp,
                         const unsigned short var, compar_d_fn_t cmp)
{
  switch (var)
  {
    case UINT32:
      // 32-bit
      *(uint32_t *)tmp = *(uint32_t *)min_cmp(a, b, cmp);
      *(uint32_t *)b = *(uint32_t *)max_cmp(a, b, cmp);
      *(uint32_t *)a = *(uint32_t *)tmp;
      break;
    case UINT64:
      *(uint64_t *)tmp = *(uint64_t *)min_cmp(a, b, cmp);
      *(uint64_t *)b = *(uint64_t *)max_cmp(a, b, cmp);
      *(uint64_t *)a = *(uint64_t *)tmp;
      break;
    case ULONG:
      *(unsigned long *)tmp = *(unsigned long *)min_cmp(a, b, cmp);
      *(unsigned long *)b = *(unsigned long *)max_cmp(a, b, cmp);
      *(unsigned long *)a = *(unsigned long *)tmp;
      break;
    default:
      memcpy(tmp, min_cmp(a, b, cmp), s);
      memcpy(b, max_cmp(a, b, cmp), s);
      memcpy(a, tmp, s);
      break;
  }
}

inline static void sort3(void *p0, void *p1, void *p2, const size_t s,
                         void *tmp, const enum dtype var, compar_d_fn_t cmp)
{
  sort2(p0, p1, s, tmp, var, cmp);
  sort2(p1, p2, s, tmp, var, cmp);
  sort2(p0, p1, s, tmp, var, cmp);
}

inline static void sort4(void *p0, void *p1, void *p2, void *p3, const size_t s,
                         void *tmp, const enum dtype var, compar_d_fn_t cmp)
{
  sort2(p0, p1, s, tmp, var, cmp);
  sort2(p2, p3, s, tmp, var, cmp);
  sort2(p0, p2, s, tmp, var, cmp);
  sort2(p1, p3, s, tmp, var, cmp);
  sort2(p1, p2, s, tmp, var, cmp);
}

inline static void sort6(void *p0, void *p1, void *p2, void *p3, void *p4,
                         void *p5, const size_t s, void *tmp,
                         const enum dtype var, compar_d_fn_t cmp)
{
  sort3(p0, p1, p2, s, tmp, var, cmp);
  sort3(p3, p4, p5, s, tmp, var, cmp);
  sort2(p0, p3, s, tmp, var, cmp);
  sort2(p2, p5, s, tmp, var, cmp);
  sort4(p1, p2, p3, p4, s, tmp, var, cmp);
}

void fast_ins_sort(void *b, size_t n, const size_t s, compar_d_fn_t cmp)
{
  char *base = (char *)b;
  unsigned short c = 1;
  char *v = alloca(s);
  for (size_t i = 1; i < n; ++i)
  {
    size_t j = i - 1;
    if ((*cmp)(base + (j * s), base + (i * s)) > 0)
    {
      memcpy(v, base + (i * s), s);

      do
      {
        /* base[j + 1] = base[j]; */
        memcpy(base + ((j + 1) * s), base + (j * s), s);
        if (j == 0)
        {
          c = 0;
          goto outer;
        }
        j--;
      } while ((*cmp)(base + (j * s), v) > 0);

    outer:
      /* base[j + c] = v; */
      memcpy(base + ((j + c) * s), v, s);
    }
  }
}

inline static void even_faster_ins_sort(void *b, size_t n, const size_t s,
                                        const enum dtype var, compar_d_fn_t cmp)
{
  char *v = alloca(s);
  uint32_t u32v;
  uint64_t u64v;
  unsigned long ulv;

  char *base = (char *)b;
  switch (n)
  {
    case 2:
      sort2(base, base + s, s, v, var, cmp);
      return;
    case 3:
      sort3(base, base + s, base + (2 * s), s, v, var, cmp);
      return;
    case 4:
      sort4(base, base + s, base + (2 * s), base + (3 * s), s, v, var, cmp);
      return;
  }

  unsigned short c = 1;
  switch (var)
  {
    case UINT32:
      for (size_t i = 1; i < n; ++i)
      {
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          u32v = ((uint32_t *)base)[i];

          do
          {
            ((uint32_t *)base)[j + 1] = ((uint32_t *)base)[j];
            if (j == 0)
            {
              c = 0;
              goto outer1;
            }
            j--;
          } while ((*cmp)(base + (j * s), &u32v) > 0);

        outer1:
          ((uint32_t *)base)[j + c] = u32v;
        }
      }
      return;
    case UINT64:
      for (size_t i = 1; i < n; ++i)
      {
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          u64v = ((uint64_t *)base)[i];

          do
          {
            ((uint64_t *)base)[j + 1] = ((uint64_t *)base)[j];
            if (j == 0)
            {
              c = 0;
              goto outer2;
            }
            j--;
          } while ((*cmp)(base + (j * s), &u64v) > 0);

        outer2:
          ((uint64_t *)base)[j + c] = u64v;
        }
      }
      return;
    case ULONG:
      for (size_t i = 1; i < n; ++i)
      {
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          ulv = ((unsigned long *)base)[i];

          do
          {
            ((unsigned long *)base)[j + 1] = ((unsigned long *)base)[j];
            if (j == 0)
            {
              c = 0;
              goto outer3;
            }
            j--;
          } while ((*cmp)(base + (j * s), &ulv) > 0);

        outer3:
          ((unsigned long *)base)[j + c] = ulv;
        }
      }
      return;
    default:
      for (size_t i = 1; i < n; ++i)
      {
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          memcpy(v, base + (i * s), s);

          do
          {
            /* base[j + 1] = base[j]; */
            memcpy(base + ((j + 1) * s), base + (j * s), s);
            if (j == 0)
            {
              c = 0;
              goto outer4;
            }
            j--;
          } while ((*cmp)(base + (j * s), v) > 0);

        outer4:
          /* base[j + c] = v; */
          memcpy(base + ((j + c) * s), v, s);
        }
      }
      return;
  }
}

static void msort_with_network_recur(const struct msort_param *p, void *b,
                                     size_t n, const size_t threshold)
{
  char *b1, *b2;
  size_t n1, n2;

  char *tmp = p->t;
  const size_t s = p->s;
  const compar_d_fn_t cmp = p->cmp;

  if (n <= 1) return;

  if (n < threshold)
  {
    even_faster_ins_sort(b, n, s, p->var, cmp);
    return;
  }

  n1 = n / 2;
  n2 = n - n1;
  b1 = b;
  b2 = (char *)b + (n1 * p->s);

  msort_with_network_recur(p, b1, n1, threshold);
  msort_with_network_recur(p, b2, n2, threshold);

  switch (p->var)
  {
    case UINT32:
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
    case UINT64:
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
    case ULONG:
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
    default:
      while (n1 > 0 && n2 > 0)
      {
        if ((*cmp)(b1, b2) <= 0)
        {
          tmp = (char *)mempcpy(tmp, b1, s);
          b1 += s;
          --n1;
        }
        else
        {
          tmp = (char *)mempcpy(tmp, b2, s);
          b2 += s;
          --n2;
        }
      }
      break;
  }

  if (n1 > 0) mempcpy(tmp, b1, n1 * s);
  mempcpy(b, p->t, (n - n2) * s);
}

void msort_heap_with_network(void *b, size_t n, size_t s, compar_d_fn_t cmp,
                             const size_t threshold)
{
  // Assume the array is of sufficient size to require a heap allocation, and
  // that we are sorting primitives.
  if (n <= 1) return;

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
  p.var = DEFAULT;
  p.cmp = cmp;
  p.t = tmp;

  // TODO: Guess the type?
  if ((s & (sizeof(uint32_t) - 1)) == 0 &&
      ((char *)b - (char *)0) % __alignof__(uint32_t) == 0)
  {
    if (s == sizeof(uint32_t))
      p.var = UINT32;
    else if (s == sizeof(uint64_t) &&
             ((char *)b - (char *)0) % __alignof__(uint64_t) == 0)
      p.var = UINT64;
    else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
             ((char *)b - (char *)0) % __alignof__(unsigned long) == 0)
      p.var = ULONG;
  }

  msort_with_network_recur(&p, b, n, threshold);

  free(tmp);
}
