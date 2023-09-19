#ifndef TESTS_HPP
#define TESTS_HPP

#include <stdint.h>

#include <algorithm>
#include <array>
#include <boost/random.hpp>
#include <cstring>
#include <iostream>
#include <limits>

extern "C"
{
#include "sort.h"
}

#define MAX_ARRAY_SIZE 1024UL
#define PAD_SIZE 4096UL

typedef struct
{
  // Volatile to prevent optimizer from removing this altogether.
  volatile char c[PAD_SIZE];
  int64_t val;
} LargeStruct;

template <typename T>
int compare(void* a, void* b)
{
  T* A = (T*)a;
  T* B = (T*)b;

  if (*A < *B) return -1;
  if (*A > *B) return 1;
  return 0;
}

template <>
int compare<LargeStruct>(void* a, void* b);

enum DTYPE
{
  ASCENDING = 1,
  DESCENDING,
  RANDOM,
  PIPEORGAN,
  SINGLENUM,
};

template <typename T, const size_t N = MAX_ARRAY_SIZE>
struct TestDataFixture
{
  T* ascending;
  T* descending;
  T* random;
  T* pipeOrgan;
  T* singleNum;
  T* working;
  const size_t n;
  const size_t s;
  const size_t size;
  compar_d_fn_t cmp;

  TestDataFixture()
      : n(std::min(N, (size_t)std::numeric_limits<T>::max())),
        s(sizeof(T)),
        size(n * sizeof(T)),
        cmp((compar_d_fn_t)compare<T>)
  {
    const size_t seed = 42;
    boost::random::mt19937 gen{seed};
    boost::random::uniform_int_distribution<size_t> dist{0, n};

    ascending = (T*)std::calloc(n, s);
    descending = (T*)std::calloc(n, s);
    random = (T*)std::calloc(n, s);
    pipeOrgan = (T*)std::calloc(n, s);
    singleNum = (T*)std::calloc(n, s);
    working = (T*)std::calloc(n, s);

    if (ascending == NULL || descending == NULL || random == NULL ||
        pipeOrgan == NULL || singleNum == NULL)
    {
      std::cerr
          << "[FATAL]: Unable to allocate memory for test arrays! Requested "
          << size << " byte(s) for " << n << " elements\n";
      abort();
    }

    // Ascending
    for (size_t i = 0; i < n; ++i)
    {
      ascending[i] = i;
    }

    // Descending
    for (size_t i = 0; i < n; ++i)
    {
      descending[i] = n - i;
    }

    // Random
    for (size_t i = 0; i < n; ++i)
    {
      random[i] = dist(gen);
    }

    // Pipe Organ
    for (size_t i = 0; i < n / 2; ++i)
    {
      pipeOrgan[i] = i;
    }
    for (size_t i = n / 2; i < n; ++i)
    {
      pipeOrgan[i] = n - i;
    }

    // Single Num
    for (size_t i = 0; i < n; ++i)
    {
      singleNum[i] = 12;
    }
  };

  ~TestDataFixture()
  {
    free(ascending);
    free(descending);
    free(random);
    free(pipeOrgan);
    free(singleNum);
    free(working);
  };

  bool is_sorted(const DTYPE type)
  {
    T* qsortMem = (T*)malloc(size);
    if (!qsortMem)
    {
      std::cout << "Memory error!\n";
      abort();
    }
    switch (type)
    {
      case ASCENDING:
        std::memcpy(qsortMem, ascending, size);
        break;
      case DESCENDING:
        std::memcpy(qsortMem, descending, size);
        break;
      case RANDOM:
        std::memcpy(qsortMem, random, size);
        break;
      case PIPEORGAN:
        std::memcpy(qsortMem, pipeOrgan, size);
        break;
      case SINGLENUM:
        std::memcpy(qsortMem, singleNum, size);
        break;
    }
    std::qsort(qsortMem, n, sizeof(T), cmp);

    bool sorted = std::memcmp(qsortMem, working, size) == 0;

    free(qsortMem);

    return sorted;
  }
};

template <>
struct TestDataFixture<LargeStruct>
{
  LargeStruct* ascending;
  LargeStruct* descending;
  LargeStruct* random;
  LargeStruct* pipeOrgan;
  LargeStruct* singleNum;
  LargeStruct* working;
  const size_t n;
  const size_t s;
  const size_t size;
  compar_d_fn_t cmp;

  TestDataFixture()
      : n(std::min(MAX_ARRAY_SIZE,
                   (size_t)std::numeric_limits<int64_t>::max())),
        s(sizeof(LargeStruct)),
        size(n * sizeof(LargeStruct)),
        cmp((compar_d_fn_t)compare<LargeStruct>)
  {
    const size_t seed = 42;
    boost::random::mt19937 gen{seed};
    boost::random::uniform_int_distribution<size_t> dist{0, n};

    ascending = (LargeStruct*)std::calloc(n, s);
    descending = (LargeStruct*)std::calloc(n, s);
    random = (LargeStruct*)std::calloc(n, s);
    pipeOrgan = (LargeStruct*)std::calloc(n, s);
    singleNum = (LargeStruct*)std::calloc(n, s);
    working = (LargeStruct*)std::calloc(n, s);

    if (ascending == NULL || descending == NULL || random == NULL ||
        pipeOrgan == NULL || singleNum == NULL)
    {
      std::cout << "Memory error!\n";
      abort();
    }

    // Ascending
    for (size_t i = 0; i < n; ++i)
    {
      ascending[i].val = i;
    }

    // Descending
    for (size_t i = 0; i < n; ++i)
    {
      descending[i].val = n - i;
    }

    // Random
    for (size_t i = 0; i < n; ++i)
    {
      random[i].val = dist(gen);
    }

    // Pipe Organ
    for (size_t i = 0; i < n / 2; ++i)
    {
      pipeOrgan[i].val = i;
    }
    for (size_t i = n / 2; i < n; ++i)
    {
      pipeOrgan[i].val = n - i;
    }

    // Single Num
    for (size_t i = 0; i < n; ++i)
    {
      singleNum[i].val = 12;
    }
  }

  ~TestDataFixture()
  {
    free(ascending);
    free(descending);
    free(random);
    free(pipeOrgan);
    free(singleNum);
    free(working);
  };

  bool is_sorted(const DTYPE type)
  {
    LargeStruct* qsortMem = (LargeStruct*)malloc(size);
    if (!qsortMem)
    {
      std::cout << "Memory error!\n";
      abort();
    }
    switch (type)
    {
      case ASCENDING:
        std::memcpy(qsortMem, ascending, size);
        break;
      case DESCENDING:
        std::memcpy(qsortMem, descending, size);
        break;
      case RANDOM:
        std::memcpy(qsortMem, random, size);
        break;
      case PIPEORGAN:
        std::memcpy(qsortMem, pipeOrgan, size);
        break;
      case SINGLENUM:
        std::memcpy(qsortMem, singleNum, size);
        break;
    }
    std::qsort(qsortMem, n, sizeof(LargeStruct), cmp);

    bool sorted = std::memcmp(qsortMem, working, size) == 0;

    free(qsortMem);

    return sorted;
  }
};

#endif  // TESTS_HPP
