#include <Adafruit_NeoPixel.h>

// --- Pin assignments ---
#define BUTTON_PIN 2
#define RGBW_PIN   11
#define RGB_PIN    12
#define PIN3       3   // Opacity Film
#define PIN4       4   // Spotlight Relay
#define TRIGPIN    9
#define ECHOPIN    10

// --- Config ---
#define NUM_LEDS        24
#define DISTANCE_THRESH 75   // cm, user detected
#define INACTIVITY_TIME 5000 // ms before reset if user leaves
#define HOLD_TIME       1000 // Relay low duration
#define PULSE_DELAY     10

// --- Phase States ---
enum Phase {
  RESET_PHASE,
  PHASE_ONE,
  PHASE_TWO
};

Phase currentPhase = RESET_PHASE;

// --- NeoPixels ---
Adafruit_NeoPixel ringRGBW(NUM_LEDS, RGBW_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ringRGB(NUM_LEDS,  RGB_PIN,  NEO_GRB  + NEO_KHZ800);

// --- Timers & Variables ---
unsigned long lastInteractionTime = 0;
unsigned long now = 0;

int distance = 9999;
int lastDistance = 9999;
unsigned long lastPing = 0;
const unsigned long pingInterval = 100;

int buttonState = HIGH;
int lastButtonState = HIGH;
bool userPresent = false;

// --- For pulsing logic ---
int brightness = 0;
int fadeAmount = 3;
unsigned long lastPulseTime = 0;

// --- Placeholder for fader ---
int faderValue = 0;

// -------------------------------
// SETUP
// -------------------------------
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIN3, OUTPUT);
  pinMode(PIN4, OUTPUT);
  digitalWrite(PIN4, HIGH); // Relay idle HIGH (off)

  ringRGBW.begin();
  ringRGB.begin();
  ringRGBW.show();
  ringRGB.show();

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  Serial.begin(9600);
}

// -------------------------------
// LOOP
// -------------------------------
void loop() {
  now = millis();

  // --- Update Distance (smooth reading) ---
  if (now - lastPing >= pingInterval) {
    lastPing = now;
    lastDistance = distance;
    distance = readDistance();
  }

  userPresent = (distance < DISTANCE_THRESH);

  // --- Update Button ---
  buttonState = digitalRead(BUTTON_PIN);

  // --- Phase Logic ---
  switch (currentPhase) {

    // ==========================
    // RESET PHASE (idle)
    // ==========================
    case RESET_PHASE:
      // Everything off
      turnOffAll();

      // Transition: user detected
      if (userPresent) {
        currentPhase = PHASE_ONE;
        Serial.println("P1");
        lastInteractionTime = now;
        brightness = 0;
      }
      break;

    // ==========================
    // PHASE ONE (pulsing lights)
    // ==========================
    case PHASE_ONE:

      if (now - lastPulseTime >= PULSE_DELAY) {
        lastPulseTime = now;
        brightness += fadeAmount;
        if (brightness <= 0 || brightness >= 255) fadeAmount = -fadeAmount;

        setAllColor(brightness, brightness, brightness / 4);
      }

      Serial.print(userPresent);
      Serial.print(", ");
      Serial.print(buttonState);
      Serial.print(", ");
      Serial.println(faderValue);

      // Record activity
      if (userPresent) lastInteractionTime = now;

      // Transition: button pressed -> Phase 2
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_TWO;
        Serial.println("P2");
        activateRelay();
        lastInteractionTime = now;
      }

      // Transition: user leaves or inactivity
      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }

      break;

    // ==========================
    // PHASE TWO (solid lights + relay)
    // ==========================
    case PHASE_TWO:
      // Placeholder: LEDs solid color here later

      // Ensure relay stays active for demonstration
      if (now - lastInteractionTime > INACTIVITY_TIME && !userPresent) {
        currentPhase = RESET_PHASE;
        Serial.println("Return to Reset");
      }

      // Example future feature:
      // if (now - phaseTwoStart > someTime) { changeButtonColor(); }

      break;
  }

  lastButtonState = buttonState;
}

// -------------------------------
// FUNCTIONS
// -------------------------------

int readDistance() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);

  long duration = pulseIn(ECHOPIN, HIGH, 30000);
  int cm = duration * 0.034 / 2;
  return cm;
}

void turnOffAll() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGBW.setPixelColor(i, 0, 0, 0, 0);
    ringRGB.setPixelColor(i, 0, 0, 0);
  }
  ringRGBW.show();
  ringRGB.show();
  digitalWrite(PIN4, HIGH); // relay off
}

void activateRelay() {
  digitalWrite(PIN4, LOW); // Relay ON
  delay(HOLD_TIME);
  digitalWrite(PIN4, HIGH); // Relay OFF again if desired
}

void setAllColor(uint8_t blueBrightness, uint8_t blueRGBW, uint8_t whiteLevel) {
  // RGBW ring (blue + subtle white)
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGBW.setPixelColor(i, ringRGBW.Color(0, 0, blueRGBW, whiteLevel));
  }
  // RGB ring (blue only)
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGB.setPixelColor(i, ringRGB.Color(0, 0, blueBrightness));
  }
  ringRGBW.show();
  ringRGB.show();
}

