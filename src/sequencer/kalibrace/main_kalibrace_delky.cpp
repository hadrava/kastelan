#include <stdio.h>
#include <unistd.h>
#include <math.h>

#include "types.h"
#include "variable_loader.h"
#include "i2c_communication.h"
#include "led_driver.h"
#include "matrix_driver.h"
#include "encoder_driver.h"
#include "motor_driver.h"
#include "our_time.h"

enum STATE {
  EMERGENCY = 0,
  FIXED_ROUTE,
  STANDBY,
  TRAVELING,
  UNLOADING
};

int state;

int init_systems();
int shut_systems();
int main_poll();

int main(int argc, char** argv) {
  return -(init_systems() || main_poll() || shut_systems());
}

int main_poll() {
  state = STANDBY; // Vychozi stav automatu
  int fix_step = 0;

  bool main_running = 1;
  while (main_running) {

    u08 bumpers;
    i2c_get_bumpers(&bumpers);
    printf("bumpers: %X\n", bumpers);
    if (bumpers)
      state = EMERGENCY;

    switch (state) {

      case STANDBY : {
        if (!i2c_start_cable_present())
          state = FIXED_ROUTE;
        break;
      }
      case EMERGENCY : {
        POS_TYPE x;
        POS_TYPE y;
        bool reverse;
        bool stop;
        motor_get_command(&x,&y,&reverse,&stop);
        motor_command(x,y,reverse, 1);
        usleep(2000000);
        state = FIXED_ROUTE;
        motor_command(x,y,reverse, stop);
        break;
      }
      case FIXED_ROUTE : {
        POS_TYPE x;
        POS_TYPE y;
        bool reverse;
        bool stop;
        motor_get_command(&x,&y,&reverse,&stop);
        if (stop) {
          switch(fix_step++) {
            case 0 : {
              motor_command(1500, 0, 0, 0);
              break;
            }
/*            case 1 : {
              motor_command(750, 1500, 0, 0);
              break;
            }
            case 2 : {
              motor_command(-750, 1500, 0, 0);
              break;
            }
            case 3 : {
              motor_command(-750, 0, 0, 0);
              break;
            }
            case 4 : {
              motor_command(0, 0, 0, 0);
              break;
            }
*//*            case 5 : {
              motor_command(1500, 1500, 1, 0);
              break;
            }
            case 6 : {
              motor_command(500, 1500, 1, 0);
              break;
            }
            case 7 : {
              motor_command(500, 500, 1, 0);
              break;
            }
*/            default : {
              motor_command(0,0,0,1);
              main_running = false;
            }
          }
        }
        break;
      }
      case TRAVELING : {
        // Cestuj
        break;
      }
      case UNLOADING : {
        // Vyprazdni Uloziste
        break;
      }
      default : {
        return -1;
      }
    }
    usleep(main_loop_usleep);
  }

  while (!i2c_start_cable_present())
    usleep(main_loop_usleep);

  return 0;
}

// Inicializace hardware, planovani, zpracovani obrazu
int init_systems() {

  // Nacti vsechny promenne
  var_load(CONFIG_FILE);

  led_init();
  printf("init_systems: 1\n");
  matrix_init();
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
  matrix_close();
  printf("shut_systems: 4\n");
  led_close();
  printf("shut_systems: done\n");
  return 0;
}
