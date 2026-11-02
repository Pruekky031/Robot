#include "motor.h"
#include "robot.h"
// สร้าง Object ของมอเตอร์ทั้ง 4 ตัว
Motor MotorA, MotorB, MotorC, MotorD;

// ฟังก์ชันตั้งค่าขาและสั่ง pinMode
void Motor::setPin(uint8_t inA, uint8_t inB, uint8_t pwm, uint8_t encA, uint8_t encB, uint16_t motorPPR)
{
    MotorIN_A = inA;
    MotorIN_B = inB;
    MotorPWM = pwm;
    EncoderIN_A = encA;
    EncoderIN_B = encB;
    ppr = motorPPR;

    pinMode(MotorIN_A, OUTPUT);
    pinMode(MotorIN_B, OUTPUT);
    pinMode(MotorPWM, OUTPUT);
    pinMode(EncoderIN_A, INPUT_PULLUP);
    pinMode(EncoderIN_B, INPUT_PULLUP);
}

// ฟังก์ชันสั่งหมุนมอเตอร์ (-1023 ถึง 1023)
void Motor::drive(int16_t speed)
{
    speed = constrain(speed, -1023, 1023);
    if (speed > 0)
    {
        digitalWrite(MotorIN_A, HIGH);
        digitalWrite(MotorIN_B, LOW);
        analogWrite(MotorPWM, speed);
    }
    else if (speed < 0)
    {
        digitalWrite(MotorIN_A, LOW);
        digitalWrite(MotorIN_B, HIGH);
        analogWrite(MotorPWM, -speed);
    }
    else
    {
        digitalWrite(MotorIN_A, LOW);
        digitalWrite(MotorIN_B, LOW);
        analogWrite(MotorPWM, 0);
    }
}

void Motor::ISREncoder()
{
    uint32_t CurrentTime = micros();
    DeltaTime = CurrentTime - PrevTime;
    PrevTime = CurrentTime;
    if (digitalReadFast(EncoderIN_B) == LOW)
    {
        MotorDirection = 1; // Direction +
        EncoderCount++;
    }
    else
    {
        MotorDirection = -1; // Direction -
        EncoderCount--;
    }
}

void Motor::ReadRPM()
{
    if (EncoderCount == PrevEncoderCount)
    {
        if (Counter < MaxCounter)
            Counter++; // นับว่ามอเตอร์หยุดไปกี่รอบ loop แล้ว
    }
    else
    {
        Counter = 0; // reset Counter
    }
    PrevEncoderCount = EncoderCount;

    if (Counter < MaxCounter && DeltaTime > 0)
    {
        CurrentRPM = (int16_t)((60000000UL / (DeltaTime * ppr)) * MotorDirection);
    }
    else
    {
        CurrentRPM = 0;
        MotorDirection = 0;
    }
}
void updateAllRPM() // ฟังก์ชันที่จะเอาไปผูกกับ IntervalTimer
{
    MotorA.ReadRPM();
    MotorB.ReadRPM();
    MotorC.ReadRPM();
    MotorD.ReadRPM();
}
