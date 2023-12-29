#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xinerama.h>  // multi-monitor config
#include <stdlib.h>
#include <stdio.h> 
#include <unistd.h> // console command

//macro
#define MODKEYMASK Mod4Mask

//struct
typedef struct WindowNode WindowNode;
typedef struct MonitorNode MonitorNode;
struct WindowNode{
  int x,y;
  int width,height;
  int mode; // 0 or 1
  Window *window;
  WindowNode *next;
};
struct MonitorNode{
  int x,y;
  int width, height;
  WindowNode *windows;
  MonitorNode *next;
};

//variable
static Display* display;
static Window root;
static MonitorNode *monitors;

//function declaration
void setup();
void monitorsetup();
void destroynotify(XEvent*);
void maprequest(XEvent*);
void buttonpress(XEvent*);
void keypress(XEvent*);

//function definition
void setup() {
  XSelectInput(display, DefaultRootWindow(display), SubstructureRedirectMask); 
  monitorsetup();
}

void monitorsetup() {
  int numofscreens;
  XineramaScreenInfo *info = XineramaQueryScreens(display, &numofscreens);
  monitors = malloc(sizeof(MonitorNode));
  MonitorNode *current = monitors;
  
  for (unsigned i=0; i<numofscreens; i++) {
    current->x = info[i].x_org;
    current->y = info[i].y_org;
    current->width = info[i].width;
    current->height = info[i].height;
    current->next = (i==numofscreens-1)?NULL:malloc(sizeof(MonitorNode));
    current=current->next;
  }
}

void manage(WindowNode *w) {
  w->mode = 0;  //tile mode
}


void maprequest(XEvent *e) {
  XWindowAttributes wa;
  XMapRequestEvent re = e->xmaprequest;
  XGetWindowAttributes(display, re.window, &wa);
  WindowNode *w = malloc(sizeof(WindowNode));
  WindowNode *current;
  w->window = &re.window;
  if (re.window != XDefaultRootWindow(display)) {
    manage(w);
    XGrabButton(display, 1, Mod4Mask, re.window, 1, ButtonPressMask, 1, 1, None, None);
    XMapWindow(display, re.window);
    XSetInputFocus(display, re.window, RevertToParent, CurrentTime);
  }
}
void configurerequest(XEvent *e) {
  XConfigureRequestEvent er = e->xconfigurerequest;
  XWindowChanges changes;
  changes.x = er.x;
  changes.y = er.y;
  changes.width = er.width;
  changes.height = er.height;
  changes.border_width = er.border_width;
  changes.sibling = er.above;
  changes.stack_mode = er.detail;
  XConfigureWindow(display, er.window, er.value_mask, &changes);
}
void buttonpress(XEvent *e) {
  XButtonEvent be = e->xbutton;
  XDefineCursor(display, be.window, XCreateFontCursor(display, 1));
  XGrabPointer(display, be.window, True, ButtonReleaseMask|PointerMotionMask, 1, 1, None, None, CurrentTime);
}
void buttonrelease(XEvent *e) {
  XButtonEvent be = e->xbutton;
  XDefineCursor(display, be.window, XCreateFontCursor(display, 0));
  XUngrabPointer(display, CurrentTime);
}
void motionnotify(XEvent *e) {
  XMotionEvent me = e->xmotion;
  XWindowAttributes attr;
  XGetWindowAttributes(display, me.window, &attr);
  XRaiseWindow(display, me.window);
  XSetInputFocus(display, me.window, RevertToParent, CurrentTime);
  XMoveWindow(display, me.window, me.x_root-attr.width/2, me.y_root-attr.height/2);
} 


static void(*handler[LASTEvent])(XEvent *e) = {
  [ButtonPress] = buttonpress,
  [ButtonRelease] = buttonrelease,
  [MapRequest] = maprequest,
  [MotionNotify] = motionnotify,
  [ConfigureRequest] = configurerequest
};
void run() {
  XEvent e;
  // XSync(display, False);
  while (!XNextEvent(display, &e)) {
      if(handler[e.type])
        handler[e.type](&e);
  }
}
int main(int argc, char **argv)
{
  if(!(display = XOpenDisplay(NULL))) exit(0);
  setup();
  run();
  XCloseDisplay(display);
  return 0;
}
