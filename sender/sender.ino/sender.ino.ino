/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.
  
  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
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

// Replaced with drone's MAC address
uint8_t broadcastAddress[] = {0xF0, 0x08, 0xD1, 0x49, 0xD4, 0xAC};

// Structure example to send data
// Must match the receiver structure
typedef struct data_struct_t {
  float yaw;
  float pitch;
  float roll;
  float ax;
  float ay;
  float az;
  int button;
} data_struct;

// Create a struct_message called myData
data_struct myData;

esp_now_peer_info_t peerInfo;

// callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void SendData(data_struct *myData) {
  // Send message via ESP-NOW
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) myData, sizeof(*myData));
   
  if (result == ESP_OK) {
    Serial.println("Sent with success");
  }
  else {
    Serial.println("Error sending the data");
  }
}

void PrintDataStruct() {
  Serial.print(mpu6050.Angle.Yaw);
  Serial.print("\t");
  Serial.print(mpu6050.Angle.Pitch);
  Serial.print("\t");
  Serial.print(mpu6050.Angle.Roll);  
  Serial.print("\t");
  Serial.print(mpu6050.Accel.X);
  Serial.print("\t");
  Serial.print(mpu6050.Accel.Y);
  Serial.print("\t");
  Serial.print(mpu6050.Accel.Z);
  Serial.print("\t");
  Serial.println("");
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT);

  // Init Serial Monitor
  Serial.begin(115200);

  mpu6050.DeviceDriverSet_MPU6050_Init();

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  esp_now_register_send_cb((esp_now_send_cb_t)OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
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

  myData.yaw = mpu6050.Angle.Yaw;
  myData.pitch = mpu6050.Angle.Pitch;
  myData.roll = mpu6050.Angle.Roll;
  myData.ax = mpu6050.Accel.X;
  myData.ay = mpu6050.Accel.Y;
  myData.az = mpu6050.Accel.Z;
  
  myData.button = button_value;

  if (button_value == 1 && previous_button_value == 0) {
    // Serial.println("Button pressed! Sending data.");
    // SendData(&myData);
  }

  PrintDataStruct();

  previous_button_value = button_value;
}