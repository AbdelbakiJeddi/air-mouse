// imu-acc-calib — compute and print the accelerometer bias using the
// Mpu6050 library. Keep the sensor still on a flat surface (+Z up).

#include <Mpu6050.h>

Mpu6050 imu(9, 8);  // SDA=9, SCL=8

void setup() {
  Serial.begin(115200);
  imu.begin();
  delay(1000);

  Serial.println(F("Calibrating accelerometer... Keep the sensor still."));
  imu.calibrateAccel();

  Serial.println(F("Accelerometer Calibration Done:"));
  Serial.print(F("X Offset = ")); Serial.println(imu.accelOffsetX());
  Serial.print(F("Y Offset = ")); Serial.println(imu.accelOffsetY());
  Serial.print(F("Z Offset = ")); Serial.println(imu.accelOffsetZ());
}

void loop() {
  // Calibration prints once in setup().
}
