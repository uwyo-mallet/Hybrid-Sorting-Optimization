#include <iostream>

#include "io.hpp"
#include "sort.hpp"
#include <chrono>

/*
 * TODO
 * Implement iterative insertion sort                            ✓
 * Implement vanilla recursive quicksort.                        ✓
 * Implement iterative optimized quicksort.                      ✓
 * Reference glibc qsort.                                        ✓
 * Glibc qsort switches to insertion sort at subarray length 4   ✓
 * Generate inputs of varying sizes, disparity, etc.
 * Benchmark all sorts and STL qsort.
 */

int compareInt(const void* a, const void* b);

#define NUM_RANDOM 1000000
#define	MIN_RANDOM 0
#define MAX_RANDOM 100
#define MAX_PRINT 25;

int main()
{
	std::vector<int> random;
	gen_random(random, NUM_RANDOM, MIN_RANDOM, MAX_RANDOM);
#if NUM_RANDOM < MAX_PRINT
	print(random);
#endif
	std::cout << "Sorted: " << is_sorted(random.data(), random.size()) << std::endl;

	auto start_time = std::chrono::high_resolution_clock::now();
	// optimized_quicksort(random.data(), random.size(), sizeof(*random.data()), compareInt);
	size_t num_calls = vanilla_quicksort(random.data(), random.size());
	std::cout << "Number of recursive calls: " << num_calls << std::endl;
	auto end_time = std::chrono::high_resolution_clock::now();

#if NUM_RANDOM < MAX_PRINT
	print(random);
#endif
	std::cout << "Sorted: " << is_sorted(random.data(), random.size()) << std::endl;
	
	/* Getting number of milliseconds as an integer. */
	std::chrono::duration<double, std::milli> ms_double = end_time - start_time;
	std::cout << "Execution time " << ms_double.count() << "ms" << std::endl;

	return 0;
}

int compareInt(const void* a, const void* b) {
	return (*(int*)a - *(int*)b);
}