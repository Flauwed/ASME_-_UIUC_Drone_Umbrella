/*STM8S003F3 MPU6050 IIC */
class DeviceDriverSet_MPU6050
{
public:
  void DeviceDriverSet_MPU6050_Init(void);
  void DeviceDriverSet_MPU6050::DeviceDriverSet_MPU6050_getYawPitchRoll(float *yaw, float *pitch, float *roll);
private:
};

#endif

