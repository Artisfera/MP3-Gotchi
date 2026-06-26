#include "MotionSensor.h"
#include "PinMap.h"
#include "ProjectConfig.h"
#include <math.h>

bool MotionSensor::begin(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
  found = false;
  Wire.end();
  delay(10);
  pinMode(sdaPin, INPUT_PULLUP);
  pinMode(sclPin, INPUT_PULLUP);
  Wire.begin(sdaPin, sclPin, frequency);
  hasAccelSample = false;
  lastAccelMagnitude = 9.80665f;
  currentFrequency = frequency;

  const uint8_t addresses[] = {0x68, 0x69};
  for (uint8_t i = 0; i < sizeof(addresses); i++) {
    InitStage failedStage = InitStage::Address;
    uint8_t who = 0;
    Serial.print("MPU6050 try SDA GPIO");
    Serial.print(sdaPin);
    Serial.print(", SCL GPIO");
    Serial.print(sclPin);
    Serial.print(", I2C ");
    Serial.print(frequency);
    Serial.print(", address 0x");
    Serial.println(addresses[i], HEX);

    if (beginAtAddress(addresses[i], failedStage, who)) {
      found = true;
      currentAddress = addresses[i];
      currentWhoAmI = who;
      Serial.print("MPU6050 OK address 0x");
      Serial.print(currentAddress, HEX);
      Serial.print(", WHO_AM_I 0x");
      Serial.println(currentWhoAmI, HEX);
      return true;
    }

    Serial.print("MPU6050 fail at ");
    Serial.print(stageName(failedStage));
    Serial.print(", WHO_AM_I 0x");
    Serial.println(who, HEX);
  }

  return false;
}

void MotionSensor::scanBus(uint8_t sdaPin, uint8_t sclPin, uint32_t frequency) {
  Wire.end();
  delay(10);
  pinMode(sdaPin, INPUT_PULLUP);
  pinMode(sclPin, INPUT_PULLUP);
  Wire.begin(sdaPin, sclPin, frequency);

  Serial.print("I2C scan SDA GPIO");
  Serial.print(sdaPin);
  Serial.print(", SCL GPIO");
  Serial.print(sclPin);
  Serial.print(", I2C ");
  Serial.println(frequency);

  uint8_t foundCount = 0;
  for (uint8_t address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    uint8_t error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("  ACK 0x");
      if (address < 0x10) Serial.print('0');
      Serial.println(address, HEX);
      foundCount++;
    }
  }

  if (foundCount == 0) {
    Serial.println("  No I2C devices found.");
  }
}

bool MotionSensor::addressResponds(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool MotionSensor::writeReg(uint8_t address, uint8_t reg, uint8_t value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool MotionSensor::readReg(uint8_t address, uint8_t reg, uint8_t& value) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(address, (uint8_t)1) != 1) return false;
  value = Wire.read();
  return true;
}

bool MotionSensor::readBytes(uint8_t address, uint8_t reg, uint8_t* buffer, size_t len) {
  Wire.beginTransmission(address);
  Wire.write(reg);
  if (Wire.endTransmission(false) != 0) return false;
  if ((size_t)Wire.requestFrom(address, (uint8_t)len) != len) return false;
  for (size_t i = 0; i < len; i++) {
    buffer[i] = Wire.read();
  }
  return true;
}

bool MotionSensor::beginAtAddress(uint8_t address, InitStage& failedStage, uint8_t& who) {
  failedStage = InitStage::Address;
  if (!addressResponds(address)) return false;

  failedStage = InitStage::WhoAmI;
  if (!readReg(address, 0x75, who)) return false;
  if (who != 0x68 && who != 0x70 && who != 0x71) return false;

  failedStage = InitStage::Wake;
  if (!writeReg(address, 0x6B, 0x00)) return false;
  delay(100);

  // Accelerometer +/-4g, gyro +/-250 dps, DLPF around 94 Hz.
  writeReg(address, 0x1C, 0x08);
  writeReg(address, 0x1B, 0x00);
  writeReg(address, 0x1A, 0x02);

  failedStage = InitStage::ReadSample;
  uint8_t raw[14] = {0};
  if (!readBytes(address, 0x3B, raw, sizeof(raw))) return false;

  failedStage = InitStage::Ok;
  return true;
}

const char* MotionSensor::stageName(InitStage stage) {
  switch (stage) {
    case InitStage::Address: return "address ACK";
    case InitStage::WhoAmI: return "WHO_AM_I";
    case InitStage::Wake: return "wake write";
    case InitStage::ReadSample: return "sample read";
    case InitStage::Ok: return "ok";
  }
  return "unknown";
}

bool MotionSensor::update() {
  if (!found) return false;

  uint8_t raw[14] = {0};
  if (!readBytes(currentAddress, 0x3B, raw, sizeof(raw))) return false;

  int16_t axRaw = (int16_t)((raw[0] << 8) | raw[1]);
  int16_t ayRaw = (int16_t)((raw[2] << 8) | raw[3]);
  int16_t azRaw = (int16_t)((raw[4] << 8) | raw[5]);
  int16_t gxRaw = (int16_t)((raw[8] << 8) | raw[9]);
  int16_t gyRaw = (int16_t)((raw[10] << 8) | raw[11]);
  int16_t gzRaw = (int16_t)((raw[12] << 8) | raw[13]);

  constexpr float G_TO_MS2 = 9.80665f;
  constexpr float ACCEL_SCALE_4G = 8192.0f;
  constexpr float GYRO_SCALE_250DPS = 131.0f;
  constexpr float GYRO_DEG_TO_RAD = 0.01745329252f;

  float ax = ((float)axRaw / ACCEL_SCALE_4G) * G_TO_MS2;
  float ay = ((float)ayRaw / ACCEL_SCALE_4G) * G_TO_MS2;
  float az = ((float)azRaw / ACCEL_SCALE_4G) * G_TO_MS2;
  float gx = ((float)gxRaw / GYRO_SCALE_250DPS) * GYRO_DEG_TO_RAD;
  float gy = ((float)gyRaw / GYRO_SCALE_250DPS) * GYRO_DEG_TO_RAD;
  float gz = ((float)gzRaw / GYRO_SCALE_250DPS) * GYRO_DEG_TO_RAD;

  float accelMagnitude = sqrtf(
    ax * ax +
    ay * ay +
    az * az
  );

  float gyroMagnitude = sqrtf(
    gx * gx +
    gy * gy +
    gz * gz
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
