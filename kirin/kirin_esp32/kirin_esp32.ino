// ============================================================
// KIRIN — ESP32 Main Sketch (standalone, no TouchDesigner)
// MPU-6050 → gyro/accel → ST7789 TFT + LED eyes
// ============================================================
// Libraries needed (Arduino Library Manager):
//   Adafruit MPU6050         (+ Adafruit Unified Sensor)
//   Adafruit ST7735 and ST7789 Library
//   Adafruit NeoPixel
// ============================================================

#include <Wire.h>
#include <SPI.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ST7789.h>
#include <Adafruit_NeoPixel.h>

// ── PIN ASSIGNMENTS ─────────────────────────────────────────
// ST7789 TFT (SPI)
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4
// MOSI=23, SCLK=18 (ESP32 hardware SPI, no define needed)

// MPU-6050 (I2C)
#define I2C_SDA  21
#define I2C_SCL  22

// LED ring (WS2812 / NeoPixel)
#define LED_PIN   5
#define LED_COUNT 12   // ← 실제 LED 개수로 수정

// TFT 해상도
#define TFT_W 240
#define TFT_H 240

// ── OBJECTS ─────────────────────────────────────────────────
Adafruit_MPU6050  mpu;
Adafruit_ST7789   tft  = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
Adafruit_NeoPixel ring(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// ── STATE ───────────────────────────────────────────────────
float pitch  = 0;
float roll   = 0;
float energy = 0;  // 0.0 ~ 1.0, smoothed "life energy"

#define N_RINGS 6
float ringRadius[N_RINGS];
float ringAge[N_RINGS];

// ── SETUP ───────────────────────────────────────────────────
void setup() {
  Serial.begin(115200);

  Wire.begin(I2C_SDA, I2C_SCL);
  if (!mpu.begin()) {
    Serial.println("MPU-6050 not found — check wiring!");
    while (1) delay(10);
  }
  mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  Serial.println("MPU-6050 OK");

  tft.init(TFT_W, TFT_H);
  tft.setRotation(2);
  tft.fillScreen(ST77XX_BLACK);
  Serial.println("TFT OK");

  ring.begin();
  ring.setBrightness(100);
  ring.clear();
  ring.show();

  for (int i = 0; i < N_RINGS; i++) {
    ringAge[i]    = (float)i / N_RINGS;
    ringRadius[i] = 0;
  }
}

// ── MAIN LOOP ────────────────────────────────────────────────
void loop() {
  static uint32_t lastMs = 0;
  uint32_t now = millis();
  if (now - lastMs < 16) return;  // ~60fps
  lastMs = now;

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float ax = accel.acceleration.x;
  float ay = accel.acceleration.y;
  float az = accel.acceleration.z;
  float gx = gyro.gyro.x;
  float gy = gyro.gyro.y;

  pitch = atan2f(ay, az) * 180.0f / PI;
  roll  = atan2f(ax, az) * 180.0f / PI;

  float rawEnergy = (fabsf(gx) + fabsf(gy)) / 8.0f;
  rawEnergy = constrain(rawEnergy, 0.0f, 1.0f);
  energy = energy * 0.88f + rawEnergy * 0.12f;

  drawRingWaves(pitch, roll, energy);
  updateEyes(energy);

  Serial.printf("pitch=%.1f roll=%.1f energy=%.2f\n", pitch, roll, energy);
}

// ── TFT: EXPANDING RING WAVES ────────────────────────────────
uint16_t energyToColor(float e, float alpha = 1.0f) {
  uint8_t r = (uint8_t)(e * 255 * alpha);
  uint8_t g = (uint8_t)(e * 140 * alpha);
  uint8_t b = (uint8_t)((0.4f + (1.0f - e) * 0.6f) * 200 * alpha);
  return tft.color565(r, g, b);
}

void drawRingWaves(float pitch, float roll, float energy) {
  float speed = 0.5f + energy * 3.5f;
  int cx = (int)constrain(TFT_W / 2 + roll * 0.6f, 30, TFT_W - 30);
  int cy = (int)constrain(TFT_H / 2 + pitch * 0.6f, 30, TFT_H - 30);

  for (int i = 0; i < N_RINGS; i++) {
    int oldR = (int)ringRadius[i];
    if (oldR > 0) {
      tft.drawCircle(cx, cy, oldR,     ST77XX_BLACK);
      tft.drawCircle(cx, cy, oldR + 1, ST77XX_BLACK);
    }

    ringAge[i] += speed * 0.012f;
    ringRadius[i] = ringAge[i] * 120.0f;

    if (ringAge[i] >= 1.0f) {
      ringAge[i]    = 0.0f;
      ringRadius[i] = 0.0f;
    }

    float fade = 1.0f - ringAge[i];
    uint16_t col = energyToColor(energy * 0.5f + 0.5f, fade);
    int r = (int)ringRadius[i];
    tft.drawCircle(cx, cy, r,     col);
    tft.drawCircle(cx, cy, r + 1, col);
  }

  uint16_t coreCol = energyToColor(energy * 0.6f + 0.4f);
  int coreR = (int)(4 + energy * 8);
  tft.fillCircle(cx, cy, coreR, coreCol);
}

// ── LED EYES ─────────────────────────────────────────────────
void updateEyes(float energy) {
  float t = (float)millis() / 1000.0f;

  for (int i = 0; i < LED_COUNT; i++) {
    uint8_t r, g, b;
    if (energy < 0.15f) {
      float breath = (sinf(t * 1.4f) + 1.0f) * 0.5f;
      uint8_t lum  = (uint8_t)(18 + breath * 50);
      r = lum;
      g = lum;
      b = (uint8_t)min((int)(lum * 1.3f), 255);
    } else {
      uint8_t lum = (uint8_t)(60 + energy * 195);
      r = (uint8_t)(lum * (1.0f - energy * 0.5f));
      g = lum;
      b = lum;
    }
    ring.setPixelColor(i, ring.Color(r, g, b));
  }
  ring.show();
}
