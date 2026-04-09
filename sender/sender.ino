/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Modified for MPU6050 angle normalization to -1.0 to 1.0 range
*/

#include <stdint.h>
#include <esp_now.h>
#include <WiFi.h>
#include <Wire.h>
#include <Arduino.h>
#include "DeviceDriverSet_xxx0.h"

DeviceDriverSet_MPU6050 mpu6050;

#define LED_BUILTIN 2
#define BUTTON_PIN 15

int previous_button_value = 0;
unsigned long lastDataSent = 0; // Changed to unsigned long to prevent overflow

// Drone's MAC address
uint8_t broadcastAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

// Structure must match the receiver exactly
typedef struct data_struct_t {
  float yaw;
  float pitch;
  float roll;
  float ax;
  float ay;
  float az;
  int button;
} data_struct;

data_struct myData;
esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void SendData(data_struct *myData) {
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) myData, sizeof(*myData));
}

void PrintDataStruct() {
  Serial.print("P: "); Serial.print(myData.pitch);
  Serial.print("\tR: "); Serial.print(myData.roll);  
  Serial.print("\tAX: "); Serial.print(myData.ax);
  Serial.print("\tAY: "); Serial.print(myData.ay);
  Serial.print("\tAZ: "); Serial.print(myData.az);
  Serial.println("");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  Serial.begin(115200);

  mpu6050.DeviceDriverSet_MPU6050_Init();

  WiFi.mode(WIFI_STA);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
}
 
void loop() {
  int button_value = digitalRead(BUTTON_PIN);
  digitalWrite(LED_BUILTIN, button_value);

  mpu6050.DeviceDriverSet_MPU6050_getYawPitchRoll();
  mpu6050.DeviceDriverSet_MPU6050_getAccelerationXYZ();

  // 1. Normalize Pitch and Roll
  // Divides degrees by 45.0 so that a 45-degree hand tilt = 1.0 (Full Stick)
  float max_tilt_angle = 45.0; 
  
  myData.yaw   = mpu6050.Angle.Yaw; // Yaw tends to drift without a compass, keeping raw
  myData.pitch = constrain(mpu6050.Angle.Pitch / max_tilt_angle, -1.0, 1.0);
  myData.roll  = constrain(mpu6050.Angle.Roll / max_tilt_angle, -1.0, 1.0);

  // 2. Normalize Accelerations
  // Convert int16_t to G-forces (assuming standard MPU6050 ±2g range where 1g = 16384)
  myData.ax = constrain(mpu6050.Accel.X / 16384.0, -1.0, 1.0);
  myData.ay = constrain(mpu6050.Accel.Y / 16384.0, -1.0, 1.0);
  myData.az = constrain(mpu6050.Accel.Z / 16384.0, -1.0, 1.0); // 1.0 equals 1g of resting gravity
  
  myData.button = button_value;

  // Send data to receiver at 50Hz (every 20ms) instead of 10Hz (100ms)
  // This provides a much smoother control link for CRSF
  if (millis() - lastDataSent >= 20) {
    lastDataSent = millis();
    SendData(&myData);
    // PrintDataStruct(); // Comment out to avoid serial lag once debugging is done
  }

  previous_button_value = button_value;
}