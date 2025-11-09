// Random serial sender: "A", "B" several times per second, and "X" occasionally

unsigned long lastABTime = 0;
unsigned long lastXTime = 0;

void setup() {
  Serial.begin(9600);
  randomSeed(analogRead(A0)); // Seed RNG
}

void loop() {
  unsigned long now = millis();

  // --- Send A or B randomly a few times per second ---
  if (now - lastABTime > random(150, 400)) {  // every 0.15–0.4 seconds
    if (random(2) == 0)
      Serial.println("A");
    else
      Serial.println("B");
    lastABTime = now;
    delay(random(10, 300));
  }

  // --- Send X once every few seconds ---
  if (now - lastXTime > random(3000, 7000)) {  // every 3–7 seconds
    Serial.println("X");
    lastXTime = now;
  }
}
