// imu-calib — calibrate (accel + gyro) and stream unit-converted values.
// Demonstrates the full Mpu6050 library API: begin / calibrate / read.

#include <Mpu6050.h>

Mpu6050 imu(9, 8);  // SDA=9, SCL=8

void setup() {
  Serial.begin(115200);
  imu.begin();
  delay(1000);
  imu.calibrate();
  Serial.println(F("\nStarting Final Reading...\n"));
}

void loop() {
  imu.read();

  // Stream readable values (g, °/s) at ~50 Hz.
  static uint32_t lastPrint = 0;
  if (millis() - lastPrint >= 20) {
    lastPrint = millis();

    Serial.print(F("Acc(g): "));
    Serial.print(imu.accelX(), 3); Serial.print(F(", "));
    Serial.print(imu.accelY(), 3); Serial.print(F(", "));
    Serial.println(imu.accelZ(), 3);

    Serial.print(F("Gyro(dps): "));
    Serial.print(imu.gyroX(), 3);  Serial.print(F(", "));
    Serial.print(imu.gyroY(), 3);  Serial.print(F(", "));
    Serial.println(imu.gyroZ(), 3);

    Serial.println(F("---------------------"));
  }
}
