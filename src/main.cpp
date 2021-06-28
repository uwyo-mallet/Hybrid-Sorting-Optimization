#include <argp.h>

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "io.hpp"
#include "sort.hpp"

// Argument Parsing
const char* argp_program_version = "1.0.0";
const char* argp_program_bug_address = "jarulsam@uwyo.edu";

// Documentation
static char doc[] = "TODO";
static char args_doc[] = "INPUT";

// Accepted methods
static struct argp_option options[] = {
    {"method", 'm', "METHOD", 0, "Sorting method"},
    {"output", 'o', "FILE", 0, "Output to FILE instead of STDOUT"},
    {0},
};

struct arguments
{
  std::string in_file;
  std::string method;
  std::string out_file;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

// Sort function type
typedef void (*srt_func)(int[], const size_t&);

int main(int argc, char** argv)
{
  struct arguments arguments;

  // Default CLI options
  arguments.method = "vanilla_quicksort";
  arguments.out_file = "-";

  argp_parse(&argp, argc, argv, 0, 0, &arguments);

  srt_func method;
  // Map str name of function to actual function
  if (arguments.method == "vanilla_quicksort")
  {
    method = vanilla_quicksort;
  }
  else if (arguments.method == "insertion_sort")
  {
    method = insertion_sort;
  }
  else if (arguments.method == "qsort_c")
  {
    method = qsort_c;
  }
  else
  {
    std::cerr << "Invalid method selected." << std::endl;
    return EXIT_FAILURE;
  }

  // Load the data
  std::vector<int> data;
  try
  {
    from_disk(data, arguments.in_file);
  }
  catch (std::ios_base::failure& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  // Ensure the output is sorted
  bool valid = false;

  // Timing
  auto start_time = std::chrono::high_resolution_clock::now();

  method(data.data(), data.size());

  auto end_time = std::chrono::high_resolution_clock::now();
  auto elapsed_time = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);

  // Ensure the data is actually sorted correctly
  valid = is_sorted(data.data(), data.size());

  // Output
  if (arguments.out_file == "-")
  {
    std::cout << "Method: " << arguments.method << std::endl;
    std::cout << "Input: " << arguments.in_file << std::endl;
    std::cout << "Length: " << data.size() << std::endl;
    std::cout << "Elapsed Time (microseconds): " << elapsed_time.count()
              << std::endl;

    std::cout << "Valid: " << (valid ? "True" : "False") << std::endl;
  }
  else
  {
    // Open and verify
    std::fstream out_file(arguments.out_file,
                          std::ios::in | std::ios::out | std::ios::app);
    if (!out_file)
    {
      std::cerr << "Couldn't open destination file!" << std::endl;
      return EXIT_FAILURE;
    }

    // Check if file is empty, if so, add the CSV header.
    out_file.seekg(0, std::ios::end);
    if (out_file.tellg() == 0)
    {
      out_file.clear();
      out_file << "Method,Input,Length,Elapsed Time (microseconds),Valid"
               << std::endl;
    }

    out_file << arguments.method << "," << arguments.in_file << ","
             << data.size() << "," << elapsed_time.count() << ","
             << (valid ? "True" : "False") << std::endl;

    out_file.close();
  }

  return EXIT_SUCCESS;
}

/* Parse a single option. */
static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* args = (struct arguments*)state->input;

  switch (key)
  {
    case 'm':
      args->method = std::string(arg);
      break;

    case 'o':
      args->out_file = std::string(arg);
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 1)
      {
        // Too many arguments
        argp_usage(state);
      }
      args->in_file = std::string(arg);
      break;

    case ARGP_KEY_END:
      if (state->arg_num < 1)
      {
        // Not enough arguments
        argp_usage(state);
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}
// int compareInt(const void *a, const void *b);
//
// int main()
// {
//   constexpr size_t NUM_ELEMENTS = 1000000;
//   constexpr size_t NUM_TESTS = 1000;
//   constexpr size_t MIN_RANDOM = 0;
//   constexpr size_t MAX_RANDOM = 1000;
//   constexpr size_t MAX_PRINT = 25;
//
//   std::vector<int> qsort_c_set;
//   std::vector<int> validate_set;
//
//   for (size_t i = 0; i < NUM_TESTS; i++)
//   {
//     gen_random(qsort_c_set, NUM_ELEMENTS, MIN_RANDOM, MAX_RANDOM);
//     validate_set = qsort_c_set;  // Deep copy random into validate_set
//
//     qsort_c(qsort_c_set.data(), qsort_c_set.size(), sizeof(int), compareInt);
//     std::sort(validate_set.begin(), validate_set.end());
//
//     if (!(qsort_c_set == validate_set))
//     {
//       std::cout << "ERROR" << std::endl;
//       print(qsort_c_set);
//       return 1;
//     }
//     else
//     {
//       std::cout << "PASSED: " << i << " / " << NUM_TESTS << std::endl;
//     }
//
//     qsort_c_set.clear();
//     validate_set.clear();
//   }
//   return 0;
// }
//
// /* Comparator for qsort_c */
// int compareInt(const void *a, const void *b) { return (*(int *)a - *(int
// *)b); }
