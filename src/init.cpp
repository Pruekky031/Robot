#include "motor.h"
#include "robot.h"
#include "init.h"

IntervalTimer encoderTimer;

///////////////////////// Pin Motor A
#define MOTOR_A_DIR_1 2
#define MOTOR_A_DIR_2 1
#define MOTOR_A_PWM 0

#define ENCODER_A_IN_A 14
#define ENCODER_A_IN_B 15

///////////////////////// Pin Motor B
#define MOTOR_B_DIR_1 4
#define MOTOR_B_DIR_2 5
#define MOTOR_B_PWM 3

#define ENCODER_B_IN_A 17
#define ENCODER_B_IN_B 16

///////////////////////// Pin Motor C
#define MOTOR_C_DIR_1 24
#define MOTOR_C_DIR_2 12
#define MOTOR_C_PWM 11

#define ENCODER_C_IN_A 23
#define ENCODER_C_IN_B 22

///////////////////////// Pin Motor D
#define MOTOR_D_DIR_1 9
#define MOTOR_D_DIR_2 10
#define MOTOR_D_PWM 6

#define ENCODER_D_IN_A 20
#define ENCODER_D_IN_B 21

// PPR Motor
#define PPR 370


void PinModeMotor()
{
    // Settings analogWrite Resolution To 10 Bit
    analogWriteResolution(10);

    // SetPin & PinMode
    MotorA.setPin(MOTOR_A_DIR_1, MOTOR_A_DIR_2, MOTOR_A_PWM, ENCODER_A_IN_A, ENCODER_A_IN_B, PPR);
    MotorB.setPin(MOTOR_B_DIR_1, MOTOR_B_DIR_2, MOTOR_B_PWM, ENCODER_B_IN_A, ENCODER_B_IN_B, PPR);
    MotorC.setPin(MOTOR_C_DIR_1, MOTOR_C_DIR_2, MOTOR_C_PWM, ENCODER_C_IN_A, ENCODER_C_IN_B, PPR);
    MotorD.setPin(MOTOR_D_DIR_1, MOTOR_D_DIR_2, MOTOR_D_PWM, ENCODER_D_IN_A, ENCODER_D_IN_B, PPR);
    
    // Attach Interrupt สำหรับอ่าน Encoder
    attachInterrupt(digitalPinToInterrupt(ENCODER_A_IN_A), []()
                    { MotorA.ISREncoder(); }, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_B_IN_A), []()
                    { MotorB.ISREncoder(); }, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_C_IN_A), []()
                    { MotorC.ISREncoder(); }, RISING);
    attachInterrupt(digitalPinToInterrupt(ENCODER_D_IN_A), []()
                    { MotorD.ISREncoder(); }, RISING);

    // ตั้งเวลาอ่านค่า RPM ทุกๆ 1000 us (1 ms)
    encoderTimer.begin(updateAllRPM, 1000);
}
