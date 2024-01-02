//
// Created by andrew on 12/28/23.
//

#ifndef AMBILIGHT_APP_SETTINGS_H
#define AMBILIGHT_APP_SETTINGS_H

#include <QtCore/QSettings>

#include "capturer_config.h"
#include "color.h"

class AppSettings {
public:
    explicit AppSettings(QSettings &settings) {
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
};

#endif //AMBILIGHT_APP_SETTINGS_H
