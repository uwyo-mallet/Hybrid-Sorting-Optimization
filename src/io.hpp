#pragma once
#ifndef _IO_HPP
#define _IO_HPP

#include <cstddef>
#include <string>
#include <vector>

template <typename T>
void print(const std::vector<T> &arr);

void gen_random(std::vector<int> &res, const size_t num_elements, long long min,
                long long max);

void to_disk(const std::vector<int> &vec, std::string filename);

void from_disk_txt(std::vector<int> &vec, std::string filename);
void from_disk_gz(std::vector<int> &vec, std::string filename);

#endif /* _IO_HPP */
