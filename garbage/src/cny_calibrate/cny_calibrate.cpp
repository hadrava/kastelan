#include "variable_loader.h"
#include "i2c_communication.h"
#include "cny.h"
#include <stdio.h>
#include <unistd.h>
#include <termios.h>

#define time 250000

void vypisuj(void)
{
  struct termios oldset, newset;
  tcgetattr(fileno(stdin), &oldset);
  newset = oldset;
  newset.c_lflag &= (~ICANON);
  newset.c_cc[VMIN] = 0;
  newset.c_cc[VTIME] = 2;
  tcsetattr(fileno(stdin), TCSANOW, &newset);

  int val;
  while (getc(stdin) == -1) {
    val = check_cny();
    printf("\r%i ", val);
    fflush(stdout);
//    usleep(time);
  }

  tcsetattr(fileno(stdin), TCSANOW, &oldset);
}

int main()
{
  var_load("variable_list");
  i2c_init();
  
  printf("Remove all pucks from CNY and press ENTER\n");
  getchar();
  //vypisuj();
  cnycolorred  = check_cny_multiple(&cnycolorredmin,  &cnycolorredmax,  40, 100000, 1);
  printf("White: min=%i max=%i avg=%i\n", cnycolorredmin, cnycolorredmax, cnycolorred);


  printf("Put RED puck under CNY and press ENTER\n");
  getchar();
  //vypisuj();
  cnycolorred  = check_cny_multiple(&cnycolorredmin,  &cnycolorredmax,  40, 100000, 1);
  printf("Red: min=%i max=%i avg=%i\n", cnycolorredmin, cnycolorredmax, cnycolorred);
  
  printf("Put BLUE puck under CNY and press ENTER\n");
  getchar();
  //vypisuj();
  cnycolorblue = check_cny_multiple(&cnycolorbluemin, &cnycolorbluemax, 40, 100000, 1);
  printf("Blue: min=%i max=%i avg=%i\n", cnycolorbluemin, cnycolorbluemax, cnycolorblue);
  
  cnycolormax = (cnycolorredmax > cnycolorbluemax) ? cnycolorredmax : cnycolorbluemax;
  var_save("variable_list");
}
