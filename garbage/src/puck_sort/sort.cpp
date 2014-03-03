#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include "variable_loader.h"
#include "i2c_communication.h"
#include "cny.h"

inline char detect_color(int value, int minimal, int maximal) {
  if ((cnycolorredmin < minimal) && (maximal < cnycolorredmax))
    return CNY_COLOR_RED;

  if ((cnycolorbluemax < minimal) && (maximal < cnycolorbluemax))
    return CNY_COLOR_BLUE;
  return CNY_COLOR_NONE;
}

void perform_detection() {
  int val, min, max;

  val = check_cny_multiple(&min, &max);
  char detected_color = detect_color(val, min, max);
  char othercolor;

  if (ourcolor == CNY_COLOR_RED)
    othercolor = CNY_COLOR_BLUE;
  else
    othercolor = CNY_COLOR_RED;

  if (detected_color == ourcolor)
    set_sd20_servo(SD20_FLIPPER, FLIPPER_LEFT_SIDE);
  if (detected_color == othercolor)
    set_sd20_servo(SD20_FLIPPER, FLIPPER_RIGHT_SIDE);
  if (detected_color == CNY_COLOR_NONE)
    set_sd20_servo(SD20_FLIPPER, FLIPPER_CENTER);
}

int main()
{
  var_load("variable_list");
  i2c_init();
  while(true) {
    perform_detection();
  }
}
