#include "i2c_communication.h"
#include "cny.h"
#include <stdio.h>
#include <unistd.h>

u16 check_cny() {
  u16 state;
  get_ad_register(AD_PUCK_COLOR_CNY, &state);
#ifdef VERBOSE
  printf("CNY state: %d\n", state);
#endif
  return state;
}

int check_cny_multiple(int *minimum, int *maximum, int num, int time, int verbose) {
  int sum = 0;
  int min = 0x0fff;
  int max = 0;
  int val;
  if (verbose == 1) {
    printf("Collecting data");
    fflush(stdout);
  }

  for (int c = 0; c < num; c++) {
    val = check_cny();
    sum += val;
    if (val < min)
      min = val;
    if (val > max)
      max = val;
    if (verbose == 1) {
      printf(".");
      fflush(stdout);
    }
    usleep(time);
  }

  if (verbose == 1) {
    printf(" done.\n");
    fflush(stdout);
  }
  if (minimum != NULL)
    *minimum = min;
  if (maximum != NULL)
    *maximum = max;
  return sum / num;
}

#ifdef TEST_CNY
int main(int argc, char** argv) {
  int i = 0;
  u16 val;

  if (i2c_init()) {
    return 1;
  }

  printf("Reading CNY 10 times with 1s interval:\n");
  while (i++ < 10) {
    printf("%x\n", check_cny());
    sleep(1);
  }

  return i2c_close();
}
#endif

