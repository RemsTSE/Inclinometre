
#include <M5Stack.h>

class IMU {
    public:
        imu();
        ~imu();

        float pitch;
        float roll ;
        float yaw ;

        void getData() {};

        uint8_t sendPitch();
        uint8_t sendRoll();





    
   
    
};
