#ifndef BENCHMARK_H_
#define BENCHMARK_H_

#include <benchmark/benchmark.h>

#include <cstdint>
#include <vector>

extern std::vector<uint64_t> orig_data;
extern uint64_t threshold;

class SortFixture : public benchmark::Fixture
{
 public:
  std::vector<uint64_t> data;
  uint64_t* phead = nullptr;
  size_t n = 0;
  const size_t dtype_size = sizeof(uint64_t);

  void SetUp(const ::benchmark::State& state);
  void TearDown(const ::benchmark::State& state);
};

#endif  // BENCHMARK_H_
