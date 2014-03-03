#include <stdio.h>
#include <stdlib.h>   // maybe not needed
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>   // maybe not needed
#include <sys/types.h>// maybe not needed
#include <sys/stat.h> // maybe not needed
#include <sys/mman.h>
#include <sys/ioctl.h>// maybe not needed
#include <sys/time.h> // maybe not needed
#include <termios.h>  // maybe not needed
#include <linux/videodev.h>

#include "jpeg.h"
unsigned char jpeg_outbuf[0x100000];

#define DEVICE_FILE "/dev/video0"
#define WIDTH 1280
#define HEIGHT 1024
#define DEFAULT_FMT VIDEO_PALETTE_UYVY
#define DEFAULT_LEN 16
#define DEFAULT_RATE 30

long get_cur_ms() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  return (tv.tv_sec * 1000 + tv.tv_usec / 1000);
}

int frame_rate = DEFAULT_RATE;
char * outfile = NULL;
char pix_buf[(HEIGHT * WIDTH * 2)];

int main () {
  int err = -1;
  int j = 0;
  int video = 0;
  char * vbuf = NULL, *ptr = NULL;
  int frame_size = 0;

  struct video_capability vidcap;
  struct video_mbuf       vmbuf;
  struct video_mmap       vmmap;
  struct video_picture    pict;
  int  frame_id  = 0;
  int  frame_cnt = 0;
  long start_ms  = 0, now_ms = 0, offset_ms = 0;

  video = open(DEVICE_FILE, O_RDWR);
  if (video < 0) {
    printf("cannot open %s\n", DEVICE_FILE);
    return -1;
  }

  err=ioctl(video, VIDIOCGCAP, &vidcap);
  if (err!=0) {
    printf("cannot get device capabilities: %s.\n",strerror(errno));
    return -1;
  }
  fprintf(stdout, "found %s device. (maxsize=%ix%i)\n",vidcap.name, vidcap.maxwidth, vidcap.maxheight);

  err=ioctl(video, VIDIOCGPICT, &pict);
  if (err<0){
    printf("could not get picture properties: %s\n",strerror(errno));
    return -1;
  }
  fprintf(stdout, "default picture properties: brightness=%i,hue=%i,colour=%i,contrast=%i,depth=%i, palette=%i.\n",
      pict.brightness,pict.hue,pict.colour, pict.contrast,pict.depth, pict.palette);

  pict.palette=DEFAULT_FMT;
  pict.depth=DEFAULT_LEN;
  err=ioctl(video,VIDIOCSPICT,pict);
  if (err<0){
    printf("could not set picture properties: %s\n",strerror(errno));
    printf("unsupported video pixel format.\n");
    return -1;
  }

  struct video_window win;
  memset(&win,0,sizeof(win));
  win.x=win.y=0;
  win.width=WIDTH;
  win.height=HEIGHT;
  win.flags=0;
  win.clips=NULL;
  win.clipcount=0;
  err=ioctl(video,VIDIOCSWIN,&win);
  if (err<0){
    printf("could not set window size: %s\n",strerror(errno));
    return -1;
  }
  err=ioctl(video, VIDIOCGWIN, &win);
  if (err<0){
    printf("could not get window size: %s\n",strerror(errno));
    return -1;
  }
  if (WIDTH!=win.width || HEIGHT!=win.height){
    printf("capture size is not what we expected: asked for %ix%i and get %ix%i\n",
        WIDTH, HEIGHT, win.width, win.height);
    return -1;
  }

  memset((void*)&vmbuf,0,sizeof(vmbuf));
  err=ioctl(video,VIDIOCGMBUF,&vmbuf);
  if (err<0) {
    printf("could not get mmap properties: %s\n",strerror(errno));
  }
  vbuf = mmap(NULL,vmbuf.size,PROT_READ,MAP_PRIVATE,video,0);
  if (vbuf == (void*)-1) {
    printf("could not mmap: %s\n",strerror(errno));
  }

  frame_size = vmbuf.size;
  printf("Frame size: %d\n", frame_size);
  frame_id = 0;

  /* start to grab */
  vmmap.height = HEIGHT;
  vmmap.width = WIDTH;
  vmmap.format = pict.palette;

  for (j = 0; j < vmbuf.frames; j++) {
    vmmap.frame = j;
    ioctl(video, VIDIOCMCAPTURE, &vmmap);
  }

  /* capture */
  start_ms = get_cur_ms();
   while (1) {
    while (ioctl(video, VIDIOCSYNC, &frame_id) < 0 &&
        (errno == EAGAIN || errno == EINTR));
      ptr = vbuf + vmbuf.offsets[frame_id];

    printf("got frame: ms = %lu\n", get_cur_ms());
    int jy,jx;
    for (jy = 0; jy< HEIGHT/4; jy++)
      for (jx = 0; jx< WIDTH/4; jx++){
        ptr[(jx+jy*WIDTH/4)*2]=ptr[(jx*4+jy*WIDTH*4)*2];
        ptr[(jx+jy*WIDTH/4)*2+1]=ptr[(jx*4+jy*WIDTH*4)*2+1];
      }

    int ix;
    unsigned char * out = encode_image((unsigned char*)ptr, jpeg_outbuf, 500, FOUR_TWO_TWO, WIDTH/4, HEIGHT/4);
    unsigned long len = (out - jpeg_outbuf);
    printf("##IMJ5%c%c%c%c", len & 0xff, (len >> 8) & 0xff, (len >> 16) & 0xff, 0);
    for (ix=0; ix<len; ix++)
      putchar(jpeg_outbuf[ix]);
    fflush(0);

    /* setup to capture the next frame */
    vmmap.frame = frame_id;
    if (ioctl(video, VIDIOCMCAPTURE, &vmmap) < 0) {
      perror("VIDIOCMCAPTURE");
      return -1;
    }

    /* this is now the grabbing frame */
    frame_id = (frame_id + 1) % vmbuf.frames;

    /* capture rate control */
    frame_cnt++;
    now_ms = get_cur_ms() - start_ms;
    offset_ms = frame_cnt * 1000 / frame_rate;
    if (offset_ms > now_ms)
      usleep((offset_ms - now_ms) * 1000);
  }
  return 0;
}
