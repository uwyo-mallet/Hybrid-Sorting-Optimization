#include "io.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

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

void from_disk(std::vector<int> &vec, std::string filename)
{
  std::ifstream in_file(filename);

  if (!in_file.is_open() || !in_file.good())
  {
    throw std::ios_base::failure("Couldn't open input file");
  }

  int buffer;
  while (in_file >> buffer)
  {
    vec.push_back(buffer);
  }

  in_file.close();
}
