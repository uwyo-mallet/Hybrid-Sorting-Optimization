# ------------------------------------------------------------------------------
# Optimized hybrid quicksort (with insertion sort for subarrays) for uin64_t.
# C Equivalent:
# TODO
#
# Function prototype
# void qsort_asm(uint64_t* arr, const size_t n, const size_t thresh)
# ------------------------------------------------------------------------------


    .global qsort_asm
    .intel_syntax noprefix
    .text

# RAX -> current lo
# RBX -> current mid
# RCX -> current hi

# R8 -> scratch
# R9 -> scratch
# R10 -> scratch
# R11 -> scratch
# R12 -> scratch
# R13 -> scratch
# R14 -> scratch

# RDI -> arr base ptr
# RSI -> n in # elements
# RDX -> thresh in # elements


qsort_asm:
    push    RBP                             # Stack setup
    mov     RBP,RSP                         #
    push    RBX                             #
    push    R12                             #
    push    R13                             #
    push    R14                             #

    push    -1                              # Push a constant so we know where the bottom of stack is.

    cmp     RSI,1                           # If len of array <= 1, we're done.
    jbe     .done                           #

    cmp     RSI,RDX                         # If len of array is less than threshold, just use ins sort.
    jb      .ins_sort                       #

    shl     RDX,3                           # Convert threshold from num elements to num bits.

    lea     RCX,[RDI + (RSI * 8) - 8]       # Set RCX to last element of array.

    push    RDI                             # lo
    push    RCX                             # hi


.stack_not_empty:

    # Pop hi then lo, reverse of push order.
    pop     RCX
    pop     RAX

    # Calculate midpoint of lo and hi
    # Extra shifts to account for even sizes.
    mov     RBX,RCX
    sub     RBX,RAX
    shr     RBX,4
    shl     RBX,3
    add     RBX,RAX

# Median of three --------------------------------------------------------------
    # (*mid < *lo)
    mov     R10,[RBX]
    cmp     R10,[RAX]
    jge     .swap_mid_hi

    # swap(mid, lo)
    mov     R11,[RAX]
    mov     [RBX],R11
    mov     [RAX],R10

.swap_mid_hi:
    # (*hi < *mid)
    mov       R10,[RBX]
    cmp       [RCX],R10
    jge       .jump_over

    # swap(mid, hi)
    mov     R11,[RCX]
    mov     [RBX],R11
    mov     [RCX],R10

    # check mid lo again, last check so just go to jump over when done.
    mov     R10,[RBX]
    cmp     R10,[RAX]
    jge     .jump_over

    # swap(mid, lo)
    mov     R11,[RAX]
    mov     [RBX],R11
    mov     [RAX],R10

# At this point, the values of lo <= mid <= hi, left = lo + 1, right = hi - 1
.jump_over:
    lea     R8,[RAX + 8]
    lea     R9,[RCX - 8]

# Main partition loop ----------------------------------------------------------
.partition:
    mov     R12,[RBX]

.mov_left:
    cmp     [R8],R12
    jge     .mov_right
    add     R8,8
    jmp     .mov_left

.mov_right:
    cmp     R12,[R9]
    jge     .left_right_check
    sub     R9,8
    jmp     .mov_right

.left_right_check:
    cmp     R8,R9
    jg      .partition_check
    je      .left_equal_right

.left_less_than_right:
    # swap(left, right)
    mov     R12,[R8]
    mov     R13,[R9]
    mov     [R8],R13
    mov     [R9],R12

    cmp     RBX,R8
    cmove   RBX,R9

    # cmp     RBX,R9
    # je      .mid_equal_right

.mid_equal_left:
    mov     RBX,R9
    jmp     .inc_left_right

.mid_equal_right:
    mov     RBX,R8
    jmp     .inc_left_right

.inc_left_right:
    add     R8,8
    sub     R9,8

.partition_check:
    cmp     R8,R9
    jbe     .partition
    jmp     .stack_ops

.left_equal_right:
    add     R8,8
    sub     R9,8

# Stack additions --------------------------------------------------------------
.stack_ops:
    mov     R10,R9
    sub     R10,RAX
    cmp     R10,RDX
    jbe     .push_hi

    cmp     R9,RAX
    jbe     .push_hi

    push    RAX
    push    R9

.push_hi:
    mov     R10,RCX
    sub     R10,R8
    cmp     R10,RDX
    jbe     .stack_check

    cmp     R8,RCX
    jge     .stack_check

    push    R8
    push    RCX

.stack_check:
    mov     R12,[RSP]
    cmp     R12,-1
    jne     .stack_not_empty

.ins_sort:
    lea     RCX,[RDI]                 # Pointer to first element of array
    lea     RDX,[RDI + (8 * RSI) - 8] # Pointer to last element of array

    # RCX -> Start
    # RDX -> End
    # RAX -> i
    # RBX -> i - 1
    # R8 -> swap 1
    # R9 -> swap 2

.outer:
    add     RCX,8
    cmp     RCX,RDX
    jg      .done
    mov     RAX,RCX
    lea     RBX,[RAX - 8]

.inner:
    # while j > start
    cmp     RAX,RDI
    jbe     .outer

    # && *j < *(j - 1)
    mov     R8,[RAX]
    mov     R9,[RBX]
    cmp     R8,R9
    jge     .outer

    # swap(*j, *(j - 1))
    mov     R8,[RBX]
    mov     R9,[RAX]
    mov     [RAX],R8
    mov     [RBX],R9

    sub     RAX,8
    sub     RBX,8
    jmp     .inner


.done:
    pop     R11

    pop     R14
    pop     R13
    pop     R12

    pop     RBX
    pop     RBP


    ret
