#include <stdio.h>
#include <stdlib.h>

#ifdef MEM_ALLOC
int main(int argc, char** argv) {
  int size = 100;
  unsigned char* shm = new unsigned char[size];
  printf("%s: Allocated size of: %dB at address: 0x%x\n", argv[0], size, shm);

  char mw[100];
  char mr[100];
  sprintf(mw, "./mem_write %d %d", size, shm);
  sprintf(mr, "./mem_read %d %d", size, shm);
  system(mw);
  system(mr);

  return 0;
}
#endif

#ifdef MEM_READ
int main(int argc, char** argv) {
  int size;
  unsigned char* shm;

  if (argc != 3) {
    return 1;
  }

  size = atoi(argv[1]);
  shm = (unsigned char*)atoi(argv[2]);
  printf("%s: Successfuly received memory of: %dB at address: 0x%x\n\n", argv[0], size, shm);
  printf("------------------------------------------------\n");
  for (int i=0; i<size; i++) {
    printf("%x ", shm[i]);
    if ((i+1) % 10 == 0) {
      printf("\n");
    }
  }
  printf("\n");

  return 0;
}
#endif

#ifdef MEM_WRITE
int main(int argc, char** argv) {
  int size;
  unsigned char* shm;

  if (argc != 3) {
    return 1;
  }

  size = atoi(argv[1]);
  shm = (unsigned char*)atoi(argv[2]);
  printf("%s: Successfuly received memory of: %dB at address: 0x%x\n\n", argv[0], size, shm);
  printf("Writing some shit...\n");
  shm[0] = 0xba;
  shm[11] = 0xdf;
  shm[22] = 0xac;
  shm[33] = 0xe1;
  shm[44] = 0x23;

  return 0;
}
#endif

