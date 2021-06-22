#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

#include "io.hpp"

template <typename T>
void print(const std::vector<T> &arr)
{
	std::cout << "[ ";
	for (T i : arr)
	{
		std::cout << i << " ";
	}
	std::cout << "]" << std::endl;
}
template void print(const std::vector<int> &arr);
template void print(const std::vector<float> &arr);

void gen_random(std::vector<int> &res, const size_t num_elements, long long min,
				long long max)
{
	// Generating random numbers
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(min, max);

	for (size_t i = 0; i < num_elements; i++)
	{
		res.push_back(dist(rng));
	}
}

void to_disk(const std::vector<int> &vec, std::string filename)
{
	std::ofstream out_file(filename);

	if (!out_file.good())
	{
		std::cerr << "Couldn't open" << filename << std::endl;
		exit(1);
	}

	for (int i : vec)
	{
		out_file << i << std::endl;
	}

	out_file.close();
}

void from_disk(std::vector<int> &vec, std::string filename)
{
	std::ifstream in_file(filename);

	if (!in_file.good())
	{
		std::cerr << "Couldn't open" << filename << std::endl;
		exit(1);
	}

	int buffer;

	while (in_file >> buffer)
	{
		vec.push_back(buffer);
	}

	in_file.close();
}
