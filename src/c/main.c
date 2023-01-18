#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#include "benchmark.h"
#include "platform.h"

#define ARRAY_SIZE(x) ((sizeof x) / (sizeof *x))

// zlib specific
#define SET_BINARY_MODE(file)
#define CHUNK 32768
#define WINDOW_BITS 15
#define ENABLE_ZLIB_GZIP 32

#define BILLION 1000000000

// Argument Parsing
const char* argp_program_version = "TODO";
const char* argp_program_bug_address = "<jarulsam@uwyo.edu>";
static const char doc[] = "TODO";
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

ssize_t is_method(char* m, bool* is_threshold_method);
int read_txt(FILE* fp, int64_t** dst, size_t* n);
int read_zip(FILE* fp, int64_t** dst, size_t* n);
int write_results(const struct arguments* args, const struct times* results,
                  const size_t num_results);
bool is_sorted(int64_t* data, const size_t n);

typedef enum
{
  SUCCESS,       /* OK */
  FAIL,          /* An error occured, and was already reported with perror(). */
  PARSE_ERROR,   /* strtoll() parse error. */
  UNKNOWN_ERROR, /* Take a guess... */
} QST_RET;

// Not being able to keep this with the methods enum is a little unfortunate...
const char* METHODS[] = {
    "qsort",
    "msort_heap",
    NULL, /* Methods from this point support a threshold value. */
    "msort_heap_hybrid_ins",
    "msort_heap_with_basic_ins",
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
  int64_t* data = NULL;
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
  int64_t* to_sort_buffer = malloc(sizeof(int64_t) * n);
  if (to_sort_buffer == NULL)
  {
    free(data);
    perror("malloc");
    return EXIT_FAILURE;
  }

  struct times* results = calloc(sizeof(struct times), arguments.runs);
  if (results == NULL)
  {
    free(data);
    free(to_sort_buffer);
    perror("calloc");
    return EXIT_FAILURE;
  }

  bool checked = false;
  for (int64_t i = 0; i < arguments.runs; ++i)
  {
    memcpy(to_sort_buffer, data, n * sizeof(int64_t));
    results[i] = measure_sort_time(
        arguments.method, to_sort_buffer, n, arguments.threshold);
    if (!checked)
    {
      checked = true;
      if (!is_sorted(to_sort_buffer, n))
      {
        fprintf(stderr, "Array was not sorted correctly!\n");
        for (size_t j = 0; j < n; ++j)
        {
          fprintf(stderr, "%li\n", to_sort_buffer[i]);
        }
        free(data);
        free(to_sort_buffer);
        free(results);
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
      fprintf(dump_fp, "%li\n", to_sort_buffer[i]);
    }
    fclose(dump_fp);
  }

  if (write_results(&arguments, results, arguments.runs) != SUCCESS)
  {
    // Cleanup after thy self.
    free(data);
    free(to_sort_buffer);
    free(results);
    return EXIT_FAILURE;
  }

  // Cleanup after thy self.
  free(data);
  free(to_sort_buffer);
  free(results);
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
        printf("Invalid method selected: '%s\n'", arg);
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

/*
 * TODO
 */
int read_txt(FILE* fp, int64_t** dst, size_t* n)
{
  char* endptr = NULL;

  // Allocate a buffer for data from text file.
  size_t alloc = CHUNK;
  *n = 0;
  *dst = malloc(sizeof(int64_t) * alloc);
  if (dst == NULL)
  {
    perror("memory");
    exit(ENOMEM);
  }

  // Track errors from stroll
  errno = 0;

  char line[1024];
  while (fgets(line, 1024, fp))
  {
    int64_t val = strtoll(line, &endptr, 10);

    if (errno == ERANGE || errno == EINVAL || endptr == line || *endptr == 0)
    {
      // Some error occurred in parsing
      free(*dst);
      *dst = NULL;

      if (errno == 0)
      {
        errno = EINVAL;
      }
      return PARSE_ERROR;
    }

    if (*n > alloc)
    {
      // Increase the buffer size as necessary.
      alloc *= 2;
      *dst = realloc(*dst, alloc);
      if (*dst == NULL)
      {
        perror("memory");
        exit(ENOMEM);
      }
    }

    (*dst)[*n] = val;
    ++(*n);
  }
  if (ferror(fp))
  {
    free(*dst);
    *dst = NULL;
    return UNKNOWN_ERROR;
  }

  return SUCCESS;
}

int read_zip(FILE* fp, int64_t** dst, size_t* n)
{
  int ret;
  unsigned char in[CHUNK];

  z_stream stream;

  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = 0;
  stream.next_in = Z_NULL;

  size_t avail;
  size_t inflated_n = 0;
  size_t inflated_alloc = CHUNK;
  unsigned char* inflated_contents;

  inflated_contents = malloc(inflated_alloc);
  if (inflated_contents == NULL)
  {
    perror("malloc");
    exit(ENOMEM);
  }

  ret = inflateInit2(&stream, WINDOW_BITS | ENABLE_ZLIB_GZIP);
  if (ret != Z_OK)
  {
    return ret;
  }

  // Decompress the file into memory
  do
  {
    stream.avail_in = fread(in, 1, CHUNK, fp);
    if (ferror(fp))
    {
      free(inflated_contents);
      *dst = NULL;
      inflateEnd(&stream);
      return Z_ERRNO;
    }
    if (stream.avail_in == 0)
    {
      break;
    }

    stream.next_in = in;

    do
    {
      avail = inflated_alloc - inflated_n;
      stream.avail_out = avail;
      stream.next_out = &inflated_contents[inflated_n];
      ret = inflate(&stream, Z_NO_FLUSH);

      switch (ret)
      {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR;
          /* Fallthrough */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          inflateEnd(&stream);
          return ret;
      }

      inflated_n += avail - stream.avail_out;

      if (inflated_n >= inflated_alloc)
      {
        inflated_alloc *= 2;
        inflated_contents = realloc(inflated_contents, inflated_alloc);
        if (inflated_contents == NULL)
        {
          inflateEnd(&stream);
          return Z_ERRNO;
        }
      }
    } while (stream.avail_out == 0);

  } while (ret != Z_STREAM_END);

  inflateEnd(&stream);
  if (ret != Z_STREAM_END)
  {
    printf("EROR\n");
    return Z_DATA_ERROR;
  }

  /* Done reading zip into memory, parse all the ints */

  unsigned char* endptr = NULL;
  unsigned char* start = inflated_contents;
  unsigned char* i = inflated_contents;
  unsigned char const* inflated_end = &inflated_contents[inflated_n];
  int64_t val = 0;

  *n = 0;
  size_t alloc = inflated_alloc / 4;
  *dst = malloc(sizeof(int64_t) * alloc);
  if (dst == NULL)
  {
    free(inflated_contents);
    perror("malloc");
    exit(ENOMEM);
  }

  errno = 0;
  while (i != inflated_end)
  {
    if (*i == '\n')
    {
      *i = '\0';
      val = strtoll((char*)start, NULL, 10);
      if (errno == ERANGE || errno == EINVAL || endptr == start)
      {
        // Some error occurred in parsing
        free(inflated_contents);
        free(*dst);
        *dst = NULL;

        if (errno == 0)
        {
          errno = EINVAL;
        }
        return PARSE_ERROR;
      }
      (*dst)[*n] = val;
      ++(*n);
      start = i + 1;
    }
    if (*n > alloc)
    {
      alloc *= 2;
      *dst = realloc(*dst, alloc);
      if (dst == NULL)
      {
        free(inflated_contents);
        perror("malloc");
        exit(ENOMEM);
      }
    }
    ++i;
  }

  free(inflated_contents);
  return Z_OK;
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
            "method,input,size,threshold,wall_nsecs,user_nsecs,system_nsecs");
    if (args->vals != NULL)
    {
      fprintf(out_file, ",%s", args->cols);
    }
    fputc('\n', out_file);
  }

  for (size_t i = 0; i < num_results; ++i)
  {
    const struct times r = results[i];
    const intmax_t wall = (r.wall_secs * BILLION) + r.wall_nsecs;
    fprintf(out_file,
            "%s,%s,%lu,%lu,%li,%lu,%lu",
            METHODS[args->method],
            args->in_file,
            args->in_file_len,
            args->is_threshold_method ? args->threshold : 0,
            wall,
            r.user,
            r.system);

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

bool is_sorted(int64_t* data, const size_t n)
{
  for (size_t i = 0; i < n - 1; ++i)
  {
    if (data[i + 1] < data[i])
    {
      return false;
    }
  }

  return true;
}
