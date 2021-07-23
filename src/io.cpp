#include "io.hpp"

#include <algorithm>
#include <boost/iostreams/copy.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filter/zlib.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>

namespace fs = boost::filesystem;
namespace mp = boost::multiprecision;

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

void from_disk_txt(std::vector<mp::cpp_int> &vec, fs::path filename)
{
  std::ifstream in_file(filename.string());

  if (!in_file.is_open() || !in_file.good())
  {
    throw std::ios_base::failure("Couldn't open input file");
  }

  // Handle reading floats, but just cast them into ints
  mp::cpp_int buffer;
  while (in_file >> buffer)
  {
    vec.push_back(buffer);
  }

  in_file.close();
}

void from_disk_gz(std::vector<mp::cpp_int> &vec, fs::path filename)
{
  std::ifstream in_file(filename.string(),
                        std::ios_base::in | std::ios_base::binary);
  if (!in_file.is_open() || !in_file.good())
  {
    throw std::ios_base::failure("Couldn't open input file");
  }

  boost::iostreams::filtering_streambuf<boost::iostreams::input> inbuf;

  inbuf.push(boost::iostreams::gzip_decompressor());
  inbuf.push(in_file);

  // Convert streambuf to istream
  std::istream instream(&inbuf);

  // TODO: Handle floats here.
  mp::cpp_int line;
  while (instream >> line)
  {
    vec.push_back(line);
  }
  // Cleanup
  in_file.close();
}
