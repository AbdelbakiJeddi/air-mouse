// air-mouse — Relative Gyro Laser Pointer mode.
// HID:      USB HID mouse via the board's USB stack (ESP32-S2/S3, RP2040, Teensy).
// Sensor:   Mpu6050 library at ../lib/Mpu6050/Mpu6050.h (auto-resolved).
//
// Wiring: MPU6050 SDA=9, SCL=8.

#include "../../lib/Mpu6050/Mpu6050.h"
#include "USBHIDMouse.h"

Mpu6050 imu(9, 8);
USBHIDMouse Mouse;

// Onboard WS2812 status LED pin for ESP32-S3 Zero
#ifdef RGB_BUILTIN
  #define STATUS_LED_PIN RGB_BUILTIN
#else
  #define STATUS_LED_PIN 21
#endif

// ---- Laser-Pointer Tuning --------------------------------------------------

// Sensitivity for each axis (adjust to speed up / slow down cursor movement)
const float SENSITIVITY_X = 0.80f;
const float SENSITIVITY_Y = 1.6f;

// Smoothing factor (EMA). 0.1 = heavy filtering (smooth but laggy), 1.0 = no filtering.
const float SMOOTHING = 0.6f;

// Noise/tremor threshold in degrees/sec. Angular speeds below this are ignored.
const float GYRO_DEADZONE = 2.0f;

// Update rate in milliseconds (10 ms = 100 Hz filter tick)
const uint32_t UPDATE_MS = 10;

void setup() {
  Serial.begin(115200);

  // Set LED to Blue during initialization
  neopixelWrite(STATUS_LED_PIN, 0, 0, 40);

  imu.begin();
  delay(500);

  // Set LED to Orange/Red during calibration (keep device still)
  neopixelWrite(STATUS_LED_PIN, 40, 15, 0);
  imu.calibrate();        // Zero-offset calibration for gyro/accel. Must remain still.

  Mouse.begin();

  // Set LED to Green to indicate the air-mouse is ready and active
  neopixelWrite(STATUS_LED_PIN, 0, 30, 0);
  Serial.println(F("Air-mouse ready in Laser Pointer mode."));
}

void loop() {
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

  // Adaptive (speed-based) smoothing: alpha = min(SMOOTHING_MIN + SMOOTHING_SLOPE * combinedSpeed, SMOOTHING_MAX)
  static float smoothYaw = 0.0f;
  static float smoothPitch = 0.0f;

  smoothYaw = SMOOTHING * rawYaw + (1.0f - SMOOTHING) * smoothYaw;
  smoothPitch = SMOOTHING * rawPitch + (1.0f - SMOOTHING) * smoothPitch;

  // Calculate target pixel movements with independent sensitivity
  // - Yaw left -> cursor left. Yaw right -> cursor right.
  // - Pitch up -> cursor up. Pitch down -> cursor down.
  // Standard mouse coordinates: dx positive is right, dy positive is down.
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

  // Debug logging to serial monitor
  Serial.print("rawY="); Serial.print(rawYaw, 1);
  Serial.print(" rawP="); Serial.print(rawPitch, 1);
  Serial.print(" smY="); Serial.print(smoothYaw, 2);
  Serial.print(" smP="); Serial.print(smoothPitch, 2);
  Serial.print(" dx="); Serial.print(dx);
  Serial.print(" dy="); Serial.println(dy);

  // Visual status LED feedback based on movement
  static bool wasMoving = false;
  bool isMoving = (dx != 0 || dy != 0);
  if (isMoving != wasMoving) {
    if (isMoving) {
      neopixelWrite(STATUS_LED_PIN, 0, 30, 30); // Cyan for movement
    } else {
      neopixelWrite(STATUS_LED_PIN, 0, 30, 0);  // Green for stationary
    }
    wasMoving = isMoving;
  }

  if (isMoving) {
    Mouse.move(dx, dy, 0);
  }
}
