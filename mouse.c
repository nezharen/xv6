#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "traps.h"
#include "x86.h"
#include "spinlock.h"
#include "gui.h"
#include "mouse.h"
#include "window.h"

struct spinlock mouse_lock;
struct mouseinfo mouse_info;
static int to_read = 1;
static int left_down = 0;
static int right_down = 0;
static int x_sign = 0;
static int y_sign = 0;
static int x_overflow = 0;
static int y_overflow = 0;
//static int counter = 0;
static int dis_x = 0 , dis_y = 0;

//mouse event
static int left_btn_down = 0;
static int right_btn_down = 0;
static int last_tick = -20;
static int event = 0;

extern WindowQueue windowQueue;
extern WindowQueue *lastWindow;

void mouseinit()
{
  outb(0x64, 0xa8);
  outb(0x64, 0xd4);
  outb(0x60, 0xf4);
  outb(0x64, 0x60);
  outb(0x60, 0x47);
  cprintf("mouse initialized.\n");
  setMousePosition(SCREEN_WIDTH / 2, SCREEN_HEIGHT / 2);
  initlock(&mouse_lock, "mouse");
  picenable(IRQ_MOUSE);
  ioapicenable(IRQ_MOUSE, 0);
}

void moveMousePosition(int x, int y)
{
  setMousePosition(mouse_info.x_position + x, mouse_info.y_position + y);
}

int inWindowRange(Window *window, int x, int y)
{
  if (x >= window->leftTopX && x < window->leftTopX + window->width && y >= window->leftTopY && y < window->leftTopY + window->height)
    return 1;
  return 0;
}

int inWidgetRange(Widget* widget, int x, int y)
{
  int leftTopX, leftTopY, width, height;
  if (widget->type == iconView)
  {
    leftTopX = widget->context.iconView->leftTopX;
    leftTopY = widget->context.iconView->leftTopY;
    width = widget->context.iconView->width;
    height = widget->context.iconView->height;
    if (x >= leftTopX && x < leftTopX + width && y >= leftTopY && y < leftTopY + height)
    {
      return 1;
    }
  }
  return 0;
}

WindowQueue* getClickedWindowQueue()
{
  WindowQueue *p = lastWindow;
  int x = mouse_info.x_position;
  int y = mouse_info.y_position;
  //get clicked window
  while (p)
  {
    if (inWindowRange(p->window, x, y))
    {
      return (p);
    }
    p = p->prev;
  }
  return 0;
}

Widget* getClickedWidget(Window* pwindow)
{
  int x = mouse_info.x_position;
  int y = mouse_info.y_position;
  int i;
  for (i = 0; i < pwindow->widgetsNum; i++)
  {
    if (inWidgetRange(pwindow->widgets + i, x, y))
    {
      return(pwindow->widgets + i);
    }
  }
  return 0;
}

void handleLeftClick()
{
  WindowQueue *pwindowQueue;
  Widget *pwidget;
  pwindowQueue = getClickedWindowQueue();
  if (pwindowQueue)
  {
    pwidget = getClickedWidget(pwindowQueue->window);
    if (pwidget)
    {
      cprintf("hit.\n");
    }
  }
}

void handleLeftDoubleClick()
{
  WindowQueue *pwindowQueue;
  Widget *pwidget;
  pwindowQueue = getClickedWindowQueue();
  if (pwindowQueue)
  {
    pwidget = getClickedWidget(pwindowQueue->window);
    if (pwidget)
      switch (pwidget->type)
      {
      case iconView:
cprintf("start\n");
        cli();
        switchuvm(pwindowQueue->proc);
        pwidget->context.iconView->onDoubleClick();
        if (proc == 0)
          switchkvm();
        else
          switchuvm(proc);
        sti();
cprintf("end\n");
        break;
      default:
        break;
      }
  }
}

void handleRightClick()
{
  WindowQueue *pwindowQueue;
  Widget *pwidget;
  pwindowQueue = getClickedWindowQueue();
  if (pwindowQueue)
  {
    pwidget = getClickedWidget(pwindowQueue->window);
    if (pwidget)
    {
      cprintf("show menu.\n");
    }
  }
}

void setMousePosition(int x, int y)
{
  if (x < 0)
    x = 0;
  if (x > SCREEN_WIDTH)
    x = SCREEN_WIDTH;
  if (y < 0)
    y = 0;
  if (y > SCREEN_HEIGHT)
    y = SCREEN_HEIGHT;
  mouse_info.x_position = x;
  mouse_info.y_position = y;
  updateMouse();
}

void updateMouseEvent(uint tick)
{
  //cprintf("tick = %d\n", tick);
  event = 0;
  if (left_down)
  {
    if (!left_btn_down)
    {
      left_btn_down = 1;
    }
    else
    {
      event = MOUSE_DRAGGING;
    }
  }
  else
  {
    if (left_btn_down)
    {
      left_btn_down = 0;
      if (tick - last_tick <= 20)
        event = LEFT_DOUBLE_CLICK;
      else
      {
        if (mouse_info.event == MOUSE_DRAGGING)
          event = 0;
        else
          event = LEFT_CLICK;
      }
      if (event == LEFT_DOUBLE_CLICK)
        last_tick = -20;
      else
        last_tick = tick;
    }
  }
  if (right_down)
  {
    if (!right_btn_down)
      right_btn_down = 1;
  }
  else
  {
    if (right_btn_down)
    {
      right_btn_down = 0;
      event = RIGHT_CLICK;
    }
  }
  mouse_info.event = event;
  if (event == LEFT_CLICK)
  {
    handleLeftClick();
    //cprintf("LEFT_CLICK\n");
  }
  if (event == LEFT_DOUBLE_CLICK)
  {
    handleLeftDoubleClick();
    //cprintf("LEFT_DOUBLE_CLICK\n");
  }
  // if (event == MOUSE_DRAGGING)
  //   cprintf("MOUSE_DRAGGING\n");
  if (event == RIGHT_CLICK)
  {
    handleRightClick();
    //cprintf("RIGHT_CLICK\n");
  }
}

void mouseint(uint tick)
{
  uint ch;

  ch = inb(0x64);
  if ((ch & 0x01) == 0)
  {
    //cprintf("no data\n");
    to_read = READ_MOUSE_INFO;
    return;
  }

  acquire(&mouse_lock);
  ch = inb(0x60);
  //cprintf("ch=%d\n", ch);
  if (to_read == READ_MOUSE_INFO)
  {
    if ((ch & 0x08) == 0 || (ch & 0x04) != 0)
    {
       cprintf("Error\n");
      	release(&mouse_lock);
      	return;
    }
    //cprintf("Counter: %d\n", counter++);
    left_down = (ch & 0x01) ? 1 : 0;
    right_down = (ch & 0x02) ? 1 : 0;
    x_sign = (ch & 0x10) ? 1 : 0;
    y_sign = (ch & 0x20) ? 1 : 0;
    x_overflow = (ch & 0x40) ? 1 : 0;
    y_overflow = (ch & 0x80) ? 1 : 0;

    //cprintf("mouse_down: left_down=%d, right_down=%d\n", left_down, right_down);
    //cprintf("move direction: x_sign=%d, y_sign=%d\n", x_sign, y_sign);
    //cprintf("overflow: x_overflow = %d, y_overflow = %d\n",x_overflow,  y_overflow);
    to_read = READ_X_MOVEMENT;
    release(&mouse_lock);
    return;
  }
  else if (to_read == READ_X_MOVEMENT)
  {
    dis_x = ch;
    if (x_sign == 1)
      	dis_x = ch - 256;
    	//cprintf("x_movement: %d\n", dis);
    	to_read = READ_Y_MOVEMENT;
    	release(&mouse_lock);
    	//moveMousePosition(dis, 0);
    	return;
  }
  else if (to_read == READ_Y_MOVEMENT)
  {
    dis_y = ch;
    if (y_sign == 1)
      dis_y = ch - 256;
    dis_y = - dis_y;
    //cprintf("y_movement: %d\n", dis);
    to_read = READ_MOUSE_INFO;
    release(&mouse_lock);
    //cprintf("x: %d, y: %d\n", mouse_info.x_position, mouse_info.y_position);
    updateMouseEvent(tick);
    moveMousePosition(dis_x, dis_y);
  }
}
