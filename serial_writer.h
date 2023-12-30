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
        serialPort->setBaudRate(250000);
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
            qDebug() << "RECIEVED SOME DATA: " << data;
        });
 }

public Q_SLOTS:

    void write(const QVector<Color> &colors) {
        // remove each 2nd color to reduce data size
//        QVector<Color> reducedColors;
//        for (int i = 0; i < colors.size(); i++) {
//            if (i % 2 == 0) {
//                reducedColors.append(colors[i]);
//            }
//        }

        auto data = Color::toByteArray(colors);

        // if "Ada" is contained among data, print ADA IN DATA to stdout
        bool ada = false;
        for (int i; i < data.size() - 2; i++) {
            if (data[i] == 'A' && data[i + 1] == 'd' && data [i + 2] == 'a') {
                ada = true;
                break;
            }
        }

        if (ada) {
            qDebug() << "ADDDDDDDDAAAAAAAA DSJKFHSDJKFHKJDSHFKJSDHF";
        }


        serialPort->write("Ada");

//        qDebug() << "Sending array where first color is " << colors.first() << " and last color is " << colors.last();
        serialPort->write(data);
        serialPort->flush();
//
//        if (!serialPort->waitForBytesWritten(1000)) {
//            throw std::runtime_error("Write error");
//        }

        sendCount++;
    }

private:
    QSerialPort *serialPort;

    size_t sendCount = 0;

    Color firstColor;
    Color lastColor;
};

#endif //AMBILIGHT_SERIAL_WRITER_H
