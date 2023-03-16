#include "imu.h"
#include <cmath>


IMU::imu(){
    pitch= 0.0F;
    roll= 0.0F;
    yaw= 0.0F;
};

IMU::~imu(){

};

void IMU::getData(){
 
        M5.IMU.getAhrsData(
            &pitch, &roll, &yaw);  
         
        Serial.printf("pitch,  roll\n");
        Serial.printf("%5.2f  %5.2f  \n", pitch, roll);


        // M5.Lcd.setCursor(0, 120);
        // M5.Lcd.printf("pitch,  roll,  yaw");
        // M5.Lcd.setCursor(0, 142);
        // M5.Lcd.printf("%5.2f  %5.2f  deg", pitch, roll);

};

int16_t IMU::sendPitch(){
    double floor_pitch = floor(pitch);
    int16_t int_pitch = static_cast<int16_t>(floor_pitch);
    return int_pitch;
};

int16_t IMU::sendRoll(){
    double floor_roll = floor(roll);
    int16_t int_roll = static_cast<int16_t>(floor_roll);
    return int_roll;
};
