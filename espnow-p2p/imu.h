
#include <M5Stack.h>

class IMU
{
public:
    imu();
    ~imu();

    float pitch;
    float roll;
    float yaw;

    void getData();
    int16_t getPitch();
    int16_t getRoll();
};
