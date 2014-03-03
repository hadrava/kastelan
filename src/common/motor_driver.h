#ifndef MOTOR_DRIVER_H_
#define MOTOR_DRIVER_H_

#include <stdio.h>
#include "variable_loader.h"
#include "i2c_communication.h"

#define VERBOSE

extern FILE *motor_h;
void motor_init();
void motor_close();

void motor_stop();
int rotate_to_pos(POS_TYPE want_x, POS_TYPE want_y, u08* bumpers, u08* flags);
int rotate_to_pos_rev(POS_TYPE want_x, POS_TYPE want_y, u08* bumpers, u08* flags);
int rotate_to(POS_TYPE want_a, u08* bumpers, u08* flags);
int go_to_pos();
int go_to_pos_rev();
void motor_command(POS_TYPE want_x, POS_TYPE want_y, bool reverse, bool stop, int distance_no_angle);
void motor_get_command(POS_TYPE *want_x, POS_TYPE *want_y, bool *reverse, bool *stop, int *distance_no_angle);

#endif
