#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "types.h"
#include "variable_loader.h"
#include "i2c_communication.h"
#include "led_driver.h"
#include "input_event_driver.h"
#include "encoder_driver.h"
#include "motor_driver.h"
#include "our_time.h"

#define BUMPER_RIGHT_FRONT  1
#define BUMPER_RIGHT_SIDE   2
#define BUMPER_RIGHT_REAR   4
#define BUMPER_LEFT_FRONT  32
#define BUMPER_LEFT_SIDE   16
#define BUMPER_LEFT_REAR    8

#define time_remain_unload 30

enum STATE {
  EMERGENCY = 0,
  FIXED_ROUTE,
  STANDBY,
  TRAVELING,
  UNLOADING
};

int state;
int last_state;

int init_systems();
int shut_systems();
int main_poll();

int main(int argc, char** argv) {
  return -(init_systems() || main_poll() || shut_systems());
}

int main_poll() {
  state = STANDBY; // Vychozi stav automatu
  last_state = STANDBY;
  int fix_step = 0;
  int unl_step = 0;
  u08 last_bumpers = 0;
  int route_variant = 1;

  bool main_running = 1;
  while (main_running) {

    u08 bumpers;
    get_bumpers(&bumpers);
    printf("bumpers: %X\n", bumpers);

    // Skoncila hra, zastavim
    if (time_remaining_s() <= 0) {
       motor_command(0,0,0,1, pos_error_allow);
       main_running = false;
    }

    // Narazim, resim problem
//    else if (bumpers & (~last_bumpers)) {
    else if (bumpers && (bumpers != last_bumpers)) {
      printf("bump_detection: stop\n");
      motor_command(0, 0, 0, 1, pos_error_allow);
      last_state = state == EMERGENCY ? last_state : state;
      state = EMERGENCY;
    }

    last_bumpers = bumpers;

    switch (state) {
      case STANDBY : {
        led_swap(0);
        if (!i2c_start_cable_present()) {
          state = FIXED_ROUTE;
          start_game_watchdog();
          i2c_taos_sort_enable();
        }
        break;
      }
      case EMERGENCY : {
        enc_type pos;
        enc_get(&pos);
        POS_TYPE dest_x;
        POS_TYPE dest_y;

        bool stop;
        motor_get_command(NULL, NULL, NULL, &stop, NULL);
        if (stop) {
          // Narazim zepredu, popojedu pozpatku dozadu
          if ((bumpers & BUMPER_LEFT_FRONT) || (bumpers & BUMPER_RIGHT_FRONT)) {
            dest_x = pos.pos_x - cos(pos.pos_a * PI / 180.0) * emergency_run_dist;
            dest_y = pos.pos_y - sin(pos.pos_a * PI / 180.0) * emergency_run_dist;
            i2c_taos_sort_disable();
            printf("bump_action: FRONT: go back\n");
            motor_command(dest_x, dest_y, 1, 0, pos_error_allow);
          }

          // Narazim zezadu, popojedu popredu dopredu
          else if ((bumpers & BUMPER_LEFT_REAR) || (bumpers & BUMPER_RIGHT_REAR)) {
            if ((last_state == UNLOADING) && (unl_step == 4)) { // reverse
              motor_command(0,0,0,1, pos_error_allow);
              printf("bump_action: UNLOAD REAR: stop\n");
              state = last_state;
            } else {
              dest_x = pos.pos_x + cos(pos.pos_a * PI / 180.0) * emergency_run_dist;
              dest_y = pos.pos_y + sin(pos.pos_a * PI / 180.0) * emergency_run_dist;
              printf("bump_action: REAR: go forward\n");
              motor_command(dest_x, dest_y, 0, 0, pos_error_allow);
            }
          }

          // Narazim zleva, popojedu dopredu a natocim se doleva
          else if (bumpers & BUMPER_LEFT_SIDE) {
            dest_x = pos.pos_x - cos((pos.pos_a - emergency_run_turn) * PI / 180.0) * emergency_run_dist;
            dest_y = pos.pos_y - sin((pos.pos_a - emergency_run_turn) * PI / 180.0) * emergency_run_dist;
            printf("bump_action: LEFT: go left forward\n");
            motor_command(dest_x, dest_y, 0, 0, pos_error_allow);
          }

          // Narazim zprava, popojedu dopredu a natocim se doprava
          else if (bumpers & BUMPER_RIGHT_SIDE) {
            dest_x = pos.pos_x - cos((pos.pos_a + emergency_run_turn) * PI / 180.0) * emergency_run_dist;
            dest_y = pos.pos_y - sin((pos.pos_a + emergency_run_turn) * PI / 180.0) * emergency_run_dist;
            printf("bump_action: RIGHT: go right forward\n");
            motor_command(dest_x, dest_y, 0, 0, pos_error_allow);
          }
          else if (!bumpers) {
            i2c_taos_sort_enable();
            state = last_state;
          }
        }

        break;
      }
      case FIXED_ROUTE : {
        bool stop;
        motor_get_command(NULL, NULL, NULL, &stop, NULL);
        if (!stop)
          break;
        if (route_variant == 1) {
          switch(fix_step++) {
            case 0 :
              motor_command(1440, 0, 0, 0, pos_error_allow);
              break;
            case 1 :
              motor_command(915, 1500, 0, 0, pos_error_allow);
              break;
            case 2 :
              motor_command(-100, 1500, 0, 0, pos_error_allow);
              break;
            default :
              motor_command(0,0,0,1, pos_error_allow);
              fix_step = 0;
              route_variant = 2;
              last_state = state;
              state = UNLOADING;
          }
        } else if (route_variant == 2) {
          switch(fix_step++) {
            case 0 :
              motor_command(610, 800, 0, 0, pos_error_allow);
              break;
            case 1 :
              motor_command(1220, 800, 0, 0, pos_error_allow);
              break;
            case 2 :
              motor_command(610, 150, 0, 0, pos_error_allow);
              break;
            case 3 :
              motor_command(0, 1500, 0, 0, pos_error_allow);
              break;
            case 4 :
              motor_command(-100, 1500, 0, 0, pos_error_allow);
              break;
            default :
              motor_command(0,0,0,1, pos_error_allow);
              fix_step = 0;
              route_variant = 1;
              last_state = state;
              state = UNLOADING;
          }
        }
        break;
      }
      case TRAVELING : {
        // Cestuj
        break;
      }
      case UNLOADING : {
        i2c_taos_sort_disable();
        bool stop;
        motor_get_command(NULL, NULL, NULL, &stop, NULL);

        if (!stop)
          break;
        switch(unl_step++) {
          case 0 :
            motor_command(-50,800,0,0,pos_error_allow);
            break;
          case 1 :
            motor_command(-50,-50,0,0,pos_error_allow);
            break;
          case 2 :
            motor_command(200,-50,0,0,pos_error_allow);
            break;
          case 3 :
            i2c_taos_sort_disable();
            motor_command(-200,-50,1,0,pos_error_allow);
            break;
          case 4 :
            enc_type pos;
            enc_get(&pos);
            if ((pos.pos_x > 0) || (pos.pos_y > 50)) {
              motor_command(610, 800, 0, 0, pos_error_allow);
              unl_step = 0;
              break;
            }
            if (ourside == 'L') {
              i2c_servo_set_servo(SERVO_LEFT, servo_left_open);
            } else {
              i2c_servo_set_servo(SERVO_RIGHT, servo_right_open);
            }
            usleep(750000);
            motor_command(800,-50,0,0,pos_error_allow);
            break;
          case 5 :
            if (ourside == 'L') {
              i2c_servo_set_servo(SERVO_LEFT, servo_left_close);
            } else {
              i2c_servo_set_servo(SERVO_RIGHT, servo_right_close);
            }
            usleep(750000);
            unl_step = 0;
            last_state = state;
            state = FIXED_ROUTE;
            i2c_taos_sort_enable();
            break;
        }
        break;
      }
      default : {
        return -1;
      }
    }
    usleep(main_loop_usleep);
  }

  return 0;
}

// Inicializace hardware, planovani, zpracovani obrazu
int init_systems() {

  // Nacti vsechny promenne
  var_load(CONFIG_FILE);

  led_init();
  printf("init_systems: 1\n");
  input_event_init();
  printf("init_systems: 2\n");
  enc_init();
  printf("init_systems: 3\n");
  motor_init();
  printf("init_systems: 4\n");
  i2c_init();
  printf("init_systems: done\n");
  return 0;
}

// Zavreni vsech souboru, hardware, ukonceni ostatnich procesu
int shut_systems() {

  // Uloz vsechny promenne
  var_save(CONFIG_FILE);

  i2c_close();
  printf("shut_systems: 1\n");
  motor_close();
  printf("shut_systems: 2\n");
  enc_close();
  printf("shut_systems: 3\n");
  input_event_close();
  printf("shut_systems: 4\n");
  led_close();
  printf("shut_systems: done\n");
  return 0;
}
