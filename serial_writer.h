//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_SERIAL_WRITER_H
#define AMBILIGHT_SERIAL_WRITER_H

#include <QtSerialPort/QSerialPort>
#include <QtSerialPort/QSerialPortInfo>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QDebug>

#include "color.h"

class SerialWriter : public QObject {
Q_OBJECT

public:
    explicit SerialWriter() {
        serialPort = new QSerialPort();

        auto availablePort = QSerialPortInfo::availablePorts().first();

        serialPort->setPortName(availablePort.portName());
        serialPort->setBaudRate(250'000);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

        qDebug() << "Opening serial port";
        if (serialPort->open(QIODevice::ReadWrite)) {
            QByteArray data;
            if (serialPort->waitForReadyRead(5000)) {
                data = serialPort->readAll();
                while (serialPort->waitForReadyRead(100)) {
                    data += serialPort->readAll();
                }

                if (data == "al_ready") {
                    qDebug() << "Arduino is ready to receive data";
                } else {
                    qDebug() << "Received unexpected data:" << data;
                    throw std::runtime_error("Received unexpected data");
                }
            } else {
                throw std::runtime_error("Serial port timeout");
            }
        } else {
            qDebug() << "Error opening serial port: " << serialPort->errorString();
        }

        connect(serialPort, &QSerialPort::readyRead, [&]() {
            QByteArray data = serialPort->readAll();
            data.chop(2);
            qDebug() << "Sent" << sendCount << "times, received" << data;
            qDebug() << "First color sent:" << firstColor << ", last color sent:" << lastColor;
            sendCount = 0;
        });
 }

public slots:

    void write(const QVector<Color> &colors) {
        auto data = Color::toByteArray(colors);

        firstColor = colors.first();
        lastColor = colors.last();

        serialPort->write(data);
//        serialPort->flush();

        if (!serialPort->waitForBytesWritten(1000)) {
            throw std::runtime_error("Write error");
        }

        sendCount++;
    }

private:
    QSerialPort *serialPort;

    size_t sendCount = 0;

    Color firstColor;
    Color lastColor;
};

#endif //AMBILIGHT_SERIAL_WRITER_H
