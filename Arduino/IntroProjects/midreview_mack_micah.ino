// as of 9/22/2025, this code works for Mack. 
// 9/24/25, LL, Compiles with manual MIDI note sending. Added on top of tested Scintillator-triggered LED flashes.

// #include <MIDI_Controller.h> // Include the library
#include <FastLED.h>
#include <Control_Surface.h>

#define LED_PIN        6       // Data pin for LED strip
#define LED_BUILTIN   13       // Data pin for LED strip
#define NUM_LEDS       8       // Only 8 LEDs now
#define MASTER_PIN     3       // Master ON/OFF button (to GND)
#define TRIGGER_PIN    2       // Trigger orb button (to GND)
#define HUE_POT        A1
#define INTENSITY_POT  A2
#define VOLUME_POT     A3      // layup for Micah


const uint8_t velocity = 0b1111111; // Maximum velocity (0b1111111 = 0x7F = 127)
const uint8_t C4 = 60;              // Note number 60 is defined as middle C in the MIDI specification


CRGB leds[NUM_LEDS];

bool masterOn = false;
bool running  = false;
int orbPos = 0;
unsigned long lastUpdate = 0;


// Create a new instance of the class 'Digital', called 'button', on pin 2, that sends MIDI messages with note 'C4' (60) on channel 1, with velocity 127
// Digital button(TRIGGER_PIN, C4, 1, velocity);

HardwareSerialMIDI_Interface midi {Serial, MIDI_BAUD};



// Orb config
const int tailLength = 4;       // currently short because of the 8 leds, will need to be adjusted for much more LEDS
// const int orbDelay   = 1250;    // ~10 sec total for 8 LEDs, to change do seconds/num leds for orb progression
const int orbDelay   = 10;    // ~10 sec total for 8 LEDs, to change do seconds/num leds for orb progression
const int glowLength = 1;

bool lastTriggerState = HIGH;
bool lastMasterState  = HIGH;




void setup() {
  Control_Surface.begin();

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear();
  FastLED.show();

  // pinMode(TRIGGER_PIN, INPUT_PULLUP);
  // pinMode(TRIGGER_PIN, INPUT_PULLUP);

  pinMode(MASTER_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW); // Start OFF
  Serial.begin(9600);         // Initialize serial communication at 9600 baud rate

}

void loop() {
  // // ----- MASTER BUTTON -----
  // bool masterState = digitalRead(MASTER_PIN);
  // if (lastMasterState == HIGH && masterState == LOW) {
  //   masterOn = !masterOn;  // toggle state
  //   digitalWrite(LED_BUILTIN, masterOn ? HIGH : LOW);

  //   if (!masterOn) {
  //     running = false;
  //     FastLED.clear();
  //     FastLED.show();
  //   }
  // }
  // lastMasterState = masterState;

  // if (!masterOn) return; // skip everything if master is OFF

  // ----- TRIGGER BUTTON -----
  // MIDI_Controller.refresh();
  bool triggerState = digitalRead(TRIGGER_PIN);
  if (lastTriggerState == HIGH && triggerState == LOW) {
    running = true;
    orbPos = 0;
    Serial.println("Coin ");
    digitalWrite(LED_BUILTIN, HIGH);

    midi.sendNoteOn({60, CHANNEL_1}, 100);
    delay(3);
    midi.sendNoteOff({60, CHANNEL_1}, 0);
    // delay(500);
  }
  lastTriggerState = triggerState;

  // ----- ORB ANIMATION -----
  if (running && millis() - lastUpdate >= orbDelay) {
  // if (running) {
    lastUpdate = millis();

    // Limit hue range so it doesn’t wrap back to red
    // int baseHue = map(analogRead(HUE_POT), 0, 1023, 0, 192); // red → blue // doesn't go up to 255 for no repeat hues
    // float intensityScale = analogRead(INTENSITY_POT) / 1023.0;
    float intensityScale=1;

    FastLED.clear();

    // Tail
    for (int i = 0; i < tailLength; i++) {
      int pos = orbPos - i;
      if (pos >= 0 && pos < NUM_LEDS) {
        uint8_t brightness = (uint8_t)(255 * (1.0 - (float)i / tailLength) * intensityScale);
        leds[pos] = CHSV(200, 255, brightness);
      }
    }

    // Glow ahead
    for (int g = 1; g <= glowLength; g++) {
      int pos = orbPos + g;
      if (pos < NUM_LEDS) {
        uint8_t glowBrightness = (uint8_t)(150 * intensityScale * (1.0 - (float)(g - 1) / glowLength));
        leds[pos] = CHSV(100, 255, glowBrightness);
      }
    }

    FastLED.show();
    // Serial.println();


    orbPos++;
    if (orbPos - tailLength >= NUM_LEDS) {
      running = false;
      FastLED.clear();
      FastLED.show();
      digitalWrite(LED_BUILTIN, HIGH);
      Serial.println("Coin Over");

    }
  }
}
