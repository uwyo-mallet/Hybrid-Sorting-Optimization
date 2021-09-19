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

template <typename T>
std::vector<T> from_disk_txt(const fs::path &filename)
{
  std::vector<T> vec;
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

  return vec;
}

template <typename T>
std::vector<T> from_disk_gz(const fs::path &filename)
{
  std::vector<T> vec;
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

  return vec;
}

template <typename T>
std::vector<T> from_disk(const fs::path &filename)
{
  if (filename.extension().string() == ".gz")
  {
    return from_disk_gz<T>(filename);
  }
  return from_disk_txt<T>(filename);
}

#endif /* IO_HPP_ */
