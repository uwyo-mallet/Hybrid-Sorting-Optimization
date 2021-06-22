#include <algorithm>
#include <chrono>
#include <iostream>

#include "io.hpp"
#include "sort.hpp"

int compareInt(const void *a, const void *b);

int main()
{
	constexpr size_t NUM_ELEMENTS = 1000000;
	constexpr size_t NUM_TESTS = 1000;
	constexpr size_t MIN_RANDOM = 0;
	constexpr size_t MAX_RANDOM = 1000;
	constexpr size_t MAX_PRINT = 25;

	std::vector<int> qsort_c_set;
	std::vector<int> validate_set;

	for (size_t i = 0; i < NUM_TESTS; i++)
	{

		gen_random(qsort_c_set, NUM_ELEMENTS, MIN_RANDOM, MAX_RANDOM);
		validate_set = qsort_c_set; // Deep copy random into validate_set

		qsort_c(qsort_c_set.data(), qsort_c_set.size(), sizeof(int), compareInt);
		std::sort(validate_set.begin(), validate_set.end());

		if (!(qsort_c_set == validate_set))
		{
			std::cout << "ERROR" << std::endl;
			print(qsort_c_set);
			return 1;
		}
		else
		{
			std::cout << "PASSED: " << i << " / " << NUM_TESTS << std::endl;
		}

		qsort_c_set.clear();
		validate_set.clear();
	}
	return 0;
}

/* Comparator for qsort_c */
int compareInt(const void *a, const void *b)
{
	return (*(int *)a - *(int *)b);
}
