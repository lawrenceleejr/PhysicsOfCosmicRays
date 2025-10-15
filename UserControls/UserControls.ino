#include <Adafruit_NeoPixel.h>

#define BUTTON_OUT 6

#define BUTTON_LED 12
#define BUTTON_LED_COUNT 24

#define SLIDER_IN 1
#define SLIDER_OUT 0

#define SLIDER_LED 11 
#define SLIDER_LED_COUNT 16


int sliderValue;
int buttonValue;

String message;

// Create the NeoPixel object (RGBW mode)
Adafruit_NeoPixel ring(BUTTON_LED_COUNT, BUTTON_LED, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel strip(SLIDER_LED_COUNT, SLIDER_LED, NEO_GRBW + NEO_KHZ800);


void setup() {
    ring.begin();
    ring.show();  // Initialize all pixels to 'off'
    strip.begin();
    strip.show();  // Initialize all pixels to 'off'
    Serial.begin(9600);

    pinMode(11, OUTPUT);
    pinMode(12, OUTPUT);
}

void loop() {
    sliderValue = analogRead(SLIDER_OUT);
    buttonValue = digitalRead(BUTTON_OUT);

    message = String(sliderValue) + ", " + String(buttonValue);
    Serial.println(message);


    if (buttonValue == 1) {
        for (int i = 0; i < BUTTON_LED_COUNT; i++) {
            ring.setPixelColor(i, ring.Color(218, 165, float(sliderValue)/1005.*255, 0));
        }  

        for (int i = 0; i < SLIDER_LED_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(218, 165, 32));  // WS2812 = RGB only
        }

        digitalWrite(11, 1);
        digitalWrite(12, 1);
    } else {
        for (int i = 0; i < BUTTON_LED_COUNT; i++) {
            ring.setPixelColor(i, ring.Color(0, 0, 0, 0));
        }  

        for (int i = 0; i < SLIDER_LED_COUNT; i++) {
            strip.setPixelColor(i, strip.Color(0, 0, 0));  // WS2812 = RGB only
        }
        digitalWrite(11, 0);
        digitalWrite(12, 0);
    }

    strip.show();
    ring.show();
    // delay(10);
}

// void pulseRing(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
//   // Fade in
//   for (int brightness = 0; brightness <= 255; brightness++) {
//     setRingColor(r, g, b, w, brightness);
//     delay(10); // Adjust speed of pulsing
//   }
//   // Fade out
//   for (int brightness = 255; brightness >= 0; brightness--) {
//     setRingColor(r, g, b, w, brightness);
//     delay(10);
//   }
// }

// void setRingColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w, int brightness) {
//   // Scale color by brightness
//   float scale = brightness / 255.0;
//   uint8_t rAdj = r * scale;
//   uint8_t gAdj = g * scale;
//   uint8_t bAdj = b * scale;
//   uint8_t wAdj = w * scale;

//   for (int i = 0; i < BUTTON_LED_COUNT; i++) {
//     ring.setPixelColor(i, ring.Color(rAdj, gAdj, bAdj, wAdj));
//   }
//   ring.show();
// }
