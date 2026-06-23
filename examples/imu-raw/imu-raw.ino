// imu-raw — stream raw-but-unit-converted IMU values to Serial.
// Demonstrates basic Mpu6050 library use (no calibration applied here).

#include <Mpu6050.h>

Mpu6050 imu(9, 8);  // SDA=9, SCL=8

void setup() {
  Serial.begin(115200);
  imu.begin();
  delay(1000);
  Serial.println(F("MPU6050 ready (no calibration)."));
}

void loop() {
  imu.read();

  Serial.print(F("Acc (g): "));
  Serial.print(imu.accelX(), 3); Serial.print(F(", "));
  Serial.print(imu.accelY(), 3); Serial.print(F(", "));
  Serial.println(imu.accelZ(), 3);

  Serial.print(F("Gyro (dps): "));
  Serial.print(imu.gyroX(), 3);  Serial.print(F(", "));
  Serial.print(imu.gyroY(), 3);  Serial.print(F(", "));
  Serial.println(imu.gyroZ(), 3);

  Serial.println(F("----------------------------"));
  delay(200);
}
