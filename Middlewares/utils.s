.syntax unified
.thumb
.thumb_func
.global ror16

@uint16_t ror16(uint16_t value, uint16_t shift)
ror16:
    and r1, r1, #15
    uxth r0, r0
    ror r0, r0, r1
    uxth r0, r0
    bx lr