#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define MIN(a,b)               (((a)<(b))?(a):(b))

#define C328_ADDRESS           0xAA
#define C328_TIME              120000
//#define C328_TIME            22000 //sometimes it if enought


#define C328_INITIAL           0x01
#define C328_GET_PICTURE       0x04
#define C328_SNAPSHOT          0x05
#define C328_SET_PACKAGE_SIZE  0x06
#define C328_SET_BAUDRATE      0x07
#define C328_RESET             0x08
#define C328_POWER_OFF         0x09
#define C328_DATA              0x0A
#define C328_SYNC              0x0D
#define C328_ACK               0x0E
#define C328_NAK               0x0F

#define C328_NONE              0x00000000

#define C328_GRAY_2_BIT        (0x01 << 16)
#define C328_GRAY_4_BIT        (0x02 << 16)
#define C328_GRAY_8_BIT        (0x03 << 16)
#define C328_COLOR_12_BIT      (0x05 << 16)
#define C328_COLOR_16_BIT      (0x06 << 16)
#define C328_JPEG              (0x07 << 16)

#define C328_PREVIEW_80_60       (0x01 << 8)
#define C328_PREVIEW_160_120     (0x03 << 8)
#define C328_PREVIEW_VGA         (0x07 << 8)

#define C328_JPEG_80_64          0x01
#define C328_JPEG_160_128        0x03
#define C328_JPEG_320_240        0x05
#define C328_JPEG_640_480        0x07

#define C328_SNAPSHOT_PICTURE    (0x01 << 24)
#define C328_PREVIEW_PICTURE     (0x02 << 24)
#define C328_JPEG_PICTURE        (0x05 << 24)

#define C328_COMPRESSED_PICTURE    (0x00 << 24)
#define C328_UNCOMPRESSED_PICTURE  (0x01 << 24)

#define C328_SKIP_FRAME(skip)      (((skip) & 0xFF << 16) | ((skip) & 0xFF00 << 8))

#define C328_PACKAGE_SIZE(size)    ((0x08 << 24) | ((size) & 0xFF << 16) | ((size) & 0xFF00 << 8))

#define C328_B_7200     ((ff << 24) | (01 << 16))
#define C328_B_9600     ((bf << 24) | (01 << 16))
#define C328_B_14400    ((7f << 24) | (01 << 16))
#define C328_B_19200    ((5f << 24) | (01 << 16))
#define C328_B_28800    ((3f << 24) | (01 << 16))
#define C328_B_38400    ((2f << 24) | (01 << 16))
#define C328_B_57600    ((1f << 24) | (01 << 16))
#define C328_B_115200   ((0f << 24) | (01 << 16))
#define C328_BAUDRATE_DIVIDER(first, second)    (((first) << 24) | ((second) << 16))

#define C328_RESET_SPECIAL        0xFF
#define C328_RESET_FULL           (0x00 << 24)
#define C328_RESET_HALF           (0x01 << 24)

#define C328_ACK_COMMAND(command)      ((command) << 24)
#define C328_ACK_PACKAGE_ID(packageid) (packageid)

typedef unsigned char u08;
typedef unsigned short u16;
typedef unsigned long u32;


int c328_fd;
u08 image[614400];
int image_len = 0;

void c328_send_command(u08 command, u32 parameter = C328_NONE){
  u08 buff[6];
  buff[0] = C328_ADDRESS;
  buff[1] = command;
  buff[2] = parameter >> 24;
  buff[3] = parameter >> 16;
  buff[4] = parameter >> 8;
  buff[5] = parameter;

  printf("SENDING 0x%x %x %x %x %x %x\n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  write(c328_fd, buff, sizeof(buff));
}

int c328_command(u08 command, u32 parameter = C328_NONE) {
  u08 buff[6];
  c328_send_command(command, parameter);

  ssize_t count = read(c328_fd, &buff[0], sizeof(u08)*6);
  printf("Answer 0x%x %x %x %x %x %x\n", buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  if (count < 0) {
    printf("command 0x%X: No Answer, data length: %i\n", command, count);
    return -1;
  }
  else if ((buff[0] == C328_ADDRESS) && (buff[1] == C328_ACK)) {// camera bug: we get ack of command C328_DATA insted of C328_SYNC
    printf("received ACK number %i of command 0x%X\n", buff[3], buff[2]);
    return 0;
  }
  else {
    printf("UNKNOWN Answer\n");
    return 1;
  }
}

void c328_open() {
  c328_fd = open ("/dev/ttyBF1", O_RDWR | O_NOCTTY | O_SYNC);

  struct termios tty;
  if (tcgetattr(c328_fd, &tty) != 0) {
    printf("EE tggetattr\n");
    return;
  }

  cfsetospeed(&tty, B115200);
  cfsetispeed(&tty, B115200);
//  cfsetospeed(&tty, B19200);
//  cfsetispeed(&tty, B19200);

  tty.c_iflag &= ~IGNBRK;         // ignore break signal
  tty.c_lflag = 0;                // no signaling chars, no echo,
                                  // no canonical processing
  tty.c_lflag &= ~ICANON;         // paranoia
  tty.c_oflag = 0;                // no remapping, no delays
  tty.c_cc[VMIN]  = 0;
  tty.c_cc[VTIME] = 2;

  if (tcsetattr(c328_fd, TCSANOW, &tty) != 0)
    printf("EE tcsetattr\n");
}

void c328_close() {
  close(c328_fd);
}

int c328_sync() {
  int maxcount = 60;
  int ret;
  while ((maxcount --) && ((ret = c328_command(C328_SYNC)) != 0))
    ;
  u08 buff[6];

  ssize_t count = read(c328_fd, &buff[0], sizeof(u08)*6);
  if ((count == 6) && (buff[0] == C328_ADDRESS) && (buff[1] == C328_DATA)) {// C328_DATA instead of C328_SYNC because of bug in the camera firmware
    printf("accepted sync request\nsending ACK\n");
    c328_send_command(C328_ACK, C328_ACK_COMMAND(C328_SYNC));
  }
  else {
    printf("EE: NOTaccepted sync request\nNOTsending ACK\n");
    printf("Answer len: %i 0x%x %x %x %x %x %x\n", count, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  }
  return ret;
}

void c328_receive() {
  u08 buff[6];
  ssize_t count = read(c328_fd, buff, sizeof(buff));
  if ((count == 6) && (buff[0] == C328_ADDRESS) && (buff[1] == C328_DATA))
    printf("Data header received\n");

  int len = (buff[5] << 16) | (buff[4] << 8) | buff[3];
  printf("Answer len: %i 0x%x %x %x %x %x %x\n", count, buff[0], buff[1], buff[2], buff[3], buff[4], buff[5]);
  printf("imagetype %i\n", buff[2]);
  printf("imagelen %i\n", len);

  int j=0;
  for (int i = 0; i < len;) {
    if (int count = read(c328_fd, &image[i], MIN(100, len - i))) {
      i += count;
      j++;
      if (j==10) {
        printf("\rBytes read: %i ", i);
        fflush(stdout);
        j=0;
      }
    }
  }
  image_len = len;

  printf("\rImage loaded - sending ACK\n");
  c328_send_command(C328_ACK, C328_ACK_COMMAND(C328_DATA));
}

void save_image(const char* pathname) {
  int fd = open(pathname, O_WRONLY | O_CREAT | O_TRUNC);
  printf("saving %i bytes to file %s, fd %i\n", image_len, pathname, fd);
  for (int i = 0; i < image_len;) {
    if (int count = write(fd, &image[i], MIN(256, image_len - i))) {
      i += count;
    }
  }
  close(fd);
}

int main() {
  int ret;

  c328_open();
  usleep(C328_TIME);
  ret = c328_sync();
  printf("main: sync: %i\n", ret);
  if (ret != 0)
    return 1;

  usleep(C328_TIME);
  ret = c328_command(C328_INITIAL, C328_COLOR_12_BIT | C328_PREVIEW_80_60 | C328_JPEG_80_64);
  printf("main: initial: %i\n", ret);
  if (ret != 0)
    return 1;



  usleep(C328_TIME);
  ret = c328_command(C328_SNAPSHOT, C328_UNCOMPRESSED_PICTURE);
  printf("main: snapshot: %i\n", ret);
  if (ret != 0)
    return 1;

  usleep(C328_TIME);
  ret = c328_command(C328_GET_PICTURE, C328_PREVIEW_PICTURE);
  printf("main: getpicture: %i\n", ret);
  if (ret != 0)
    return 1;
  c328_receive();
  save_image("image.raw");


  usleep(C328_TIME);
  ret = c328_command(C328_POWER_OFF);;
  printf("main: poweroff: %i\n", ret);
  if (ret != 0)
    return 1;

  c328_close();
  return 0;
}
