#pragma once
#include <Arduino.h>

class Motor
{
private:
    ////////////////////////////////////// Motor Pin
    uint8_t MotorIN_A;   // IN Motor A
    uint8_t MotorIN_B;   // IN Motor B
    uint8_t MotorPWM;    // PWM
    uint8_t EncoderIN_A; // IN Encoder A
    uint8_t EncoderIN_B; // IN Encoder B
    uint16_t ppr;        // PPR Encoder
    ////////////////////////////////////// Motor Pin

    ////////////////////////////////////////////////////////// Encder Variable

    // func ISR
    volatile uint32_t DeltaTime = 0;    // คาบเวลา pulse To pulse
    volatile uint32_t PrevTime = 0;     // เวลาที่ pulse ก่อนหน้า
    volatile int8_t MotorDirection = 0; // ทิศทางการหมุนของมอเตอร์
    volatile int32_t EncoderCount = 0;  // จำนวน pulse encoder

    // func ReadRPM
    int32_t PrevEncoderCount = 0; // จำนวน pulse encoder ก่อนหน้า
    uint8_t Counter = 0;          // Counter ใช้นับว่ามอเตอร์หยุดไปนานแค่ไหน
    uint8_t MaxCounter = 30;      // จำนวน Counter ที่จะตัดว่าเมื่อไหร่มอเตอร์หยุดแล้ว ค่ายิ่งมาก RPMจะเป็น 0 ช้า ค่าน้อยรอบต่ำๆ RPM จะจับมอเตอร์เป็น 0 บ่อย
    ////////////////////////////////////////////////////////// Encder Variable

public:
    volatile int16_t CurrentRPM = 0; 
    void setPin(uint8_t inA, uint8_t inB, uint8_t pwm, uint8_t encA, uint8_t encB, uint16_t motorPPR);
    void drive(int16_t speed);
    void ISREncoder();
    void ReadRPM();
};
extern Motor MotorA, MotorB, MotorC, MotorD;

void updateAllRPM(); // ฟังก์ชันที่จะเอาไปผูกกับ IntervalTimer
