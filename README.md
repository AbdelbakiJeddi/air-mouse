# air-mouse

A gyro-based air-mouse for ESP32-S2/S3 (and any Arduino core with native USB HID support — RP2040, Teensy). Tilt the IMU to move the cursor; the device appears as a USB mouse.

## Layout

```
air-mouse/
├── lib/
│   └── Mpu6050/          # Reusable header-only MPU6050 driver
│       ├── Mpu6050.h
│       └── keywords.txt
├── src/
│   └── air-mouse/        # Main Arduino sketch (the "main" of this project)
│       └── air-mouse.ino
└── examples/             # Standalone test / calibration sketches
    ├── imu-raw/
    ├── imu-acc-calib/
    ├── imu-gyr-calib/
    └── imu-calib/
```

## Running the main sketch

Open `src/air-mouse/air-mouse.ino` in the Arduino IDE. The library folder is auto-detected, so no manual install is needed. Select your board (e.g. ESP32-S3) and upload.

The sketch:
- Wakes the MPU6050 and runs an accel + gyro bias calibration (keep the board still — the on-board LED turns orange).
- Streams the live accel / gyro values to Serial.
- Maps yaw → cursor X, pitch → cursor Y, with a deadzone, EMA smoothing, and a sub-pixel accumulator.
- Drives the cursor via USB HID.

### Tuning

In `src/air-mouse/air-mouse.ino`:

| Constant       | Meaning                                                |
| -------------- | ------------------------------------------------------ |
| `SENSITIVITY_X` / `SENSITIVITY_Y` | Cursor speed per axis. Higher = faster.  |
| `SMOOTHING`    | EMA factor. Lower = smoother but laggier.              |
| `GYRO_DEADZONE`| Ignore angular speeds below this many deg/s.           |
| `UPDATE_MS`    | Filter tick period. `10` = 100 Hz.                     |

## Wiring

| MPU6050 | Board        |
| ------- | ------------ |
| VCC     | 3V3          |
| GND     | GND          |
| SDA     | GPIO 9       |
| SCL     | GPIO 8       |

## Examples

Each example lives in its own folder and can be opened standalone.

| Sketch                  | What it does                                                        |
| ----------------------- | ------------------------------------------------------------------- |
| `examples/imu-raw`      | Streams accel/gyro at 5 Hz. No calibration. Quick sanity check.     |
| `examples/imu-acc-calib`| Computes and prints the accelerometer bias only.                     |
| `examples/imu-gyr-calib`| Computes and prints the gyroscope bias only.                        |
| `examples/imu-calib`    | Full accel + gyro calibration, then streams calibrated readings.    |

## Mpu6050 library API

```cpp
#include <Mpu6050.h>

Mpu6050 imu(9, 8);          // SDA, SCL
imu.begin();                // wake, configure, set DLPF
imu.calibrate();            // accel + gyro bias (keep still)
imu.calibrateAccel();       // accel only
imu.calibrateGyro();        // gyro only
imu.read();                 // burst-read, offsets applied

float ax_g  = imu.accelX(); // g
float ay_g  = imu.accelY();
float az_g  = imu.accelZ();
float gx_dps = imu.gyroX(); // deg/s
float gy_dps = imu.gyroY();
float gz_dps = imu.gyroZ();

// Programmatic offsets (e.g. loaded from EEPROM)
imu.setAccelOffsets(ax, ay, az);
imu.setGyroOffsets(gx, gy, gz);
```

Scales are fixed: accel ±2 g (16384 LSB/g), gyro ±250 dps (131 LSB/dps).
