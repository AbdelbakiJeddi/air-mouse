// air-mouse-bt — Bluetooth Low Energy (BLE) HID Air Mouse.
// Transport: BLE HID mouse via HijelHID_BLEMouse (NimBLE stack).
// Sensor:    Mpu6050 library at ../lib/Mpu6050/Mpu6050.h.
//
// Dependencies (install via Arduino Library Manager):
//   1. NimBLE-Arduino  (>= 2.3.8)
//   2. HijelHID        (search "HijelHID")
//
// Wiring: MPU6050 SDA=9, SCL=8.

#include "../../lib/Mpu6050/Mpu6050.h"
#include <HijelHID_BLEMouse.h>

Mpu6050 imu(9, 8);
HijelBLEMouse mouse("Air Mouse");

// Onboard WS2812 status LED pin for ESP32-S3 Zero
#ifdef RGB_BUILTIN
  #define STATUS_LED_PIN RGB_BUILTIN
#else
  #define STATUS_LED_PIN 21
#endif

// ---- Laser-Pointer Tuning --------------------------------------------------

// Sensitivity for each axis (adjust to speed up / slow down cursor movement)
const float SENSITIVITY_X = 0.80f;
const float SENSITIVITY_Y = 0.8f;

// Smoothing factor (EMA). 0.1 = heavy filtering (smooth but laggy), 1.0 = no filtering.
const float SMOOTHING = 0.4f;

// Noise/tremor threshold in degrees/sec. Angular speeds below this are ignored.
const float GYRO_DEADZONE = 3.0f;

// Update rate in milliseconds (10 ms = 100 Hz filter tick)
const uint32_t UPDATE_MS = 10;

// ---- LED helpers ------------------------------------------------------------

// Purple blink interval while advertising (not yet paired)
const uint32_t BLINK_MS = 500;

void ledBlue()    { neopixelWrite(STATUS_LED_PIN, 0,  0,  40); }
void ledOrange()  { neopixelWrite(STATUS_LED_PIN, 40, 15, 0);  }
void ledPurple()  { neopixelWrite(STATUS_LED_PIN, 30, 0,  30); }
void ledGreen()   { neopixelWrite(STATUS_LED_PIN, 0,  30, 0);  }
void ledCyan()    { neopixelWrite(STATUS_LED_PIN, 0,  30, 30); }
void ledOff()     { neopixelWrite(STATUS_LED_PIN, 0,  0,  0);  }

// ---- Setup ------------------------------------------------------------------

void setup() {
  Serial.begin(115200);

  // Blue during hardware init
  ledBlue();

  imu.begin();
  delay(500);

  // Orange during calibration — keep device still!
  ledOrange();
  imu.calibrate();

  // Start BLE advertising
  mouse.begin();

  // Purple — waiting for a BLE connection
  ledPurple();
  Serial.println(F("Air-mouse BLE ready. Waiting for pairing..."));
}

// ---- Main loop --------------------------------------------------------------

void loop() {
  // ── Advertising state: blink purple until paired ──
  if (!mouse.isPaired()) {
    static uint32_t lastBlink = 0;
    static bool blinkOn = true;
    uint32_t now = millis();

    if (now - lastBlink >= BLINK_MS) {
      lastBlink = now;
      blinkOn = !blinkOn;
      if (blinkOn) ledPurple(); else ledOff();
    }
    return;  // nothing else to do until paired
  }

  // ── Paired: run the motion pipeline at UPDATE_MS intervals ──

  static bool justConnected = true;
  if (justConnected) {
    ledGreen();
    Serial.println(F("BLE paired! Cursor active."));
    justConnected = false;
  }

  static uint32_t lastUpdate = 0;
  uint32_t now = millis();

  if (now - lastUpdate < UPDATE_MS) return;
  lastUpdate = now;

  imu.read();

  // Read angular velocities directly:
  // - Yaw (around Z-axis) -> horizontal motion
  // - Pitch (around Y-axis) -> vertical motion
  float rawYaw = imu.gyroZ();
  float rawPitch = imu.gyroY();

  // Apply deadzone to filter out hand tremor and stationary sensor noise
  if (abs(rawYaw) < GYRO_DEADZONE) rawYaw = 0.0f;
  if (abs(rawPitch) < GYRO_DEADZONE) rawPitch = 0.0f;

  // EMA smoothing
  static float smoothYaw = 0.0f;
  static float smoothPitch = 0.0f;

  smoothYaw = SMOOTHING * rawYaw + (1.0f - SMOOTHING) * smoothYaw;
  smoothPitch = SMOOTHING * rawPitch + (1.0f - SMOOTHING) * smoothPitch;

  // Calculate target pixel movements with independent sensitivity
  float targetDx = -smoothYaw * SENSITIVITY_X;
  float targetDy =  smoothPitch * SENSITIVITY_Y;

  // Sub-pixel accumulator to prevent "stickiness" during very slow pointing
  static float subPx = 0.0f;
  static float subPy = 0.0f;

  subPx += targetDx;
  subPy += targetDy;

  int dx = (int)subPx;
  int dy = (int)subPy;

  // Keep the fractional remainder for the next tick
  subPx -= dx;
  subPy -= dy;

  // Visual status LED feedback based on movement
  static bool wasMoving = false;
  bool isMoving = (dx != 0 || dy != 0);
  if (isMoving != wasMoving) {
    if (isMoving) {
      ledCyan();   // Cyan for movement
    } else {
      ledGreen();  // Green for stationary
    }
    wasMoving = isMoving;
  }

  if (isMoving) {
    mouse.move(dx, dy);
  }

  // If BLE disconnects mid-session, reset state so we re-enter advertising blink
  if (!mouse.isPaired()) {
    justConnected = true;
    wasMoving = false;
    Serial.println(F("BLE disconnected. Re-advertising..."));
  }
}
