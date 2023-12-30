#include <FastLED.h>

#define LED_PIN 13
#define NUM_LEDS 118
#define BUFFER_SIZE 30 // Should be a multiple of 3!
#define INCREMENT BUFFER_SIZE / 3

CRGB leds[NUM_LEDS];
uint8_t prefix[] = {'A', 'd', 'a'}, i;

unsigned long previousMillis = 0;
unsigned int readCount = 0;

void setup()
{
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);  // инициализация светодиодов
  FastLED.show(); // Initialize all pixels to 'off'
  Serial.begin(250000);

  Serial.print("al_ready");
}

void loop()
{
  //    unsigned long currentMillis = millis();
  //    if (currentMillis - previousMillis >= 1000) {
  //        previousMillis = currentMillis;
  //
  //        Serial.println(readCount);
  //        Serial.print("First LED RGB: ");
  //        printRGB(leds[0]);
  //        Serial.print("Last LED RGB: ");
  //        printRGB(leds[NUM_LEDS - 1]);
  //
  //        readCount = 0;
  //    }
   for (i = 0; i < sizeof prefix; ++i) {
waitLoop: while (Serial.available() <= 0) ;;
    if (prefix[i] == Serial.read()) continue;
    i = 0;
    Serial.print("miss");
    goto waitLoop;
  }

//  memset(leds, 0, NUM_LEDS * sizeof(struct CRGB));

  for (uint8_t i = 0; i < NUM_LEDS; i+=2) {
    byte r, g, b;
    // Read data for each color
     while (Serial.available() <= 0);
    r = Serial.read();

    while (Serial.available() <= 0);
    g = Serial.read();

    while (Serial.available() <= 0);
    b = Serial.read();

    leds[i].r = r;
    leds[i].g = g;
    leds[i].b = b;

    leds[i + 1].r = r;
    leds[i + 1].g = g;
    leds[i + 1].b = b;
  }

  readCount++;
  FastLED.show();
}

void printRGB(CRGB color)
{
  Serial.print("(");
  Serial.print(color.red);
  Serial.print(", ");
  Serial.print(color.green);
  Serial.print(", ");
  Serial.print(color.blue);
  Serial.println(")");
}