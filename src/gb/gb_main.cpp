#include <argp.h>
#include <benchmark/benchmark.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
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

#define VERSION                                                              \
  QST_VERSION "\n\tC COMPILER : " C_COMPILER_ID " " C_COMPILER_VERSION       \
              "\n\tCXX COMPILER : " CXX_COMPILER_ID " " CXX_COMPILER_VERSION \
              "\n\tType: " CMAKE_BUILD_TYPE "\n\tASM Methods: [" ASM_ENABLED \
              "]"

const char* argp_program_version = VERSION;
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";

// Documentation
static char doc[] =
    "A simple CLI app to save the runtime of sorting using various methods.";
static char args_doc[] = "INPUT";

#define VERSION_JSON_SHORT_OPT 0x80
// #define COLS_SHORT_OPT 0x81
// #define VALS_SHORT_OPT 0x82
static struct argp_option options[] = {
    {"threshold", 't', "THRESH", 0, "Threshold to switch to insertion sort."},
    // {"cols", COLS_SHORT_OPT, "COLS", 0,
    //  "Additional columns to pass through to the output CSV."},
    // {"vals", VALS_SHORT_OPT, "VALS", 0,
    //  "Values to use for the additional columns."},
    {"version-json", VERSION_JSON_SHORT_OPT, 0, 0,
     "Output version information in machine readable format."},
    {0},
};

struct arguments
{
  fs::path in_file;
  int64_t threshold;
  // std::vector<std::string> cols;
  // std::vector<std::string> vals;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

std::vector<uint64_t> orig_data;

int main(int argc, char** argv)
{
  struct arguments arguments;

  // Default CLI options
  // Threshold is only used for supported sorting methods.
  arguments.threshold = 4;

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

  // Load the data
  orig_data = from_disk<uint64_t>(arguments.in_file);

  // Pass the rest of the CLI args to google benchmark, and run.
  int num_benchmark_parameters = benchmark_parameters.size();
  ::benchmark::Initialize(&num_benchmark_parameters,
                          benchmark_parameters.data());
  ::benchmark::RunSpecifiedBenchmarks();

  return 0;
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

      // case COLS_SHORT_OPT:
      //   args->cols = parse_comma_sep_args(std::string(arg));
      //   break;

      // case VALS_SHORT_OPT:
      //   args->vals = parse_comma_sep_args(std::string(arg));
      //   break;

    case VERSION_JSON_SHORT_OPT:
      // version_json();
      exit(EXIT_SUCCESS);

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
