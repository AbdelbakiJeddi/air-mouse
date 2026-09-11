#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleMouse.h>

// ===== Pin definitions (change if needed) =====
#define SDA_PIN 8
#define SCL_PIN 9
#define LEFT_BTN  4
#define RIGHT_BTN 5

// ===== Settings (tune these) =====
float sensitivity = 1.8;      // Higher = faster cursor
int deadzone = 3;             // Ignore small movements (reduce jitter)
float smooth = 0.75;          // 0.0–1.0 (higher = smoother but laggy)

Adafruit_MPU6050 mpu;
BleMouse bleMouse("ESP32-S3 Air Mouse", "xAI", 100);

// Filtered values
float filteredX = 0;
float filteredY = 0;

void setup() {
  Serial.begin(115200);
  delay(500);

  // Buttons with internal pull-up
  pinMode(LEFT_BTN, INPUT_PULLUP);
  pinMode(RIGHT_BTN, INPUT_PULLUP);

  // I2C
  Wire.begin(SDA_PIN, SCL_PIN);

  // MPU6050 init
  if (!mpu.begin()) {
    Serial.println("MPU6050 not found! Check wiring.");
    while (1) delay(10);
  }

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  Serial.println("MPU6050 ready");

  // Start BLE Mouse
  bleMouse.begin();
  Serial.println("BLE Mouse started. Pair it now!");
}

void loop() {
  if (!bleMouse.isConnected()) {
    delay(100);
    return;
  }

  // Read sensor
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Use gyroscope (better for air mouse movement)
  float gx = g.gyro.x;   // roll-ish
  float gy = g.gyro.y;   // pitch-ish

  // Simple low-pass filter
  filteredX = smooth * filteredX + (1.0 - smooth) * gx;
  filteredY = smooth * filteredY + (1.0 - smooth) * gy;

  // Apply deadzone
  int moveX = 0;
  int moveY = 0;

  if (abs(filteredX) > deadzone) {
    moveX = constrained(filteredX * sensitivity);
  }
  if (abs(filteredY) > deadzone) {
    moveY = constrained(filteredY * sensitivity);
  }

  // Invert axes if needed (test and flip signs)
  bleMouse.move(moveX, -moveY);   // try flipping signs if direction is wrong

  // Buttons
  static bool leftPressed = false;
  static bool rightPressed = false;

  bool leftState = digitalRead(LEFT_BTN) == LOW;
  bool rightState = digitalRead(RIGHT_BTN) == LOW;

  if (leftState && !leftPressed) {
    bleMouse.press(MOUSE_LEFT);
    leftPressed = true;
  } else if (!leftState && leftPressed) {
    bleMouse.release(MOUSE_LEFT);
    leftPressed = false;
  }

  if (rightState && !rightPressed) {
    bleMouse.press(MOUSE_RIGHT);
    rightPressed = true;
  } else if (!rightState && rightPressed) {
    bleMouse.release(MOUSE_RIGHT);
    rightPressed = false;
  }

  delay(8);   // ~125 Hz – good balance of speed & stability
}

// Helper: keep movement values in safe range for BleMouse
int constrained(float val) {
  if (val > 127) return 127;
  if (val < -127) return -127;
  return (int)val;
}