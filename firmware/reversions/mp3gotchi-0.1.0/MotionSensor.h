#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

class MotionSensor {
public:
  bool begin();
  bool update();
  bool available() const { return found; }

private:
  Adafruit_MPU6050 mpu;
  bool found = false;
  unsigned long lastShakeMs = 0;
};
