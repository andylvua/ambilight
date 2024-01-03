#include <QApplication>
#include <QSettings>

#include "displayer.h"
#include "tray_app.h"
#include "xdisplay.h"
#include "ambilight.h"

TrayApp *TrayApp::instance = nullptr;

namespace {
    XDisplay getDisplay() {
        Display *display = XOpenDisplay(nullptr);
        Window root = DefaultRootWindow(display);

        XWindowAttributes gwa;
        XGetWindowAttributes(display, root, &gwa);
        int width = gwa.width;
        int height = gwa.height;

        return {
                .display = display,
                .root = root,
                .width = width,
                .height = height
        };
    }
}
int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    AppSettings appSettings(QSettings("../config.ini", QSettings::IniFormat));
    appSettings.enableGUI = false;

    Ambilight ambilight(appSettings, getDisplay());
    return Ambilight::run();
}
