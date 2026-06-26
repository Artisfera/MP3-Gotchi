#include "MotionSensor.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include <math.h>

bool MotionSensor::begin() {
  Wire.begin(Pins::MPU_SDA, Pins::MPU_SCL);
  found = mpu.begin(0x68, &Wire);

  if (!found) return false;

  mpu.setAccelerometerRange(MPU6050_RANGE_4_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_94_HZ);
  return true;
}

bool MotionSensor::update() {
  if (!found) return false;

  sensors_event_t accel, gyro, temp;
  mpu.getEvent(&accel, &gyro, &temp);

  float accelMagnitude = sqrtf(
    accel.acceleration.x * accel.acceleration.x +
    accel.acceleration.y * accel.acceleration.y +
    accel.acceleration.z * accel.acceleration.z
  );

  float gyroMagnitude = sqrtf(
    gyro.gyro.x * gyro.gyro.x +
    gyro.gyro.y * gyro.gyro.y +
    gyro.gyro.z * gyro.gyro.z
  );

  unsigned long now = millis();
  float accelChange = hasAccelSample ? fabsf(accelMagnitude - lastAccelMagnitude) : 0.0f;
  lastAccelMagnitude = accelMagnitude;
  hasAccelSample = true;

  if ((now - lastShakeMs) < Config::SHAKE_COOLDOWN_MS) return false;

  bool strongAccel =
    fabsf(accelMagnitude - 9.80665f) > Config::SHAKE_ACCEL_DELTA_THRESHOLD ||
    accelChange > Config::SHAKE_ACCEL_CHANGE_THRESHOLD;
  bool strongGyro = gyroMagnitude > Config::SHAKE_GYRO_THRESHOLD;

  if (strongAccel || strongGyro) {
    lastShakeMs = now;
    return true;
  }

  return false;
}
