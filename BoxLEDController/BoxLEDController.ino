#include <Adafruit_NeoPXL8.h>

#define NUM_LEDS   100
#define NUM_STRIPS 2

// SCORPIO pins 0–7
int8_t pins[8] = { 16, 17, 18, 19, 20, 21, 22, 23 };
Adafruit_NeoPXL8 leds(NUM_LEDS, pins, NEO_GRB);

// Input pins
#define PIN_TRIGGER_WAVE  9
#define PIN_TOP_PULSE     10
#define PIN_BOTTOM_PULSE  11

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

  uint32_t delayBottom = 10; // ms
  float decay = 1000.0;      // ms

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

  int stripIndex;
  uint32_t color;
  uint32_t duration = 200;

public:
  StripPulse(int strip, uint32_t c) : stripIndex(strip), color(c) {}

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
        addPixel(i, brightness);
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
// Controller
// ---------------------------------------------------------------------------
class EffectController {
private:
  WavePacket wave;
  GlobalPulse globalPulse;
  StripPulse topPulse;
  StripPulse bottomPulse;

public:
  EffectController() :
    topPulse(0, leds.Color(255, 100, 0)),
    bottomPulse(1, leds.Color(255, 100, 0)) {}

  void begin() {
    leds.begin();
    leds.show();
  }

  void update() {
    uint32_t now = millis();

    // Trigger checks (all non-blocking, all can overlap)
    if (digitalRead(PIN_TRIGGER_WAVE) == LOW) {
      Serial.println("X");
      wave.trigger(now);
      globalPulse.trigger(now);
    }

    if (digitalRead(PIN_TOP_PULSE) == LOW){
      Serial.println("A");
      topPulse.trigger(now);
    }

    if (digitalRead(PIN_BOTTOM_PULSE) == LOW){
      Serial.println("B");
      bottomPulse.trigger(now);
    }

    // Frame render
    leds.clear();

    globalPulse.update(now);
    wave.update(now);
    topPulse.update(now);
    bottomPulse.update(now);

    leds.show();
  }
};

EffectController controller;

// ---------------------------------------------------------------------------
// Arduino Entry Points
// ---------------------------------------------------------------------------
void setup() {
  // pinMode(PIN_TRIGGER_WAVE, INPUT_PULLUP);
  pinMode(PIN_TOP_PULSE, INPUT_PULLUP);
  pinMode(PIN_BOTTOM_PULSE, INPUT_PULLUP);
  pinMode(PIN_TRIGGER_WAVE, INPUT);
  // pinMode(PIN_TOP_PULSE, INPUT);
  // pinMode(PIN_BOTTOM_PULSE, INPUT);  
  controller.begin();
  Serial.begin(9600);
  while (!Serial) {
    ; // Wait for Serial to connect (needed for native USB)
  }
  Serial.println("Hello from Adafruit Scorpio!");
}

void loop() {
  controller.update();
}