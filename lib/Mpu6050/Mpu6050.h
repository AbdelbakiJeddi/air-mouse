// Mpu6050.h — header-only driver for MPU6050 over I2C.
//
// Usage:
//   #include <Mpu6050.h>
//   Mpu6050 imu(9, 8);          // SDA, SCL pins
//   imu.begin();
//   imu.calibrate();            // run both accel + gyro bias, keep sensor still
//   // — or call just one —
//   imu.calibrateAccel();       // bias the accelerometer only
//   imu.calibrateGyro();        // bias the gyroscope only
//   // in loop():
//   imu.read();                 // burst-read accel + gyro, apply offsets
//   float ax_g = imu.accelX();  // in g
//   float ay_g = imu.accelY();
//   float az_g = imu.accelZ();
//   float gx_dps = imu.gyroX(); // in deg/s
//   float gy_dps = imu.gyroY();
//   float gz_dps = imu.gyroZ();
//
// Scales (fixed): accel ±2g (16384 LSB/g), gyro ±250 dps (131 LSB/dps).

#ifndef MPU6050_H
#define MPU6050_H

#include <Arduino.h>
#include <Wire.h>

#define MPU6050_ADDR        0x68
#define MPU6050_ACCEL_XOUT  0x3B
#define MPU6050_PWR_MGMT_1  0x6B
#define MPU6050_SMPLRT_DIV  0x19
#define MPU6050_CONFIG      0x1A

#define MPU6050_ACCEL_SCALE  16384.0f   // LSB per g    (±2g FS)
#define MPU6050_GYRO_SCALE   131.0f     // LSB per dps  (±250 dps FS)
#define MPU6050_ONE_G        16384      // raw LSB for 1g at ±2g

class Mpu6050 {
public:
  Mpu6050(uint8_t sda, uint8_t scl) : _sda(sda), _scl(scl) {}

  // Wake the sensor, set 1 kHz / DLPF=3, fast-mode I2C.
  void begin() {
    Wire.begin(_sda, _scl);
    Wire.setClock(400000);
    writeReg(MPU6050_PWR_MGMT_1, 0);  // wake
    writeReg(MPU6050_SMPLRT_DIV, 0);  // 1 kHz
    writeReg(MPU6050_CONFIG, 3);      // DLPF 44 Hz (fast settle)
    delay(100);
  }

  // Run both bias routines back-to-back. Keep the sensor still on a flat
  // surface with +Z up. Each routine takes ~1 s (200 samples @ 5 ms).
  void calibrate() {
    calibrateAccel();
    calibrateGyro();
  }

  // Run only the accelerometer bias routine. Keep the sensor still.
  void calibrateAccel() {
    long sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < 200; i++) {
      readRaw();
      sx += _ax;
      sy += _ay;
      sz += _az;
      delay(5);
    }
    _accOffX = sx / 200;
    _accOffY = sy / 200;
    _accOffZ = (sz / 200) - MPU6050_ONE_G;
  }

  // Run only the gyroscope bias routine. Keep the sensor still.
  void calibrateGyro() {
    long sx = 0, sy = 0, sz = 0;
    for (int i = 0; i < 200; i++) {
      readRaw();
      sx += _gx;
      sy += _gy;
      sz += _gz;
      delay(5);
    }
    _gyrOffX = sx / 200;
    _gyrOffY = sy / 200;
    _gyrOffZ = sz / 200;
  }

  // Burst-read 14 bytes (accel+temp+gyro) and apply offsets in one I2C txn.
  void read() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_ACCEL_XOUT);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom(MPU6050_ADDR, 14, true) < 14) return;

    _ax = read16() - _accOffX;
    _ay = read16() - _accOffY;
    _az = read16() - _accOffZ;
    read16();                                       // skip temp
    _gx = read16() - _gyrOffX;
    _gy = read16() - _gyrOffY;
    _gz = read16() - _gyrOffZ;
  }

  // Converted values (post-scale, offset-compensated).
  float accelX()  const { return _ax / MPU6050_ACCEL_SCALE; }  // g
  float accelY()  const { return _ay / MPU6050_ACCEL_SCALE; }
  float accelZ()  const { return _az / MPU6050_ACCEL_SCALE; }
  float gyroX()   const { return _gx / MPU6050_GYRO_SCALE;  }  // deg/s
  float gyroY()   const { return _gy / MPU6050_GYRO_SCALE;  }
  float gyroZ()   const { return _gz / MPU6050_GYRO_SCALE;  }

  // Raw offset-corrected LSBs (for advanced use).
  int16_t accelRawX()  const { return _ax; }
  int16_t accelRawY()  const { return _ay; }
  int16_t accelRawZ()  const { return _az; }
  int16_t gyroRawX()   const { return _gx; }
  int16_t gyroRawY()   const { return _gy; }
  int16_t gyroRawZ()   const { return _gz; }

  // Programmatic override of computed offsets (e.g. loaded from EEPROM).
  void setAccelOffsets(int16_t x, int16_t y, int16_t z) {
    _accOffX = x; _accOffY = y; _accOffZ = z;
  }
  void setGyroOffsets(int16_t x, int16_t y, int16_t z) {
    _gyrOffX = x; _gyrOffY = y; _gyrOffZ = z;
  }
  int16_t accelOffsetX() const { return _accOffX; }
  int16_t accelOffsetY() const { return _accOffY; }
  int16_t accelOffsetZ() const { return _accOffZ; }
  int16_t gyroOffsetX()  const { return _gyrOffX; }
  int16_t gyroOffsetY()  const { return _gyrOffY; }
  int16_t gyroOffsetZ()  const { return _gyrOffZ; }

private:
  static void writeReg(uint8_t reg, uint8_t val) {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(reg);
    Wire.write(val);
    Wire.endTransmission();
  }

  int16_t read16() {
    uint8_t msb = Wire.read();
    uint8_t lsb = Wire.read();
    return (int16_t)((msb << 8) | lsb);
  }

  // Internal raw read used by calibration (no offset applied).
  void readRaw() {
    Wire.beginTransmission(MPU6050_ADDR);
    Wire.write(MPU6050_ACCEL_XOUT);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom(MPU6050_ADDR, 14, true) < 14) return;
    _ax = read16();
    _ay = read16();
    _az = read16();
    read16(); // skip temp
    _gx = read16();
    _gy = read16();
    _gz = read16();
  }

  uint8_t _sda, _scl;
  int16_t _ax = 0, _ay = 0, _az = 0;
  int16_t _gx = 0, _gy = 0, _gz = 0;
  int16_t _accOffX = 0, _accOffY = 0, _accOffZ = 0;
  int16_t _gyrOffX = 0, _gyrOffY = 0, _gyrOffZ = 0;
};

#endif // MPU6050_H