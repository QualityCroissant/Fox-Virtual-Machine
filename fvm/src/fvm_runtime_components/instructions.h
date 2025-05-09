#ifndef FVMR_INSTRUCTIONS_H

#define FVMR_INSTRUCTIONS_H

// Instruction functions:
// (Each instruction returns 0 if it executes successfully, and 1 if it fails)

#include "global.h"
#include "fvmgl.h"

extern _Bool (*instructions[NO_INSTRUCTIONS])(void); // Array of function-pointers for each instruction

extern _Bool place(void); // pl <value> <register>
extern _Bool move(void); // mv <register> <register>
extern _Bool store(void); // st <mdr> at <mar> in <mch>
extern _Bool load(void); // ld to <mdr> from <mar> in <mch>
extern _Bool jump(void); // jm <address>
extern _Bool jump_if_set(void); // js <address>
extern _Bool jump_if_clear(void); // jc <address>
extern _Bool accumulator_add(void); // a+
extern _Bool accumulator_sub(void); // a-
extern _Bool accumulator_not(void); // a!
extern _Bool accumulator_increment(void); // ai
extern _Bool accumulator_decrement(void); // ad
extern _Bool accumulator_mul(void); // a*
extern _Bool accumulator_div(void); // a/
extern _Bool accumulator_and(void); // a&
extern _Bool accumulator_or(void); // a|
extern _Bool accumulator_xor(void); // a^
extern _Bool accumulator_lsh(void); // al
extern _Bool accumulator_rsh(void); // ar
extern _Bool accumulator_gt(void); // gt
extern _Bool accumulator_lt(void); // lt
extern _Bool accumulator_ge(void); // ge
extern _Bool accumulator_le(void); // le
extern _Bool accumulator_eq(void); // eq
extern _Bool accumulator_ne(void); // ne
extern _Bool call_address(void); // cl
extern _Bool return_address(void); // rt

#endif
