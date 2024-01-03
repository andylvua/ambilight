//
// Created by andrew on 1/3/24.
//

#ifndef AMBILIGHT_SENDER_H
#define AMBILIGHT_SENDER_H

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QDebug>

#include "serial_writer.h"
#include "color.h"
#include "constants.h"


class ColorSender : public QObject {
Q_OBJECT

public:
    explicit ColorSender(int sendRate) : sendRate(sendRate) {}

    void send() {
        if (isProcessing) {
            serialWriter->writeColors(processedColors);
            isProcessing = false;
        } else {
            timeoutCounter++;
            if (timeoutCounter >= Constants::Sender::TIMEOUT_SLOTS) {
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
        serialWriter = new SerialWriter();
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

private:
    QTimer *sendTimer = nullptr;
    SerialWriter *serialWriter = nullptr;

    QVector<Color> processedColors;

    bool isProcessing = false;
    bool timeout = false;
    int timeoutCounter = 0;

    int sendRate;
};

#endif //AMBILIGHT_SENDER_H
