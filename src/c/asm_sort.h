#ifndef ASM_SORT_H_
#define ASM_SORT_H_

#ifndef __clang__
#error("Assembly methods required clang!")
#endif  // __clang__

void Sort3AlphaDev(int* buffer);
void Sort4AlphaDev(int* buffer);
void Sort5AlphaDev(int* buffer);
void Sort6AlphaDev(int* buffer);
void Sort7AlphaDev(int* buffer);
void Sort8AlphaDev(int* buffer);

void VarSort3AlphaDev(int* buffer);
void VarSort4AlphaDev(int* buffer);
void VarSort5AlphaDev(int* buffer);

#endif  // ASM_SORT_H_
