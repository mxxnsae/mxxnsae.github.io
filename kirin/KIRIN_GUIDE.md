# KIRIN 구현 가이드 — 이틀 플랜

## 아키텍처

```
MPU-6050 (I2C) ──┐
ST7789 TFT (SPI) ─┤  ESP32  ──── WiFi/UDP ──── TouchDesigner
WS2812 LEDs ──────┘                             (OSCin CHOP)
```

---

## 배선표

### MPU-6050 → ESP32
| MPU-6050 핀 | ESP32 핀 |
|-------------|----------|
| VCC         | 3.3V     |
| GND         | GND      |
| SDA         | GPIO 21  |
| SCL         | GPIO 22  |
| AD0         | GND (주소 0x68) |

### ST7789 TFT → ESP32
| TFT 핀 | ESP32 핀 |
|--------|----------|
| VCC    | 3.3V     |
| GND    | GND      |
| SCL    | GPIO 18 (VSPI CLK) |
| SDA    | GPIO 23 (VSPI MOSI) |
| RES    | GPIO 4   |
| DC     | GPIO 2   |
| CS     | GPIO 15  |
| BL     | 3.3V (항상 켜기) 또는 GPIO로 PWM 제어 |

### LED Ring → ESP32
| LED 핀 | ESP32 핀 |
|--------|----------|
| VCC    | 5V (or 3.3V if ring tolerates) |
| GND    | GND      |
| DIN    | GPIO 5   |

> **주의:** WS2812는 5V 동작. 3.3V ESP32 신호가 인식 안 되면 레벨 시프터(74HCT125) 필요. 또는 `LED_PIN` 앞에 330Ω 저항 삽입 후 테스트.

---

## 라이브러리 설치 (Arduino IDE → Library Manager)

```
Adafruit MPU6050
Adafruit Unified Sensor
Adafruit ST7735 and ST7789 Library
Adafruit NeoPixel
OSC  (by Adrian Freed, CNMAT)
```

ESP32 보드 패키지: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`  
보드 선택: **ESP32 Dev Module**

---

## 스케치 수정 사항 (kirin_esp32.ino)

```cpp
// 3군데만 수정
const char* WIFI_SSID     = "YOUR_SSID";       // ← 공유기 이름
const char* WIFI_PASSWORD = "YOUR_PASSWORD";    // ← 비밀번호
const char* TD_IP         = "192.168.0.XXX";   // ← TD 컴퓨터 IP
                                                //   (cmd: ipconfig / ifconfig)
#define LED_COUNT 12  // ← 실제 LED 개수로 수정
```

TFT 해상도가 240x240이 아닌 경우:
```cpp
tft.init(240, 240);  // 실제 픽셀 수로 변경 (e.g. 170, 320)
#define TFT_W 240
#define TFT_H 240
```

---

## TouchDesigner OSC 수신 설정

1. **OSCin CHOP** 추가
   - Port: `8000` (스케치의 `TD_PORT`와 일치)
   - Protocol: UDP
   - Active: On

2. 수신되는 채널명:
   ```
   /kirin/pitch    — 앞뒤 기울기 (-90 ~ +90°)
   /kirin/roll     — 좌우 기울기 (-90 ~ +90°)
   /kirin/energy   — 움직임 강도 (0.0 ~ 1.0)
   /kirin/accel/x, y, z
   /kirin/gyro/x, y
   ```

3. **기본 TD 연결 예시:**
   ```
   OSCin CHOP → Math CHOP (range map) → Null CHOP
                                              ↓
                                    Geo COMP scale / noise seed
                                    or Particle SOP force
                                    or GLSL TOP uniform
   ```

4. `energy` 채널 → Particle SOP의 `force` 또는 GLSL의 `uniform float uEnergy` 에 연결하면 기린 흔들림이 TD 비주얼을 격동시킴.

---

## 2일 타임라인

### Day 1 — 전자부품

| 시간 | 할 일 | 완료 기준 |
|------|-------|----------|
| AM   | 배선 연결 + Serial 디버그 | `pitch=XX roll=XX energy=XX` 시리얼 출력 확인 |
| 점심 | TFT 테스트 (링 웨이브 보임) | 기린 흔들 때 링 색상/속도 변함 |
| PM   | LED 눈 테스트 | calm=천천히 숨 쉼, shake=밝게 번쩍 |
| 저녁 | WiFi 연결 + TD OSCin 수신 | TD CHOP viewer에서 채널 값 변동 확인 |

### Day 2 — 기구 + 통합

| 시간 | 할 일 |
|------|-------|
| AM   | 타공 (기린 형상 → TFT 창, LED 눈 구멍) |
| AM   | ESP32 + 배선 내부 탑재, 배터리/전원 고정 |
| PM   | TD 비주얼 완성 (energy 값으로 파티클/노이즈 구동) |
| PM   | 전체 동선 테스트 (기린 들고 흔들기) |
| 저녁 | 마감 + 문서화 |

---

## Plan B — TFT가 너무 느리거나 조립이 어려울 때

### 상황별 대안

**TFT 애니메이션이 깜빡이거나 너무 느리면:**
→ TFT 제거, LED 링을 눈 2개로 배치하고 목/몸통에 **NeoPixel 스트립** 추가.
기린 목을 따라 에너지 리플이 올라가는 효과가 TFT보다 더 immersive.

```
몸통 하단 LED (조용) → 목 LED (흔들림 시 리플) → 눈 LED (강렬)
```

FastLED 기반 리플 코드:
```cpp
// 흔들림 감지 시 목 시작 픽셀부터 위로 에너지 전파
void rippleUp(float energy) {
  for (int i = 0; i < NECK_LEDS; i++) {
    float t = (float)i / NECK_LEDS;
    float wave = sin(millis() * 0.003f - t * PI) * energy;
    uint8_t lum = (uint8_t)(wave * 200 + 20);
    neck[i] = CRGB(lum * 0.5, lum, lum);
  }
  FastLED.show();
}
```

**WiFi OSC가 불안정하면:**
→ Serial OSC (`TouchDesigner → SerialDAT` 경유), 또는 TD 없이 ESP32 단독으로 TFT + LED만으로 완결된 경험 구성.

**부품 불량으로 MPU-6050 사용 불가하면:**
→ `analogRead`로 flex sensor / FSR 대체, 또는 가속도만 쓰는 간이 버전.

---

## 타공 가이드

- **TFT 창**: TFT 실제 뷰 영역 크기 + 1~2mm 여유로 직사각형 타공
  - 일반 ST7789 240x240: 뷰 27x27mm 내외 (실측 후 진행)
- **LED 눈**: 31mm 링 기준, **29~30mm 원형** 타공 후 링 뒤에서 밀어 넣기
- 재질에 따라: 아크릴/PLA → 드레멜/CNC. 폼/하드보드 → 커터칼 + 원형 커터

---

## 빠른 트러블슈팅

| 증상 | 원인 | 해결 |
|------|------|------|
| TFT 흰 화면 | DC/RST 핀 오연결 | 배선 재확인 |
| TFT 아무것도 안 뜸 | init 해상도 불일치 | `tft.init(W, H)` 수정 |
| MPU "not found" | I2C 주소 충돌 또는 배선 | AD0=GND 확인, `Wire.begin(21,22)` |
| LED 안 켜짐 | 레벨 시프터 문제 | 330Ω 저항 + 직결 테스트 |
| TD OSC 수신 안 됨 | 방화벽 or 포트 불일치 | Windows 방화벽 UDP 8000 허용 |
| WiFi 연결 안 됨 | SSID 오타 / 5GHz | 2.4GHz 네트워크 사용 (ESP32는 2.4GHz only) |
