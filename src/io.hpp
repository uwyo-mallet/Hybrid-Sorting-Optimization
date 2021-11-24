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

/**
 * Load a text file to a vector.
 * Assumes that each element is either space or newline delimited.
 *
 * @param filename: Path to file to load.
 * @return Vector with contents of file.
 */
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

/**
 * Load a compressed text file to a vector.
 * Assumes that each element is either space or newline delimited.
 *
 * @param filename: Path to file to load.
 * @return Vector with contents of file.
 */
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

/**
 * Load a file to a vector based on the file extension.
 * Supports either .gz or .txt. Other extensions are assumed to be plain text.
 *
 * @param filename: Path to file to load.
 * @return Vector with contents of file.
 * @see from_disk_txt()
 * @see from_disk_gz()
 */
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
