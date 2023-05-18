int nums[] = {4, 3, 2, 1};
void* genericNums = &nums;

// We can access nums knowing the size of each individual item
char* tmp = (char*)genericNums;
void* thirdElementOfNums = (void*)(tmp + (sizeof(int) * 2));

// The conventional way of assigning to a generic ptr.
int fourtyTwo = 42;
memcpy(thirdElementOfNums, &fourtyTwo, sizeof(int));

// Given the alignment, we can 'guess' a type.
// Meaning we can assign to it just like any other other ptr
if (__alignof__(thirdElementOfNums) == __alignof__(int) && ...)
{
  *(int*)thirdElementOfNums = 42;
  // nums => {4, 3, 42, 1};
}
