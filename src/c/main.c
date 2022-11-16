#include <assert.h>
#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>

#define SET_BINARY_MODE(file)

#define CHUNK 16384

int read_txt(FILE* fp, int64_t** dst, size_t* n);
int read_zip(FILE* fp, int64_t** dst, size_t* n);

/* #define SUCCESS 1 */
/* #define FAILURE 0 */

typedef enum
{
  SUCCESS,
  PARSE_ERROR,
  UNKNOWN_ERROR,
} QST_RET;

int main(int argc, char** argv)
{
  (void)argc;
  (void)argv;

  /* const char* fname = "../data_test/ascending/0.gz"; */
  const char* fname = "foo.txt.gz";
  FILE* fp = fopen(fname, "rb");
  if (fp == NULL)
  {
    perror(fname);
    return EXIT_FAILURE;
  }

  size_t n = 0;
  int64_t* data = NULL;

  if (read_zip(fp, &data, &n) != Z_OK)
  {
    fclose(fp);
    return EXIT_FAILURE;
  }

  for (size_t i = 0; i < n; ++i)
  {
    printf("%li\n", data[i]);
  }

  free(data);
  fclose(fp);
  return EXIT_SUCCESS;
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
  size_t alloc = CHUNK;
  *n = 0;
  *dst = malloc(sizeof(int64_t) * alloc);
  if (dst == NULL)
  {
    perror("memory");
    exit(ENOMEM);
  }

  size_t to_copy = 0;
  int ret;

  char* endptr = NULL;
  unsigned char in[CHUNK];
  unsigned char out[CHUNK];

  z_stream stream;

  stream.zalloc = Z_NULL;
  stream.zfree = Z_NULL;
  stream.opaque = Z_NULL;
  stream.avail_in = 0;
  stream.next_in = Z_NULL;

#define WINDOW_BITS 15
#define ENABLE_ZLIB_GZIP 32
  ret = inflateInit2(&stream, WINDOW_BITS | ENABLE_ZLIB_GZIP);
  if (ret != Z_OK)
  {
    free(*dst);
    return ret;
  }

  do
  {
    stream.avail_in = fread(in, 1, CHUNK, fp);
    if (ferror(fp))
    {
      free(*dst);
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
      stream.avail_out = CHUNK;
      stream.next_out = out;
      ret = inflate(&stream, Z_NO_FLUSH);

      /* Handle Errors */
      switch (ret)
      {
        case Z_NEED_DICT:
          ret = Z_DATA_ERROR;
          /* fallthrough */
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          free(*dst);
          fprintf(stderr, "[ERROR]: %d: %s\n", ret, stream.msg);
          inflateEnd(&stream);
          return ret;
      }
      to_copy = CHUNK - stream.avail_out;

      // Expand the size of the output buffer if need be.
      if (alloc < *n)
      {
        // Double space allocated on each realloc.
        alloc *= 2;
        *dst = realloc(*dst, alloc);
        if (*dst == NULL)
        {
          perror("realloc");
          exit(ENOMEM);
        }
      }

      /* memcpy(&*dst[*n], in, CHUNK); */
      fwrite(out, 1, to_copy, stdout);
      int64_t val = strtoll((char*)out, &endptr, 10);
      printf("%li\n", val);

    } while (stream.avail_out == 0);

  } while (ret != Z_STREAM_END);

  inflateEnd(&stream);

  return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}
