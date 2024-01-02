//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_CAPTURER_H
#define AMBILIGHT_CAPTURER_H

#include <QtWidgets/QApplication>
#include <QtWidgets/QWidget>
#include <QtCore/QVector>
#include <QtCore/QThreadPool>
#include <QtCore/QFuture>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtSerialPort/QSerialPort>
#include <QtGui/QPainter>
#include <QtConcurrent/QtConcurrent>

// use tbb concurrent bounded
#include <tbb/concurrent_queue.h>

#include <utility>
#include <vector>
#include <iostream>

#include "capturer_config.h"
#include "color.h"
#include "displayer.h"
#include "app_settings.h"
#include "serial_writer.h"
#include "xdisplay.h"


class ColorSender : public QObject {
Q_OBJECT

public:
    explicit ColorSender(int sendRate) : sendRate(sendRate) {
        connect(this, &ColorSender::sendColors, &serialWriter, &SerialWriter::write);
    }

    void send() {
//        qDebug() << "ColorSender send in thread: " << QThread::currentThreadId();
        if (isProcessing) {
            Q_EMIT sendColors(processedColors);
            isProcessing = false;
        } else {
            timeoutCounter++;
            if (timeoutCounter >= timeoutSlots) {
                timeout = true;
                timeoutCounter = 0;
            }
        }

        if (timeout) {
            disconnect(sendTimer, &QTimer::timeout, this, &ColorSender::send);
        }
    }

public Q_SLOTS:

    void start() {
        sendTimer = new QTimer(nullptr);
        connect(sendTimer, &QTimer::timeout, this, &ColorSender::send);
        sendTimer->setInterval(1000 / sendRate);
        sendTimer->start();
    }

    void processColors(const QVector<Color> &colors) {
        if (timeout) {
            timeout = false;
            connect(sendTimer, &QTimer::timeout, this, &ColorSender::send);
        }

        processedColors = colors;
        isProcessing = true;
    }

Q_SIGNALS:

    void sendColors(QVector<Color> colors);

private:
    static constexpr auto timeoutSlots = 3;

    QTimer *sendTimer;
    SerialWriter serialWriter;

    QVector<Color> processedColors;

    bool isProcessing = false;
    bool timeout = false;
    int timeoutCounter = 0;

    int sendRate;
};

class Capturer : public QObject {
Q_OBJECT

public:
    Capturer(AppSettings &settings, XDisplay &display, Displayer &displayer) :
            settings(settings), display(display), displayer(displayer), colorSender(settings.capturerConfig.targetFPS) {
        initZoneProperties();

        previousColors = QVector<Color>(colorsSize());
        colors = QVector<Color>(colorsSize());

        qDebug() << "Enter this number of LEDs in the Arduino sketch: " << colorsSize();

        QThreadPool::globalInstance()->setMaxThreadCount(4);

        connect(this, &Capturer::sendColors, &colorSender, &ColorSender::processColors);
        connect(this, &Capturer::displayColors, &displayer, &Displayer::render);

        auto *colorSenderThread = new QThread();
        colorSender.moveToThread(colorSenderThread);

        QObject::connect(colorSenderThread, &QThread::started, &colorSender, &ColorSender::start);

        colorSenderThread->start();
    }

    void initZoneProperties() {
        auto &config = settings.capturerConfig;

        zoneConfig.widthY = zoneConfig.widthX = display.width / (config.ledX + 2);
        zoneConfig.widthY += config.zoneOffsetX;
        zoneConfig.heightX = zoneConfig.heightY = display.height / (config.ledY + 2);
        zoneConfig.heightX += config.zoneOffsetY;

        zoneConfig.rowOffset = (display.width % (config.ledX + 2)) / 2;
        zoneConfig.colOffset = (display.height % (config.ledY + 2)) / 2;
    }

    [[nodiscard]] qsizetype colorsSize() const {
        return settings.capturerConfig.ledX * 2 + settings.capturerConfig.ledY * 2;
    }

    Color getAverageColor(XImage *image, int x, int y, int width, int height) const {
        int red = 0;
        int green = 0;
        int blue = 0;

        int counter = 0;
        int skip = width * height / settings.capturerConfig.pixelsPerZone;
        for (int i = 0; i < width * height; i += skip) {
            int pixel = XGetPixel(image, x + i % width, y + i / width);
            red += pixel >> 16 & 0xFF;
            green += pixel >> 8 & 0xFF;
            blue += pixel & 0xFF;
            counter++;
        }
        red /= counter;
        green /= counter;
        blue /= counter;
        return {red, green, blue};
    }

    [[nodiscard]] QVector<Color> getColors() const {
        QVector<Color> res;
        res.reserve(colorsSize());

        for (int i = 0; i < settings.capturerConfig.ledX; i++) {
            auto color = getAverageColor(screenImage.bottomPanel,
                                         i * zoneConfig.widthX,
                                         0,
                                         zoneConfig.widthX,
                                         zoneConfig.heightX);
            res.push_back(color);
        }
        for (int i = settings.capturerConfig.ledY - 1; i >= 0; i--) {
            auto color = getAverageColor(screenImage.rightPanel,
                                         0,
                                         i * zoneConfig.heightY,
                                         zoneConfig.widthY,
                                         zoneConfig.heightY);
            res.push_back(color);
        }
        for (int i = settings.capturerConfig.ledX - 1; i >= 0; i--) {
            auto color = getAverageColor(screenImage.topPanel,
                                         i * zoneConfig.widthX,
                                         0,
                                         zoneConfig.widthX,
                                         zoneConfig.heightX);
            res.push_back(color);
        }
        for (int i = 0; i < settings.capturerConfig.ledY; i++) {
            auto color = getAverageColor(screenImage.leftPanel,
                                         0,
                                         i * zoneConfig.heightY,
                                         zoneConfig.widthY,
                                         zoneConfig.heightY);
            res.push_back(color);
        }

        return res;
    }

    void interpolateColors() {
        QVector<Color> interpolatedColors;
        interpolatedColors.resize(colors.size());

        auto totalElapsed = QElapsedTimer();
        totalElapsed.start();

        auto targetFPS = settings.capturerConfig.targetFPS;
        auto captureFPS = settings.capturerConfig.captureFPS;

        auto totalFramesToInterpolate = targetFPS - captureFPS;
        auto framesToInterpolate = totalFramesToInterpolate / captureFPS;
        auto framesToRender = framesToInterpolate + 1;
        auto captureTimeMs = 1000 / captureFPS;
        auto frameDuration = captureTimeMs / framesToRender;

        QVector<Color> initColors = colors;

        for (auto i = 0; i < framesToRender; i++) {
            for (auto j = 0; j < colors.size(); j++) {
                double interpolationRate = static_cast<double>(i) / framesToInterpolate;
                auto dColor = initColors[j] - previousColors[j];
                auto interpolatedColor = previousColors[j] + dColor * interpolationRate;
                interpolatedColors[j] = interpolatedColor;
            }

            colors = interpolatedColors;

            renderColors();

            if (totalElapsed.elapsed() >= frameDuration * framesToRender) {
                break;
            } else if (i != framesToInterpolate) {
                QThread::msleep(frameDuration - 3);
            }
        }
    }

    QVector<Color> convolveColors(QVector<Color> &&_colors) const {
        auto convolveSize = settings.capturerConfig.convolveSize;
        if (convolveSize <= 1) {
            return _colors;
        }

        QVector<Color> convolvedColors;
        convolvedColors.resize(_colors.size());

        for (auto i = 0; i < _colors.size(); i++) {
            Color convolvedColor = {0, 0, 0};
            for (auto j = 0; j < convolveSize; j++) {
                long index = i + j - convolveSize / 2;
                if (index < 0) {
                    index = 0;
                } else if (index >= _colors.size()) {
                    index = _colors.size() - 1;
                }
                convolvedColor = convolvedColor + _colors[index];
            }
            convolvedColors[i] = convolvedColor * (1.0 / convolveSize);
        }

        return convolvedColors;
    }

    void captureImages() {
        auto leftPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display.display,
                             display.root,
                             0,
                             zoneConfig.heightY + zoneConfig.colOffset,
                             zoneConfig.widthY,
                             display.height - (zoneConfig.heightY - zoneConfig.colOffset) * 2,
                             AllPlanes, ZPixmap);
        });
        auto rightPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display.display,
                             display.root,
                             display.width - zoneConfig.widthY,
                             zoneConfig.heightY + zoneConfig.colOffset,
                             zoneConfig.widthY,
                             display.height - (zoneConfig.heightY - zoneConfig.colOffset) * 2,
                             AllPlanes, ZPixmap);
        });
        auto topPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display.display,
                             display.root,
                             zoneConfig.widthX + zoneConfig.rowOffset,
                             0,
                             display.width - (zoneConfig.widthX - zoneConfig.rowOffset) * 2,
                             zoneConfig.heightX,
                             AllPlanes, ZPixmap);
        });
        auto bottomPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display.display,
                             display.root,
                             zoneConfig.widthX + zoneConfig.rowOffset,
                             display.height - zoneConfig.heightX,
                             display.width - (zoneConfig.widthX - zoneConfig.rowOffset) * 2,
                             zoneConfig.heightX,
                             AllPlanes, ZPixmap);
        });

        screenImage.leftPanel = leftPanelFuture.result();
        screenImage.rightPanel = rightPanelFuture.result();
        screenImage.topPanel = topPanelFuture.result();
        screenImage.bottomPanel = bottomPanelFuture.result();
    }

    void destroyImages() const {
        if (screenImage.leftPanel != nullptr) {
            XDestroyImage(screenImage.leftPanel);
            XDestroyImage(screenImage.rightPanel);
            XDestroyImage(screenImage.topPanel);
            XDestroyImage(screenImage.bottomPanel);
        }
    }

    void start() {
        qDebug() << "Capturer running in thread: " << QThread::currentThreadId();

        captureTimer = new QTimer(nullptr);

        connect(captureTimer, &QTimer::timeout, this, &Capturer::capture);
        captureTimer->setInterval(1000 / settings.capturerConfig.captureFPS);
        captureTimer->start();

        capture();
    }

    void restart() {
        if (settings.enableStaticColor) {
            setStaticColor();
            return;
        }
        captureTimer->start();

        capture();
    }



    void turnOff() {
        auto staticColors = Color::toColors({0, 0, 0}, colorsSize());
        animateColorChange(staticColors);
    }


    void animateColorChange(QVector<Color> &_colors) {
        previousColors = colors;
        colors = _colors;
        interpolateColors();
    }

    void renderColors() {
        auto _colors = colors;

        auto brightness = settings.brightness / 100.0;
        for (auto &color: _colors) {
            color = color * brightness;
        }

        if (settings.enableGUI) {
            displayer.setColors(_colors);
            Q_EMIT displayColors();
        } else {
            Q_EMIT sendColors(_colors);
        }
    }



private Q_SLOTS:

    void capture() {
//        qDebug() << "Capturer capture in thread: " << QThread::currentThreadId();
        destroyImages();
        captureImages();

        previousColors = colors;

        colors = convolveColors(getColors());
        interpolateColors();
    }

public Q_SLOTS:
    void stop(bool turnOff = true) {
        captureTimer->stop();

        if (turnOff) {
            this->turnOff();
        }
    }

    void enableStaticColor() {
        if (settings.enableStaticColor) {
            stop(false);
            setStaticColor();
        } else {
            restart();
        }
    }

    void setStaticColor() {
        if (!settings.enableStaticColor) {
            return;
        }

        auto staticColors = Color::toColors(settings.staticColor, colorsSize());

        animateColorChange(staticColors);
    }

    void setBrightness() {
        renderColors();
    }

Q_SIGNALS:

    void sendColors(QVector<Color> colors);

    void displayColors();

private:
    AppSettings &settings;
    Displayer &displayer;
    XDisplay display;

    struct ScreenImage {
        XImage *leftPanel = nullptr;
        XImage *rightPanel = nullptr;
        XImage *topPanel = nullptr;
        XImage *bottomPanel = nullptr;
    } screenImage;

    struct ZoneConfig {
        int widthX = 0;
        int heightY = 0;
        int widthY = 0;
        int heightX = 0;
        int rowOffset = 0;
        int colOffset = 0;
    } zoneConfig;

    QTimer *captureTimer;

    QVector<Color> previousColors;
    QVector<Color> colors;

    ColorSender colorSender;
};

#endif //AMBILIGHT_CAPTURER_H
