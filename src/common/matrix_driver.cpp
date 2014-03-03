#include <stdio.h>
#include <pthread.h>
#include <sched.h>

#include "types.h"
#include "matrix_driver.h"

FILE *kbd;

bool matrix_running;
pthread_t matrix_loop_tid;
pthread_mutex_t matrix_lock; // zamyka matrix_raw a matrix_decode
pthread_mutex_t matrix_waiting; // slouzi k suspendovani vlakna cekajiciho na klavesu
pthread_mutex_t matrix_reading; // slouzi k suspendovani matrix_loop dokud si neprectu aktualni stav

u08 matrix_raw;
char matrix_char;

void *matrix_loop(void *arg) {
  while (matrix_running) {
    u08 klavesa;

    pthread_mutex_lock(&matrix_waiting);
     fscanf(kbd, "%c", &klavesa);
     pthread_mutex_lock(&matrix_lock);
      matrix_raw = klavesa;
      matrix_char = matrix_decode(matrix_raw);
     pthread_mutex_unlock(&matrix_lock);
    pthread_mutex_unlock(&matrix_waiting);

    pthread_mutex_lock(&matrix_reading);
    pthread_mutex_unlock(&matrix_reading);
  }
  pthread_exit(NULL);
}

int matrix_init() {
  kbd = fopen("/dev/kbd0", "r");
  if (!kbd)
    return -1;

  matrix_running = 1;
  pthread_mutex_init(&matrix_lock, NULL);
  pthread_mutex_init(&matrix_waiting, NULL);
  pthread_mutex_init(&matrix_reading, NULL);
  pthread_attr_t *thAttr = NULL;
  pthread_create(&matrix_loop_tid, thAttr, matrix_loop, NULL);
  return 0;
}

void matrix_close() {
  matrix_running = 0;
//  void *matrix_loop_return;
//  pthread_join(matrix_loop_tid, &matrix_loop_return);

//  fclose(kbd);
}

u08 matrix_wait_for_raw() {
  u08 klavesa;
  pthread_mutex_lock(&matrix_reading); // do not start reading new key
  pthread_mutex_lock(&matrix_waiting); // wait for key
  pthread_mutex_lock(&matrix_lock); // we can read key now
   klavesa = matrix_raw;
  pthread_mutex_unlock(&matrix_lock);
  pthread_mutex_unlock(&matrix_waiting);
  pthread_mutex_unlock(&matrix_reading);
  return klavesa;
}

char matrix_wait_for_char() {
  char klavesa;
  pthread_mutex_lock(&matrix_reading); // do not start reading new key
  pthread_mutex_lock(&matrix_waiting); // wait for key
  pthread_mutex_lock(&matrix_lock); // we can read key now
   klavesa = matrix_char;
  pthread_mutex_unlock(&matrix_lock);
  pthread_mutex_unlock(&matrix_waiting);
  pthread_mutex_unlock(&matrix_reading);
  return klavesa;
}

u08 matrix_get_last_raw() {
  u08 klavesa;
  pthread_mutex_lock(&matrix_lock); // we can read key now
   klavesa = matrix_raw;
  pthread_mutex_unlock(&matrix_lock);
  return klavesa;
}

char matrix_get_last_char() {
  char klavesa;
  pthread_mutex_lock(&matrix_lock); // we can read key now
   klavesa = matrix_char;
  pthread_mutex_unlock(&matrix_lock);
  return klavesa;
}

char matrix_wait_for_char_press() {
  char klavesa = matrix_wait_for_char();
  while ((klavesa == 'U') || (klavesa == 'E')) {
    sched_yield(); // pthread_yield() on some systems
    klavesa = matrix_wait_for_char();
  }
  return klavesa;
}

char matrix_decode(u08 i) {
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
