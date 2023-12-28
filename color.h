//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_COLOR_H
#define AMBILIGHT_COLOR_H

#include <iostream>

#include <QtCore/QDebug>
#include <QtCore/QByteArray>
#include <QtGui/QColor>


struct Color {
    int red;
    int green;
    int blue;

    Color(int red, int green, int blue) {
        this->red = red;
        this->green = green;
        this->blue = blue;
    }

    Color(const QColor &color) {
        this->red = color.red();
        this->green = color.green();
        this->blue = color.blue();
    }

    Color() = default;

    Color operator+(const Color &other) const {
        return Color{
                red + other.red,
                green + other.green,
                blue + other.blue,
        };
    }

    Color operator-(const Color &other) const {
        return Color{
                red - other.red,
                green - other.green,
                blue - other.blue,
        };
    }

    Color operator*(const double coef) const {
        return Color{
                static_cast<int>(red * coef),
                static_cast<int>(green * coef),
                static_cast<int>(blue * coef),
        };
    }

    static QByteArray toByteArray(const QVector<Color> &colors) {
        QByteArray data;
        for (const Color &color: colors) {
            data.append(static_cast<char>(color.red));
            data.append(static_cast<char>(color.green));
            data.append(static_cast<char>(color.blue));
        }

        return data;
    }

    static QVector<Color> toColors(const Color &color, qsizetype size) {
        QVector<Color> colors(size);
        colors.fill(color);
        return colors;
    }

//    static QVector<Color> toColors(const QColor &color, qsizetype size) {
//        QVector<Color> colors(size);
//        colors.fill(Color(color));
//        return colors;
//    }

    friend QDebug operator<<(QDebug debug, const Color &color) {
        QDebugStateSaver saver(debug);
        debug.nospace() << "Color(" << color.red << ", " << color.green << ", " << color.blue << ")";
        return debug;
    }
};

#endif //AMBILIGHT_COLOR_H
