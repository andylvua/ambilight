//
// Created by andrew on 12/28/23.
//

#ifndef AMBILIGHT_APP_SETTINGS_H
#define AMBILIGHT_APP_SETTINGS_H

#include <QtCore/QSettings>

#include "capturer_config.h"
#include "color.h"

enum AspectRatioEnum {
    STANDARD,
    WIDE_21_9,
    WIDE_24_10,
    AUTO,
};

struct AspectRatio {
    AspectRatioEnum value;

    explicit AspectRatio(AspectRatioEnum value) : value(value) {}

    [[nodiscard]] constexpr double getRatio() const {
        switch (value) {
            case STANDARD:
                return 16.0 / 9.0;
            case WIDE_21_9:
                return 21.0 / 9.0;
            case WIDE_24_10:
                return 24.0 / 10.0;
            case AUTO:
                break;
        }

        return 0;
    }

    void setAspectRatio(AspectRatioEnum _value) {
        this->value = _value;
    }
};


class AppSettings {
public:
    explicit AppSettings(const QSettings &settings) {
        capturerConfig = {
                .ledX = settings.value("ledX", 31).toInt(),
                .ledY = settings.value("ledY", 18).toInt(),
                .zoneOffsetX = settings.value("zoneOffsetX", 200).toInt(),
                .zoneOffsetY = settings.value("zoneOffsetY", 80).toInt(),
                .captureFPS = settings.value("captureFPS", 5).toInt(),
                .targetFPS = settings.value("targetFPS", 60).toInt(),
                .convolveSize = settings.value("convolveSize", 5).toInt(),
                .pixelsPerZone = settings.value("pixelsPerZone", 70).toInt(),
        };
    }

    CapturerConfig capturerConfig;
    Color staticColor = Color(0, 0, 0);
    bool enableStaticColor = false;
    bool enableGUI = true;
    int brightness = 100;
    AspectRatio aspectRatio = AspectRatio(STANDARD);
};

#endif //AMBILIGHT_APP_SETTINGS_H
