#include <Control_Surface.h>
USBMIDI_Interface midi;

#define COINCIDENCE_PIN 3

unsigned long noteOnTime;
unsigned long noteDuration = 1;
bool noteActive;
int coincidenceState;

void setup() {
  Control_Surface.begin();
  pinMode(COINCIDENCE_PIN, INPUT_PULLUP);
}

void loop() {
  coincidenceState = digitalRead(COINCIDENCE_PIN);

  if (coincidenceState == 1) {
    midi.sendNoteOn(Channel_1, 127);
    noteOnTime = millis();
    noteActive = 1;
  }
  if (noteActive && millis() - noteOnTime >= noteDuration) {
    midi.sendNoteOff(Channel_1, 127);
    noteActive = 0;
  }
}