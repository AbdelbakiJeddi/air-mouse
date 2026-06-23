// imu-gyr-calib — compute and print the gyroscope bias using the
// Mpu6050 library. Keep the sensor still on a flat surface.

#include <Mpu6050.h>

Mpu6050 imu(9, 8);  // SDA=9, SCL=8

void setup() {
  Serial.begin(115200);
  imu.begin();
  delay(1000);

  Serial.println(F("Calibrating gyroscope... Keep the sensor still."));
  imu.calibrateGyro();

  Serial.println(F("Gyroscope Calibration Done:"));
  Serial.print(F("X Offset = ")); Serial.println(imu.gyroOffsetX());
  Serial.print(F("Y Offset = ")); Serial.println(imu.gyroOffsetY());
  Serial.print(F("Z Offset = ")); Serial.println(imu.gyroOffsetZ());
}

void loop() {
  // Calibration prints once in setup().
}
