#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <iostream>
#include "motor_driver.h"
#include "encoder_driver.h"
#include "variable_loader.h"

#ifndef TEST_JIZDA
#include "check.h"
#endif

#define F_MOTOR          "/dev/ttyAMA0"
#define MOTOR_LEFT       0x80
#define MOTOR_RIGHT      0x00
#define MOTOR_ZERO       0x40

FILE *motor_h;

bool motor_running;
pthread_t motor_loop_tid;
pthread_mutex_t motor_lock;

bool command_reverse;
bool command_stop;
POS_TYPE command_want_x;
POS_TYPE command_want_y;
int command_distance_square = 0;
int command_distance_last_square = 0;
int command_distance_no_angle;
POS_TYPE isum_l;
POS_TYPE isum_r;
enc_type pos_last;
enc_type pos_now;


inline int limit_speed(int s) {
  if (s > speedmax)
    return speedmax;
  if (s < -speedmax)
    return -speedmax;
  return s;
}

int norm_angle(int angle) {
  if (angle < -180)
    return norm_angle(angle + 360);
  if (angle > 180)
    return norm_angle(angle - 360);
  return angle;
}

void motor_go(int speed, int ang_speed) {
  int dt = pos_now.time - pos_last.time;
  if (dt <= 0)
    return;

  int wantspeed_l = speed - ang_speed;
  int wantspeed_r = speed + ang_speed;
  int dl = (pos_now.enc_l - pos_last.enc_l)/wheelparam2;
  int dr = (pos_now.enc_r - pos_last.enc_r)/wheelparam2;
  int v_l = limit_speed(dl/dt);
  int v_r = limit_speed(dr/dt);
  int a_l = v_l/dt;
  int a_r = v_r/dt;
  isum_l += (wantspeed_l-v_l)*dt;
  isum_r += (wantspeed_r-v_r)*dt;
  int v_new_l = limit_speed(wantspeed_l + pid_prop_const*(wantspeed_l-v_l)/1000 +
                            pid_integ_const*isum_l/1000 +
                            pid_deriv_const*a_l/1000);
  int v_new_r = limit_speed(wantspeed_r + pid_prop_const*(wantspeed_r-v_r)/1000 +
                            pid_integ_const*isum_r/1000 +
                            pid_deriv_const*a_r/1000);
  printf("motor_go: dt = %d\n", dt);
  printf("motor_go: dl=%d v_l=%d wantspeed_l=%d v_new_l%d\n", dl, v_l, wantspeed_l, v_new_l);
  printf("motor_go: dr=%d v_r=%d wantspeed_r=%d v_new_r%d\n", dr, v_r, wantspeed_r, v_new_r);
  fputc(MOTOR_LEFT  + MOTOR_ZERO + v_new_l, motor_h);
  fputc(MOTOR_RIGHT + MOTOR_ZERO + v_new_r, motor_h);
  fflush(motor_h);
}

void *motor_loop(void *arg) {
  while (motor_running) {
    pthread_mutex_lock(&motor_lock);
     if (command_stop)
       motor_stop();
     else {
       printf("motor_loop: not stopped\n");
       fflush(stdout);
       if (command_reverse)
         go_to_pos_rev();
       else
         go_to_pos();
       if ((command_distance_square > command_distance_last_square) && (command_distance_square < (command_distance_no_angle*command_distance_no_angle))) {
         command_stop = 1;
       }
     }
    pthread_mutex_unlock(&motor_lock);
    usleep(pid_loop_usleep);
  }
  pthread_exit(NULL);
}

int go_to_pos() {//go to command_want_x, command_want_y
  pos_last = pos_now;
  enc_get(&pos_now);
  POS_TYPE diff_x = command_want_x - pos_now.pos_x;
  POS_TYPE diff_y = command_want_y - pos_now.pos_y;
  POS_TYPE ang_to_pos = atan(diff_y / diff_x) * 180.0 / PI;
  if (diff_x < 0)
    ang_to_pos += 180;
  POS_TYPE diff_a = ang_to_pos - pos_now.pos_a;
  command_distance_last_square = command_distance_square;
  command_distance_square = diff_x * diff_x + diff_y * diff_y;
  if (command_distance_square < command_distance_no_angle*command_distance_no_angle)//do not correct angle, if we are close to final point
    diff_a = 0;
  diff_a = norm_angle(diff_a);
  POS_TYPE speed = limit_speed((diff_x * diff_x + diff_y * diff_y) / 40000 + 20);
  printf("Wheels: %i, %i I'am at %f, %f, %f ", pos_now.enc_l, pos_now.enc_r, pos_now.pos_x, pos_now.pos_y, pos_now.pos_a);
  printf("I want move with speed %f = %i ", speed, (int) speed);
  printf("I want to rotate %f = %i degrees to left\n", diff_a, (int) diff_a);
  motor_go(speed, diff_a);
  return speed;
}

int go_to_pos_rev() {//go to command_want_x, command_want_y
  pos_last = pos_now;
  enc_get(&pos_now);
  POS_TYPE diff_x = command_want_x - pos_now.pos_x;
  POS_TYPE diff_y = command_want_y - pos_now.pos_y;
  POS_TYPE ang_to_pos = atan(diff_y / diff_x) * 180.0 / PI;
  if (diff_x >= 0)
    ang_to_pos -= 180;
  POS_TYPE diff_a = ang_to_pos - pos_now.pos_a;
  command_distance_last_square = command_distance_square;
  command_distance_square = diff_x * diff_x + diff_y * diff_y;
  if (command_distance_square < command_distance_no_angle*command_distance_no_angle)//do not correct angle, if we are close to final point
    diff_a = 0;
  diff_a = norm_angle(diff_a);
  POS_TYPE speed = - limit_speed((diff_x * diff_x + diff_y * diff_y) / 40000 + 20);
  printf("Wheels: %i, %i I'am at %f, %f, %f ", pos_now.enc_l, pos_now.enc_r, pos_now.pos_x, pos_now.pos_y, pos_now.pos_a);
  printf("I want move with speed %f = %i ", speed, (int) speed);
  printf("I want to rotate %f = %i degrees to left\n", diff_a, (int) diff_a);
  motor_go(speed, diff_a);
  return speed;
}

void motor_init() { // pozor, doporučuje se vypnout echo (stty -F /dev/ttyAMA0 -echo) soubor /etc/rc.local. Jinak motory mohou jezdit nahodne.
  motor_h = fopen (F_MOTOR, "w");
  fputc(0x00, motor_h);

  printf("motor_init: starting thread... \n");
  motor_running = 1;
  command_want_x = 0;
  command_want_y = 0;
  command_reverse = 0;
  command_stop = 1;
  command_distance_square = 0;
  command_distance_last_square = 0;
  command_distance_no_angle = 0;

  isum_l = 0;
  isum_r = 0;
  pthread_mutex_init(&motor_lock, NULL);
  pthread_attr_t *thAttr = NULL;
  pthread_create(&motor_loop_tid, thAttr, motor_loop, NULL);
  printf("motor_init \n");
}

void motor_command(POS_TYPE want_x, POS_TYPE want_y, bool reverse, bool stop, int distance_no_angle) {
  pthread_mutex_lock(&motor_lock);
   command_want_x  = want_x;
   command_want_y  = want_y;
   command_reverse = reverse;
   command_stop    = stop;
   POS_TYPE diff_x = command_want_x - pos_now.pos_x;
   POS_TYPE diff_y = command_want_y - pos_now.pos_y;
   command_distance_square      = diff_x * diff_x + diff_y * diff_y;
   command_distance_last_square = command_distance_square;
   command_distance_no_angle    = distance_no_angle;
   if (stop)
     motor_stop();
  pthread_mutex_unlock(&motor_lock);
}

void motor_get_command(POS_TYPE *want_x, POS_TYPE *want_y, bool *reverse, bool *stop, int *distance_no_angle) {
  pthread_mutex_lock(&motor_lock);
  if (want_x != NULL)
    *want_x  = command_want_x;
  if (want_y != NULL)
    *want_y  = command_want_y;
  if (reverse != NULL)
    *reverse = command_reverse;
  if (stop != NULL)
    *stop    = command_stop;
  if (distance_no_angle != NULL)
    *distance_no_angle = command_distance_no_angle;
  pthread_mutex_unlock(&motor_lock);
}

void motor_close() {
  motor_running = 0;
  void *motor_loop_return;
  pthread_join(motor_loop_tid, &motor_loop_return);

  fputc(0x00, motor_h);
  fflush(motor_h);
  fputc(0x00, motor_h);
  fflush(motor_h);
  fclose(motor_h);
}

void motor_stop() {
  fputc(0x00, motor_h);
  fflush(motor_h);
  fputc(0x00, motor_h);
  fflush(motor_h);
}
