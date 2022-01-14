#include "benchmark.hpp"

#include "sort.h"
#include "sort.hpp"
#include "utils.hpp"

void SortFixture::SetUp(const ::benchmark::State& state)
{
  // TODO: Mind when this runs, ensure not to often, otherwise this will be
  // painfully slow from repeated allocations.
  data = orig_data;
  phead = data.data();
  n = data.size();
}

void SortFixture::TearDown(const ::benchmark::State& state)
{
  // TODO: Mind when this runs, ensure not to often, otherwise this will be
  // painfully slow from repeated deallocations.
  data.clear();
  phead = nullptr;
  n = 0;
}

BENCHMARK_DEFINE_F(SortFixture, insertion_sort)(benchmark::State& st)
{
  for (auto _ : st)
  {
    st.PauseTiming();
    data = orig_data;
    st.ResumeTiming();
    insertion_sort(phead, n, compare<uint64_t>);
  }
}

BENCHMARK_DEFINE_F(SortFixture, qsort_sanity)(benchmark::State& st)
{
  for (auto _ : st)
  {
    st.PauseTiming();
    data = orig_data;
    st.ResumeTiming();
    qsort_sanity(phead, n, dtype_size, compare<uint64_t>);
  }
}

BENCHMARK_DEFINE_F(SortFixture, std)(benchmark::State& st)
{
  for (auto _ : st)
  {
    st.PauseTiming();
    data = orig_data;
    st.ResumeTiming();
    std::sort(data.begin(), data.end());
  }
}

BENCHMARK_DEFINE_F(SortFixture, qsort_c)(benchmark::State& st)
{
  for (auto _ : st)
  {
    st.PauseTiming();
    data = orig_data;
    st.ResumeTiming();
    qsort_c(phead, n, dtype_size, compare<uint64_t>, threshold);
  }
}

BENCHMARK_DEFINE_F(SortFixture, qsort_cpp)(benchmark::State& st)
{
  for (auto _ : st)
  {
    st.PauseTiming();
    data = orig_data;
    st.ResumeTiming();
    qsort_cpp(phead, n, threshold, compare<uint64_t>);
  }
}

// clang-format off
BENCHMARK_REGISTER_F(SortFixture, insertion_sort) ->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SortFixture, qsort_sanity)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SortFixture, std)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SortFixture, qsort_c)->Unit(benchmark::kMillisecond);
BENCHMARK_REGISTER_F(SortFixture, qsort_cpp)->Unit(benchmark::kMillisecond);
// clang-format on
