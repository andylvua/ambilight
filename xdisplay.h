//
// Created by andrew on 12/28/23.
//

#ifndef AMBILIGHT_XDISPLAY_H
#define AMBILIGHT_XDISPLAY_H

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#undef Bool
#undef CursorShape
#undef Expose
#undef KeyPress
#undef KeyRelease
#undef FocusIn
#undef FocusOut
#undef FontChange
#undef None
#undef Status
#undef Unsorted
#undef Always
#undef RevertToNone


struct XDisplay {
    Display *display;
    Window root;
    int width;
    int height;
};

#endif //AMBILIGHT_XDISPLAY_H
