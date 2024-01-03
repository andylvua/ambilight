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
#include "constants.h"


class SerialWriter : public QObject {
Q_OBJECT

public:
    explicit SerialWriter() {
        serialPort = new QSerialPort();

        auto availablePort = QSerialPortInfo::availablePorts().first();

        serialPort->setPortName(availablePort.portName());
        serialPort->setBaudRate(Constants::Serial::BAUD_RATE);
        serialPort->setDataBits(QSerialPort::Data8);
        serialPort->setParity(QSerialPort::NoParity);
        serialPort->setStopBits(QSerialPort::OneStop);
        serialPort->setFlowControl(QSerialPort::NoFlowControl);

        qDebug() << "Opening serial port";
        if (serialPort->open(QIODevice::ReadWrite)) {
            QByteArray data;
            if (serialPort->waitForReadyRead(Constants::Serial::INIT_TIMEOUT)) {
                data = serialPort->readAll();
                while (serialPort->waitForReadyRead(Constants::Serial::READ_TIMEOUT)) {
                    data += serialPort->readAll();
                }

                if (data == Constants::Serial::INIT_MESSAGE) {
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
            qDebug() << "RECIEVED DATA: " << data;
        });
    }

    void write(const QVector<Color> &colors) {
        auto data = Color::toByteArray(colors);

        serialPort->write(Constants::Serial::RECEIVE_MESSAGE);

        uint16_t count = colors.size();
        uint8_t highByte = count >> 8;
        uint8_t lowByte = count & 0xFF;
        uint8_t checksum = highByte ^ lowByte ^ Constants::Serial::CHECKSUM_MAGIC;

        serialPort->write(reinterpret_cast<const char *>(&highByte), 1);
        serialPort->write(reinterpret_cast<const char *>(&lowByte), 1);
        serialPort->write(reinterpret_cast<const char *>(&checksum), 1);

        serialPort->write(data);
        serialPort->flush();
    }

private:
    QSerialPort *serialPort;
};

#endif //AMBILIGHT_SERIAL_WRITER_H
