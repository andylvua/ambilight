//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_DISPLAYER_H
#define AMBILIGHT_DISPLAYER_H

#include <QtWidgets/QWidget>
#include <QtGui/QPainter>
#include <QtCore/QVector>

#include "color.h"
#include "capturer_config.h"
#include "app_settings.h"
#include "xdisplay.h"


class Displayer : public QWidget {

public:

    Displayer(AppSettings &settings, const XDisplay &display) : settings(settings), display(display) {
        setWindowTitle("Ambilight");
        setWindowIcon(QIcon("../ambilight.png"));

        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_NoSystemBackground);
        setAttribute(Qt::WA_OpaquePaintEvent);

        setStyleSheet("background:transparent;");
    }

    void render() {
        repaint();
    }

public Q_SLOTS:

    void setColors(QVector<Color> &_colors) {
        this->colors = _colors;
        render();
    }


protected:
    void paintEvent(QPaintEvent *event) override {
        QPainter painter(this);
        int width = this->width();
        int height = this->height();
        double scaleX = static_cast<double>(display.width) / width;
        double scaleY = static_cast<double>(display.height) / height;

        auto &config = settings.capturerConfig;

        int xLedWidth = width / (config.ledX + 2);
        int yLedHeight = height / (config.ledY + 2);
        int xLedHeight =
                config.zoneOffsetY == 0 ? yLedHeight : yLedHeight + static_cast<int>(config.zoneOffsetY / scaleY);
        int yLedWidth =
                config.zoneOffsetX == 0 ? xLedWidth : xLedWidth + static_cast<int>(config.zoneOffsetX / scaleX);
        int ledX = config.ledX;
        int ledY = config.ledY;
        int ledNumber = 0;

        auto drawLed = [&](int x, int y, int width, int height) {
            auto color = colors[ledNumber];
            painter.fillRect(x, y, width, height, QColor(color.red, color.green, color.blue));
            painter.drawRect(x, y, width, height);
            painter.drawText(x, y, width, height, Qt::AlignCenter, QString::number(++ledNumber));
        };

        for (int i = 0; i < ledX; i++) {
            auto x = (i + 1) * xLedWidth;
            auto y = height - xLedHeight;
            drawLed(x, y, xLedWidth, xLedHeight);
        }
        for (int i = ledY - 1; i >= 0; i--) {
            auto x = width - yLedWidth;
            auto y = (i + 1) * yLedHeight;
            drawLed(x, y, yLedWidth, yLedHeight);
        }
        for (int i = ledX - 1; i >= 0; i--) {
            auto x = (i + 1) * xLedWidth;
            auto y = 0;
            drawLed(x, y, xLedWidth, xLedHeight);
        }
        for (int i = 0; i < ledY; i++) {
            auto x = 0;
            auto y = (i + 1) * yLedHeight;
            drawLed(x, y, yLedWidth, yLedHeight);
        }
    }

private:
    AppSettings &settings;
    XDisplay display;
    QVector<Color> colors;
};

#endif //AMBILIGHT_DISPLAYER_H
