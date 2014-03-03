#include "i2c_communication.h"
#include "variable_loader.h"

#include <sys/io.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int fAD;
int fGPIO;
int fTAOS;
int fSERVO;

inline int i2c_write(int file, void* buf, int size) {
  int r = write(file, buf, size);
  if (r != size) {
    fprintf(stderr, "Error writing to i2c, file %d\n", file);
    return -1;
  }
  return 0;
}

inline int i2c_read(int file, void* buf, int size) {
  int r = read(file, buf, size);
  if (r != size) {
    fprintf(stderr, "Error reading from i2c, file %d\n", file);
    return -1;
  }
  return 0;
}

inline int i2c_open(const char* busname, int address) {
  int f = open(busname, O_RDWR);

  if (f < 0)  {
    fprintf(stderr, "Error opening i2c bus for device %x\n", address);
    return -1;
  }
  if (ioctl(f, I2C_SLAVE_FORCE, address) < 0) {
    fprintf(stderr, "Error setting up ioctl() for device %x\n", address);
    return -1;
  }

  return f;
}

int i2c_init() {
//  fAD = i2c_open("/dev/i2c-0", ADDRESS_AD);
  if (i2c_init_gpio() == -1)
    return -1;
  if (i2c_init_taos() == -1)
    return -1;
  if (i2c_init_servo() == -1)
    return -1;
  return 0;
}

int i2c_init_gpio() {
  fGPIO = i2c_open("/dev/i2c-0", ADDRESS_GPIO);

  if (fGPIO != -1) {
    set_gpio_config();
    return 0;
  } else {
    return -1;
  }
}

int i2c_init_ad() {
  fAD = i2c_open("/dev/i2c-0", ADDRESS_AD);
  return fAD == -1;
}

int i2c_init_taos() {
  fTAOS = i2c_open("/dev/i2c-0", ADDRESS_TAOS);
  if (fTAOS == -1)
    return -1;

  i2c_taos_config();
  return 0;
}

int i2c_init_servo() {
  fSERVO = i2c_open("/dev/i2c-0", ADDRESS_SERVO);
  if (fSERVO == -1)
    return -1;

  i2c_servo_set_servo(SERVO_LEFT, servo_left_close);
  i2c_servo_set_servo(SERVO_RIGHT, servo_right_close);
  u08 buf[2];
  buf[0] = 0x00, buf[1] = SERVO_AS_SR_DEF;
  i2c_write(fSERVO, buf, 2);
  return 0;
}

int i2c_close() {
  u08 buf[2];

  buf[0] = 0x00, buf[1] = 0x00;
  i2c_write(fSERVO, buf, 2);

  buf[0] = 0x00, buf[1] = 0x00;
  i2c_write(fTAOS, buf, 2);

  return close(fGPIO) && close(fTAOS) && close(fSERVO);
}

int i2c_taos_set_servo(u08 servo, u08 value) {
  u08 buf[2];
  buf[0] = servo, buf[1] = value;
  i2c_write(fTAOS, buf, 2);
}

int i2c_servo_set_servo(u08 servo, u08 value) {
  u08 buf[2];
  buf[0] = servo, buf[1] = value;
  i2c_write(fTAOS, buf, 2);
}

int i2c_taos_config() {
  i2c_taos_set_servo(TAOS_FLIPPER, taos_flipper_center);

  u08 buf[2];
  buf[0] = 0x00, buf[1] = TAOS_PSSR_DEF;
  i2c_write(fTAOS, buf, 2);

  if (((ourside == 'L') && (ourcolor == 'R')) || ((ourside != 'L') && (ourcolor != 'R'))) {
    buf[0] = TAOS_PSFPR, buf[1] = taos_flipper_left;
    i2c_write(fTAOS, buf, 2);
    buf[0] = TAOS_PSFPB, buf[1] = taos_flipper_right;
    i2c_write(fTAOS, buf, 2);
  }
  else {
    buf[0] = TAOS_PSFPB, buf[1] = taos_flipper_left;
    i2c_write(fTAOS, buf, 2);
    buf[0] = TAOS_PSFPR, buf[1] = taos_flipper_right;
    i2c_write(fTAOS, buf, 2);
  }
  buf[0] = TAOS_PSFPC, buf[1] = taos_flipper_center;
  i2c_write(fTAOS, buf, 2);
  return 0;
}

int get_ad_register(u08 chreg, u16* value) {
  u08 buf[2];
  buf[0] = (chreg << 4) | 0x80 ;

  if (i2c_write(fAD, buf, 1) < 0 || i2c_read(fAD, buf, 2) < 0) {
    return 1;
  }

  *value =  ((buf[0] & 0x0f ) << 8) | buf[1];
  return 0;
}

// Vstupy B jsou ovladany kernelem, nehrabat na ne
int set_gpio_pullup(u08 ioa) {
  u08 buf[] = {0x0c, ioa};
  return i2c_write(fGPIO, buf, 2) < 0;
}

int set_gpio_config() {
  set_gpio_pullup(0xff);
  return 0;
}

int get_gpio_aports(u08* value) {
  u08 buf[] = {0x12};
  return i2c_write(fGPIO, buf, 1) < 0 || i2c_read(fGPIO, value, 1) < 0;
}

void i2c_taos_get_color() {
  u08 buf[2];
  buf[0] = TAOS_PSSR, buf[1] = TAOS_PSSR_DEF | TAOS_PSSR_GC;
  i2c_write(fTAOS, buf, 2);
}

void i2c_taos_fetch_color(u16* r, u16* g, u16* b, u16* w) {
  u08 buf[1];

  buf[0] = TAOS_PSCRL; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *r = buf[0];
  buf[0] = TAOS_PSCRH; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *r |= buf[0] << 8;
  buf[0] = TAOS_PSCWL; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *w = buf[0];
  buf[0] = TAOS_PSCWH; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *w |= buf[0] << 8;
  buf[0] = TAOS_PSCBL; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *b = buf[0];
  buf[0] = TAOS_PSCBH; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *b |= buf[0] << 8;
  buf[0] = TAOS_PSCGL; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *g = buf[0];
  buf[0] = TAOS_PSCGH; i2c_write(fTAOS, buf, 1); i2c_read(fTAOS, buf, 1); *g |= buf[0] << 8;
  printf("TAOS (RGBW): %X %X %X %X\n", *r, *g, *b, *w);
}

bool i2c_start_cable_present() {
  u08 buf[] = {0x12}; //port A
  i2c_write(fGPIO, buf, 1);
  i2c_read(fGPIO, buf, 1);
  if (buf[0] & 0x40)
    return 1;
  else
    return 0;
}

void i2c_get_bumpers(u08* value) {
  u08 buf[] = {0x12}; //port A
  i2c_write(fGPIO, buf, 1);
  i2c_read(fGPIO, buf, 1);
  *value = (~buf[0]) & 0x3F;
}

void i2c_taos_sort_enable() {
  u08 buf[2];
  buf[0] = TAOS_PSSR, buf[1] = TAOS_PSSR_DEF | TAOS_PSSR_SE;
  i2c_write(fTAOS, buf, 2);
}
void i2c_taos_sort_disable() {
  u08 buf[2];
  buf[0] = TAOS_PSSR, buf[1] = TAOS_PSSR_DEF;
  i2c_write(fTAOS, buf, 2);
  i2c_taos_set_servo(TAOS_FLIPPER, taos_flipper_center);
}
