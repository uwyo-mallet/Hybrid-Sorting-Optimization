#ifndef DATA_H_
#define DATA_H_

#include <stdlib.h>

#include "sort.h"

typedef enum
{
  SUCCESS,       /* OK */
  FAIL,          /* An error occured, and was already reported with perror(). */
  PARSE_ERROR,   /* strtoll() parse error. */
  UNKNOWN_ERROR, /* Take a guess... */
} STATUS;

int read_txt(FILE* fp, sort_t** dst, size_t* n);
int read_zip(FILE* fp, sort_t** dst, size_t* n);

#endif  // DATA_H_
