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

enum Effects {
    PACIFICA = 1,
};

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

    void writeColors(const QVector<Color> &colors) {
        auto data = Color::toByteArray(colors);

        uint8_t count = colors.size();
        uint8_t opcode = 1;

        writeHeader({count, opcode});

        serialPort->write(data);
        serialPort->flush();
    }

    void writeEffect(Effects effect) {
        uint8_t count = 0;
        uint8_t opcode = 2;

        QByteArray data;
        int retries = 0;

        disconnect(serialPort, &QSerialPort::readyRead, nullptr, nullptr);
        while (retries < Constants::Serial::SEND_RETRIES) {
            writeHeader({count, opcode});

            if (serialPort->waitForReadyRead(Constants::Serial::READ_TIMEOUT)) {
                data = serialPort->readAll();
                while (serialPort->waitForReadyRead(Constants::Serial::READ_TIMEOUT)) {
                    data += serialPort->readAll();
                }

                if (data == Constants::Serial::EFFECT_MESSAGE) {
                    break;
                } else {
                    retries++;
                }
            } else {
                retries++;
            }
        }
        connect(serialPort, &QSerialPort::readyRead, [&]() {
            QByteArray data = serialPort->readAll();
            qDebug() << "RECIEVED DATA: " << data;
        });

        if (retries == Constants::Serial::SEND_RETRIES) {
            qDebug() << "Failed to send effect";
            return;
        } else {
            qDebug() << "Effect sent";
        }

        serialPort->write(reinterpret_cast<const char *>(&effect), 1);
        serialPort->flush();
    }

private:
    QSerialPort *serialPort;

    struct SendHeader {
        uint8_t count;
        uint8_t opcode;
        uint8_t checksum;

        SendHeader(uint8_t count, uint8_t opcode) : count(count), opcode(opcode) {
            checksum = count ^ opcode ^ Constants::Serial::CHECKSUM_MAGIC;
        }
    };

    void writeHeader(SendHeader header) {
        serialPort->write(Constants::Serial::RECEIVE_MESSAGE);
        serialPort->write(reinterpret_cast<const char *>(&header), 3);
        serialPort->flush();
    }
};

#endif //AMBILIGHT_SERIAL_WRITER_H
