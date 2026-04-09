#include <WiFi.h>
#include <WiFiUdp.h>
#include <HardwareSerial.h>

#define RXD2 16
#define TXD2 17

HardwareSerial CRSFSerial(2);
WiFiUDP udp;

// Cross-platform struct padding: enforces exact 28-byte size
typedef struct __attribute__((packed)) {
  float yaw;
  float pitch;
  float roll;
  float ax;
  float ay;
  float az;       // We will use 'az' as our Throttle slider (0.0 to 1.0)
  int32_t button; // 1 = Arm, 0 = Disarm
} data_struct;

data_struct myData;

unsigned long lastCrsfTime = 0;
unsigned long lastPacketTime = 0; // For failsafe tracking

const uint8_t CRSF_SYNC = 0xC8;
const uint8_t CRSF_FRAMETYPE_RC_CHANNELS_PACKED = 0x16;
uint16_t crsfChannels[16];

// [Keep your existing crsf_crc8, constrainCrsf, mapFloatToCRSF, and sendCRSFChannels functions here]
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

  // Default failsafe channels
  for (int i = 0; i < 16; i++) crsfChannels[i] = 992;
  crsfChannels[2] = 191; // Throttle low

  // 1. Start Wi-Fi Access Point
  Serial.println("Starting AP...");
  WiFi.softAP("Drone_Net", "12345678"); // SSID and Password
  
  // ESP32 default AP IP is usually 192.168.4.1
  Serial.print("AP IP address: ");
  Serial.println(WiFi.softAPIP());

  // 2. Start UDP Server
  udp.begin(4444);
  Serial.println("UDP Server listening on port 4444");
}

void loop() {
  // 1. Check for incoming UDP packets from the iPhone
  int packetSize = udp.parsePacket();
  if (packetSize == sizeof(myData)) {
    udp.read((char *)&myData, sizeof(myData));
    lastPacketTime = millis(); // Reset failsafe timer
  }

  // 2. Process Failsafe (Extremely Important)
  // If no UDP packet arrives for 200ms, force disarm and zero throttle
  if (millis() - lastPacketTime > 200) {
    myData.button = 0;
    myData.pitch = 0.0;
    myData.roll = 0.0;
    myData.yaw = 0.0;
    myData.az = 0.0; // Throttle to 0
  }

  // 3. Send CRSF to Flight Controller every 10ms
  if (millis() - lastCrsfTime >= 10) {
    lastCrsfTime = millis();

    // Map iPhone data to CRSF (192 to 1792)
    crsfChannels[0] = mapFloatToCRSF(myData.roll, -1.0, 1.0);
    crsfChannels[1] = mapFloatToCRSF(myData.pitch, -1.0, 1.0);
    crsfChannels[2] = mapFloatToCRSF(myData.az, 0.0, 1.0);     // Throttle
    crsfChannels[3] = mapFloatToCRSF(myData.yaw, -1.0, 1.0);   // Yaw
    crsfChannels[4] = myData.button ? 1792 : 191;              // Arm switch

    sendCRSFChannels(crsfChannels);

    // Drain telemetry buffer so it doesn't crash the ESP32
    while (CRSFSerial.available()) { CRSFSerial.read(); }
  }
}