#include <Arduino.h>
#include "robot.h"
#include "motor.h"
#include "init.h"

void setup()
{
    Serial.begin(115200);
    PinModeMotor();
}

void loop()
{
    // สั่งจ่าย PWM 300 (จาก 1023) ให้ MotorA หมุน
    MotorA.drive(1023);

    // แสดงผลค่า RPM ของ MotorA ผ่าน Serial Monitor
    Serial.print("MotorA RPM: ");
    Serial.println(MotorA.CurrentRPM);

    delay(10);
}
