#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "benchmark.h"
#include "data.h"
#include "platform.h"
#include "sort.h"

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

// Argument Parsing
const char* argp_program_version = "1.1.0";
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";
static const char doc[] =
    "Evaluating sorting algorithms with homebrew methods.";
static const char args_doc[] = "INFILE";

#define COLS_OPT 0x80
#define VALS_OPT 0x81
#define METH_OPT 0x82
#define DUMP_OPT 0x83

// clang-format off
static struct argp_option options[] = {
    {"output",       'o',      "FILE",   0, "Output to FILE instead of STDOUT"                 },
    {"method",       'm',      "METHOD", 0, "Sorting method to use."                           },
    {"runs",         'r',      "N",      0, "Number of times to repeatedly sort the same data."},
    {"threshold",    't',      "THRESH", 0, "Threshold to switch sorting methods."             },
    {"cols",         COLS_OPT, "COLS",   0, "Columns to pass through to CSV."                  },
    {"vals",         VALS_OPT, "VALS",   0, "Values to pass through to CSV."                   },
    {"show-methods", METH_OPT, "TYPE",   OPTION_ARG_OPTIONAL, "Print supported methods"        },
    {"dump-sorted",  DUMP_OPT, "TYPE",   OPTION_ARG_OPTIONAL, "Dump the resulting sorted data" },
    {0},
};
// clang-format on

struct arguments
{
  char* in_file;
  char* out_file;
  ssize_t method;
  int64_t runs;
  int64_t threshold;
  char* cols;
  char* vals;

  size_t in_file_len;
  bool is_threshold_method;
  bool print_standard_methods;
  bool print_threshold_methods;
  bool dump_sorted;
};
static error_t parse_opt(int key, char* arg, struct argp_state* state);
static struct argp argp = {options, parse_opt, args_doc, doc};

// Helper methods
ssize_t is_method(char* m, bool* is_threshold_method);
bool is_sorted(sort_t* data, const size_t n);
int write_results(const struct arguments* args, const struct times* results,
                  const size_t num_results);

// Not being able to keep this with the methods enum is a little unfortunate...
const char* METHODS[] = {
    "qsort",
    "msort_heap",
    "basic_ins",
    "fast_ins",
    "shell",
    NULL, /* Methods from this point support a threshold value. */
    "msort_heap_with_old_ins",
    "msort_heap_with_basic_ins",
    "msort_heap_with_shell",
    "msort_heap_with_fast_ins",
    "msort_heap_with_network",
    "msort_with_network",
    "quicksort_with_ins",
    "quicksort_with_fast_ins",
    NULL,
};

int main(int argc, char** argv)
{
  // Default arguments
  struct arguments arguments = {0};
  arguments.runs = 1;
  arguments.threshold = 4;

  if (argp_parse(&argp, argc, argv, 0, 0, &arguments) != 0)
  {
    return EXIT_FAILURE;
  }

  // Handle printing
  if (arguments.print_standard_methods || arguments.print_threshold_methods)
  {
    const char** std_ptr = METHODS;
    if (arguments.print_standard_methods)
    {
      while (*std_ptr)
      {
        printf("%s\n", *std_ptr);
        std_ptr++;
      }
    }

    if (arguments.print_threshold_methods)
    {
      const char** threshold_ptr = METHODS;
      // Increment past all the nonthreshold methods.
      while (*threshold_ptr++)
        ;
      while (*threshold_ptr)
      {
        printf("%s\n", *threshold_ptr);
        threshold_ptr++;
      }
    }

    return EXIT_SUCCESS;
  }

  // Detect if txt or gz input.
  const size_t in_file_len = strlen(arguments.in_file);
  bool is_txt = true;
  if (in_file_len >= 3)
  {
    if (strcmp(&arguments.in_file[in_file_len - 3], ".gz") == 0)
    {
      is_txt = false;
    }
  }

  // Read the input data into a buffer
  size_t n = 0;
  sort_t* data = NULL;
  FILE* in_file;
  if (is_txt)
  {
    in_file = fopen(arguments.in_file, "r");
    if (in_file == NULL)
    {
      perror(arguments.in_file);
      return EXIT_FAILURE;
    }
    if (read_txt(in_file, &data, &n) != SUCCESS)
    {
      fprintf(stderr, "Error reading input file '%s'\n", arguments.in_file);
      fclose(in_file);
      return EXIT_FAILURE;
    }
  }
  else
  {
    in_file = fopen(arguments.in_file, "rb");
    if (in_file == NULL)
    {
      perror(arguments.in_file);
      return EXIT_FAILURE;
    }
    if (read_zip(in_file, &data, &n) != Z_OK)
    {
      fprintf(stderr, "Error reading input file '%s'\n", arguments.in_file);
      fclose(in_file);
      return EXIT_FAILURE;
    }
  }
  fclose(in_file);

  arguments.in_file_len = n;
  sort_t* to_sort_buffer = malloc(sizeof(sort_t) * n);
  if (to_sort_buffer == NULL)
  {
    free(data);
    perror("malloc");
    return EXIT_FAILURE;
  }

  // Set up timer objects
  struct times* results = calloc(sizeof(struct times), arguments.runs);
  if (results == NULL)
  {
    free(data);
    free(to_sort_buffer);
    perror("calloc");
    return EXIT_FAILURE;
  }

  // Set up performance counters
  struct perf_fds perf;
  perf_event_open(&perf);

  bool checked = false;
  for (int64_t i = 0; i < arguments.runs; ++i)
  {
    memcpy(to_sort_buffer, data, n * sizeof(sort_t));

    results[i] = measure_sort_time(
        arguments.method, to_sort_buffer, n, arguments.threshold, &perf);

    if (!checked)
    {
      checked = true;
      if (!is_sorted(to_sort_buffer, n))
      {
        fprintf(stderr, "Array was not sorted correctly!\n");
        for (size_t j = 0; j < n; ++j)
        {
#ifdef SORT_LARGE_STRUCTS
          fprintf(stderr, "%li\n", to_sort_buffer[j].val);
#else
          fprintf(stderr, "%li\n", to_sort_buffer[j]);
#endif  // SORT_LARGE_STRUCTS
        }
        free(data);
        free(to_sort_buffer);
        free(results);
        perf_event_close(&perf);
        return EXIT_FAILURE;
      }
    }
  }

  if (arguments.dump_sorted)
  {
#define DEBUG_DUMP_FILENAME "./debug_dump.txt"
    FILE* dump_fp = fopen(DEBUG_DUMP_FILENAME, "w");
    if (dump_fp == NULL)
    {
      perror(DEBUG_DUMP_FILENAME);
      return EXIT_FAILURE;
    }
    for (size_t i = 0; i < n; ++i)
    {
#ifdef SORT_LARGE_STRUCTS
      fprintf(dump_fp, "%li\n", to_sort_buffer[i].val);
#else
      fprintf(dump_fp, "%li\n", to_sort_buffer[i]);
#endif  // SORT_LARGE_STRUCTS
    }
    fclose(dump_fp);
  }

  if (write_results(&arguments, results, arguments.runs) != SUCCESS)
  {
    // Cleanup after thy self.
    free(data);
    free(to_sort_buffer);
    free(results);
    perf_event_close(&perf);
    return EXIT_FAILURE;
  }

  // Cleanup after thy self.
  free(data);
  free(to_sort_buffer);
  free(results);
  perf_event_close(&perf);
  return EXIT_SUCCESS;
}

static error_t parse_opt(int key, char* arg, struct argp_state* state)
{
  struct arguments* args = (struct arguments*)state->input;

  switch (key)
  {
    case 'o':
      args->out_file = arg;
      break;
    case 'm':
      if ((args->method = is_method(arg, &args->is_threshold_method)) < 0)
      {
        printf("Invalid method selected: '%s'\n", arg);
        return ARGP_ERR_UNKNOWN;
      }
      break;
    case 'r':
      args->runs = strtoll(arg, NULL, 10);
      // TODO: Error check here;
      fprintf(stderr, "[WARN]: No error checking for args->runs\n");
      break;
    case 't':
      args->threshold = strtoll(arg, NULL, 10);
      // TODO: Error check here;
      fprintf(stderr, "[WARN]: No error checking for args->threshold\n");
      break;
    case COLS_OPT:
      args->cols = arg;
      break;
    case VALS_OPT:
      args->vals = arg;
      break;
    case METH_OPT:
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
    case DUMP_OPT:
      args->dump_sorted = true;
      break;
    case ARGP_KEY_ARG:
      if (state->arg_num >= 1)
      {
        // Too many arguments
        argp_usage(state);
      }
      args->in_file = arg;
      break;
    case ARGP_KEY_END:
      if (state->arg_num < 1 && !args->print_standard_methods &&
          !args->print_threshold_methods)
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

ssize_t is_method(char* m, bool* is_threshold_method)
{
  // Lowercase the method name
  for (size_t i = 0; m[i]; ++i)
  {
    m[i] = tolower(m[i]);
  }

  for (size_t i = 0; i < ARRAY_SIZE(METHODS); ++i)
  {
    if (METHODS[i] == NULL)
    {
      *is_threshold_method = true;
      continue;
    }

    if (strcmp(m, METHODS[i]) == 0)
    {
      return i;
    }
  }

  return -1;
}

int write_results(const struct arguments* args, const struct times* results,
                  const size_t num_results)
{
  FILE* out_file;
  bool write_header = true;

  if (args->out_file == NULL)
  {
    out_file = stdout;
  }
  else
  {
    write_header = access(args->out_file, F_OK) != 0;
    out_file = fopen(args->out_file, "a");
    if (out_file == NULL)
    {
      perror(args->out_file);
      return FAIL;
    }
  }

  if (write_header)
  {
    fprintf(out_file,
            "method,"
            "input,"
            "size,"
            "threshold,"
            "wall_nsecs,"
            "user_nsecs,"
            "system_nsecs,"
            "hw_cpu_cycles,"
            "hw_instructions,"
            "hw_cache_references,"
            "hw_cache_misses,"
            "hw_branch_instructions,"
            "hw_branch_misses,"
            "hw_bus_cycles,"
            "sw_cpu_clock,"
            "sw_task_clock,"
            "sw_page_faults,"
            "sw_context_switches,"
            "sw_cpu_migrations");
    if (args->vals != NULL)
    {
      fprintf(out_file, ",%s", args->cols);
    }
    fputc('\n', out_file);
  }

  for (size_t i = 0; i < num_results; ++i)
  {
    const struct times r = results[i];
    const intmax_t wall = (r.wall_secs * 1000000000) + r.wall_nsecs;
    // clang-format off
    fprintf(out_file,
            "%s,"
            "%s,"
            "%lu,"
            "%lu,"
            "%li,"
            "%lu,"
            "%lu,"
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64 ","
            "%" PRIu64,
            METHODS[args->method],
            args->in_file,
            args->in_file_len,
            args->is_threshold_method ? args->threshold : 0,
            wall,
            r.user,
            r.system,
            r.perf.count_hw_cpu_cycles,
            r.perf.count_hw_instructions,
            r.perf.count_hw_cache_references,
            r.perf.count_hw_cache_misses,
            r.perf.count_hw_branch_instructions,
            r.perf.count_hw_branch_misses,
            r.perf.count_hw_bus_cycles,
            r.perf.count_sw_cpu_clock,
            r.perf.count_sw_task_clock,
            r.perf.count_sw_page_faults,
            r.perf.count_sw_context_switches,
            r.perf.count_sw_cpu_migrations);
    // clang-format on

    if (args->vals != NULL)
    {
      fprintf(out_file, ",%s", args->vals);
    }

    fputc('\n', out_file);
  }

  if (out_file != stdout)
  {
    fclose(out_file);
  }
  return SUCCESS;
}

bool is_sorted(sort_t* data, const size_t n)
{
  for (size_t i = 0; i < n - 1; ++i)
  {
#ifdef SORT_LARGE_STRUCTS
    if (data[i + 1].val < data[i].val)
    {
      return false;
    }
#else
    if (data[i + 1] < data[i])
    {
      return false;
    }
#endif
  }

  return true;
}
