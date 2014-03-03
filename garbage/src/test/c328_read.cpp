#include <stdio.h>
#include <unistd.h>
#include <termios.h>


typedef unsigned char u08;
typedef unsigned short u16;
typedef unsigned long u32;



FILE *c328_fd;
struct termios orig_set;

struct termios input_conf(FILE *func_in)//nastavi vstup
{
  struct termios func_orig_set, func_act_set;
  
  tcgetattr(fileno(func_in), &func_orig_set);
  func_act_set = func_orig_set;
  func_act_set.c_lflag &= (~ICANON & ~ECHO);//nezobrazovat + okamzite posilat programu
  tcsetattr(fileno(func_in), TCSANOW, &func_act_set);
  return func_orig_set;
}

void input_deconf(FILE *func_in, struct termios func_orig_set)//vrati nastaveni vstupu na vychozi
{
  tcsetattr(fileno(func_in), TCSANOW, &func_orig_set);
}

void c328_open(){
  c328_fd = fopen ("/dev/ttyBF1","rw");
  orig_set = input_conf(c328_fd);
}

void c328_close(){
  input_deconf(c328_fd, orig_set);
  fclose(c328_fd);
}

int main() {
  c328_open();

  u08 data;
  int count;
  printf("data\n");
  while ((count = fscanf(c328_fd, "%c", &data)) >= -1) {
    printf("data 0x%X\n", data);
    fflush(stdout);
  }

//  printf("%i\n",count);
  
  c328_close();
  return 0;
}
