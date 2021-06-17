#include <iostream>

#include "io.hpp"
#include "sort.hpp"

/*
 * TODO
 * Implement insertion sort ✓
 * Implement quicksort
 *     * Segfault :(
 *
 * Reference glibc qsort
 * Determine at which point the STL switches from insertion sort to qsort (15?)
 * Generate inputs of varying sizes, disparity, etc.
 *
 * Essentially, do all the nitty gritty empirical work first.
 *
 * Stay motivated <3
 */

int main()
{
  std::vector<int> random;
  gen_random(random, 10, 0, 100);

  print(random);
  std::cout << is_sorted(random.data(), random.size()) << std::endl;

  quick_sort(random.data(), random.size());

  print(random);
  std::cout << is_sorted(random.data(), random.size()) << std::endl;

  return 0;
}
