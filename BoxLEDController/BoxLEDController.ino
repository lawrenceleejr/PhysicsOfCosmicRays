#include <Adafruit_NeoPXL8.h>

#define NUM_LEDS   100
#define NUM_STRIPS 3


// Input pins
#define PIN_TRIGGER_WAVE  9
#define PIN_TOP_PULSE     10
#define PIN_BOTTOM_PULSE  11

// Inputs
#define PIN_AMB  5
#define PIN_VO_PULSE      6  // momentary pulse when audio crosses threshold

float led2Brightness = 0.0;     // current brightness (0–1)
bool pulseActive = false;
unsigned long pulseStart = 0;
const unsigned long pulseDuration = 1000;  // ms
const float glowBrightness = 0.3;          // 30%

volatile bool triggerWaveFlag = false;
volatile bool triggerTopFlag = false;
volatile bool triggerBottomFlag = false;
volatile bool triggerVOFlag = false;
volatile bool turnOnAmb = false;
volatile bool turnOffAmb = false;


void onWaveTrigger() {
  triggerWaveFlag = true;
}

void onTopTrigger() {
  triggerTopFlag = true;
}

void onBottomTrigger() {
  triggerBottomFlag = true;
}

void onVOTrigger() {
  triggerVOFlag = true;
}

void onAmbOn() {
  turnOnAmb = true;
}

void onAmbOff() {
  turnOffAmb = true;
}

// SCORPIO pins 0–7
int8_t pins[8] = { 16, 17, 18, 19, 20, 21, 22, 23 };
Adafruit_NeoPXL8 leds(NUM_LEDS, pins, NEO_GRB);

int led = LED_BUILTIN;


unsigned long lastSendTime = 0;
const unsigned long timeout = 5000; // ms

// ---------------------------------------------------------------------------
// Base Effect Class
// ---------------------------------------------------------------------------
class Effect {
public:
  virtual void trigger(uint32_t now) = 0;
  virtual void update(uint32_t now) = 0;
  virtual bool isActive() const = 0;
  virtual ~Effect() {}
};

// ---------------------------------------------------------------------------
// Multi-instance Global Pulse (soft white flash)
// ---------------------------------------------------------------------------
class GlobalPulse : public Effect {
private:
  struct Instance {
    uint32_t startTime;
    uint32_t duration;
    uint32_t color;
  };
  static const int MAX = 8;
  Instance inst[MAX];
  int count = 0;

public:
  void trigger(uint32_t now) override {
    if (count >= MAX) return;
    inst[count++] = { now, 1500, leds.Color(100, 100, 100) };
  }

  void update(uint32_t now) override {
    if (count == 0) return;
    for (int i = 0; i < count; i++) {
      float t = now - inst[i].startTime;
      if (t > inst[i].duration) continue;

      float intensity = exp(-t / 800.0);
      uint8_t r = ((inst[i].color >> 16) & 0xFF) * intensity;
      uint8_t g = ((inst[i].color >> 8) & 0xFF) * intensity;
      uint8_t b = (inst[i].color & 0xFF) * intensity;

      for (int s = 0; s < NUM_STRIPS; s++)
        for (int p = 0; p < NUM_LEDS; p++)
          leds.setPixelColor(s * NUM_LEDS + p,
            leds.getPixelColor(s * NUM_LEDS + p) |
            leds.Color(r, g, b));
    }
    cleanup(now);
  }

  bool isActive() const override { return count > 0; }

private:
  void cleanup(uint32_t now) {
    int j = 0;
    for (int i = 0; i < count; i++)
      if (now - inst[i].startTime <= inst[i].duration)
        inst[j++] = inst[i];
    count = j;
  }
};

// ---------------------------------------------------------------------------
// Multi-instance Wave Packets (top then bottom)
// ---------------------------------------------------------------------------
class WavePacket : public Effect {
private:
  struct Instance {
    uint32_t startTime;
    uint32_t color;
  };
  static const int MAX = 8;
  Instance inst[MAX];
  int count = 0;

  uint32_t delayBottom = 200; // ms
  float decay = 800.0;      // ms

public:
  void trigger(uint32_t now) override {
    if (count >= MAX) return;
    inst[count++] = { now, leds.Color(0, 100, 255) };
  }

  void update(uint32_t now) override {
    if (count == 0) return;
    for (int w = 0; w < count; w++) {
      float t = now - inst[w].startTime;
      if (t > decay) continue;

      float progress = t / decay;
      int centerTop = progress * NUM_LEDS;
      int centerBottom = progress * NUM_LEDS;

      // Top strand
      for (int i = 0; i < NUM_LEDS; i++) {
        float dist = abs(i - centerTop);
        float brightness = exp(-dist / 8.0) * (1.0 - progress);
        addPixel(0, i, inst[w].color, brightness);
      }

      // Bottom strand (delayed)
      if (t > delayBottom) {
        for (int i = 0; i < NUM_LEDS; i++) {
          float dist = abs(i - centerBottom);
          float brightness = exp(-dist / 8.0) * (1.0 - progress);
          addPixel(1, i, inst[w].color, brightness);
        }
      }
    }
    cleanup(now);
  }

  bool isActive() const override { return count > 0; }

private:
  void addPixel(int strip, int i, uint32_t color, float brightness) {
    uint8_t r = ((color >> 16) & 0xFF) * brightness;
    uint8_t g = ((color >> 8) & 0xFF) * brightness;
    uint8_t b = (color & 0xFF) * brightness;
    leds.setPixelColor(strip * NUM_LEDS + i,
      leds.getPixelColor(strip * NUM_LEDS + i) | leds.Color(r, g, b));
  }

  void cleanup(uint32_t now) {
    int j = 0;
    for (int i = 0; i < count; i++)
      if (now - inst[i].startTime <= decay)
        inst[j++] = inst[i];
    count = j;
  }
};

// ---------------------------------------------------------------------------
// Multi-instance Quick Strip Pulses
// ---------------------------------------------------------------------------
class StripPulse : public Effect {
private:
  struct Instance {
    uint32_t startTime;
  };
  static const int MAX = 16;
  Instance inst[MAX];
  int count = 0;


public:
  StripPulse(int strip, uint32_t c) : stripIndex(strip), color(c) {}
  float brightnessFraction = 0.1;
  uint32_t duration = 100;
  uint32_t color;
  int stripIndex;

  void trigger(uint32_t now) override {
    if (count >= MAX) return;
    inst[count++] = { now };
  }

  void update(uint32_t now) override {
    if (count == 0) return;
    for (int k = 0; k < count; k++) {
      float t = now - inst[k].startTime;
      if (t > duration) continue;
      float brightness = exp(-t / 100.0);
      for (int i = 0; i < NUM_LEDS; i++)
        addPixel(i, brightness*brightnessFraction);
    }
    cleanup(now);
  }

  bool isActive() const override { return count > 0; }

private:
  void addPixel(int i, float brightness) {
    uint8_t r = ((color >> 16) & 0xFF) * brightness;
    uint8_t g = ((color >> 8) & 0xFF) * brightness;
    uint8_t b = (color & 0xFF) * brightness;
    leds.setPixelColor(stripIndex * NUM_LEDS + i,
      leds.getPixelColor(stripIndex * NUM_LEDS + i) | leds.Color(r, g, b));
  }

  void cleanup(uint32_t now) {
    int j = 0;
    for (int i = 0; i < count; i++)
      if (now - inst[i].startTime <= duration)
        inst[j++] = inst[i];
    count = j;
  }
};

// ---------------------------------------------------------------------------
// Smooth On/Off Glow Effect (inherits from StripPulse)
// ---------------------------------------------------------------------------
class StripGlow : public StripPulse {
private:
  bool targetOn = false;
  float brightness = 0.0f;
  float fadeSpeed = 0.02f; // rate per frame; smaller = slower
  float maxFraction;

public:
  StripGlow(int strip, uint32_t c, float maxBright = 0.3f)
    : StripPulse(strip, c), maxFraction(maxBright) {}

  void on()  { targetOn = true; }
  void off() { targetOn = false; }

  void update(uint32_t now) override {
    // smoothly approach target brightness
    float target = targetOn ? maxFraction : 0.0f;
    brightness += (target - brightness) * fadeSpeed;

    if (brightness < 0.001f && !targetOn) brightness = 0; // cleanup small noise

    // Apply brightness to all LEDs on the strip
    for (int i = 0; i < NUM_LEDS; i++) {
      uint8_t r = ((color >> 16) & 0xFF) * brightness;
      uint8_t g = ((color >> 8) & 0xFF) * brightness;
      uint8_t b = (color & 0xFF) * brightness;
      leds.setPixelColor(stripIndex * NUM_LEDS + i, r, g, b);
    }
  }

  bool isActive() const override {
    // Consider active while glowing or fading
    return (brightness > 0.001f) || targetOn;
  }
};

// ---------------------------------------------------------------------------
// Controller
// ---------------------------------------------------------------------------
class EffectController {
private:
  WavePacket wave;
  GlobalPulse globalPulse;
  StripPulse topPulse;
  StripPulse bottomPulse;
  StripPulse voPulse;
  StripPulse amb;

public:
  EffectController() :
    amb(2, leds.Color(255, 255, 255)),
    voPulse(2, leds.Color(255, 255, 255)),
    topPulse(0, leds.Color(255, 100, 0)),
    bottomPulse(1, leds.Color(255, 100, 0)) {}

  void begin() {
    voPulse.brightnessFraction = 1;
    voPulse.duration = 300;
    amb.brightnessFraction = 1;
    // amb.duration = 2000;
    leds.begin();
    leds.show();
  }

  void update() {
    uint32_t now = millis();
    String serialIn = "";  //read until timeout

    // if(Serial.available()) {     //wait for data available
    //   String serialIn = Serial.readString();  //read until timeout
    //   serialIn.trim();                        // remove any \r \n whitespace at the end of the String
    // }

    // Trigger checks (all non-blocking, all can overlap)
    if (triggerWaveFlag || serialIn=="X") {
      noInterrupts();
      triggerWaveFlag = false;
      interrupts();
      // Serial.println("X");
      // delay(5);
      wave.trigger(now);
      // globalPulse.trigger(now);
    }

    if (triggerTopFlag || serialIn=="A"){
      noInterrupts();
      triggerTopFlag = false;
      interrupts();
      // Serial.println("A");
      // delay(5);
      topPulse.trigger(now);
    }

    if (triggerBottomFlag || serialIn=="B"){
      noInterrupts();
      triggerBottomFlag = false;
      interrupts();
      // Serial.println("B");
      // delay(5);
      bottomPulse.trigger(now);
    }


    if (triggerVOFlag){
      noInterrupts();
      triggerVOFlag = false;
      interrupts();
      // Serial.println("B");
      // delay(5);
      voPulse.trigger(now);
    }

    if(turnOnAmb){
      noInterrupts();
      turnOnAmb = false;
      interrupts();
      amb.trigger(now);
    }
    // if(turnOffAmb){
    //   noInterrupts();
    //   turnOffAmb = false;
    //   interrupts();
    //   amb.off();
    // }

    // if (serialIn=="W"){
    //   Serial.println("W");
    //   bottomPulse.trigger(now);
    // }

    // Frame render
    leds.clear();

    globalPulse.update(now);
    wave.update(now);
    topPulse.update(now);
    bottomPulse.update(now);
    voPulse.update(now);
    amb.update(now);

    leds.show();
  }
};

EffectController controller;

StripGlow amb(2, leds.Color(255, 255, 255));


// ---------------------------------------------------------------------------
// Arduino Entry Points
// ---------------------------------------------------------------------------
void setup() {
  pinMode(PIN_TOP_PULSE, INPUT);
  pinMode(PIN_BOTTOM_PULSE, INPUT);
  pinMode(PIN_TRIGGER_WAVE, INPUT);
  pinMode(PIN_VO_PULSE, INPUT);
  pinMode(PIN_AMB, INPUT);
  // pinMode(PIN_TRIGGER_WAVE, INPUT_PULLUP);
  // pinMode(PIN_TOP_PULSE, INPUT);
  // pinMode(PIN_BOTTOM_PULSE, INPUT);
  // pinMode(PIN_TOP_PULSE, INPUT);
  // pinMode(PIN_BOTTOM_PULSE, INPUT);  
  attachInterrupt(digitalPinToInterrupt(PIN_TRIGGER_WAVE), onWaveTrigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_TOP_PULSE), onTopTrigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_BOTTOM_PULSE), onBottomTrigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(PIN_VO_PULSE), onVOTrigger, FALLING);
  // attachInterrupt(digitalPinToInterrupt(PIN_AMB), onAmbOn, FALLING);
  // attachInterrupt(digitalPinToInterrupt(PIN_AMB), onAmbOff, RISING);



  controller.begin();
  Serial.begin(9600);
  Serial.setTimeout(100);
  // while (!Serial) {
  //   ; // Wait for Serial to connect (needed for native USB)
  // }
  // delay(1000);
  Serial.println("Connected to Adafruit Scorpio");
  delay(5);

  digitalWrite(led, HIGH);   // turn the LED on (HIGH is the voltage level)
  delay(100);                // wait for a half second
  digitalWrite(led, LOW);    // turn the LED off by making the voltage LOW
}

void loop() {
  uint32_t now = millis();
  controller.update();
  // if(digitalRead(PIN_AMB)) amb.on();
  // else amb.off();
  // amb.update(now);
  int stripIndex=2;
  float brightnessFraction = 0.03;
  if(digitalRead(PIN_AMB)){
    for(int i=0; i<NUM_LEDS; i++){
      leds.setPixelColor(stripIndex * NUM_LEDS + i,
        leds.getPixelColor(stripIndex * NUM_LEDS + i) | leds.Color(255*brightnessFraction, 255*brightnessFraction, 255*brightnessFraction));
    }
  } else {
    for(int i=0; i<NUM_LEDS; i++){
      leds.setPixelColor(stripIndex * NUM_LEDS + i,
        leds.getPixelColor(stripIndex * NUM_LEDS + i) | leds.Color(0,0,0));
    }
  }
  leds.show();

  // if (!Serial||!Serial.dtr()){
  //   // Serial.end();
  //   // Serial.begin(9600);
  //   while (!Serial) {
  //     ; // Wait for Serial to connect (needed for native USB)
  //   }
  //   Serial.println("Reconnected");
  //   delay(20);
  // }
  // delay(10000);

  // if(Serial.available()) {     //wait for data available
  //     String serialIn = Serial.readString();  //read until timeout
  //     // serialIn.trim();                        // remove any \r \n whitespace at the end of the String
  //     Serial.println(serialIn);
  //     if(serialIn.indexOf("R") >= 0){
  //         Serial.end();
  //         Serial.begin(9600);
  //     }
  // }


  // if (millis() - lastSendTime > 100) {
  //   if (Serial.dtr()) {          // Host is listening
  //     Serial.println("PING");
  //     lastSendTime = millis();
  //   }
  // }

  // // Check if host has stopped reading for too long
  // if (millis() - lastSendTime > timeout) {
  //   // Reset USB CDC stack
  //   Serial.end();
  //   delay(50);
  //   Serial.begin(9600);
  //   lastSendTime = millis(); // reset timer
  // }
}