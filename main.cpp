#include <QApplication>
#include <QSettings>
#include <QStyleFactory>

#include "displayer.h"
#include "tray_app.h"
#include "capturer.h"
#include "capturer_config.h"

#include <X11/Xlib.h>

Displayer *Displayer::singleton = nullptr;


int main(int argc, char *argv[]) {
    Display *display = XOpenDisplay(nullptr);
    Window root = DefaultRootWindow(display);

    XWindowAttributes gwa;
    XGetWindowAttributes(display, root, &gwa);
    int width = gwa.width;
    int height = gwa.height;

    QApplication app(argc, argv);
    QSettings settings("../config.ini", QSettings::IniFormat);

    CapturerConfig config = {
            .ledX = settings.value("ledX", 31).toInt(),
            .ledY = settings.value("ledY", 18).toInt(),
            .screenWidth = width,
            .screenHeight = height,
            .zoneOffsetX = settings.value("zoneOffsetX", 200).toInt(),
            .zoneOffsetY = settings.value("zoneOffsetY", 80).toInt(),
            .captureFPS = settings.value("captureFPS", 5).toInt(),
            .targetFPS = settings.value("targetFPS", 60).toInt(),
            .convolveSize = settings.value("convolveSize", 5).toInt(),
            .pixelsPerZone = settings.value("pixelsPerZone", 70).toInt(),

            .GUI = false
    };

    Displayer::getInstance()->setConfig(config);

    Capturer capturer(config, display, root);

    // move capturer to a separate thread
    auto capturerThread = new QThread();

    capturer.moveToThread(capturerThread);
    QObject::connect(capturerThread, &QThread::started, &capturer, &Capturer::start);

//    Displayer::getInstance()->show();

    TrayApp trayApp(capturer);

    capturerThread->start();
    return QApplication::exec();
}

