// DeviceDriverSet_xxx0.h

#include <Arduino.h>

class DeviceDriverSet_MPU6050
{
public:
  typedef struct
  {
    float Yaw;
    float Pitch;
    float Roll;
  } YawPitchRoll;

  typedef struct
  {
    int16_t X;
    int16_t Y;
    int16_t Z;
  } AccelerationXYZ;

  YawPitchRoll Angle;
  AccelerationXYZ Accel;

public:
  void DeviceDriverSet_MPU6050_Init(void);
  void DeviceDriverSet_MPU6050_getYawPitchRoll(void);
  void DeviceDriverSet_MPU6050_getAccelerationXYZ(void);
};