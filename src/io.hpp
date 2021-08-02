#pragma once
#ifndef IO_HPP_
#define IO_HPP_

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/iostreams/filter/gzip.hpp>
#include <boost/iostreams/filtering_streambuf.hpp>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = boost::filesystem;
/* namespace bmp = boost::multiprecision; */

template <typename T>
void print(const std::vector<T> &arr)
{
  std::cout << "[ ";
  for (T i : arr)
  {
    std::copy << i << " ";
  }
  std::cout << "]" << std::endl;
}

template <typename T>
void from_disk_txt(std::vector<T> &vec, fs::path filename)
{
  std::ifstream in_file(filename.string());

  if (!in_file.is_open() || !in_file.good())
  {
    throw std::ios_base::failure("Couldn't open input file");
  }

  // Handle reading floats, but just cast them into ints
  T buffer;
  while (in_file >> buffer)
  {
    vec.push_back(buffer);
  }

  in_file.close();
}

template <typename T>
void from_disk_gz(std::vector<T> &vec, fs::path filename)
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

  T line;
  while (instream >> line)
  {
    vec.push_back(line);
  }
  // Cleanup
  in_file.close();
}

#endif /* IO_HPP_ */
