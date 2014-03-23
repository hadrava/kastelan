#ifndef I2C_COMMUNICATION_H_
#define I2C_COMMUNICATION_H_

#include "types.h"

#define ADDRESS_AD      0x22
#define ADDRESS_GPIO    0x27
#define ADDRESS_TAOS    0x71
#define ADDRESS_SERVO   0x73
#define ADDRESS_ENCODER 0x30

#define TAOS_PSSR       0x00
#define TAOS_PSSR_GC    (1<<2)
#define TAOS_PSSR_SE    (1<<1)
#define TAOS_PSSR_DEF   0x09
#define TAOS_FLIPPER    0x0D
#define TAOS_PSCRL  0x01
#define TAOS_PSCRH  0x02
#define TAOS_PSCWL  0x03
#define TAOS_PSCWH  0x04
#define TAOS_PSCBL  0x05
#define TAOS_PSCBH  0x06
#define TAOS_PSCGL  0x07
#define TAOS_PSCGH  0x08
#define TAOS_PSFPR  0x0A
#define TAOS_PSFPB  0x0B
#define TAOS_PSFPC  0x0C


#define SERVO_AS_SR_DEF 0x08
#define SERVO_RIGHT     0x0F
#define SERVO_LEFT      0x10

// Blackfin - little endian
// AD7998 - big endian

int i2c_init();
int i2c_init_gpio();
int i2c_init_ad();
int i2c_init_taos();
int i2c_init_servo();
int i2c_close();
int i2c_taos_config();
void i2c_taos_get_color();
void i2c_taos_fetch_color(u16* r, u16* g, u16* b, u16* w);
int i2c_taos_set_servo(u08 servo, u08 value);
int i2c_servo_set_servo(u08 servo, u08 value);
int get_ad_register(u08 chreg, u16* value);
int set_gpio_pullup(u08 ioa);
int set_gpio_config();
int get_gpio_aports(u08* value);
bool i2c_start_cable_present();
void i2c_get_bumpers(u08* value);
void i2c_taos_sort_enable();
void i2c_taos_sort_disable();

int i2c_init_encoder();
int i2c_close_encoder();

int i2c_init_encoder();
int i2c_close_encoder();
void i2c_encoder_get(u08* buf);

#endif
