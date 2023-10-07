#include "data.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <zlib.h>

// zlib specific
#define CHUNK 32768
#define WINDOW_BITS 15
#define ENABLE_ZLIB_GZIP 32

int read_txt(FILE* fp, sort_t** dst, size_t* n)
{
  char* endptr = NULL;

  // Allocate a buffer for data from text file.
  size_t alloc = CHUNK;
  *n = 0;
  *dst = malloc(sizeof(sort_t) * alloc);
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
#ifdef SORT_LARGE_STRUCTS
    (*dst)[*n].val = val;
#else
    (*dst)[*n] = val;
#endif  // SORT_LARGE_STRUCTS

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

int read_zip(FILE* fp, sort_t** dst, size_t* n)
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
    if (stream.avail_in == 0)
    {
      size_t read = fread(in, 1, CHUNK, fp);
      stream.avail_in = read;
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
      stream.next_in = &in[read - stream.avail_in];
    }

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

      if (inflated_n == inflated_alloc)
      {
        inflated_alloc *= 2;
        unsigned char* const tmp = realloc(inflated_contents, inflated_alloc);
        if (tmp == NULL)
        {
          free(inflated_contents);
          inflateEnd(&stream);
          return Z_ERRNO;
        }
        inflated_contents = tmp;
      }
    } while (stream.avail_out == 0);

    if (ret == Z_STREAM_END)
    {
      if (inflateReset(&stream) != Z_OK)
      {
        return Z_DATA_ERROR;
      }
    }

  } while (stream.avail_in > 0 || ret != Z_STREAM_END);

  inflateEnd(&stream);
  if (ret != Z_STREAM_END && ret != Z_OK)
  {
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
  *dst = malloc(sizeof(sort_t) * alloc);
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

#ifdef SORT_LARGE_STRUCTS
      (*dst)[*n].val = val;
#else
      (*dst)[*n] = val;
#endif  // SORT_LARGE_STRUCTS

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
