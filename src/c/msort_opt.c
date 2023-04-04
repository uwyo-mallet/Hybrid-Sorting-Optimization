#define _GNU_SOURCE

#include <alloca.h>
#include <errno.h>
#include <inttypes.h>
#include <limits.h>
#include <stdatomic.h>
#include <string.h>
#include <unistd.h>

#include "sort.h"

#define min_cmp(a, b, cmp) (((*cmp))((a), (b)) < 0 ? (a) : (b))
#define max_cmp(a, b, cmp) (((*cmp))((a), (b)) >= 0 ? (a) : (b))

/*
 * The following functions implement optimal sorting networks up to n = 6.
 *
 * Reference:
 *  The Art of Computer Programming, Volume 3: Sorting and Searching (Second
 *  ed.). Addison–Wesley. pp. 219–247. ISBN 978-0-201-89685-5. Section 5.3.4:
 *  Networks for Sorting.
 */

static inline void sort2(const struct msort_param *p, void *a, void *b)
{
  const compar_d_fn_t cmp = p->cmp;
  const size_t s = p->s;
  char *const tmp = p->t;

  void *const min = min_cmp(a, b, cmp);
  void *const max = a == min ? b : a;

  switch (p->var)
  {
    case UINT32:
      // 32-bit
      *(uint32_t *)tmp = *(uint32_t *)min;
      *(uint32_t *)b = *(uint32_t *)max;
      *(uint32_t *)a = *(uint32_t *)tmp;
      return;
    case UINT64:
      *(uint64_t *)tmp = *(uint64_t *)min;
      *(uint64_t *)b = *(uint64_t *)max;
      *(uint64_t *)a = *(uint64_t *)tmp;
      return;
    case PTR:
      // clang-format off
      *(const void **)tmp = min_cmp((*(const void **)a), (*(const void **)b), cmp);
      *(const void **)b = max_cmp((*(const void **)a), (*(const void **)b), cmp);
      *(const void **)a = *(void **)tmp;
      // clang-format on
      return;
    default:
      memcpy(tmp, min, s);
      memcpy(b, max, s);
      memcpy(a, tmp, s);
      return;
  }
}

void sort3(const struct msort_param *p, void *b)
{
  char *base = (char *)b;
  sort2(p, base, base + (2 * p->s));
  sort2(p, base, base + p->s);
  sort2(p, base + p->s, base + (2 * p->s));
}

void sort4(const struct msort_param *p, void *b)
{
  char *base = (char *)b;
  sort2(p, base, base + (2 * p->s));
  sort2(p, base + p->s, base + (3 * p->s));
  sort2(p, base, base + p->s);
  sort2(p, base + (2 * p->s), base + (3 * p->s));
  sort2(p, base + p->s, base + (2 * p->s));
}

void sort5(const struct msort_param *p, void *b)
{
  char *base = (char *)b;
  sort2(p, base, base + (3 * p->s));
  sort2(p, base + p->s, base + (4 * p->s));
  sort2(p, base, base + (2 * p->s));
  sort2(p, base + p->s, base + (3 * p->s));
  sort2(p, base, base + p->s);
  sort2(p, base + (2 * p->s), base + (4 * p->s));
  sort2(p, base + p->s, base + (2 * p->s));
  sort2(p, base + (3 * p->s), base + (4 * p->s));
  sort2(p, base + (2 * p->s), base + (3 * p->s));
}

void sort6(const struct msort_param *p, void *b)
{
  char *base = (char *)b;
  sort2(p, base, base + (5 * p->s));
  sort2(p, base + p->s, base + (3 * p->s));
  sort2(p, base + (2 * p->s), base + (4 * p->s));
  sort2(p, base + p->s, base + (2 * p->s));
  sort2(p, base + (3 * p->s), base + (4 * p->s));
  sort2(p, base, base + (3 * p->s));
  sort2(p, base + (2 * p->s), base + (5 * p->s));
  sort2(p, base, base + p->s);
  sort2(p, base + (2 * p->s), base + (3 * p->s));
  sort2(p, base + (4 * p->s), base + (5 * p->s));
  sort2(p, base + p->s, base + (2 * p->s));
  sort2(p, base + (3 * p->s), base + (4 * p->s));
}

void ins_sort(const struct msort_param *const p, void *b, size_t n)
{
  const compar_d_fn_t cmp = p->cmp;
  const size_t s = p->s;
  const unsigned var = p->var;
  char *tmp = p->t;

  unsigned c;
  char *base = (char *)b;
  switch (var)
  {
    case UINT32:
      for (size_t i = 1; i < n; ++i)
      {
        c = 1;
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          *(uint32_t *)tmp = ((uint32_t *)base)[i];

          do
          {
            ((uint32_t *)base)[j + 1] = ((uint32_t *)base)[j];
            if (j == 0)
            {
              c = 0;
              break;
            }
            j--;
          } while ((*cmp)(base + (j * s), tmp) > 0);

          ((uint32_t *)base)[j + c] = *(uint32_t *)tmp;
        }
      }
      return;
    case UINT64:
      for (size_t i = 1; i < n; ++i)
      {
        c = 1;
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          *(uint64_t *)tmp = ((uint64_t *)base)[i];

          do
          {
            ((uint64_t *)base)[j + 1] = ((uint64_t *)base)[j];
            if (j == 0)
            {
              c = 0;
              break;
            }
            j--;
          } while ((*cmp)(base + (j * s), tmp) > 0);

          ((uint64_t *)base)[j + c] = *(uint64_t *)tmp;
        }
      }
      return;
    case PTR:
      for (size_t i = 1; i < n; ++i)
      {
        c = 1;
        size_t j = i - 1;
        if ((*cmp)(((const void **)base)[j], ((const void **)base)[i]) > 0)
        {
          *(void **)tmp = ((void **)base)[i];
          do
          {
            ((void **)base)[j + 1] = ((void **)base)[j];
            if (j == 0)
            {
              c = 0;
              break;
            }
            --j;
          } while ((*cmp)(((const void **)base)[j], *(const void **)tmp) > 0);

          ((void **)base)[j + c] = *(void **)tmp;
        }
      }

      return;
    default:
      for (size_t i = 1; i < n; ++i)
      {
        c = 1;
        size_t j = i - 1;
        if ((*cmp)(base + (j * s), base + (i * s)) > 0)
        {
          memcpy(tmp, base + (i * s), s);

          do
          {
            /* base[j + 1] = base[j]; */
            memcpy(base + ((j + 1) * s), base + (j * s), s);
            if (j == 0)
            {
              c = 0;
              break;
            }
            j--;
          } while ((*cmp)(base + (j * s), tmp) > 0);

          /* base[j + c] = v; */
          memcpy(base + ((j + c) * s), tmp, s);
        }
      }
      return;
  }
}

void fast_ins_sort(void *b, size_t n, size_t s, compar_d_fn_t cmp)
{
  unsigned var = DEFAULT;
  if ((s & (sizeof(uint32_t) - 1)) == 0 &&
      ((uintptr_t)b) % __alignof__(uint32_t) == 0)
  {
    if (s == sizeof(uint32_t))
    {
      var = UINT32;
    }
    else if (s == sizeof(uint64_t) &&
             ((uintptr_t)b) % __alignof__(uint64_t) == 0)
    {
      var = UINT64;
    }
    else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
             ((uintptr_t)b) % __alignof__(unsigned long) == 0)
    {
      var = ULONG;
    }
  }

  char *tmp = alloca(s);
  const struct msort_param p = {s, var, cmp, tmp};
  ins_sort(&p, b, n);
}

static void msort_with_network_recur(const struct msort_param *const p, void *b,
                                     const size_t n, const size_t threshold)
{
  if (n <= 1)
  {
    return;
  }

  if (n < threshold)
  {
    char *base = (char *)b;
    switch (n)
    {
      case 2:
        sort2(p, base, (base + p->s));
        return;
      case 3:
        sort3(p, base);
        return;
      case 4:
        sort4(p, base);
        return;
      case 5:
        sort5(p, base);
        return;
      case 6:
        sort6(p, base);
        return;
    }

    ins_sort(p, b, n);
    return;
  }

  char *b1, *b2;
  size_t n1, n2;

  char *tmp = p->t;
  const size_t s = p->s;
  const compar_d_fn_t cmp = p->cmp;

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
    case PTR:
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

  // Use alignment to avoid some syscalls later down the line.
  if ((s & (sizeof(uint32_t) - 1)) == 0 &&
      ((uintptr_t)b) % __alignof__(uint32_t) == 0)
  {
    if (s == sizeof(uint32_t))
    {
      p.var = UINT32;
    }
    else if (s == sizeof(uint64_t) &&
             ((uintptr_t)b) % __alignof__(uint64_t) == 0)
    {
      p.var = UINT64;
    }
    else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
             ((uintptr_t)b) % __alignof__(unsigned long) == 0)
    {
      p.var = ULONG;
    }
  }

  msort_with_network_recur(&p, b, n, threshold);

  free(tmp);
}

void msort_with_network(void *b, size_t n, size_t s, compar_d_fn_t cmp,
                        const size_t threshold)
{
  if (n <= 1) return;

  size_t size = n * s;
  char *tmp = NULL;

  struct msort_param p;
  p.s = s;
  p.var = DEFAULT;
  p.cmp = cmp;
  /* p.arg = arg; */

  if (s > 32)
  {
    size = 2 * n * sizeof(void *) + s;
  }

  if (size < 1024)
  {
    // Size is small, so stack allocate it.
    p.t = alloca(size);
  }
  else
  {
    // Avoid allocating too much memory

    static long int phys_pages;
    static int pagesize;

    if (pagesize == 0)
    {
      phys_pages = __sysconf(_SC_PHYS_PAGES);

      if (phys_pages == -1)
        /* Error while determining the memory size.  So let's
           assume there is enough memory.  Otherwise the
           implementer should provide a complete implementation of
           the `sysconf' function.  */
        phys_pages = (long int)(~0ul >> 1);

      /* The following determines that we will never use more than
         a quarter of the physical memory.  */
      phys_pages /= 4;

      /* Make sure phys_pages is written to memory.  */
      /* atomic_write_barrier(); */

      pagesize = __sysconf(_SC_PAGESIZE);
    }

    /* Just a comment here.  We cannot compute
         phys_pages * pagesize
         and compare the needed amount of memory against this value.
         The problem is that some systems might have more physical
         memory then can be represented with a `size_t' value (when
         measured in bytes.  */

    /* If the memory requirements are too high don't allocate memory.  */
    if (size / pagesize > (size_t)phys_pages)
    {
      fprintf(stderr, "Page memory error\n");
      abort();
    }

    /* It's somewhat large, so malloc it.  */
    int save = errno;
    tmp = malloc(size);
    errno = save;
    /* __set_errno(save); */
    if (tmp == NULL)
    {
      /* Couldn't get space, so use the slower algorithm
         that doesn't need a temporary array.  */
      fprintf(stderr, "Page memory error 2\n");
      abort();
    }
    p.t = tmp;
  }

  if (s > 32)
  {
    /* Indirect sorting.  */
    char *ip = (char *)b;
    void **tp = (void **)(p.t + n * sizeof(void *));
    void **t = tp;
    void *tmp_storage = (void *)(tp + n);

    while ((void *)t < tmp_storage)
    {
      *t++ = ip;
      ip += s;
    }
    p.s = sizeof(void *);
    p.var = PTR;
    msort_with_network_recur(&p, tp, n, threshold);

    /* tp[0] .. tp[n - 1] is now sorted, copy around entries of
       the original array.  Knuth vol. 3 (2nd ed.) exercise 5.2-10.  */

    char *kp;
    size_t i;
    for (i = 0, ip = (char *)b; i < n; i++, ip += s)
      if ((kp = tp[i]) != ip)
      {
        size_t j = i;
        char *jp = ip;
        memcpy(tmp_storage, ip, s);

        do
        {
          size_t k = (kp - (char *)b) / s;
          tp[j] = jp;
          memcpy(jp, kp, s);
          j = k;
          jp = kp;
          kp = tp[k];
        } while (kp != ip);

        tp[j] = jp;
        memcpy(jp, tmp_storage, s);
      }
  }
  else
  {
    // Use alignment to avoid some syscalls later down the line.
    if ((s & (sizeof(uint32_t) - 1)) == 0 &&
        ((uintptr_t)b) % __alignof__(uint32_t) == 0)
    {
      if (s == sizeof(uint32_t))
      {
        p.var = UINT32;
      }
      else if (s == sizeof(uint64_t) &&
               ((uintptr_t)b) % __alignof__(uint64_t) == 0)
      {
        p.var = UINT64;
      }
      else if ((s & (sizeof(unsigned long) - 1)) == 0 &&
               ((uintptr_t)b) % __alignof__(unsigned long) == 0)
      {
        p.var = ULONG;
      }
    }

    msort_with_network_recur(&p, b, n, threshold);
  }

  free(tmp);
}
