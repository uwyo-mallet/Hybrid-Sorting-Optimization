#include <argp.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <chrono>
#include <csignal>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <thread>

#include "config.hpp"
#include "io.hpp"
#include "sort.hpp"

namespace fs = boost::filesystem;
namespace mp = boost::multiprecision;

// Argument Parsing
#define VERSION \
  QST_VERSION "\nCompiled with: " CXX_COMPILER_ID " " CXX_COMPILER_VERSION

const char* argp_program_version = VERSION;
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";

// Documentation
static char doc[] =
    "A simple CLI app to save the runtime of sorting using various methods.";
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
  fs::path in_file;
  std::string method;
  fs::path out_file;
  int threshold;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

void signal_handler(int signum);
void write(struct arguments args, size_t size, std::string time, bool valid);

struct arguments arguments;
bool valid = false;
std::string elapsed_time = "-1";
size_t size = 0;

int main(int argc, char** argv)
{
  // Default CLI options
  arguments.description = "N/A";
  arguments.method = "qsort_c";
  arguments.out_file = "-";
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
  if (arguments.method != "qsort_c")
  {
    arguments.threshold = 0;
  }

  // Load the data
  std::vector<mp::cpp_int> data;
  try
  {
    from_disk_gz(data, arguments.in_file);
  }
  catch (std::ios_base::failure& e)
  {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }
  size = data.size();

  // Signal handler
  signal(SIGINT, signal_handler);
  signal(SIGTERM, signal_handler);

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
  else if (arguments.method == "std")
  {
    std::sort(data.begin(), data.end());
  }
  else
  {
    std::cerr << "Invalid method selected." << std::endl;
    return EXIT_FAILURE;
  }

  auto end_time = std::chrono::high_resolution_clock::now();
  auto runtime = std::chrono::duration_cast<std::chrono::microseconds>(
      end_time - start_time);

  // Convert runtime to string
  elapsed_time = std::to_string((size_t)runtime.count());

  // Ensure the data is actually sorted correctly
  valid = is_sorted(data.data(), data.size());

  // Output
  try
  {
    write(arguments, data.size(), elapsed_time, valid);
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
      args->out_file = std::string(arg);
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

void signal_handler(int signum)
{
  // Try to write the output on failure.
  try
  {
    write(arguments, size, elapsed_time, valid);
  }
  catch (std::ios_base::failure& e)
  {
    std::cerr << e.what() << std::endl;
  }
  exit(EXIT_FAILURE);
}

void write(struct arguments args, size_t size, std::string time, bool valid)
{
  if (args.out_file == "-")
  {
    std::cout << "Method: " << args.method << std::endl;
    std::cout << "Input: " << args.in_file << std::endl;
    std::cout << "Description: " << args.description << std::endl;
    std::cout << "Size: " << size << std::endl;
    std::cout << "Elapsed Time (microseconds): " << elapsed_time << std::endl;

    std::cout << "Threshold: " << args.threshold << std::endl;
    std::cout << "Valid: " << (valid ? "True" : "False") << std::endl;
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
      out_file << "Method,Input,Description,Size,Elapsed Time "
                  "(microseconds),Threshold,Valid"
               << std::endl;
    }

    // Write the actual data
    out_file << args.method << "," << args.in_file << "," << args.description
             << "," << size << "," << time << "," << args.threshold << ","
             << (valid ? "True" : "False") << std::endl;

    out_file.close();
  }
}
