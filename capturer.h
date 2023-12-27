//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_CAPTURER_H
#define AMBILIGHT_CAPTURER_H

#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtCore/QVector>
#include <QtCore/QThreadPool>
#include <QtCore/QFuture>
#include <QtConcurrent/QtConcurrent>
#include <QtCore/QElapsedTimer>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtSerialPort/QSerialPort>

#include <vector>
#include <iostream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include "capturer_config.h"
#include "color.h"
#include "displayer.h"
#include "serial_writer.h"


class Capturer : public QObject {
Q_OBJECT

public:
    explicit Capturer(CapturerConfig &config, Display *display, Window root) : serialWriter() {
        this->config = config;
        this->display = display;
        this->root = root;

        initZoneProperties();

        previousColors = QVector<Color>(config.ledX * 2 + config.ledY * 2);
        colors = QVector<Color>(config.ledX * 2 + config.ledY * 2);

        qDebug() << "Enter this number of LEDs in the Arduino sketch: " << config.ledX * 2 + config.ledY * 2;

        QThreadPool::globalInstance()->setMaxThreadCount(4);

        // connect sendColors signal to serialWriter write slot
        connect(this, &Capturer::sendColors, &serialWriter, &SerialWriter::write);
    }

    void initZoneProperties() {
        zoneConfig.widthY = zoneConfig.widthX = config.screenWidth / (config.ledX + 2);
        zoneConfig.widthY += config.zoneOffsetX;
        zoneConfig.heightX = zoneConfig.heightY = config.screenHeight / (config.ledY + 2);
        zoneConfig.heightX += config.zoneOffsetY;

        zoneConfig.rowOffset = (config.screenWidth % (config.ledX + 2)) / 2;
        zoneConfig.colOffset = (config.screenHeight % (config.ledY + 2)) / 2;
    }

    Color getAverageColor(XImage *image, int x, int y, int width, int height) const {
        int red = 0;
        int green = 0;
        int blue = 0;

        int counter = 0;
        int skip = width * height / config.pixelsPerZone;
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
        res.reserve(config.ledX * 2 + config.ledY * 2);

        for (int i = 0; i < config.ledX; i++) {
            auto color = getAverageColor(screenImage.bottomPanel,
                                         (i + 1) * zoneConfig.widthX + zoneConfig.rowOffset,
                                         0,
                                         zoneConfig.widthX,
                                         zoneConfig.heightX);
            res.push_back(color);
        }
        for (int i = config.ledY - 1; i >= 0; i--) {
            auto color = getAverageColor(screenImage.rightPanel,
                                         0,
                                         (i + 1) * zoneConfig.heightY + zoneConfig.colOffset,
                                         zoneConfig.widthY,
                                         zoneConfig.heightY);
            res.push_back(color);
        }
        for (int i = config.ledX - 1; i >= 0; i--) {
            auto color = getAverageColor(screenImage.topPanel,
                                         (i + 1) * zoneConfig.widthX + zoneConfig.rowOffset,
                                         0,
                                         zoneConfig.widthX,
                                         zoneConfig.heightX);
            res.push_back(color);
        }
        for (int i = 0; i < config.ledY; i++) {
            auto color = getAverageColor(screenImage.leftPanel,
                                         0,
                                         (i + 1) * zoneConfig.heightY + zoneConfig.colOffset,
                                         zoneConfig.widthY,
                                         zoneConfig.heightY);
            res.push_back(color);
        }

        return res;
    }

    void interpolateColors() {
        QVector<Color> interpolatedColors;
        interpolatedColors.resize(config.ledX * 2 + config.ledY * 2);

        auto totalElapsed = QElapsedTimer();
        totalElapsed.start();

        auto totalFramesToInterpolate = config.targetFPS - config.captureFPS;
        auto framesToInterpolate = totalFramesToInterpolate / config.captureFPS;
        auto framesToRender = framesToInterpolate + 1;
        auto captureTimeMs = 1000 / config.captureFPS;
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
        QVector<Color> convolvedColors;
        convolvedColors.resize(_colors.size());

        for (auto i = 0; i < _colors.size(); i++) {
            Color convolvedColor = {0, 0, 0};
            for (auto j = 0; j < config.convolveSize; j++) {
                auto index = i + j - config.convolveSize / 2;
                if (index < 0) {
                    index = 0;
                } else if (index >= _colors.size()) {
                    index = _colors.size() - 1;
                }
                convolvedColor = convolvedColor + _colors[index];
            }
            convolvedColors[i] = convolvedColor * (1.0 / config.convolveSize);
        }

        return convolvedColors;
    }

    void captureImages() {
        auto leftPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, 0, zoneConfig.heightY, zoneConfig.widthY,
                             config.screenHeight - zoneConfig.heightY, AllPlanes, ZPixmap);
        });
        auto rightPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, config.screenWidth - zoneConfig.widthY, zoneConfig.heightY,
                             zoneConfig.widthY, config.screenHeight - zoneConfig.heightY, AllPlanes, ZPixmap);
        });
        auto topPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, zoneConfig.widthX, 0, config.screenWidth - zoneConfig.widthX,
                             zoneConfig.heightX, AllPlanes, ZPixmap);
        });
        auto bottomPanelFuture = QtConcurrent::run([this] {
            return XGetImage(display, root, zoneConfig.widthX, config.screenHeight - zoneConfig.heightX,
                             config.screenWidth - zoneConfig.widthX, zoneConfig.heightX, AllPlanes, ZPixmap);
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
        captureTimer = new QTimer(this);

        connect(captureTimer, &QTimer::timeout, this, &Capturer::capture);
        captureTimer->setInterval(1000 / config.captureFPS);
        captureTimer->start();

        capture();
    }

    void restart() {
        if (staticColorEnabled) {
            auto staticColors = QVector<Color>(config.ledX * 2 + config.ledY * 2);
            staticColors.fill(Color(staticColor.red(), staticColor.green(), staticColor.blue()));
            animateColorChange(staticColors);
            return;
        }
        captureTimer->start();

        capture();
    }

    void stop(bool turnOff = true) {
        captureTimer->stop();
        if (turnOff) {
            this->turnOff();
        }
    }

    void turnOff() {
        auto staticColors = QVector<Color>(config.ledX * 2 + config.ledY * 2);
        staticColors.fill(Color(0, 0, 0));
        animateColorChange(staticColors);
    }

    void setBrightness(int value) {
        colorConfig.brightness = value;

        renderColors();
    }

    void animateColorChange(QVector<Color> &_colors) {
        previousColors = colors;
        colors = _colors;
        interpolateColors();
    }

    void renderColors() {
        static int changeColorCounter = 0;
        static Color firstColor = {45, 65, 112};
        static Color secondColor = {75, 213, 50};

        auto _colors = colors;

        auto brightness = colorConfig.brightness / 100.0;
        for (auto &color: _colors) {
            color = color * brightness;
        }

        if (config.GUI) {
            Displayer::getInstance()->display(_colors);
        } else {
            QVector<Color> testColors = QVector<Color>(config.ledX * 2 + config.ledY * 2);
            if (changeColorCounter % 300 == 0) {
                // assign random colors
                firstColor = {rand() % 255, rand() % 255, rand() % 255};
                secondColor = {rand() % 255, rand() % 255, rand() % 255};
            }
            testColors.first() = firstColor;
            testColors.last() = secondColor;
            emit sendColors(testColors);
            changeColorCounter++;
        }
    }

    void setStaticColor(bool enabled, QColor color = QColor()) {
        if (enabled) {
            staticColorEnabled = true;
            staticColor = color;
            stop(false);
            setStaticColor(color);
        } else {
            staticColorEnabled = false;
            restart();
        }
    }

    void setStaticColor(QColor color) {
        staticColor = color;

        auto staticColors = QVector<Color>(config.ledX * 2 + config.ledY * 2);
        staticColors.fill(Color(color.red(), color.green(), color.blue()));

        if (config.GUI) {
            animateColorChange(staticColors);
        } else {
            emit sendColors(staticColors);
        }
    }

private slots:

    void capture() {
        destroyImages();
        captureImages();

        previousColors = colors;

        colors = convolveColors(getColors());
        interpolateColors();
    }


signals:

    void sendColors(QVector<Color> colors);

private:
    CapturerConfig config{};

    Display *display = nullptr;
    Window root = 0;

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

    struct ColorConfig {
        int brightness = 100;
    } colorConfig;

    QTimer *captureTimer;

    QVector<Color> previousColors;
    QVector<Color> colors;

    QColor staticColor;
    bool staticColorEnabled = false;

    SerialWriter serialWriter;
};

#endif //AMBILIGHT_CAPTURER_H
