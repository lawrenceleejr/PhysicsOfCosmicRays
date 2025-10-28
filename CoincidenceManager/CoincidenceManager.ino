// #include <Control_Surface.h>
// USBMIDI_Interface midi;

#define X_IN 3
#define A_IN 4
#define B_IN 5
#define X_OUT 7
#define A_OUT 12
#define B_OUT 13

// unsigned long noteDuration = 5;

// unsigned long coincidenceNoteBegin;
// unsigned long aNoteBegin;
// unsigned long bNoteBegin;

// bool coincidenceNoteActive;
// bool aNoteActive;
// bool bNoteActive;

int xState;
int aState;
int bState;

void setup() {
  // Control_Surface.begin();
  // Serial.begin(9600);
  pinMode(X_IN, INPUT_PULLUP);
  pinMode(X_OUT, OUTPUT);
  pinMode(A_IN, INPUT_PULLUP);
  pinMode(A_OUT, OUTPUT);
  pinMode(B_IN, INPUT_PULLUP);
  pinMode(B_OUT, OUTPUT);
}

void loop() {
  xState = digitalRead(X_IN);
  // aState = digitalRead(A_IN);
  // bState = digitalRead(B_IN);

  if (xState == 1) {
    // midi.sendNoteOn(Channel_4, 127);
    digitalWrite(X_OUT, 0);
    // midi.sendNoteOff(Channel_4, 127);
    digitalWrite(X_OUT, 1);
    // Serial.println("COINC ON");
    // coincidenceNoteBegin = millis();
    // coincidenceNoteActive = 1;
  } 
  // if (aState = 1) {
  //   midi.sendNoteOn(Channel_2, 127);
  //   digitalWrite(A_OUT, 1);
  //   midi.sendNoteOff(Channel_2, 127);
  //   digitalWrite(A_OUT, 0);
  //   // Serial.println("A ON");
  //   // aNoteBegin = millis();
  //   // aNoteActive = 1;
  // }
  // if (bState = 1) {
  //   midi.sendNoteOn(Channel_3, 127);
  //   digitalWrite(B_OUT, 1);
  //   midi.sendNoteOff(Channel_3, 127);
  //   digitalWrite(B_OUT, 0);
  //   // Serial.println("B ON");
  //   // bNoteBegin = millis();
  //   // bNoteActive = 1;
  // }

  // if ((coincidenceNoteActive) && (millis() - coincidenceNoteBegin >= noteDuration)) {
  //   midi.sendNoteOff(Channel_1, 127);
  //   digitalWrite(COINCIDENCE_OUT, 0);
  //   // Serial.println("COINC OFF");
  //   coincidenceNoteActive = 0;
  // }
  // if ((aNoteActive) && (millis() - aNoteBegin >= noteDuration)) {
  //   midi.sendNoteOff(Channel_2, 127);
  //   digitalWrite(A_OUT, 0);
  //   // Serial.println("A OFF");
  //   aNoteActive = 0;
  // }
  // if ((bNoteActive) && (millis() - bNoteBegin >= noteDuration)) {
  //   midi.sendNoteOff(Channel_3, 127);
  //   digitalWrite(B_OUT, 0);
  //   // Serial.println("B OFF");
  //   bNoteActive = 0;
  // }
}