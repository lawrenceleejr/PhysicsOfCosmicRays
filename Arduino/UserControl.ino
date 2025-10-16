// To-Do
// * Implement fader input on analog pin
// * Implement "modes": a la Reset, Phase-I, Phase-II, etc
// * Implement serial Rx from computer host for changing "modes"


#include <Adafruit_NeoPixel.h>

// --- Pin assignments ---
#define BUTTON_PIN 2    // Big Button
#define RGBW_PIN   11   // RGBW ring (Adafruit)
#define RGB_PIN    12   // WS2812 RGB ring
#define PIN3       3    // Opacity Film
#define PIN4       4    // Spotlight

#define TRIGPIN 9  // Distance Sensor Trigger
#define ECHOPIN 10 // Distance Sensor Output

// --- Config ---
#define NUM_LEDS     24
#define PULSE_DELAY  10     // ms between brightness updates
#define FLASH_TIME   200    // Pin 3 HIGH duration (ms)
#define HOLD_TIME    1000   // Pin 4 LOW duration (ms)

Adafruit_NeoPixel ringRGBW(NUM_LEDS, RGBW_PIN, NEO_GRBW + NEO_KHZ800);
Adafruit_NeoPixel faderRGB(NUM_LEDS, RGB_PIN,  NEO_GRB  + NEO_KHZ800);

int brightness = 0; // temp variable
int fadeAmount = 3; // controls speed of pulsating lights
bool lastButtonState = HIGH; // temp variable

// --- Timers ---
bool pin4Active = false;
unsigned long pin4StartTime = 0;
unsigned long lastPulseTime = 0;

// --- Distance Sensor ---
int distance = 999999;
int lastDistance = 999999;
unsigned long lastPing = 0;
const unsigned long pingInterval = 100; // ms

int distanceThreshold = 70; //cm

// Info to send to host
int userPresent = 0;
int buttonState = 0;
int faderValue = 0;


void setup() {
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(PIN3, OUTPUT);
  pinMode(PIN4, OUTPUT);

  // Pin 4 idle HIGH (flipped logic) — it will be driven LOW to ground on press
  digitalWrite(PIN4, HIGH);

  ringRGBW.begin();
  faderRGB.begin();
  ringRGBW.show();
  faderRGB.show();

  pinMode(TRIGPIN, OUTPUT);
  pinMode(ECHOPIN, INPUT);

  Serial.begin(9600);         // Initialize serial communication at 9600 baud rate

}

void loop() {
  unsigned long now = millis();

  if (now - lastPing >= pingInterval) {
    lastPing = now;
    lastDistance = distance;
    distance = readDistance();
  }

  // If a user is detected within distanceThreshold in either of the last two measurements, say they're present
  if (distance > distanceThreshold && lastDistance > distanceThreshold) {
    userPresent = 0;
  } else {
    userPresent = 1;
  }

  if (!userPresent) brightness = 0;

  // --- End pin4 LOW after HOLD_TIME (non-blocking) ---
  if (pin4Active && now - pin4StartTime >= HOLD_TIME) {
    digitalWrite(PIN4, HIGH);   // Return to idle HIGH
    pin4Active = false;
  }

  // --- Handle button press (active LOW) ---
  buttonState = digitalRead(BUTTON_PIN);
  if (buttonState == LOW && lastButtonState == HIGH) {
    triggerFlash(now);
  }
  lastButtonState = buttonState;

  // --- Blue pulsation (non-blocking) ---
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
}

int readDistance(){

  // Send a 10 microsecond pulse to trigger pin
  digitalWrite(TRIGPIN, LOW);
  digitalWrite(TRIGPIN, HIGH);
  delayMicroseconds(1);
  digitalWrite(TRIGPIN, LOW);

  // Read the echo pin
  long duration = pulseIn(ECHOPIN, HIGH,100000);

  // Calculate distance in cm
  distance = duration * 0.034 / 2;

  // Print distance for debugging
  // Serial.print("Distance: ");
  // Serial.print(distance);
  // Serial.println(" cm");

}

void setAllColor(uint8_t blueBrightness, uint8_t blueRGBW, uint8_t whiteLevel) {
  // RGBW ring (blue + subtle white)
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGBW.setPixelColor(i, ringRGBW.Color(0, 0, blueRGBW, whiteLevel));
  }
  // RGB ring (blue only)
  for (int i = 0; i < NUM_LEDS; i++) {
    faderRGB.setPixelColor(i, faderRGB.Color(0, 0, blueBrightness));
  }
  ringRGBW.show();
  faderRGB.show();
}

void triggerFlash(unsigned long now) {
  // Immediately drive PIN4 to ground (LOW) and start non-blocking timer
  digitalWrite(PIN4, LOW);
  pin4Active = true;
  pin4StartTime = now;

  // Flash both rings bright white immediately
  for (int i = 0; i < NUM_LEDS; i++) {
    ringRGBW.setPixelColor(i, ringRGBW.Color(255, 255, 255, 255));
    faderRGB.setPixelColor(i, faderRGB.Color(255, 255, 255));
  }
  ringRGBW.show();
  faderRGB.show();

  // Pin 3 pulse (200 ms)
  digitalWrite(PIN3, HIGH);
  delay(FLASH_TIME);
  digitalWrite(PIN3, LOW);
}
