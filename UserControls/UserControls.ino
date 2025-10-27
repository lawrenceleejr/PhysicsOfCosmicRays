#include <Adafruit_NeoPixel.h>

// -------------------- Pin Definitions --------------------
#define BUTTON_PIN   2
#define RGBW_PIN     11
#define RGB_PIN      12
#define SPOTLIGHT    4
#define TRIGPIN      9
#define ECHOPIN      10
#define POT_PIN      A2

// -------------------- Constants --------------------
#define NUM_LEDS         24
#define DISTANCE_THRESH  75      // cm
#define INACTIVITY_TIME  5000    // ms before reset
#define PULSE_DELAY      10
#define FADE_TIME        2000
#define STEP_DELAY       20
#define HOLD_RED_DELAY   15000   // 15s after spotlight on

// -------------------- Phases --------------------
enum Phase {
  RESET_PHASE,
  PHASE_ONE,   // Person detected → pulse blue
  PHASE_TWO,   // Solid green
  PHASE_THREE  // Spotlight on, blue → fade red, pot controls brightness
};

Phase currentPhase = RESET_PHASE;

// -------------------- LED Objects --------------------
Adafruit_NeoPixel ringRGBW(NUM_LEDS, RGBW_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ringRGB(NUM_LEDS,  RGB_PIN,  NEO_GRB  + NEO_KHZ800);

// -------------------- Timing Variables --------------------
unsigned long now = 0;
unsigned long lastInteractionTime = 0;
unsigned long lastPulseTime = 0;
unsigned long phaseStartTime = 0;
unsigned long lastPing = 0;

// -------------------- Sensor & State --------------------
bool userPresent = false;
bool redLocked = false;
bool blueInitialized = false;

int distance = 9999;
int potValue = 0;
int brightness = 0;
int fadeAmount = 3;
int lastButtonState = HIGH;

// -------------------- Function Prototypes --------------------
int readDistance();
void turnOffAll();
void setAllColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void fadeColor(uint8_t r1, uint8_t g1, uint8_t b1,
               uint8_t r2, uint8_t g2, uint8_t b2,
               int duration, int stepDelay);
int smoothAnalogRead(int pin);

// ==========================================================
// SETUP
// ==========================================================
void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPOTLIGHT, OUTPUT);
  digitalWrite(SPOTLIGHT, HIGH); // relay off by default

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  ringRGB.begin();
  ringRGBW.begin();
  ringRGB.show();
  ringRGBW.show();

}

// ==========================================================
// LOOP
// ==========================================================
void loop() {
  now = millis();
  int buttonState = digitalRead(BUTTON_PIN);
  potValue = smoothAnalogRead(POT_PIN);

  // --- Distance Sensor Update ---
  if (now - lastPing > 100) {
    lastPing = now;
    distance = readDistance();
  }
  userPresent = (distance < DISTANCE_THRESH);
  if (userPresent) lastInteractionTime = now;

  // --- Serial Debug Output ---
  Serial.print(buttonState);
  Serial.print(",");
  Serial.print(potValue);
  Serial.print(",");
  Serial.println(distance);


  // --- Phase Logic ---
  switch (currentPhase) {

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
      }

      // Button press → PHASE_TWO
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_TWO;
        Serial.println("P2");
      }
      break;

    case PHASE_TWO:
      setAllColor(0, 255, 0, 0);

      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
      }

      // Button press → PHASE_THREE
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_THREE;
        phaseStartTime = now;
        Serial.println("P3");
      }
      break;

   case PHASE_THREE:
  digitalWrite(SPOTLIGHT, LOW); // Spotlight ON

  if (!blueInitialized) {
    phaseStartTime = now;
    blueInitialized = true;
  }

  // Determine color from fade (blue → red after HOLD_RED_DELAY)
  uint8_t r = 0, g = 0, b = 255; // default blue
  if (now - phaseStartTime >= HOLD_RED_DELAY) {
    float progress = float(now - (phaseStartTime + HOLD_RED_DELAY)) / FADE_TIME;
    if (progress > 1.0) progress = 1.0;
    r = uint8_t(255 * progress);
    g = 0;
    b = uint8_t(255 * (1.0 - progress));
  }

  // Read pot and scale brightness
  int potValue = smoothAnalogRead(POT_PIN);
  int scaledBrightness = map(potValue, 20, 1023, 255, 0);


  // Apply scaled brightness while keeping color proportions
  ringRGB.setBrightness(scaledBrightness);
  ringRGBW.setBrightness(scaledBrightness);
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGB.setPixelColor(i, ringRGB.Color(r, g, b));
    ringRGBW.setPixelColor(i, ringRGBW.Color(r, g, b, 0));
  }
  ringRGB.show();
  ringRGBW.show();

  // Stay in PHASE_THREE until user leaves
  if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
    currentPhase = RESET_PHASE;
    Serial.println("P0");
  }
  break;

  }

  lastButtonState = buttonState;
}

// ==========================================================
// --- Helper Functions ---
// ==========================================================
int smoothAnalogRead(int pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / 10;
}

int readDistance() {
  digitalWrite(TRIGPIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIGPIN, LOW);
  long duration = pulseIn(ECHOPIN, HIGH, 30000);
  int cm = duration * 0.034 / 2;
  return cm > 0 ? cm : 9999;
}

void turnOffAll() {
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGB.setPixelColor(i, 0);
    ringRGBW.setPixelColor(i, 0);
  }
  ringRGB.show();
  ringRGBW.show();
  digitalWrite(SPOTLIGHT, HIGH);
}

void setAllColor(uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGB.setPixelColor(i, ringRGB.Color(r, g, b));
    ringRGBW.setPixelColor(i, ringRGBW.Color(r, g, b, w));
  }
  ringRGB.show();
  ringRGBW.show();
}

void fadeColor(uint8_t r1, uint8_t g1, uint8_t b1,
               uint8_t r2, uint8_t g2, uint8_t b2,
               int duration, int stepDelay) {
  int steps = duration / stepDelay;
  for (int i = 0; i <= steps; i++) {
    uint8_t r = r1 + (r2 - r1) * i / steps;
    uint8_t g = g1 + (g2 - g1) * i / steps;
    uint8_t b = b1 + (b2 - b1) * i / steps;
    setAllColor(r, g, b, 0);
    delay(stepDelay);
  }
}
