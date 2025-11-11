#include <Adafruit_NeoPixel.h>

// --- Pin assignments ---
#define BUTTON_PIN 2
#define RGBW_PIN   11
#define RGB_PIN    12
#define SPOTLIGHTPIN       7   
#define TRIGPIN    9
#define ECHOPIN    10
#define POT_PIN    A2  // Potentiometer

// --- Config ---
#define NUM_LEDS        24
#define DISTANCE_THRESH 75   // cm
#define INACTIVITY_TIME 5000 // ms before reset if user leaves
#define HOLD_BLUE_TIME  30000 // ms hold blue before fading red
#define PULSE_DELAY     10
#define ORB_TRAIL_LENGTH 5
#define ORB_DELAY 50
#define SPOTLIGHT_DELAY 15000 // 15s

// --- Phases ---
enum Phase {
  RESET_PHASE,
  PHASE_ONE,
  PHASE_TWO,
  PHASE_THREE,
  PHASE_FOUR,
  PHASE_FIVE
};

Phase currentPhase = RESET_PHASE;

// --- NeoPixels ---
Adafruit_NeoPixel ringRGBW(NUM_LEDS, RGBW_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel ringRGB(NUM_LEDS,  RGB_PIN,  NEO_GRB  + NEO_KHZ800);

// --- Timers & Variables ---
unsigned long lastInteractionTime = 0;
unsigned long now = 0;

int distance = 9999;
unsigned long lastPing = 0;
const unsigned long pingInterval = 100;

int buttonState = HIGH;
int lastButtonState = HIGH;
bool userPresent = false;

int potValue = 0;

int brightness = 0;
int fadeAmount = 3;
unsigned long lastPulseTime = 0;

bool blueInitialized = false;
bool redLocked = false;
unsigned long phaseStartTime = 0;

int orbPosition = 0;
uint16_t hue = 0;

// -------------------------------
// SETUP
// -------------------------------
void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SPOTLIGHTPIN, OUTPUT);
  digitalWrite(SPOTLIGHTPIN, HIGH); // Relay idle HIGH (off)

  ringRGBW.begin();
  ringRGB.begin();
  ringRGBW.show();
  ringRGB.show();

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);
  pinMode(POT_PIN, INPUT);

  Serial.begin(9600);
}

// -------------------- LOOP --------------------
void loop() {
  now = millis();

  Serial.print(buttonState);
  Serial.print(",");
  Serial.print(potValue);
  Serial.print(",");
  Serial.println(distance);

  // --- Serial Command Input ---
  if (Serial.available() > 0) {          
    char incomingByte = Serial.read();
    if (incomingByte == 'R') {          
      currentPhase = RESET_PHASE;
      Serial.println("P0");
    }
  }

  // --- Distance update ---
  if (now - lastPing >= pingInterval) {
    lastPing = now;
    distance = readDistance();
  }
  userPresent = (distance < DISTANCE_THRESH);

  // --- Button ---
  buttonState = digitalRead(BUTTON_PIN);

  // --- Slider Value ---
  potValue = analogRead(POT_PIN);

  switch (currentPhase) {

    // =================================================
    // RESET PHASE (idle state, everything off)
    // =================================================
    case RESET_PHASE:
      turnOffAll();
      blueInitialized = false;
      redLocked = false;

      if (userPresent) {
        currentPhase = PHASE_ONE;
        brightness = 0;
        fadeAmount = 3;
        lastPulseTime = now;
        lastInteractionTime = now;
        Serial.println("P1");
      }

      break;

    // =================================================
    // PHASE ONE (pulsing blue + spotlight on)
    // =================================================
    case PHASE_ONE:
      digitalWrite(SPOTLIGHTPIN, HIGH); // Spotlight OFF

      if (now - lastPulseTime >= PULSE_DELAY) {
        lastPulseTime = now;
        brightness += fadeAmount;
        if (brightness <= 0 || brightness >= 255) fadeAmount = -fadeAmount;
        setAllColor(0, 0, brightness, brightness / 5); // blue pulse
      }

      if (userPresent) lastInteractionTime = now;

      // --- Button pressed: advance to PHASE TWO ---
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_TWO;
        phaseStartTime = now;
        Serial.println("P2");
      }

      break;

    // =================================================
    // PHASE TWO (solid green)
    // =================================================
    case PHASE_TWO:
      digitalWrite(SPOTLIGHTPIN, HIGH);
      setAllColor(0, 255, 0, 0); // solid green

      if (userPresent) lastInteractionTime = now;

      // --- Button pressed: advance to PHASE THREE ---
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_THREE;
        phaseStartTime = now;
        Serial.println("P3");
      }

      break;

    // =================================================
    // PHASE THREE (rainbow orb animation)
    // =================================================
    case PHASE_THREE:
      if (now - phaseStartTime > SPOTLIGHT_DELAY){ // Time to let narration go before turning on the spotlight dramatically
        digitalWrite(SPOTLIGHTPIN, LOW); // Spotlight turns ON
      }

      setAllColor(155, 0, 255, 255); // solid white

      // LL thinks this isn't doing what you think it is. It's changing the hue by 256 every ORB_DELAY. (50 ms). I think this is the crazy christmas lights effect you were seeing. 
      // if (now - lastPulseTime >= ORB_DELAY) {
      //   lastPulseTime = now;
      //   hue += 256;
      //   if (hue > 65535) hue = 0;

      //   for (int i = 0; i < NUM_LEDS; i++) {
      //     ringRGB.setPixelColor(i, 0);
      //     ringRGBW.setPixelColor(i, 0);
      //   }

      //   for (int t = 0; t < ORB_TRAIL_LENGTH; t++) {
      //     int index = (orbPosition - t + NUM_LEDS) % NUM_LEDS;
      //     float fadeFactor = 1.0 - (float)t / ORB_TRAIL_LENGTH;
      //     uint32_t color = ringRGB.ColorHSV(hue, 255, uint8_t(255 * fadeFactor));
      //     uint8_t r = (color >> 16) & 0xFF;
      //     uint8_t g = (color >> 8) & 0xFF;
      //     uint8_t b = color & 0xFF;
      //     ringRGB.setPixelColor(index, r, g, b);
      //     ringRGBW.setPixelColor(index, r, g, b, 0);
      //   }

      //   ringRGB.show();
      //   ringRGBW.show();
      //   orbPosition = (orbPosition + 1) % NUM_LEDS;
      // }

      if (userPresent) lastInteractionTime = now;

      // --- Button pressed: advance to PHASE FOUR ---
      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_FOUR;
        phaseStartTime = now;
        Serial.println("P4");
      }

      break;

    // =================================================
    // PHASE FOUR (solid blue → fade to red → lock red)
    // =================================================
    case PHASE_FOUR:
      digitalWrite(SPOTLIGHTPIN, LOW);

      if (!blueInitialized) {
        setAllColor(0, 0, 255, 0);
        blueInitialized = true;
        phaseStartTime = now;
      }

      if (!redLocked && now - phaseStartTime > HOLD_BLUE_TIME) {
        fadeColor(0, 0, 255, 255, 0, 0, 2000, 20);
        redLocked = true;
      }

      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = PHASE_FIVE;
        Serial.println("P5");
      }

      break;

    case PHASE_FIVE:
      digitalWrite(SPOTLIGHTPIN, HIGH); // Turn off spotlight

      if (now - lastPulseTime >= PULSE_DELAY) {
        lastPulseTime = now;
        brightness -= 2;
        if (brightness >= 100) setAllColor(0, 0, brightness, brightness / 5); // blue fade out to dim
      }

      if (buttonState == LOW && lastButtonState == HIGH) {
        currentPhase = RESET_PHASE;
        Serial.println("P0");
      }

      break;
  }

  lastButtonState = buttonState;

  // --- Reset if user leaves ---
  if (!userPresent && now - lastInteractionTime > INACTIVITY_TIME) {
    currentPhase = RESET_PHASE;
    Serial.println("P0");
  }

  return;
}

// -------------------- FUNCTIONS --------------------

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
    ringRGB.setPixelColor(i, 0, 0, 0);
    ringRGBW.setPixelColor(i, 0, 0, 0, 0);
  }
  ringRGB.show();
  ringRGBW.show();
  digitalWrite(SPOTLIGHTPIN, HIGH);
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