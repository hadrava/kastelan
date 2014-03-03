#include <stdio.h>
#include <stdlib.h>

int main () {
  while (1) {
    int a;
    int len;

    a=getchar();//#
      if (a!='#') {
        printf("skip!\n");
        continue;}
    a=getchar();//#
      if (a!='#') {
        printf("skip!\n");
        continue;}
    a=getchar();//I
      if (a!='I') {
        printf("skip!\n");
        continue;}
    a=getchar();//M
      if (a!='M') {
        printf("skip!\n");
        continue;}
    a=getchar();//J
      if (a!='J') {
        printf("skip!\n");
        continue;}
    a=getchar();//5
      if (a!='5') {
        printf("skip!\n");
        continue;}

    a=getchar();
    len=a;
    a=getchar();
    len+=a<<8;
    a=getchar();
    len+=a<<16;
    a=getchar();
    
    FILE * out = fopen("file.jpg","w");
    int ix;
    for (ix=0; ix<len; ix++) {
      a=getchar();
      fputc(a,out);
    }
    fflush(out);
    fclose(out);
    system("killall display");
    system("display file.jpg&");
    system("sleep 0.05");
  }
}
