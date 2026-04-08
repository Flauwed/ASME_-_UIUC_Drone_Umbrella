/*
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Modified for SBUS Betaflight bridging
*/

#include <esp_now.h>
#include <WiFi.h>
#include "sbus.h"

// Note: Replaced the magic numbers 33/32 with your defined pins
#define RXD1 18
#define TXD1 17

// Initialize bolderflight SBUS TX on Serial1
// Note: Some older versions of this library use the rx/tx/invert bools in the constructor. 
// If true/false throws an error, update to just: bfs::SbusTx sbus_tx(&Serial1);
bfs::SbusTx sbus_tx(&Serial1, RXD1, TXD1, true, false);
bfs::SbusData data;

// Structure example to receive data
typedef struct data_struct_t {
  float ax;
  float ay;
  float az;
  float gx;
  float gy;
  float gz;
  int button;
} data_struct;

data_struct myData;

// Timer for continuous SBUS transmission
unsigned long lastSbusTime = 0;

void PrintMPUData() {
  Serial.print("ax: "); Serial.println(myData.ax);
  Serial.print("ay: "); Serial.println(myData.ay);
  Serial.print("az: "); Serial.println(myData.az);
  Serial.println("");
}

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  // PrintMPUData(); // Uncomment for debugging, but keeping prints out of callbacks is safer
}

// Helper function to map float IMU values (-1.0 to 1.0) to SBUS integer format (192 to 1792)
int mapFloatToSBUS(float val, float min_in, float max_in) {
  // Constrain to prevent SBUS channel overflow
  if (val < min_in) val = min_in;
  if (val > max_in) val = max_in;
  
  // Map range: 192 (1000us) to 1792 (2000us)
  float mapped = (val - min_in) * (1792.0 - 192.0) / (max_in - min_in) + 192.0;
  return (int)mapped;
}

void setup() {
  Serial.begin(115200);

  // Initialize SBUS Transmission
  sbus_tx.Begin();

  // Set default safe SBUS values (Centered sticks, zero throttle)
  for (int i = 0; i < 16; i++) {
    data.ch[i] = 992; // 1500us center
  }
  data.ch[2] = 192;   // Throttle (CH3) to zero (1000us)

  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
}
 
void loop() {
  // SBUS requires a continuous stream of data (~100Hz) to prevent RX_FAILSAFE
  if (millis() - lastSbusTime >= 10) {
    lastSbusTime = millis();

    // Map axes assuming -1.0g to 1.0g scale. Adjust the min/max limits 
    // depending on the actual raw output scale of your specific IMU.
    
    // CH1 (Roll): Maps horizontal Y acceleration
    data.ch[0] = mapFloatToSBUS(myData.ay, -1.0, 1.0); 
    
    // CH2 (Pitch): Maps horizontal X acceleration
    data.ch[1] = mapFloatToSBUS(myData.ax, -1.0, 1.0);
    
    // CH3 (Throttle): Maps vertical Z acceleration. 
    // DANGER: If your IMU reads 1.0g at rest due to gravity, this will send 
    // full throttle! You must subtract gravity or map this to 0.0 - 1.0.
    data.ch[2] = mapFloatToSBUS(myData.az, 0.0, 1.0);  
    
    // CH4 (Yaw): Kept centered so the drone doesn't spin
    data.ch[3] = 992;
    
    // CH5 (AUX1): Use the button to act as an Arm/Disarm switch
    // Button pressed (1) sends 2000us, released (0) sends 1000us
    data.ch[4] = myData.button ? 1792 : 192;

    // Send the packet to the flight controller
    sbus_tx.data(data);
    sbus_tx.Write();
  }
}