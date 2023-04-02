#include "imu.h"
#include <cmath>

IMU::imu(){
    pitch= 0.0F;
    roll= 0.0F;
    yaw= 0.0F;
};

IMU::~imu(){};

void IMU::getData(){
        M5.IMU.getAhrsData(&pitch, &roll, &yaw);      
};

int16_t IMU::getPitch(){
    return static_cast<int16_t>(floor(pitch));
};

int16_t IMU::getRoll(){
    return static_cast<int16_t>(floor(roll));
};