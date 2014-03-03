#ifndef CHECK_H_
#define CHECK_H_

#include <sys/time.h>
#include <time.h>

#include "i2c_communication.h"

#define BUMP_NONE        (u08)0
#define BUMP_FRONT_LEFT  (u08)1
#define BUMP_FRONT_RIGHT (u08)2
#define BUMP_REAR_LEFT   (u08)4
#define BUMP_REAR_RIGHT  (u08)8

#define USE_REVERSE      (u08)1
#define ROTATE           (u08)2
#define SKIP_TIMER_CHECK (u08)8
#define SKIP_FBUMP_CHECK (u08)16
#define SKIP_RBUMP_CHECK (u08)32

int check_timer();
void timer_reset();
int check_travel(u08* bumpers, u16* sharpa, u16* sharpb, u08* flags);

#endif
