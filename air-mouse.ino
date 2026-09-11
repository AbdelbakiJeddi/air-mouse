#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BleMouse.h>

#define SDA_PIN 8
#define SCL_PIN 9
#define LEFT_BTN 4
#define RIGHT_BTN 5

#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48
#endif

float sensitivity = 1.8;
int deadzone = 3;
float alpha = 0.98;

Adafruit_MPU6050 mpu;
BleMouse bleMouse("ESP32-S3 Air Mouse", "xAI", 100);

float filteredX = 0;
float filteredY = 0;
float angleX = 0;
float angleY = 0;
unsigned long lastTime = 0;

void setLed(uint8_t red, uint8_t green, uint8_t blue)
{
    neopixelWrite(RGB_BUILTIN, red, green, blue);
}

void ledStarting()
{
    setLed(0, 0, 40);
}

void ledCalibrating()
{
    setLed(40, 15, 0);
}

void ledError()
{
    setLed(40, 0, 0);
}

void ledDisconnected()
{
    setLed(25, 0, 25);
}

void ledIdle()
{
    setLed(0, 35, 0);
}

void ledMoving()
{
    setLed(0, 30, 30);
}

void setup()
{
    Serial.begin(115200);
    delay(500);
    ledStarting();

    pinMode(LEFT_BTN, INPUT_PULLUP);
    pinMode(RIGHT_BTN, INPUT_PULLUP);

    Wire.begin(SDA_PIN, SCL_PIN);

    ledCalibrating();
    if (!mpu.begin())
    {
        ledError();
        while (1) delay(10);
    }

    mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

    bleMouse.begin();
    ledDisconnected();

    lastTime = micros();
}

void loop()
{
    if (!bleMouse.isConnected())
    {
        ledDisconnected();
        delay(50);
        return;
    }

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    unsigned long now = micros();
    float dt = (now - lastTime) / 1000000.0;
    lastTime = now;


    float accAngleX = atan2(a.acceleration.y, a.acceleration.z) * 180.0 / PI;  // Pitch
    float accAngleY = atan2(-a.acceleration.x, a.acceleration.z) * 180.0 / PI; // Roll

    angleX = alpha * (angleX + g.gyro.x * dt) + (1.0 - alpha) * accAngleX;
    angleY = alpha * (angleY + g.gyro.y * dt) + (1.0 - alpha) * accAngleY;

    static float prevAngleX = 0;
    static float prevAngleY = 0;

    float deltaX = angleX - prevAngleX;
    float deltaY = angleY - prevAngleY;

    prevAngleX = angleX;
    prevAngleY = angleY;

    int moveX = 0;
    int moveY = 0;

    if (abs(deltaX) > deadzone * 0.1)
    { 
        moveX = constrain((int)(deltaX * sensitivity * 10), -127, 127);
    }
    if (abs(deltaY) > deadzone * 0.1)
    {
        moveY = constrain((int)(deltaY * sensitivity * 10), -127, 127);
    }

    bleMouse.move(moveX, -moveY);

    if (moveX != 0 || moveY != 0)
    {
        ledMoving();
    }
    else
    {
        ledIdle();
    }

    static bool leftPressed = false;
    static bool rightPressed = false;

    bool leftState = digitalRead(LEFT_BTN) == LOW;
    bool rightState = digitalRead(RIGHT_BTN) == LOW;

    if (leftState && !leftPressed)
    {
        bleMouse.press(MOUSE_LEFT);
        leftPressed = true;
    }
    else if (!leftState && leftPressed)
    {
        bleMouse.release(MOUSE_LEFT);
        leftPressed = false;
    }

    if (rightState && !rightPressed)
    {
        bleMouse.press(MOUSE_RIGHT);
        rightPressed = true;
    }
    else if (!rightState && rightPressed)
    {
        bleMouse.release(MOUSE_RIGHT);
        rightPressed = false;
    }

    delay(8);
}

int constrained(float val)
{
    if (val > 127)
        return 127;
    if (val < -127)
        return -127;
    return (int)val;
}