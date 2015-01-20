#include "types.h"
#include "stat.h"
#include "user.h"
#include "uwindow.h"
#include "bitmap.h"

Window window;
ImageView image;
struct RGB temp[1310720];

int main(int argc, char *argv[])
{
  memset(&window, 0, sizeof(Window));
  window.leftTopX = 300;
  window.leftTopY = 200;
  window.width = 600;
  window.height = 500;
  window.show = 1;
  window.hasCaption = 1;
  strcpy(window.caption, "ImageViewer");
  if (strcmp(argv[0], "imageviewer") != 0)
  {
    memset(&image, 0, sizeof(ImageView));
    image.image = temp;
printf(1, "%s\n", argv[0]);
    readBitmapFile(argv[0], image.image, &image.height, &image.width);
printf(1, "%d %d\n", image.width, image.height);
    image.leftTopX = window.leftTopX + (window.width >> 1) - (image.width >> 1);
    image.leftTopY = window.leftTopY + (window.height >> 1) - (image.height >> 1);
    window.widgets[0].type = imageView;
    window.widgets[0].context.imageView = &image;
    window.widgetsNum = 1;
  }
  createWindow(&window);
  exit();
}
