#ifndef MATRIX_DRIVER_H_
#define MATRIX_DRIVER_H_

#include "types.h"

int matrix_init();
void matrix_close();
u08 matrix_wait_for_raw();   // if you want to repeat it immediately, call sched_yield() / pthread_yield() on some systems
char matrix_wait_for_char(); // if you want to repeat it immediately, call sched_yield() / pthread_yield() on some systems
u08 matrix_get_last_raw();
char matrix_get_last_char();
char matrix_wait_for_char_press();
char matrix_decode(u08 i);

#endif
