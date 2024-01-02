#include <QApplication>
#include <QSettings>
#include <QStyleFactory>

#include "displayer.h"
#include "tray_app.h"
#include "capturer.h"
#include "capturer_config.h"
#include "xdisplay.h"


int main(int argc, char *argv[]) {
    Display *display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(display);

    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
    int width = gwa.width;
    int height = gwa.height;

    QApplication app(argc, argv);
    QSettings settings("../config.ini", QSettings::IniFormat);

    AppSettings appSettings(settings);
    appSettings.enableGUI = false;

    XDisplay xDisplay = {
            .display = display,
            .root = root,
            .width = width,
            .height = height
    };

    Displayer displayer(appSettings, xDisplay);

    Capturer capturer(appSettings, xDisplay, displayer);

    TrayApp trayApp(appSettings, capturer);

    auto *capturerThread = new QThread();
    capturer.moveToThread(capturerThread);

    QObject::connect(capturerThread, &QThread::started, &trayApp, &TrayApp::start);
    QObject::connect(capturerThread, &QThread::started, &capturer, &Capturer::start);

    capturerThread->start();

//    displayer.show();

    return QApplication::exec();
}

