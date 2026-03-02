    .section .text
    .globl _start
    .type __start,@function

_start:
    csrr a0,mhartid
    li t0,0x0
    beq a0,t0,_core0
_loop:
    j _loop
_core0:
    li t0,0x100
    slli t0,t0,20
    li t1,'h'
    sb t1,0(t0)
    li t1,'i'
    sb t1,0(t0)
    li t1,' '
    sb t1,0(t0)
    li t1,'g'
    sb t1,0(t0)
    li t1,'u'
    sb t1,0(t0)
    li t1,'o'
    sb t1,0(t0)

    j _loop


.end