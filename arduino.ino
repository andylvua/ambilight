#include <FastLED.h>

#define LED_PIN 13
#define NUM_LEDS 118
#define BUFFER_SIZE 30 // Should be a multiple of 3!
#define INCREMENT BUFFER_SIZE / 3

CRGB leds[NUM_LEDS];
CRGB previousColors[NUM_LEDS];

uint8_t prefix[] = {'A', 'd', 'a'}, i;

unsigned long previousMillis = 0;
unsigned int readCount = 0;

void setup() {
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    FastLED.show();

    memset(previousColors, 0, sizeof previousColors);
    memset(leds, 0, sizeof leds);

    Serial.begin(115200);

    Serial.print("al_ready");

}


void interpolateColors() {
    CRGB interpolatedColors[NUM_LEDS];

    unsigned long totalElapsed = millis();

    int targetFPS = 60;
    int captureFPS = 5;

    int totalFramesToInterpolate = targetFPS - captureFPS;
    int framesToInterpolate = totalFramesToInterpolate / captureFPS;
    int framesToRender = framesToInterpolate + 1;
    int captureTimeMs = 1000 / captureFPS;
    int frameDuration = captureTimeMs / framesToRender;

    CRGB initColors[NUM_LEDS];
    memcpy(initColors, leds, sizeof initColors);

    for (int i = 0; i < framesToRender; i++) {
        for (int j = 0; j < NUM_LEDS; j++) {
            double interpolationRate = static_cast<double>(i) / framesToInterpolate;
            int dRed = initColors[j].r - previousColors[j].r;
            int dGreen = initColors[j].g - previousColors[j].g;
            int dBlue = initColors[j].b - previousColors[j].b;
            CRGB interpolatedColor = CRGB(previousColors[j].r + dRed * interpolationRate,
                                          previousColors[j].g + dGreen * interpolationRate,
                                          previousColors[j].b + dBlue * interpolationRate);
            interpolatedColors[j] = interpolatedColor;
        }

        memcpy(leds, interpolatedColors, sizeof interpolatedColors);

        FastLED.show();

        if (millis() - totalElapsed >= (frameDuration * framesToRender) - 55) {
            break;
        } else if (i != framesToInterpolate) {
            delay(frameDuration - 3);
        }
    }
}


void loop() {
    for (i = 0; i < sizeof prefix; ++i) {
        waitLoop:
        while (Serial.available() <= 0);;
        if (prefix[i] == Serial.read()) continue;
        i = 0;
        Serial.print("miss");
        goto waitLoop;
    }

    memcpy(previousColors, leds, sizeof previousColors);

    for (uint8_t i = 0; i < NUM_LEDS; i++) {
        byte r, g, b;

        while (Serial.available() <= 0);
        r = Serial.read();
        while (Serial.available() <= 0);
        g = Serial.read();
        while (Serial.available() <= 0);
        b = Serial.read();

        leds[i].r = r;
        leds[i].g = g;
        leds[i].b = b;
    }

    interpolateColors();
}
