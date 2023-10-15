#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include "alphadev.h"


bool isSorted(int* buffer, size_t n)
{
  int* a = buffer;
  int* b = &buffer[1];
  for (size_t i = 0; i < n-1; ++i) {

    if (*a > *b) {
      return false;
    }
    a++;
    b++;
  }

  return true;
}

int main(int argc, char** argv) {

  int toSort3[] = {3, 2, 1};
  int toSort4[] = {2, 3, 1, 4};
  int toSort5[] = {2, 3, 1, 5, 4};
  int toSort6[] = {2, 3, 1, 5, 4, 6};
  int toSort7[] = {7, 2, 3, 1, 5, 4, 6};
  int toSort8[] = {7, 8, 2, 3, 1, 5, 4, 6};

  VarSort4AlphaDev(toSort4);
  if (!isSorted(toSort4, 4)) {
    printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  }

  for (int i = 0; i < 4; ++i) {
    printf("%d\n", toSort4[i]);
  }

  // Sort3AlphaDev(toSort3);
  // if (!isSorted(toSort3, 3)) {
  //   printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  // }
  //
  // Sort4AlphaDev(toSort4);
  // if (!isSorted(toSort4, 4)) {
  //   printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  // }
  //
  // Sort5AlphaDev(toSort5);
  // if (!isSorted(toSort5, 5)) {
  //   printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  // }
  //
  // Sort6AlphaDev(toSort6);
  // if (!isSorted(toSort6, 6)) {
  //   printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  // }
  //
  // Sort7AlphaDev(toSort7);
  // if (!isSorted(toSort7, 7)) {
  //   printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  // }
  //
  // Sort8AlphaDev(toSort8);
  // if (!isSorted(toSort8, 8)) {
  //   printf("%s:%d: Not sorted!\n", __FILE__, __LINE__);
  // }
  //

  return 0;
}
