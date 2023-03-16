#include "imu.h"



IMU::imu(){
    pitch= 0.0F;
    roll= 0.0F;
    yaw= 0.0F;
}

IMU::~imu(){
    
}

void IMU::getData(){
 
        M5.IMU.getAhrsData(
            &pitch, &roll, &yaw);  
         
                                

        M5.Lcd.setCursor(0, 120);
        M5.Lcd.printf("pitch,  roll,  yaw");
        M5.Lcd.setCursor(0, 142);
        M5.Lcd.printf("%5.2f  %5.2f  deg", pitch, roll);

}