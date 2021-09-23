#include <argp.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <csignal>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <set>
#include <string>

#include "config.hpp"
#include "exp.hpp"
#include "io.hpp"
#include "platform.hpp"
#include "sort.h"

namespace fs = boost::filesystem;

#define VERSION                                                             \
  QST_VERSION "\n\tC COMPILER: " C_COMPILER_ID " " C_COMPILER_VERSION       \
              "\n\tCXX COMPILER: " CXX_COMPILER_ID " " CXX_COMPILER_VERSION \
              "\n\tType: " CMAKE_BUILD_TYPE                                 \
              "\n\tBOOST CPP INT: [" BOOST_CPP_INT                          \
              "]"                                                           \
              "\n\tASM Methods: [" ASM_ENABLED "]"

const char* argp_program_version = VERSION;
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";

// Documentation
static char doc[] =
    "A simple CLI app to save the runtime of sorting using various methods.";
static char args_doc[] = "INPUT";

// Accepted methods
#define VERSION_JSON_SHORT_OPT 0x80
static struct argp_option options[] = {
    {"description", 'd', "DESCRIP", 0, "Short description of input data."},
    {"method", 'm', "METHOD", 0, "Sorting method."},
    {"output", 'o', "FILE", 0, "Output to FILE instead of STDOUT."},
    {"runs", 'r', "N", 0, "Number of times to sort the same data. Default: 1"},
    {"threshold", 't', "THRESH", 0, "Threshold to switch to insertion sort."},
    {"version-json", VERSION_JSON_SHORT_OPT, 0, 0,
     "Output version information in machine readable format."},
    {0},
};

struct arguments
{
  std::string description;
  fs::path in_file;
  std::string method;
  fs::path out_file;
  int64_t runs;
  int64_t threshold;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

void signal_handler(int signum);
void write(struct arguments args, const size_t& size,
           const std::vector<std::string>& times);
void version_json();

struct arguments arguments;
std::vector<std::string> times;
size_t size = 0;

int main(int argc, char** argv)
{
  // Default CLI options
  arguments.description = "N/A";
  arguments.method = "qsort_c";
  arguments.out_file = "-";
  arguments.runs = 1;
  // Threshold is only used for supported sorting methods.
  arguments.threshold = 4;

  // Ensure no parsing errors occured
  // The assumption is that argp_parse correctly alerts the user
  // as to what broke.
  if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0)
  {
    return EXIT_FAILURE;
  }

  // The only sort with a supported threshold is qsort_c,
  // so otherwise, just set to 0.
  if (!THRESHOLD_METHODS.count(arguments.method))
  {
    arguments.threshold = 0;
  }

#ifdef USE_BOOST_CPP_INT
  const std::vector<bmp::cpp_int> orig_data =
      from_disk<bmp::cpp_int>(arguments.in_file);
  std::vector<bmp::cpp_int> sorted_data = orig_data;
  std::vector<bmp::cpp_int> data;
#else
  const std::vector<uint64_t> orig_data =
      from_disk<uint64_t>(arguments.in_file);
  std::vector<uint64_t> sorted_data = orig_data;
  std::vector<uint64_t> data;
#endif

  std::sort(sorted_data.begin(), sorted_data.end());
  size = orig_data.size();

  // Signal handlers
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

  for (size_t i = 0; i < arguments.runs; i++)
  {
    data = orig_data;
    times.push_back(std::to_string(
        time(arguments.method, arguments.threshold, data, sorted_data)));
    data.clear();
  }

  // Output
  try
  {
    write(arguments, size, times);
  }
  catch (std::ios_base::failure& e)
  {
    std::cerr << e.what() << std::endl;
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
      args->out_file = fs::path(arg);
      break;

    case 'r':
      args->runs = std::stoi(std::string(arg));
      if (args->runs <= 0)
      {
        throw std::invalid_argument("Runs must be > 0");
      }
      break;

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

    case VERSION_JSON_SHORT_OPT:
      version_json();
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

void signal_handler(int signum)
{
  // Try to write the output on failure.
  try
  {
    write(arguments, size, times);
  }
  catch (std::ios_base::failure& e)
  {
    std::cerr << e.what() << std::endl;
  }
  exit(EXIT_FAILURE);
}

void write(struct arguments args, const size_t& size,
           const std::vector<std::string>& times)
{
  if (args.out_file == "-")
  {
    std::cout << "Method: " << args.method << std::endl;
    std::cout << "Input: " << args.in_file << std::endl;
    std::cout << "Description: " << args.description << std::endl;
    std::cout << "Size: " << size << std::endl;
    for (std::string time : times)
    {
      std::cout << "Elapsed Time (microseconds): " << time << std::endl;
    }

    std::cout << "Threshold: " << args.threshold << std::endl;
  }
  else
  {
    // Open and verify
    std::fstream out_file(args.out_file.string(),
                          std::ios::in | std::ios::out | std::ios::app);
    if (!out_file)
    {
      throw std::ios_base::failure("Couldn't open destination file!");
    }

    // Check if file is empty, if so, add the CSV header.
    out_file.seekg(0, std::ios::end);
    if (out_file.tellg() == 0)
    {
      out_file.clear();
      out_file << "method,input,description,size,elapsed_usecs,threshold"
               << std::endl;
    }

    // Write the actual data
    for (std::string time : times)
    {
      out_file << args.method << "," << args.in_file << "," << args.description
               << "," << size << "," << time << "," << args.threshold
               << std::endl;
    }

    out_file.close();
  }
}

void version_json()
{
  std::cout << "{" << std::endl;

  // version
  std::cout << "\t\"version:\": "
            << "\"" QST_VERSION "\""
            << "," << std::endl;

  // C compiler id
  std::cout << "\t\"c_compiler_id\": "
            << "\"" C_COMPILER_ID "\""
            << "," << std::endl;

  // C compiler version
  std::cout << "\t\"c_compiler_version\": "
            << "\"" C_COMPILER_VERSION "\""
            << "," << std::endl;

  // C++ compiler id
  std::cout << "\t\"cxx_compiler_id\": "
            << "\"" CXX_COMPILER_ID "\""
            << "," << std::endl;

  // C++ compiler version
  std::cout << "\t\"cxx_compiler_version\": "
            << "\"" CXX_COMPILER_VERSION "\""
            << "," << std::endl;

  // boost_cpp_int
  std::cout << "\t\"boost_cpp_int\": ";
#ifdef USE_BOOST_CPP_INT
  std::cout << "1";
#else
  std::cout << "0";
#endif  // USE_BOOST_CPP_INT
  std::cout << "," << std::endl;

  // asm_enabled
  std::cout << "\t\"asm_enabled\": ";
#ifdef ASM_ENABLED
  std::cout << "1";
#else
  std::cout << "0";
#endif  // USE_BOOST_CPP_INT
  std::cout << std::endl;

  std::cout << "}" << std::endl;
}
