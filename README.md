# ESP32-S3 BLE Air Mouse

A wireless air mouse built with an ESP32-S3, an MPU6050 motion sensor, and
Bluetooth Low Energy HID. Tilt or rotate the board to move the cursor. Two
buttons provide left and right mouse clicks.

## Hardware

- ESP32-S3 Zero
- MPU6050 IMU
- Two momentary push buttons

### Wiring

| Component | ESP32-S3 Zero |
| --- | --- |
| MPU6050 VCC | 3V3 |
| MPU6050 GND | GND |
| MPU6050 SDA | GPIO 8 |
| MPU6050 SCL | GPIO 9 |
| Left button | GPIO 4 to GND |
| Right button | GPIO 5 to GND |

The buttons use the ESP32 internal pull-up resistors.

## Required libraries

Install these libraries through the Arduino Library Manager:

- Adafruit MPU6050
- Adafruit Unified Sensor
- ESP32 BLE Mouse by T-vK

Select an ESP32-S3 board with Bluetooth Low Energy support before uploading.

## Upload and pair

1. Open `air-mouse.ino` in the Arduino IDE.
2. Select the ESP32-S3 Zero board and its USB port.
3. Upload the sketch.
4. Open the computer Bluetooth settings.
5. Pair with `ESP32-S3 Air Mouse`.
6. Move the board after the cursor becomes active.

The device advertises as a Bluetooth mouse and does not move the cursor until
it is connected.

## RGB LED states

The built-in RGB LED shows the current device state:

| Color | State |
| --- | --- |
| Blue | Starting up |
| Orange | MPU6050 setup |
| Red | MPU6050 initialization failed |
| Purple | Waiting for Bluetooth connection |
| Green | Connected and stationary |
| Cyan | Cursor movement detected |

## Buttons

- GPIO 4: left click
- GPIO 5: right click

Pressing a button connects it to GND and sends the corresponding Bluetooth
mouse button event.

## Tuning

Adjust these values near the top of `air-mouse.ino`:

- `sensitivity`: cursor speed
- `deadzone`: minimum movement threshold
- `alpha`: complementary filter weight
- `SDA_PIN` and `SCL_PIN`: I2C pins

The current loop runs at approximately 125 Hz.
