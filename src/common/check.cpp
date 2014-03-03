#include "i2c_communication.h"
#include "variable_loader.h"
#include "check.h"

#include <stdio.h>

time_t timestart;
int bumpnum;

inline int check_bumpers(u08* bumps) {
  if (get_gpio_aports(bumps)) {
    printf("Error reading GPIO ports for bumpers!!! PRUUUSER\n");
    return -1;
  }
  return *bumps;
}

inline int check_sharp(u16* sharpa, u16* sharpb) {
  int prah;
  get_ad_register(0, sharpa);
  get_ad_register(1, sharpb);
  return prah < *sharpa || prah < *sharpb;
}

int check_timer() {
  time_t now = time(NULL);
//  printf("ted je %i\nstojim od %i\n", now, timestart + timeend);
  if (now >= (timestart + timeend))
    return 2;
  else if (now >= (timestart + timewarn))
    return 1;
  else
    return 0;
}

void timer_reset() {
  timestart = time(NULL);
}

// Sharpy nemaj vybrany registry. a jestli se pouzijou
int check_travel(u08* bumpers, u16* sharpa, u16* sharpb, u08* flags) {
  if (check_timer()) {
    return 1;
  }

  if (check_bumpers(bumpers)) {
    printf("flag F %i\nR %i\n", *flags & SKIP_FBUMP_CHECK, *flags & SKIP_RBUMP_CHECK);
/*    if (((*flags & SKIP_RBUMP_CHECK) ||
        (*bumpers ^ BUMP_REAR_LEFT) &&
        (*bumpers ^ BUMP_REAR_RIGHT)) &&
        ((*flags & SKIP_FBUMP_CHECK) ||
        (*bumpers ^ BUMP_FRONT_LEFT) &&
        (*bumpers ^ BUMP_FRONT_RIGHT)))
*/
    if ((*flags & SKIP_FBUMP_CHECK) == 0) {
      printf("flag l %i\nR %i\n", *bumpers & BUMP_FRONT_LEFT, *bumpers & BUMP_FRONT_RIGHT);
      if (((*bumpers & BUMP_FRONT_LEFT) == BUMP_FRONT_LEFT) || ((*bumpers & BUMP_FRONT_RIGHT) == BUMP_FRONT_RIGHT))
        return 1;
    }
    if ((*flags & SKIP_RBUMP_CHECK) == 0) {
      printf("flag F %i\nR %i\n", *flags & SKIP_FBUMP_CHECK, *flags & SKIP_RBUMP_CHECK);
      if (((*bumpers & BUMP_REAR_LEFT) == BUMP_REAR_LEFT) || ((*bumpers & BUMP_REAR_RIGHT) == BUMP_REAR_RIGHT))
        return 1;
    }
  }
  return 0;
}
