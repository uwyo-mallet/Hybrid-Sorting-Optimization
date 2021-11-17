#include <argp.h>

#include <algorithm>
#include <boost/filesystem.hpp>
#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/path.hpp>
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

#define VERSION                                                              \
  QST_VERSION "\n\tC COMPILER : " C_COMPILER_ID " " C_COMPILER_VERSION       \
              "\n\tCXX COMPILER : " CXX_COMPILER_ID " " CXX_COMPILER_VERSION \
              "\n\tType: " CMAKE_BUILD_TYPE                                  \
              "\n\tBOOST CPP INT: [" BOOST_CPP_INT                           \
              "]"                                                            \
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
    {"id", 'i', "ID", 0, "ID of this job."},
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
  int64_t id;
  fs::path in_file;
  std::string method;
  fs::path out_file;
  int64_t runs;
  int64_t threshold;
};

// Option parser
static std::string& trim(std::string&);
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

void write(struct arguments args, const size_t& size,
           const std::vector<boost::timer::cpu_times>& times);
void version_json();

struct arguments arguments;

int main(int argc, char** argv)
{
  std::vector<boost::timer::cpu_times> times;

  // Default CLI options
  arguments.description = "N/A";
  arguments.id = -1;
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

  // Cleanup, remove whitespace and lowercase.
  trim(arguments.method);
  std::transform(arguments.method.begin(), arguments.method.end(),
                 arguments.method.begin(),
                 [](unsigned char c) { return std::tolower(c); });

  // Check if the input method supports a threshold.
  // If not set to 0 for output.
  if (THRESHOLD_METHODS.find(arguments.method) == THRESHOLD_METHODS.end())
  {
    arguments.threshold = 0;
  }

#ifdef USE_BOOST_CPP_INT
  const std::vector<bmp::cpp_int> orig_data =
      from_disk<bmp::cpp_int>(arguments.in_file);
  std::vector<bmp::cpp_int> sorted_data = orig_data;
  std::sort(sorted_data.begin(), sorted_data.end());

  const size_t DATA_LEN = orig_data.size();
  bmp::cpp_int* to_sort = new bmp::cpp_int[DATA_LEN];
#else
  const std::vector<uint64_t> orig_data =
      from_disk<uint64_t>(arguments.in_file);
  std::vector<uint64_t> sorted_data = orig_data;
  std::sort(sorted_data.begin(), sorted_data.end());

  const size_t DATA_LEN = orig_data.size();
  uint64_t* to_sort = new uint64_t[DATA_LEN];
#endif

  bool checked = false;

  for (size_t i = 0; i < arguments.runs; i++)
  {
    std::copy(orig_data.begin(), orig_data.end(), to_sort);
    boost::timer::cpu_times res =
        time(arguments.method, arguments.threshold, to_sort, DATA_LEN);
    times.push_back(res);

    if (!checked)
    {
      if (!array_equal(to_sort, sorted_data.data(), DATA_LEN))
      {
        throw std::runtime_error("Post sort array not correctly sorted.");
      }
      checked = true;
    }
  }

  // Output
  write(arguments, DATA_LEN, times);

  delete[] to_sort;

  return EXIT_SUCCESS;
}

/** Remove leading and trailing whitespace. */
std::string& trim(std::string& s)
{
  const char* t = " \t\n\r\f\v";
  s = s.erase(s.find_last_not_of(t) + 1);
  s = s.erase(0, s.find_first_not_of(t));
  return s;
}

/** Parse a single CLI option. */
static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* args = (struct arguments*)state->input;

  switch (key)
  {
    case 'd':
      args->description = std::string(arg);
      break;

    case 'i':
      args->id = std::stoi(std::string(arg));
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

/** Write results out to a file or stdout.
 *
 * @param args: Input arguments when this binary was called
 * @param size: Total number of input elements from the loaded file
 * @param times: All sort times
 */
void write(const struct arguments args, const size_t& size,
           const std::vector<boost::timer::cpu_times>& times)
{
  if (args.out_file == "-")
  {
    std::cout << "ID: " << args.id << std::endl;
    std::cout << "Method: " << args.method << std::endl;
    std::cout << "Input: " << args.in_file << std::endl;
    std::cout << "Description: " << args.description << std::endl;
    std::cout << "Size: " << size << std::endl;
    std::cout << "Threshold: " << args.threshold << std::endl;
    for (boost::timer::cpu_times time : times)
    {
      std::cout << "Elapsed Wall Time (nanoseconds): " << time.wall
                << std::endl;
      std::cout << "Elapsed User Time (nanoseconds): " << time.user
                << std::endl;
      std::cout << "Elapsed System Time (nanoseconds): " << time.system
                << std::endl;
    }
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
    // There's a potential race here, but the OS usually handles it for me...
    out_file.seekg(0, std::ios::end);
    if (out_file.tellg() == 0)
    {
      out_file.clear();
      out_file << "id,method,input,description,size,threshold,wall_nsecs,user_"
                  "nsecs,system_nsecs"
               << std::endl;
    }

    // Write the actual data
    for (boost::timer::cpu_times time : times)
    {
      out_file << args.id << "," << args.method << "," << args.in_file << ","
               << args.description << "," << size << "," << args.threshold
               << "," << time.wall << "," << time.user << "," << time.system
               << std::endl;
    }

    out_file.close();
  }
}

/** Output version in machine-readable JSON to STDOUT. */
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
