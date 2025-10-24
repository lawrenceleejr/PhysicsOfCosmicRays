#include <Adafruit_NeoPixel.h>


#define BUTTON_PIN   2
#define RGBW_PIN     11
#define RGB_PIN      12
#define SPOTLIGHT    4
#define TRIGPIN      9
#define ECHOPIN      10
#define POT_PIN      A2


#define NUM_LEDS         24
#define DISTANCE_THRESH  75      // cm
#define INACTIVITY_TIME  5000    // ms before reset
#define PULSE_DELAY      10
#define FADE_TIME        2000
#define STEP_DELAY       20
#define HOLD_RED_DELAY   15000   // 15s after spotlight on

// ------------------- Phases -------------------
enum Phase {
  RESET_PHASE,
  PHASE_ONE,  // Person detected → pulse blue
  PHASE_TWO,  // Solid green
  PHASE_THREE // Spotlight on, solid blue → fade to red, pot controls brightness
};

Phase currentPhase = RESET_PHASE;

Adafruit_NeoPixel ringRGBW(NUM_LEDS, RGBW_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ringRGB(NUM_LEDS,  RGB_PIN,  NEO_GRB  + NEO_KHZ800);

<<<<<<< HEAD
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
=======
unsigned long now = 0;
unsigned long lastPulseTime = 0;
unsigned long lastInteractionTime = 0;
unsigned long phaseStartTime = 0;
unsigned long lastPing = 0;


bool userPresent = false;
bool redLocked = false;
bool blueInitialized = false;
int distance = 9999;
int brightness = 0;
int fadeAmount = 3;
int lastButtonState = HIGH;


>>>>>>> 909f90d (restaging working, new phases. pot does not work yet. check wiring.)
void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
<<<<<<< HEAD
  pinMode(PIN3, OUTPUT);
  pinMode(PIN4, OUTPUT);
  digitalWrite(PIN4, HIGH); // Relay idle HIGH (off)

  ringRGBW.begin();
  ringRGB.begin();
  ringRGBW.show();
  ringRGB.show();
=======
  pinMode(SPOTLIGHT, OUTPUT);
  digitalWrite(SPOTLIGHT, HIGH); // relay off by default
>>>>>>> 909f90d (restaging working, new phases. pot does not work yet. check wiring.)

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  ringRGB.begin();
  ringRGBW.begin();
  ringRGB.show();
  ringRGBW.show();

  Serial.println("System Ready.");
}


void loop() {
  now = millis();
  int buttonState = digitalRead(BUTTON_PIN);
  int potValue = analogRead(POT_PIN); // 0–1023 range

  // --- Distance Sensor Update ---
  if (now - lastPing > 100) {
    lastPing = now;
    distance = readDistance();
  }
  userPresent = (distance < DISTANCE_THRESH);
  if (userPresent) lastInteractionTime = now;

  // --- Serial Debug Output ---
  Serial.print("Phase: "); Serial.print(currentPhase);
  Serial.print(" | Distance: "); Serial.print(distance);
  Serial.print(" | Pot: "); Serial.println(potValue);

  // --- Phase Logic ---
  switch (currentPhase) {

    // ===================================================
    // PHASE 0: RESET
    // ===================================================
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

    // ===================================================
    // PHASE 1: USER DETECTED (pulse blue)
    // ===================================================
    case PHASE_ONE:
      if (now - lastPulseTime >= PULSE_DELAY) {
        lastPulseTime = now;
        brightness += fadeAmount;
        if (brightness <= 0 || brightness >= 255) fadeAmount = -fadeAmount;
        setAllColor(0, 0, brightness, brightness / 10);
      }

      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
      }

      // Button press → PHASE TWO
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_TWO;
        Serial.println("P2");
      }
      break;

    // ===================================================
    // PHASE 2: SOLID GREEN
    // ===================================================
    case PHASE_TWO:
      setAllColor(0, 255, 0, 0);

      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
      }

      // Button press → PHASE THREE
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_THREE;
        phaseStartTime = now;
        Serial.println("P3");
      }
      break;

    // ===================================================
    // PHASE 3: SPOTLIGHT ON, BLUE → RED, POT CONTROLS BRIGHTNESS
    // ===================================================
    case PHASE_THREE:
      digitalWrite(SPOTLIGHT, LOW); // Spotlight ON

      if (!blueInitialized) {
        setAllColor(0, 0, 255, 0); // Start with blue
        blueInitialized = true;
        phaseStartTime = now;
      }

      // Potentiometer controls brightness of LEDs
      {
        int scaled = map(potValue, 0, 1023, 20, 255);
        ringRGB.setBrightness(scaled);
        ringRGBW.setBrightness(scaled);
      }

      if (!redLocked && now - phaseStartTime >= HOLD_RED_DELAY) {
        fadeColor(0, 0, 255, 255, 0, 0, FADE_TIME, STEP_DELAY); // Blue → red
        redLocked = true;
      }

      // Stay red unless user leaves
      if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }
      break;
  }

  lastButtonState = buttonState;
}

// ------------------- Helper Functions -------------------
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
