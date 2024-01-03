//
// Created by andrew on 1/3/24.
//

#ifndef AMBILIGHT_AMBILIGHT_H
#define AMBILIGHT_AMBILIGHT_H

#include <QtCore/QThread>
#include <QtWidgets/QApplication>


#include "app_settings.h"
#include "xdisplay.h"
#include "capturer.h"
#include "tray_app.h"
#include "displayer.h"


class Ambilight {

public:

    Ambilight(AppSettings &settings, const XDisplay &display) :
            settings(settings),
            display(display),
            capturer(settings, display),
            displayer(settings, display),
            trayApp(settings),
            colorSender(settings.capturerConfig.targetFPS) {
        performConnects();

        auto *capturerThread = new QThread();
        capturerThread->setObjectName("CapturerThread");
        capturer.moveToThread(capturerThread);

        auto *senderThread = new QThread();
        senderThread->setObjectName("SenderThread");
        colorSender.moveToThread(senderThread);

        QObject::connect(senderThread, &QThread::started, &colorSender, &ColorSender::start);
        senderThread->start();

        QObject::connect(capturerThread, &QThread::started, &trayApp, &TrayApp::start);
        QObject::connect(capturerThread, &QThread::started, &capturer, &Capturer::start);
        capturerThread->start(); // should start after sender is ready (emit started from sender)

    }

    void performConnects() {
        /* Capturer */
        // TrayApp -> Capturer
        QObject::connect(&trayApp, &TrayApp::startCapturer, &capturer, &Capturer::start);
        QObject::connect(&trayApp, SIGNAL(stopCapturer()), &capturer, SLOT(stop()));
        QObject::connect(&trayApp, &TrayApp::staticColorChanged, &capturer, &Capturer::setStaticColor);
        QObject::connect(&trayApp, &TrayApp::staticColorSwitched, &capturer, &Capturer::enableStaticColor);
        QObject::connect(&trayApp, &TrayApp::brightnessChanged, &capturer, &Capturer::setBrightness);
        QObject::connect(&trayApp, &TrayApp::aspectRatioChanged, &capturer, &Capturer::setAspectRatio);

        // Capturer -> ColorSender
        if (settings.enableGUI) {
            QObject::connect(&capturer, &Capturer::colorsReady, &displayer, &Displayer::setColors,
                             Qt::DirectConnection);
        } else {
            QObject::connect(&capturer, &Capturer::colorsReady, &colorSender, &ColorSender::processColors,
                             Qt::DirectConnection);
        }
    }

    static int run() {
        QApplication::setQuitOnLastWindowClosed(false);
        return QApplication::exec();
    }

private:
    AppSettings settings;
    XDisplay display;
    Displayer displayer;
    ColorSender colorSender;

    Capturer capturer;
    TrayApp trayApp;
};

#endif //AMBILIGHT_AMBILIGHT_H
