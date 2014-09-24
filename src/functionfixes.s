# Wrap functions with pascal calling convention
# https://github.com/autc04/Retro68/issues/6

.globl aboutFilter
.type aboutFilter, @function
aboutFilter:
    jsr aboutFilterReal
    move.l (%sp)+, %a0
# 12 is the total size of arguments; one-byte arguments count as 2 bytes.
    add.l #12, %sp
# .b is the size of the return value (1 byte); alternatives are .w for 2 bytes and .l for 4 bytes.
    move.b %d0, (%sp)
    jmp (%a0)
.size   aboutFilter, .-aboutFilter
