#ifndef CNY_H_
#define CNY_H_

#define AD_PUCK_COLOR_CNY  0x00
#define SD20_FLIPPER       0x01
#define CNY_COLOR_RED      'R'
#define CNY_COLOR_BLUE     'B'
#define CNY_COLOR_NONE     'N'
#define FLIPPER_LEFT_SIDE  0x20
#define FLIPPER_RIGHT_SIDE 0xE0
#define FLIPPER_CENTER     0x80

u16 check_cny();
int check_cny_multiple(int *minimum = 0, int *maximum = 0, int num = 20, int time = 50000, int verbose = 0);

#endif
