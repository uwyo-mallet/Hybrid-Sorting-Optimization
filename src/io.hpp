#pragma once
#ifndef IO_HPP_
#define IO_HPP_

#include <boost/filesystem.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <cstddef>
#include <string>
#include <vector>

template <typename T>
void print(const std::vector<T> &arr);

void gen_random(std::vector<int> &res, const size_t num_elements, long long min,
                long long max);

void to_disk(const std::vector<int> &vec, std::string filename);

void from_disk_txt(std::vector<boost::multiprecision::cpp_int> &vec,
                   boost::filesystem::path filename);
void from_disk_gz(std::vector<boost::multiprecision::cpp_int> &vec,
                  boost::filesystem::path filename);

#endif /* IO_HPP_ */
