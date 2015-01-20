#include "types.h"
#include "stat.h"
#include "user.h"
#include "uwindow.h"
#include "fs.h"
#include "bitmap.h"

Window window;
ImageView image;
IconView icon[50];
struct RGB temp[1310720];
struct RGB folder[50][10000];
void (*f)(void) = 0;

char*
fmtname(char *path)
{
  char *p;
  
  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;

  return p;
}

void iconOnLeftDoubleClick(Widget *widget)
{
  char *argv[] = { "file.bmp", 0 };
  if (fork() == 0)
  {
    exec("imageviewer", argv);
    exit();
  }
}

void
ls(char *path)
{
  char buf[512], *p, *tmpName;
  int fd;
  struct dirent de;
  struct stat st;
  int i = 0;

  if ((fd = open(path, 0)) < 0) 
  {
    return;
  }

  if (fstat(fd, &st) < 0)
  {
    return;
  }
  
  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';
  while (read(fd, &de, sizeof(de)) == sizeof(de))
  {
    if (de.inum == 0)
    {
      continue;
    }
    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = 0;
    if (stat(buf, &st) < 0)
    {
      continue;
    }
    tmpName = fmtname(buf);
    if (strcmp(tmpName, ".") == 0 || strcmp(tmpName, "..") == 0 || st.type == T_DEV)
    {
      continue;
    }
    memset(icon + i, 0, sizeof(IconView));
    icon[i].leftTopX = 50 + (i / 6) * 140;
    icon[i].leftTopY = 50 + (i % 6) * 140;
    icon[i].image = folder[i];
    icon[i].onLeftDoubleClickHandler.triggered = 0;
    icon[i].onLeftDoubleClickHandler.handlerFunction = iconOnLeftDoubleClick;
    switch (st.type)
    {
      case T_DIR:
        readBitmapFile("folder.bmp", icon[i].image, &icon[i].height, &icon[i].width);
        break;
      case T_FILE:
        readBitmapFile("file.bmp", icon[i].image, &icon[i].height, &icon[i].width);
        break;
    }
    strcpy(icon[i].text, tmpName);
    window.widgets[i + 1].type = iconView;
    window.widgets[i + 1].context.iconView = &icon[i];
    i++;
  }
  window.widgetsNum = i + 1;
}

int main(void)
{
  memset(&window, 0, sizeof(Window));
  window.leftTopX = 0;
  window.leftTopY = 0;
  window.width = 1280;
  window.height = 1024;
  window.show = 1;
  window.hasCaption = 0;
  memset(&image, 0, sizeof(ImageView));
  image.leftTopX = 0;
  image.leftTopY = 0;
  image.image = temp;
  readBitmapFile("desktop_background.bmp", image.image, &image.height, &image.width);
  window.widgets[0].type = imageView;
  window.widgets[0].context.imageView = &image;

  ls(".");

  createWindow(&window);
  while (1) handleEvent(&window);
}

