//
// Created by andrew on 12/26/23.
//

#ifndef AMBILIGHT_CAPTURER_CONFIG_H
#define AMBILIGHT_CAPTURER_CONFIG_H

struct CapturerConfig {
    int ledX; // number of leds on the top and bottom of the screen
    int ledY; // number of leds on the left and right of the screen
    int screenWidth; // width of the screen
    int screenHeight; // height of the screen
    int zoneOffsetX; // the width of the zone where average color is calculated (in pixels, from the left and right)
    int zoneOffsetY; // the height of the zone where average color is calculated (in pixels, from the top and bottom)
    int captureFPS; // the number of frames per second to capture
    int targetFPS; // the number of frames per second to render
    int convolveSize; // the size of the convolution window
    int pixelsPerZone; // the number of pixels to sample per zone

    bool GUI = true; // If false, the SerialPort will be used instead of the GUI
};

#endif //AMBILIGHT_CAPTURER_CONFIG_H
