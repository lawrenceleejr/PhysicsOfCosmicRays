#define COINCIDENCE_PIN 3
#define TILE_A_PIN 4
#define TILE_B_PIN 5
#define LED_BUILTIN 13

volatile int counter = 0;
volatile bool eventDetected = false;

void setup() {
  pinMode(COINCIDENCE_PIN, INPUT_PULLUP);
  pinMode(TILE_A_PIN, INPUT_PULLUP);
  pinMode(TILE_B_PIN, INPUT_PULLUP);
  pinMode(LED_BUILTIN, OUTPUT);

  digitalWrite(LED_BUILTIN, LOW);
  Serial.begin(9600);
  
  // Attach interrupt to COINCIDENCE_PIN
  // CHANGE triggers on any state change, RISING for LOW→HIGH only
  attachInterrupt(digitalPinToInterrupt(COINCIDENCE_PIN), onCoincidence, RISING);
}

void onCoincidence() {
  counter++;
  eventDetected = true;
}

void loop() {
  if (eventDetected) {
    Serial.println("Coincidences: " + String(counter));
    eventDetected = false;
  }
}