#include "io.hpp"

#include <algorithm>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
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

void from_disk_txt(std::vector<int> &vec, std::string filename)
{
  std::ifstream in_file(filename);

  if (!in_file.is_open() || !in_file.good())
  {
    throw std::ios_base::failure("Couldn't open input file");
  }

  // Handle reading floats, but just cast them into ints
  float buffer;
  while (in_file >> buffer)
  {
    vec.push_back((int)buffer);
  }

  in_file.close();
}

void from_disk_gz(std::vector<int> &vec, std::string filename)
{
  std::ifstream in_file(filename, std::ios_base::in | std::ios_base::binary);
  if (!in_file.is_open() || !in_file.good())
  {
    throw std::ios_base::failure("Couldn't open input file");
  }

  boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;

  inbuf.push(boost::iostreams::gzip_decompressor());
  inbuf.push(in_file);

  // Convert streambuf to istream
  std::istream instream(&inbuf);

  // Handle reading floats, but just cast them into ints
  float line;
  while (instream >> line)
  {
    vec.push_back((int)line);
  }
  // Cleanup
  in_file.close();
}
