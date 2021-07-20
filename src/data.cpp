#include <argp.h>

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <string>

#include "config.hpp"
#include "io.hpp"
#include "sort.hpp"

namespace fs = std::filesystem;

const size_t INCREMENT = 500000;
const size_t MIN_ELEMENTS = INCREMENT;
const size_t MAX_ELEMENTS = 15000000;

const char* argp_program_version = "1.0.0";
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";

// Documentation
static char doc[] = "Generate random data.";
static char args_doc[] = "";

// Accepted methods
static struct argp_option options[] = {
    {"output", 'o', "DIR", 0, "Directory to output data to (default ./data/)."},
    {0},
};

struct arguments
{
  std::string out_dir;
};

// Option parser
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};
struct arguments arguments;

void create_dirs(const fs::path& base_path,
                 const std::vector<std::string>& dirs);

void ascending(std::ofstream& out, const size_t num_elements);
void descending(std::ofstream& out, const size_t num_elements);
void random(std::ofstream& out, const size_t num_elements);
void single_num(std::ofstream& out, const size_t num_elements);

typedef void (*func)(std::ofstream& out, const size_t num_elements);
typedef std::pair<std::string, func> Pair;
typedef std::map<std::string, func> func_map;

int main(int argc, char** argv)
{
  // Default CLI options
  arguments.out_dir = "./run_data/";

  // String to function map
  func_map fmap;
  fmap.insert(Pair("ascending", &ascending));
  fmap.insert(Pair("descending", &descending));
  fmap.insert(Pair("random", &random));
  fmap.insert(Pair("single_num", &single_num));

  // Gather subdir names
  std::vector<std::string> subdirs;
  for (auto const& i : fmap)
  {
    subdirs.push_back(i.first);
  }

  // Ensure no parsing errors occured
  // The assumption is that argp_parse correctly alerts the user
  // as to what broke.
  if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0)
  {
    return EXIT_FAILURE;
  }

  fs::path out_dir = arguments.out_dir;

  create_dirs(out_dir, subdirs);
  for (auto const& f : fmap)
  {
    size_t i = 0;
    for (size_t num_elements = MIN_ELEMENTS; num_elements < MAX_ELEMENTS;
         num_elements += INCREMENT)
    {
      fs::path filename = std::to_string(i) + ".dat";
      fs::path subdir = f.first;
      fs::path path = (out_dir / subdir) / filename;

      std::ofstream out(path);
      f.second(out, num_elements);
      out.close();

      i++;
    }
  }
  std::cout << out_dir << std::endl;

  return EXIT_SUCCESS;
}

/* Parse a single option. */
static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* args = (struct arguments*)state->input;

  switch (key)
  {
    case 'o':
      args->out_dir = std::string(arg);
      break;

    case ARGP_KEY_ARG:
      if (state->arg_num >= 0)
      {
        // Too many arguments
        argp_usage(state);
      }
      break;

    case ARGP_KEY_END:
      if (state->arg_num > 0)
      {
        // Too many arguments
        argp_usage(state);
      }
      break;

    default:
      return ARGP_ERR_UNKNOWN;
  }

  return 0;
}

size_t rand(size_t min, size_t max)  // range : [min, max]
{
  static bool first = true;
  if (first)
  {
    srand(time(NULL));  // seeding for the first time only!
    first = false;
  }
  return min + rand() % ((max + 1) - min);
}

void create_dirs(const fs::path& base_path,
                 const std::vector<std::string>& dirs)
{
  // Remove existing data dir.
  if (fs::is_directory(base_path))
  {
    fs::remove_all(base_path);
  }

  // Create new directories
  fs::create_directory(base_path);
  for (std::string subdir : dirs)
  {
    fs::create_directory(base_path / subdir);
  }
}

void ascending(std::ofstream& out, const size_t num_elements)
{
  for (size_t i = 0; i < num_elements; i++)
  {
    out << i << std::endl;
  }
}

void descending(std::ofstream& out, const size_t num_elements)
{
  for (size_t i = num_elements; i > 0; i--)
  {
    out << i << std::endl;
  }
}

void random(std::ofstream& out, const size_t num_elements)
{
  for (size_t i = 0; i < num_elements; i++)
  {
    out << rand(0, num_elements) << std::endl;
  }
}

void single_num(std::ofstream& out, const size_t num_elements)
{
  for (size_t i = 0; i < num_elements; i++)
  {
    out << 42 << std::endl;
  }
}
