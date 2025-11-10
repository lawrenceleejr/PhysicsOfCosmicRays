#define X_IN 3 // Leonardo hardware interrupts 2, 3, 7 aren't the same as on the LVDS reciever.
#define A_IN 7 // Leonardo hardware interrupts.
#define B_IN 2 // Leonardo hardware interrupts.
#define X_OUT 8
#define A_OUT 12
#define B_OUT 13
#define FILMRELAY_OUT 9

#define GLOW_OUT 9
#define VO_OUT 10

volatile int xState;
volatile int aState;
volatile int bState;

int allowAB = true;
int allowX = true;

void setup() {
  pinMode(X_IN, INPUT_PULLUP);
  pinMode(X_OUT, OUTPUT);
  pinMode(A_IN, INPUT_PULLUP);
  pinMode(A_OUT, OUTPUT);
  pinMode(B_IN, INPUT_PULLUP);
  pinMode(B_OUT, OUTPUT);
  
  pinMode(FILMRELAY_OUT, OUTPUT);
  pinMode(GLOW_OUT, OUTPUT);
  pinMode(VO_OUT, OUTPUT);

  Serial.begin(9600);

  digitalWrite(FILMRELAY_OUT, HIGH);

  attachInterrupt(digitalPinToInterrupt(X_IN), onX, RISING);
  attachInterrupt(digitalPinToInterrupt(A_IN), onA, RISING);
  attachInterrupt(digitalPinToInterrupt(B_IN), onB, RISING);
}

void onX() {
  if (!allowX) return;
  digitalWrite(FILMRELAY_OUT, LOW);
  digitalWrite(X_OUT, 0);
  Serial.println("X");
  digitalWrite(X_OUT, 1);
  digitalWrite(FILMRELAY_OUT, HIGH);
  xState = 0;
}
void onA() {
  if (!allowAB) return;
  digitalWrite(A_OUT, 0);
  Serial.println("A");
  digitalWrite(A_OUT, 1);
  aState = 0;
}
void onB() {
  if (!allowAB) return;
  digitalWrite(B_OUT, 0);
  Serial.println("B");
  digitalWrite(B_OUT, 1);
  bState = 0;
}

void loop() {
  if (Serial.available() > 0) {
    char incoming = Serial.read();

    switch (incoming) {
      case 'R':  // disable both
        allowAB = false;
        allowX = false;
        Serial.println("R");
        break;

      case 'I':  // enable AB only
        allowAB = true;
        allowX = false;
        Serial.println("I");
        break;

      case 'J':  // enable both
        allowAB = true;
        allowX = true;
        Serial.println("J");
        break;

      case 'V':  // enable both
        Serial.println("V");
        digitalWrite(VO_OUT, 1);
        delay(5);
        digitalWrite(VO_OUT, 0);
        break;

      default:
        // ignore anything else
        break;
    }
  }
  if(allowAB)   digitalWrite(GLOW_OUT, 1);
  else digitalWrite(GLOW_OUT, 0);

}