#include <stdio.h>
#include <math.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#include "types.h"
#include "variable_loader.h"
#include "i2c_communication.h"
#include "led_driver.h"
#include "matrix_driver.h"
#include "encoder_driver.h"
#include "motor_driver.h"
#include "our_time.h"

pthread_t bumpers_loop_tid;

void *bumpers_loop(void *arg) {
  while (1) {
    u08 bumpers;
    i2c_get_bumpers(&bumpers);
    printf("bumpers: %X\n", bumpers);
    if (i2c_start_cable_present())
      printf("cable: present\n");
    else
      printf("cable: not found\n");
    usleep(main_loop_usleep);
  }
}

int main_poll() {
  for (;;) {
    char k = matrix_wait_for_char();
    if (k=='1')
      i2c_taos_set_servo(TAOS_FLIPPER, taos_flipper_left);
    if (k=='2')
      i2c_taos_set_servo(TAOS_FLIPPER, taos_flipper_center);
    if (k=='3')
      i2c_taos_set_servo(TAOS_FLIPPER, taos_flipper_right);
    if (k=='A')
      i2c_servo_set_servo(SERVO_LEFT, servo_left_open);
    if (k=='B')
      i2c_servo_set_servo(SERVO_LEFT, servo_left_close);
    if (k=='C')
      i2c_servo_set_servo(SERVO_RIGHT, servo_right_open);
    if (k=='D')
      i2c_servo_set_servo(SERVO_RIGHT, servo_right_close);

    if (k=='4') {
      while (k=='4') {
        led_swap(0);
        usleep(1000*200);
        k = matrix_get_last_char();
      }
    }
    if (k=='6') {
    }

    if (k=='7') {
      i2c_taos_sort_enable();
    }
    if (k=='9') {
      i2c_taos_sort_disable();
    }

    if (k=='5') {
      start_game_watchdog();
    }
    if (k=='8') {
      printf("our_time: zbyva %i sec, %i milisec\n", time_remaining_s(), time_remaining_ms());
    }

    if (k=='0') {
      u16 r,g,b,w;
      i2c_taos_get_color();
      usleep(1000*130);
      i2c_taos_fetch_color(&r, &g, &b, &w);
    }
    if (k=='*')
      enc_reset();// reset pozice, ne uz encoderu v jadre
    if (k=='#')
      break;
    sched_yield();
  }
  return 0;
}

int init_systems() {
  var_load(CONFIG_FILE); // Nacti vsechny promenne
  led_init();
  matrix_init(); // Klavesnice
  enc_init(); // Nastav encodery
//  motor_init(); // PID -> zapinat az po encoderech
  i2c_init(); // Priprav i2c komunikaci

  pthread_attr_t *thAttr = NULL;
  pthread_create(&bumpers_loop_tid, thAttr, bumpers_loop, NULL);
  return 0;
}

int shut_systems() {
  i2c_close(); // Zavri i2c soubory
//  motor_close(); // Zavri motory (pid, zavirat pred enc)
  enc_close(); // Zavri encodery
  matrix_close(); // Klavesnice
  led_close();
  var_save(CONFIG_FILE); //Uloz vsechny promenne
  return 0;
}

int main(int argc, char** argv) {
  if (init_systems())
    return -1;

  main_poll();

  return shut_systems();
}
