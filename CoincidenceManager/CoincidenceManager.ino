#define X_IN 3 // Leonardo hardware interrupts 2, 3, 7 aren't the same as on the LVDS reciever.
#define A_IN 7 // Leonardo hardware interrupts.
#define B_IN 2 // Leonardo hardware interrupts.
#define X_OUT 8
#define A_OUT 12
#define B_OUT 13

volatile int xState;
volatile int aState;
volatile int bState;

int delayDuration = 20; //ms

// volatile int xCounter;
// volatile int aCounter;
// volatile int bCounter;

void setup() {
  pinMode(X_IN, INPUT_PULLUP);
  pinMode(X_OUT, OUTPUT);
  pinMode(A_IN, INPUT_PULLUP);
  pinMode(A_OUT, OUTPUT);
  pinMode(B_IN, INPUT_PULLUP);
  pinMode(B_OUT, OUTPUT);
  Serial.begin(9600);

  attachInterrupt(digitalPinToInterrupt(X_IN), onX, RISING);
  attachInterrupt(digitalPinToInterrupt(A_IN), onA, RISING);
  attachInterrupt(digitalPinToInterrupt(B_IN), onB, RISING);
}

void onX() {
  // xCounter++;
  xState = 1;
}
void onA() {
  // aCounter++;
  aState = 1;
}
void onB() {
  // bCounter++;
  bState = 1;
}

void loop() {
  if (xState == 1) {
    digitalWrite(X_OUT, 0);
    delay(delayDuration);
    Serial.println("X");
    digitalWrite(X_OUT, 1);
  } 
  if (aState == 1) {
    digitalWrite(A_OUT, 0);
    delay(delayDuration);
    Serial.println("A");
    digitalWrite(A_OUT, 1);
  } 
  if (bState == 1) {
    digitalWrite(B_OUT, 0);
    delay(delayDuration);
    Serial.println("B");
    digitalWrite(B_OUT, 1);
  } 

  xState = 0;
  aState = 0;
  bState = 0;
}