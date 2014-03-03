#include <stdio.h>
//ukazkovy program pro cteni z klavesnice pomoci driveru

char decode(unsigned char i) {
  switch(i) {
    case 0x11:
      return '1';
    case 0x12:
      return '2';
    case 0x14:
      return '3';
    case 0x18:
      return 'A';
    case 0x21:
      return '4';
    case 0x22:
      return '5';
    case 0x24:
      return '6';
    case 0x28:
      return 'B';
    case 0x41:
      return '7';
    case 0x42:
      return '8';
    case 0x44:
      return '9';
    case 0x48:
      return 'C';
    case 0x81:
      return '*';
    case 0x82:
      return '0';
    case 0x84:
      return '#';
    case 0x88:
      return 'D';
    case 0x00:
      return 'U';//Up: klavesa pustena
    default:
      return 'E';//Error: napr. stisknuto vice klaves soucasne
  }
}

int main() {
  FILE *kbd = fopen("/dev/kbd0", "r");
  unsigned char klavesa;
  while (1) {
    fscanf(kbd, "%c", &klavesa);//cekej na stisk klavesy
    printf("%c\n", decode(klavesa));
  };

  return 0;
}
