#include <argp.h>
#include <benchmark/benchmark.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/path.hpp>

// Older versions of boost require this file be included.
#if __has_include(<boost/filesystem/directory.h>)
#include <boost/filesystem/directory.h>
#endif

#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "benchmark.hpp"
#include "config.hpp"
#include "exp.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "sort.h"
#include "utils.hpp"

namespace fs = boost::filesystem;

const char* argp_program_version = VERSION(GB_QST_VERSION);
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";

// Documentation
static char doc[] = GB_QST_DESCRIPTION;
static char args_doc[] = "INPUT";

#define VERSION_OPT 0x80
#define METHODS_OPT 0x82

// clang-format off
static struct argp_option options[] = {
    {"threshold", 't', "THRESH", 0, "Threshold to switch to insertion sort."},
    {"version-json", VERSION_OPT, 0, 0,
     "Output version information in machine readable format."},
    {"show-methods", METHODS_OPT, "TYPE", OPTION_ARG_OPTIONAL,
     "Print all supported methods or of type 'TYPE' (threshold, nonthreshold)."},
    {0},
};
// clang-format on

struct arguments
{
  fs::path in_file;
  int64_t threshold;

  bool print_standard_methods;
  bool print_threshold_methods;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

// Globals for test fixture
std::vector<uint64_t> orig_data;
uint64_t threshold;

int main(int argc, char** argv)
{
  struct arguments arguments;

  // Default CLI options
  // Threshold is only used for supported sorting methods.
  arguments.threshold = 4;
  arguments.print_standard_methods = false;
  arguments.print_threshold_methods = false;

  std::vector<char*> benchmark_parameters = {argv[0]};
  int num_args = argc;

  for (int i = 0; i < argc; i++)
  {
    if (std::string(argv[i]).find("--benchmark") != std::string::npos)
    {
      // Stop argp from finding the rest of these.
      num_args = i;

      // Found a google benchmark parameter, save them to their own vector.
      while (i < argc)
      {
        benchmark_parameters.push_back(argv[i]);
        i++;
      }

      break;
    }
  }

  // Ensure no parsing errors occured
  // The assumption is that argp_parse correctly alerts the user
  // as to what broke.
  if (argp_parse(&argp, num_args, argv, 0, 0, &arguments) != 0)
  {
    return EXIT_FAILURE;
  }

  if (arguments.print_standard_methods || arguments.print_threshold_methods)
  {
    if (arguments.print_standard_methods)
    {
      for (const std::string& i : METHODS)
      {
        std::cout << i << "\n";
      }
    }
    if (arguments.print_threshold_methods)
    {
      for (const std::string& i : THRESHOLD_METHODS)
      {
        std::cout << i << "\n";
      }
    }

    return EXIT_SUCCESS;
  }

  // Pass the rest of the CLI args to google benchmark.
  int num_benchmark_parameters = benchmark_parameters.size();
  ::benchmark::Initialize(&num_benchmark_parameters,
                          benchmark_parameters.data());
  if (::benchmark::ReportUnrecognizedArguments(num_benchmark_parameters,
                                               benchmark_parameters.data()))
  {
    return EXIT_FAILURE;
  }

  // Load the data
  orig_data = from_disk<uint64_t>(arguments.in_file);
  threshold = arguments.threshold;

  ::benchmark::AddCustomContext("input", arguments.in_file.string());
  ::benchmark::AddCustomContext("size", std::to_string(orig_data.size()));
  ::benchmark::AddCustomContext("threshold", std::to_string(threshold));

  // Run the actual benchmarks
  ::benchmark::RunSpecifiedBenchmarks();

  return EXIT_SUCCESS;
}

/** Parse a single CLI option. */
static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* args = (struct arguments*)state->input;

  switch (key)
  {
    case 't':
      try
      {
        args->threshold = std::stoi(std::string(arg));
      }
      catch (std::invalid_argument&)
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
    case VERSION_OPT:
      // version_json();
      exit(EXIT_SUCCESS);
    case METHODS_OPT:
      if (arg != NULL)
      {
        if (strcmp(arg, "standard") == 0 || strcmp(arg, "nonthreshold") == 0)
        {
          args->print_standard_methods = true;
        }
        else if (strcmp(arg, "threshold") == 0)
        {
          args->print_threshold_methods = true;
        }
        else
        {
          args->print_standard_methods = true;
          args->print_threshold_methods = true;
        }
        break;
      }
      args->print_standard_methods = true;
      args->print_threshold_methods = true;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1)
      {
        // Too many arguments
        argp_usage(state);
      }
      args->in_file = fs::path(arg);
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
