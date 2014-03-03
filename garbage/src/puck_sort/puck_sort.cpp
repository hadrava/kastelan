#include <stdio.h>
#include <unistd.h>
//#include <sys/time.h>
//#include <math.h>
//#include <iostream>
#include "variable_loader.h"
#include "i2c_communication.h"
#include "cny.h"

#define PUCK_LEFT        0x48
#define PUCK_RIGHT       0xd0
#define PUCK_STOP        0x80

#define LEFT_STORE_OPEN  0xff
#define LEFT_STORE_CLOS  0x70


int main() {
  // Nacti vsechny promenne
  var_load(CONFIG_FILE);

  i2c_init_sd20();
  i2c_init_ad();

  while (1) {
    int cnyval = check_cny();
    if (cnyval < cnycolormax ) {
      switch (ourcolor) {
        case 'B' : 
          if ( cnyval <= cnycolorredmax && cnyval >= cnycolorredmin )
            set_sd20_servo( SERVO_PUCK, PUCK_RIGHT);
          else
            set_sd20_servo( SERVO_PUCK, PUCK_LEFT);
          break;
        case 'R' :
          if (cnyval <= cnycolorbluemax && cnyval >= cnycolorbluemin )
            set_sd20_servo( SERVO_PUCK, PUCK_RIGHT);
          else
            set_sd20_servo( SERVO_PUCK, PUCK_LEFT);
          break;
        default :
          set_sd20_servo( SERVO_PUCK, PUCK_LEFT);
      }
    } else {
       set_sd20_servo( SERVO_PUCK, PUCK_STOP);
    }
    usleep(stepsleepus*cnycheck);
  }
}
