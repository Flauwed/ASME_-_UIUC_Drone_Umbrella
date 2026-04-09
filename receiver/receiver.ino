#include <esp_now.h>
#include <WiFi.h>
#include <esp_wifi.h>

#define RXD2 16
#define TXD2 17

HardwareSerial CRSFSerial(2);

uint8_t newMACAddress[] = {0x32, 0xAE, 0xA4, 0x07, 0x0D, 0x66};

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
unsigned long lastCrsfTime = 0;

static const uint8_t CRSF_SYNC = 0xC8;
static const uint8_t CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16;

uint16_t crsfChannels[16];

void PrintDataStruct() {
  Serial.print(myData.yaw);   Serial.print("\t");
  Serial.print(myData.pitch); Serial.print("\t");
  Serial.print(myData.roll);  Serial.print("\t");
  Serial.print(myData.ax);    Serial.print("\t");
  Serial.print(myData.ay);    Serial.print("\t");
  Serial.print(myData.az);    Serial.print("\t");
  Serial.println("");
}

void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  if (len == sizeof(myData)) {
    memcpy(&myData, incomingData, sizeof(myData));
  }
}

uint8_t crsf_crc8(const uint8_t *ptr, uint8_t len) {
  uint8_t crc = 0;
  while (len--) {
    crc ^= *ptr++;
    for (uint8_t i = 0; i < 8; i++) {
      if (crc & 0x80) crc = (crc << 1) ^ 0xD5;
      else crc <<= 1;
    }
  }
  return crc;
}

uint16_t constrainCrsf(int v) {
  if (v < 172) return 172;
  if (v > 1811) return 1811;
  return (uint16_t)v;
}

uint16_t mapFloatToCRSF(float val, float min_in, float max_in) {
  if (val < min_in) val = min_in;
  if (val > max_in) val = max_in;
  float mapped = (val - min_in) * (1792.0f - 191.0f) / (max_in - min_in) + 191.0f;
  return constrainCrsf((int)mapped);
}

void sendCRSFChannels(uint16_t *ch) {
  uint8_t frame[26] = {0};

  frame[0] = CRSF_SYNC;
  frame[1] = 24;
  frame[2] = CRSF_FRAMETYPE_RC_CHANNELS_PACKED;

  uint32_t bitbuf = 0;
  uint8_t bitsInBuf = 0;
  uint8_t outIndex = 3;

  for (int i = 0; i < 16; i++) {
    bitbuf |= ((uint32_t)(ch[i] & 0x07FF)) << bitsInBuf;
    bitsInBuf += 11;

    while (bitsInBuf >= 8) {
      frame[outIndex++] = bitbuf & 0xFF;
      bitbuf >>= 8;
      bitsInBuf -= 8;
    }
  }

  if (bitsInBuf > 0) {
    frame[outIndex++] = bitbuf & 0xFF;
  }

  frame[25] = crsf_crc8(&frame[2], 23);
  CRSFSerial.write(frame, sizeof(frame));
}

void setup() {
  Serial.begin(115200);

  CRSFSerial.begin(420000, SERIAL_8N1, RXD2, TXD2);

  for (int i = 0; i < 16; i++) crsfChannels[i] = 992;
  crsfChannels[2] = 191;

  WiFi.mode(WIFI_STA);
  if (esp_wifi_set_mac(WIFI_IF_STA, &newMACAddress[0]) == ESP_OK) {
    Serial.println("MAC Address Changed Successfully");
    Serial.println(WiFi.macAddress());
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  esp_now_register_recv_cb((esp_now_recv_cb_t)OnDataRecv);
}

void loop() {
  // 1. Drain the incoming serial buffer as fast as possible (Outside the 10ms timer)
  while (CRSFSerial.available()) {
    uint8_t incomingByte = CRSFSerial.read();
    
    if (incomingByte == 0xC8) {
      // The FC is sending telemetry! 
      // Note: Do not print this every time once you confirm it works, 
      // or you will flood the Arduino Serial Monitor at 420000 baud.
      // Serial.println("Success! Received telemetry sync byte from FC.");
    }
  }

  // 2. Send RC data exactly every 10ms
  if (millis() - lastCrsfTime >= 10) {
    lastCrsfTime = millis();

    crsfChannels[0] = mapFloatToCRSF(myData.ay, -1.0, 1.0);
    crsfChannels[1] = mapFloatToCRSF(myData.ax, -1.0, 1.0);
    crsfChannels[2] = mapFloatToCRSF(myData.az, 0.0, 1.0);
    crsfChannels[3] = 992;
    crsfChannels[4] = myData.button ? 1792 : 191;

    for (int i = 5; i < 16; i++) crsfChannels[i] = 992;

    sendCRSFChannels(crsfChannels);
    // PrintDataStruct(); // Comment this out if you need maximum UART performance
  }
}