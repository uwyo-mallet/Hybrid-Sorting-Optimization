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
#   for (uint64_t* i = start + 1; i < end; i++)
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
    push RBP                            # Required preservations
    mov RBP,RSP                         #
    push RBX                            #

    cmp     RSI,0                       # Check if length is <= 0
    je      done                        # If so, exit

    lea     R9,[RDI + (8 * RSI) - 8]    # Pointer to last element of array
    mov     RAX,RDI                     # i = start

# RDI -> Start
# R9 -> End
# RAX -> i
# RBX -> j
# RCX -> j - 1
# R10 -> Swap 1
# R11 -> Swap 2

outer:
    mov     RCX,RAX                     #
    add     RAX,8

    cmp     RAX,R9
    ja      done

    mov     RBX,RAX

inner:
    # while j > start
    cmp     RBX,RDI
    jle     outer

    # && *j < *(j - 1)
    mov     R11,[RCX]
    cmp     [RBX],R11
    jae     outer

    # swap(*j, *(j - 1))
    mov     R10,[RBX]
    mov     [RBX],R11
    mov     [RCX],R10

    sub     RBX,8
    sub     RCX,8

    jmp     inner

done:
    pop     RBX                      # Cleanup after myself
    pop     RBP

    ret                              # Return value in RAX
