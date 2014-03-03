#include <stdio.h>
#include <opencv/highgui.h>
#include <opencv/cxcore.h>
#include <opencv/cv.h>

void read_image_from(IplImage* img_input, FILE * input_file = 0, int swap = 1) {
  for (int y = 0; y < img_input->height; y++) {
    for (int x = 0; x < img_input->width && !feof(input_file); x+=2) {
      int a = fgetc(input_file);//RG
      int b = fgetc(input_file);//BR
      int c = fgetc(input_file);//GB

      img_input->imageData[(x+0 + y*img_input->width)*3 + 0] =  b & 0xF0;      //B
      img_input->imageData[(x+0 + y*img_input->width)*3 + 1] = (a & 0x0F) << 4;//G
      img_input->imageData[(x+0 + y*img_input->width)*3 + 2] =  a & 0xF0;      //R
      img_input->imageData[(x+1 + y*img_input->width)*3 + 0] = (c & 0x0F) << 4;//B
      img_input->imageData[(x+1 + y*img_input->width)*3 + 1] =  c & 0xF0;      //G
      img_input->imageData[(x+1 + y*img_input->width)*3 + 2] = (b & 0x0F) << 4;//R
    }
  }
}

int main (int argc, char** argv) {
  IplImage* img_input = cvCreateImage(cvSize(80, 60), IPL_DEPTH_8U, 3);
  FILE * input_file = stdin;

  cvNamedWindow("image");
  while (!feof(input_file)) {
    cvZero(img_input);

    read_image_from(img_input, input_file);

    cvShowImage("image", img_input);
    cvWaitKey(0);
  }
  cvDestroyWindow("image");
  cvReleaseImage(&img_input);
  return 0;
}
