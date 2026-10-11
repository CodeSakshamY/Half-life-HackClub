
/*
 * WARNING: This code was written by AI just to submit this week.
 * The code is NOT final and requires testing and validation.
 */

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MAX1704X.h>

// I2C pins
#define SDA_PIN 21
#define SCL_PIN 22

// I2C addresses
#define MCP9808_ADDR 0x18
#define MAX30102_ADDR 0x57

Adafruit_MPU6050 mpu;
Adafruit_ADS1115 ads;

bool mpuOK = false;
bool adsOK = false;
bool mcpOK = false;
bool maxOK = false;

bool devicePresent(uint8_t address) {
  Wire.beginTransmission(address);
  return Wire.endTransmission() == 0;
}

bool beginMCP9808() {
  if (!devicePresent(MCP9808_ADDR)) return false;

  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(0x05); // Ambient temperature register
  if (Wire.endTransmission(false) != 0) return false;

  if (Wire.requestFrom((int)MCP9808_ADDR, 2) != 2) return false;

  uint16_t raw = (Wire.read() << 8) | Wire.read();
  return (raw & 0x0FFF) <= 0x0FFF;
}

float readMCP9808() {
  Wire.beginTransmission(MCP9808_ADDR);
  Wire.write(0x05);
  if (Wire.endTransmission(false) != 0) return NAN;

  if (Wire.requestFrom((int)MCP9808_ADDR, 2) != 2) return NAN;

  uint16_t raw = (Wire.read() << 8) | Wire.read();
  float temperature = raw & 0x0FFF;
  temperature /= 16.0;

  if (raw & 0x1000) temperature -= 256.0;
  return temperature;
}

void scanDevices() {
  Serial.println("\n--- I2C Device Scan ---");

  for (uint8_t address = 1; address < 127; address++) {
    if (devicePresent(address)) {
      Serial.printf("Device found: 0x%02X\n", address);
    }
  }
}

void setup() {
  Serial.begin(115200);
  delay(1500);

  Serial.println("\n================================");
  Serial.println("       PSYCON PCB FIRMWARE");
  Serial.println("================================");

  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000);

  scanDevices();

  // MPU6050
  Serial.print("\nMPU6050: ");
  mpuOK = mpu.begin(0x68, &Wire);
  if (!mpuOK) mpuOK = mpu.begin(0x69, &Wire);
  Serial.println(mpuOK ? "Connected" : "Not detected");

  if (mpuOK) {
    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  // ADS1115
  Serial.print("ADS1115: ");
  adsOK = ads.begin(0x48, &Wire);
  if (!adsOK) adsOK = ads.begin(0x49, &Wire);
  if (!adsOK) adsOK = ads.begin(0x4A, &Wire);
  if (!adsOK) adsOK = ads.begin(0x4B, &Wire);
  Serial.println(adsOK ? "Connected" : "Not detected");

  if (adsOK) {
    ads.setGain(GAIN_ONE);
  }

  // MCP9808
  Serial.print("MCP9808: ");
  mcpOK = beginMCP9808();
  Serial.println(mcpOK ? "Responding" : "Not detected");

  // MAX30102 presence check
  Serial.print("MAX30102: ");
  maxOK = devicePresent(MAX30102_ADDR);
  Serial.println(maxOK ? "I2C device detected" : "Not detected");

  Serial.println("\nInitialization complete.");
  Serial.println("Live readings begin below.");
}

void loop() {
  Serial.println("\n========== PSYCON LIVE DATA ==========");

  // MPU6050 readings
  if (mpuOK) {
    sensors_event_t accel, gyro, temp;
    mpu.getEvent(&accel, &gyro, &temp);

    Serial.println("\n[MPU6050]");
    Serial.printf("Acceleration X: %.2f m/s^2\n", accel.acceleration.x);
    Serial.printf("Acceleration Y: %.2f m/s^2\n", accel.acceleration.y);
    Serial.printf("Acceleration Z: %.2f m/s^2\n", accel.acceleration.z);
    Serial.printf("Gyro X: %.2f rad/s\n", gyro.gyro.x);
    Serial.printf("Gyro Y: %.2f rad/s\n", gyro.gyro.y);
    Serial.printf("Gyro Z: %.2f rad/s\n", gyro.gyro.z);
    Serial.printf("Sensor temperature: %.2f C\n", temp.temperature);
  } else {
    Serial.println("\n[MPU6050] Unavailable");
  }

  // MCP9808 temperature
  if (mcpOK) {
    float temperature = readMCP9808();

    Serial.println("\n[MCP9808]");
    if (isnan(temperature)) {
      Serial.println("Read failed");
    } else {
      Serial.printf("Ambient temperature: %.2f C\n", temperature);
    }
  } else {
    Serial.println("\n[MCP9808] Unavailable");
  }

  // ADS1115 analog input readings
  if (adsOK) {
    Serial.println("\n[ADS1115]");
    for (uint8_t channel = 0; channel < 4; channel++) {
      int16_t raw = ads.readADC_SingleEnded(channel);
      float voltage = ads.computeVolts(raw);

      Serial.printf("A%d: raw=%d, voltage=%.4f V\n",
                    channel, raw, voltage);
    }
  } else {
    Serial.println("\n[ADS1115] Unavailable");
  }

  // MAX30102: presence only; optical measurement not initialized
  Serial.println("\n[MAX30102]");
  Serial.println(maxOK
      ? "Detected; optical readings not initialized"
      : "Unavailable");

  Serial.println("\n======================================");

  delay(2000);
}
