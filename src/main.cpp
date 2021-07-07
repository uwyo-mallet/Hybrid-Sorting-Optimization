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
const char* argp_program_version = "1.0.1";
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";

// Documentation
static char doc[] = "A simple CLI app to save the runtime of sorting using various methods.";
static char args_doc[] = "INPUT";

// Accepted methods
static struct argp_option options[] = {
    {"description", 'd', "DESCRIP", 0, "Short description of input data."},
    {"method", 'm', "METHOD", 0, "Sorting method"},
    {"output", 'o', "FILE", 0, "Output to FILE instead of STDOUT"},
    {"threshold", 't', "THRESH", 0, "Threshold to switch to insertion sort."},
    {0},
};

struct arguments
{
  std::string description;
  std::string in_file;
  std::string method;
  std::string out_file;
  int threshold;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

int main(int argc, char** argv)
{
  struct arguments arguments;

  // Default CLI options
  arguments.description = "N/A";
  arguments.method = "vanilla_quicksort";
  arguments.out_file = "-";
  arguments.threshold = 4;

  // Threshold is only used for supported sorting methods.

  // Ensure no parsing errors occured
  // The assumption is that argp_parse correctly alerts the user
  // as to what broke.
  if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0)
  {
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
  // Map str name of function to actual function
  if (arguments.method == "vanilla_quicksort")
  {
    vanilla_quicksort(data.data(), data.size());
  }
  else if (arguments.method == "insertion_sort")
  {
    insertion_sort(data.data(), data.size());
  }
  else if (arguments.method == "qsort_c")
  {
    qsort_c(data.data(), data.size(), arguments.threshold);
  }
  else
  {
    std::cerr << "Invalid method selected." << std::endl;
    return EXIT_FAILURE;
  }

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
    std::cout << "Description: " << arguments.description << std::endl;
    std::cout << "Size: " << data.size() << std::endl;
    std::cout << "Elapsed Time (microseconds): " << elapsed_time.count()
              << std::endl;

    std::cout << "Threshold: " << arguments.threshold << std::endl;
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
      out_file << "Method,Input,Description,Size,Elapsed Time "
                  "(microseconds),Threshold,Valid"
               << std::endl;
    }

    out_file << arguments.method << "," << arguments.in_file << ","
             << arguments.description << "," << data.size() << ","
             << elapsed_time.count() << "," << arguments.threshold << ","
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
    case 'd':
      args->description = std::string(arg);
      break;

    case 'm':
      args->method = std::string(arg);
      break;

    case 'o':
      args->out_file = std::string(arg);
      break;

    case 't':
      try
      {
        args->threshold = std::stoi(std::string(arg));
      }
      catch (std::invalid_argument)
      {
        std::cerr << "Invalid threshold: " << arg << std::endl;
        return 1;
      }
      // Ensure the threshold is > 0
      if (args->threshold <= 0)
      {
        std::cerr << "Invalid threshold: " << arg << std::endl;
        return 1;
      }
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
