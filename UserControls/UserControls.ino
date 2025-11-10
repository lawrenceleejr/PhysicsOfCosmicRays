#include <Adafruit_NeoPixel.h>

// ---------------------------
// Pin Definitions
// ---------------------------
#define RING_RGB_PIN   6
#define RING_RGBW_PIN  7
#define NUM_LEDS       24
#define BUTTON_PIN     2
#define POT_PIN        A0
#define SPOTLIGHT      8
#define DIST_SENSOR_PIN A1

// ---------------------------
// LED Rings
// ---------------------------

Adafruit_NeoPixel ringRGB(NUM_LEDS, RING_RGB_PIN, NEO_GRB + NEO_KHZ800);
Adafruit_NeoPixel ringRGBW(NUM_LEDS, RING_RGBW_PIN, NEO_GRBW + NEO_KHZ800);

// ---------------------------
// Phase States
// ---------------------------

enum Phase {
  RESET_PHASE,
  PHASE_ONE,
  PHASE_TWO,
  PHASE_THREE,
  PHASE_FOUR
};

Phase currentPhase = RESET_PHASE;

// ---------------------------
// Timing and Animation Control
// ---------------------------
unsigned long now = 0;
unsigned long lastPulseTime = 0;
unsigned long lastInteractionTime = 0;
unsigned long phaseStartTime = 0;

int brightness = 0;
int fadeAmount = 3;
bool blueInitialized = false;
bool redLocked = false;

const unsigned long PULSE_DELAY = 20;
const unsigned long INACTIVITY_TIME = 5000;
const unsigned long HOLD_RED_DELAY = 3000;
const unsigned long FADE_TIME = 4000;

// Rainbow orb constants
#define ORB_TRAIL_LENGTH 5
#define ORB_DELAY 50

// ---------------------------
// Utility Functions
// ---------------------------
void turnOffAll() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGB.setPixelColor(i, 0);
    ringRGBW.setPixelColor(i, 0);
  }
  ringRGB.show();
  ringRGBW.show();
  digitalWrite(SPOTLIGHT, HIGH); // Spotlight off
}

void setAllColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w = 0) {
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGB.setPixelColor(i, ringRGB.Color(r, g, b));
    ringRGBW.setPixelColor(i, ringRGBW.Color(r, g, b, w));
  }
  ringRGB.show();
  ringRGBW.show();
}

int smoothAnalogRead(int pin) {
  const int samples = 10;
  long total = 0;
  for (int i = 0; i < samples; i++) {
    total += analogRead(pin);
    delay(2);
  }
  return total / samples;
}

// ---------------------------
// Setup
// ---------------------------
void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPOTLIGHT, OUTPUT);
  digitalWrite(SPOTLIGHT, HIGH); // OFF

  ringRGB.begin();
  ringRGBW.begin();
  turnOffAll();

  Serial.println("P0");
}

// ---------------------------
// Main Loop
// ---------------------------
void loop() {
  now = millis();

  // Read inputs
  int buttonState = digitalRead(BUTTON_PIN);
  static int lastButtonState = HIGH;
  int potValue = smoothAnalogRead(POT_PIN);
  int distanceValue = smoothAnalogRead(DIST_SENSOR_PIN);
  bool userPresent = (distanceValue < 300); // Adjust for your sensor range

  // --- Telemetry line ---
  Serial.print(buttonState);
  Serial.print(",");
  Serial.print(potValue);
  Serial.print(",");
  Serial.println(distanceValue);

  // ---------------------------
  // Phase Control Logic
  // ---------------------------
  switch (currentPhase) {

    // ---------- RESET PHASE ----------
    case RESET_PHASE:
      turnOffAll();
      redLocked = false;
      blueInitialized = false;
      if (userPresent) {
        currentPhase = PHASE_ONE;
        brightness = 0;
        fadeAmount = 3;
        lastPulseTime = now;
        Serial.println("P1");
      }
      break;

    // ---------- PHASE ONE ----------
    case PHASE_ONE:
      // Pulse blue
      if (now - lastPulseTime >= PULSE_DELAY) {
        lastPulseTime = now;
        brightness += fadeAmount;
        if (brightness <= 0 || brightness >= 255) fadeAmount = -fadeAmount;
        setAllColor(0, 0, brightness, brightness / 10);
      }
      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_TWO;
        Serial.println("P2");
      }
      break;

    // ---------- PHASE TWO ----------
    case PHASE_TWO:
      setAllColor(0, 255, 0, 0); // solid green
      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_THREE;
        Serial.println("P3");
      }
      break;

    // ---------- PHASE THREE (RAINBOW ORB) ----------
    case PHASE_THREE:
    {
      static int orbPosition = 0;
      static uint16_t hue = 0;

      if (now - lastPulseTime >= ORB_DELAY) {
        lastPulseTime = now;
        hue += 256; // rainbow cycle speed
        if (hue > 65535) hue = 0;

        // Clear LEDs
        for (int i = 0; i < NUM_LEDS; i++) {
          ringRGB.setPixelColor(i, 0);
          ringRGBW.setPixelColor(i, 0);
        }

        // Draw orb + fading trail
        for (int t = 0; t < ORB_TRAIL_LENGTH; t++) {
          int index = (orbPosition - t + NUM_LEDS) % NUM_LEDS;
          float fadeFactor = 1.0 - (float)t / ORB_TRAIL_LENGTH;
          uint32_t color = ringRGB.ColorHSV(hue, 255, uint8_t(255 * fadeFactor));
          uint8_t r = (uint8_t)(color >> 16);
          uint8_t g = (uint8_t)(color >> 8);
          uint8_t b = (uint8_t)(color);

          ringRGB.setPixelColor(index, r, g, b);
          ringRGBW.setPixelColor(index, r, g, b, 0);
        }

        ringRGB.show();
        ringRGBW.show();
        orbPosition = (orbPosition + 1) % NUM_LEDS;
      }

      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }

      // Button press → next phase
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_FOUR;
        phaseStartTime = now;
        Serial.println("P4");
      }
    }
    break;

    // ---------- PHASE FOUR ----------
    case PHASE_FOUR:
      digitalWrite(SPOTLIGHT, LOW); // Spotlight ON

      if (!blueInitialized) {
        phaseStartTime = now;
        blueInitialized = true;
      }

      // Blue → red fade
      uint8_t r = 0, g = 0, b = 255;
      if (now - phaseStartTime >= HOLD_RED_DELAY) {
        float progress = float(now - (phaseStartTime + HOLD_RED_DELAY)) / FADE_TIME;
        if (progress > 1.0) progress = 1.0;
        r = uint8_t(255 * progress);
        g = 0;
        b = uint8_t(255 * (1.0 - progress));
      }

      // Pot controls brightness
      int scaledBrightness = map(potValue, 20, 1023, 255, 0);
      ringRGB.setBrightness(scaledBrightness);
      ringRGBW.setBrightness(scaledBrightness);

      for (int i = 0; i < NUM_LEDS; i++) {
        ringRGB.setPixelColor(i, ringRGB.Color(r, g, b));
        ringRGBW.setPixelColor(i, ringRGBW.Color(r, g, b, 0));
      }
      ringRGB.show();
      ringRGBW.show();

      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }
      break;
  }

  lastButtonState = buttonState;
}
