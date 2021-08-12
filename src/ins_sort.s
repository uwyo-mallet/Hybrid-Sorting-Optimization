# ------------------------------------------------------------------------------
# Insertion sort for uint64_t.
# C equivalent:
# void insertion_sort(uint64_t arr[], uint64_t n)
# {
#   if (n == 0)
#   {
#     return;
#   }
#   uint64_t* start = arr;
#   uint64_t* end = arr + n;
#
#   for (uint64_t* i = start; i < end; i++)
#   {
#     uint64_t* j = i;
#     while (j > start && *j < *(j - 1))
#     {
#       swap(j, j - 1);
#       j--;
#     }
#   }
# }
# The function has prototype:
#   void insertion_sort_asm(uint64_t arr[], const uint64_t len);
# -------------------------------------------------------------------------------

        .global insertion_sort_asm
        .intel_syntax noprefix
        .text

insertion_sort_asm:
    # Stack stuff
    push RBP
    mov RBP, RSP

    push RBX                     # Must preserve

init:
    cmp     RSI,0                     # Check if length is <= 0
    jle     done                      # If so, exit
    lea     RCX,[RDI]                 # Pointer to first element of array
    lea     RDX,[RDI + (8 * RSI) - 8] # Pointer to last element of array

    # RCX -> Start
    # RDX -> End
    # RAX -> i
    # RBX -> i - 1
    # R8 -> swap 1
    # R9 -> swap 2

outer:
    add     RCX,8
    cmp     RCX,RDX
    jg      done
    mov     RAX,RCX
    lea     RBX,[RAX - 8]

inner:
    # while j > start
    cmp     RAX,RDI
    jle     outer

    # && *j < *(j - 1)
    mov     R8,[RAX]
    mov     R9,[RBX]
    cmp     R8,R9
    jge     outer

    # swap(*j, *(j - 1))
    mov     R8,[RBX]
    mov     R9,[RAX]
    mov     [RAX],R8
    mov     [RBX],R9

    sub     RAX,8
    sub     RBX,8
    jmp     inner

done:
    pop     RBX                      # Cleanup after myself
    pop     RBP

    ret                              # Return value in RAX
