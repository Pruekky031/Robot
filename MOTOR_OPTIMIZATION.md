# ⚡ แนวทางการ Optimize โค้ด `Motor` & `Encoder RPM` สำหรับ Teensy 4.1

เอกสารนี้สรุปเทคนิคการรีดประสิทธิภาพ (MCU & RAM Optimization) สำหรับการอ่านค่า Encoder และคำนวณความเร็วรอบ (RPM) บนบอร์ด **Teensy 4.1 (ARM Cortex-M7 @ 600MHz)** เพื่อให้ใช้ทรัพยากร CPU Clock Cycles และ RAM น้อยที่สุด

---

## 🔍 1. สรุป 6 จุดที่สามารถ Optimize เพิ่มได้

| ลำดับ | จุดที่ปรับปรุง | โค้ดเดิม | โค้ดที่ Optimize แล้ว | ประโยชน์ที่ได้ |
| :---: | :--- | :--- | :--- | :--- |
| **1** | **Pre-calculate ค่าคงที่ RPM** | หาร `60000000UL / (DeltaTime * ppr)` ทุกรอบใน Timer | คำนวณ `rpmConstant = 60000000UL / ppr` เก็บไว้ตั้งแต่ตอนเรียก `setPin()` | ตัดคำสั่งคูณ (`MUL`) ในทุกๆ รอบของ Timer Interrupt เหลือแค่การหาร (`UDIV`) ครั้งเดียว |
| **2** | **ลดการเขียน RAM ใน `ISREncoder()`** | เขียนทั้ง `MotorDirection = 1` และ `EncoderCount++` ลง RAM ทุกครั้งที่เกิด Pulse | บวกลบแค่ `EncoderCount` อย่างเดียว แล้วไปเช็คทิศทางจาก `EncoderCount - PrevEncoderCount` ใน `ReadRPM()` | ลดภาระการเขียนลง RAM (`STRB`) ใน Interrupt ที่เกิดถี่หลายหมื่นครั้งต่อวินาทีลงได้ครึ่งหนึ่ง |
| **3** | **เอา `volatile` ที่ไม่จำเป็นออก** | `volatile uint32_t PrevTime = 0;` | `uint32_t PrevTime = 0;` (ไม่ต้องมี `volatile`) | เนื่องจาก `PrevTime` ถูกใช้เฉพาะใน `ISREncoder()` ที่เดียว การเอา `volatile` ออกช่วยให้ Compiler เก็บค่าไว้ใน Register ได้เร็วขึ้น |
| **4** | **ใช้ `static constexpr` กับค่าคงที่** | `uint8_t MaxCounter = 30;` (เสีย RAM ล้อละ 1 Byte) | `static constexpr uint8_t MaxCounter = 30;` | ไม่เสียพื้นที่ RAM เลยแม้แต่ไบต์เดียว เพราะค่า `30` จะถูกฝังเข้าไปในคำสั่ง Assembly (Immediate Value) โดยตรง |
| **5** | **อ่านตัวแปร `volatile` เข้า Register ครั้งเดียว** | เรียกใช้ `EncoderCount` และ `DeltaTime` ซ้ำหลายบรรทัดใน `ReadRPM()` | ก๊อปปี้ลงตัวแปร Local (`int32_t count = EncoderCount;`) แค่ครั้งเดียวตอนต้นฟังก์ชัน | ลดการวิ่งไปอ่าน RAM ซ้ำซ้อน และป้องกันบั๊ก **Race Condition** หากเกิด Interrupt แทรกกลางฟังก์ชัน |
| **6** | **ใช้ `FASTRUN` และ `digitalWriteFast`** | รันฟังก์ชันปกติ และใช้ `digitalWrite()` ใน `drive()` | เติม `FASTRUN` หน้า `ISREncoder()` และใช้ `digitalWriteFast()` | บังคับให้ฟังก์ชันวางอยู่ใน **ITCM RAM (0 Wait-State)** วิ่งเต็มความเร็ว 600 MHz โดยไม่ต้องรอ Flash Cache |

---

## 🛠️ 2. ตัวอย่างโค้ดฉบับ Optimize ขั้นสุด

### 📄 ไฟล์ `include/motor.h`

```cpp
#pragma once
#include <Arduino.h>

class Motor
{
private:
    ////////////////////////////////////// Motor Pin & Pre-calculated Constant
    uint8_t MotorIN_A;
    uint8_t MotorIN_B;
    uint8_t MotorPWM;
    uint8_t EncoderIN_A;
    uint8_t EncoderIN_B;
    uint32_t rpmConstant = 0; // เก็บค่า (60,000,000 / PPR) คำนวณครั้งเดียวตอน setPin

    ////////////////////////////////////// Encoder Variable
    // ใช้ใน ISREncoder()
    volatile uint32_t DeltaTime = 0;    // คาบเวลา pulse To pulse
    uint32_t PrevTime = 0;              // ใช้เฉพาะใน ISR ไม่ต้องเป็น volatile
    volatile int32_t EncoderCount = 0;  // จำนวน pulse encoder

    // ใช้ใน ReadRPM()
    int32_t PrevEncoderCount = 0;       // จำนวน pulse encoder ก่อนหน้า
    int8_t LastDirection = 0;           // จำทิศทางการหมุนล่าสุด (+1 หรือ -1)
    uint8_t Counter = 0;                // นับจำนวนรอบที่ไม่มี pulse เข้ามา
    static constexpr uint8_t MaxCounter = 30; // ค่าคงที่ระดับ Compile-time (ไม่กิน RAM)

public:
    volatile int16_t CurrentRPM = 0;    // ค่า RPM ปัจจุบัน (ใส่ volatile เพราะอัปเดตใน Timer)

    void setPin(uint8_t inA, uint8_t inB, uint8_t pwm, uint8_t encA, uint8_t encB, uint16_t motorPPR);
    void drive(int16_t speed);
    FASTRUN void ISREncoder();          // บังคับรันใน ITCM RAM ความเร็วสูงสุด
    void ReadRPM();
};

extern Motor MotorA, MotorB, MotorC, MotorD;

void updateAllRPM(); // ฟังก์ชันสำหรับผูกกับ IntervalTimer
```

---

### 📄 ไฟล์ `src/motor.cpp`

```cpp
#include "motor.h"
#include "robot.h"

// สร้าง Object ของมอเตอร์ทั้ง 4 ตัว
Motor MotorA, MotorB, MotorC, MotorD;

// ฟังก์ชันตั้งค่าขาและคำนวณค่าคงที่ล่วงหน้า
void Motor::setPin(uint8_t inA, uint8_t inB, uint8_t pwm, uint8_t encA, uint8_t encB, uint16_t motorPPR)
{
    MotorIN_A = inA;
    MotorIN_B = inB;
    MotorPWM = pwm;
    EncoderIN_A = encA;
    EncoderIN_B = encB;
    
    // Optimize 1: คำนวณ 60,000,000 / PPR เก็บไว้ล่วงหน้าแค่ครั้งเดียว
    rpmConstant = (motorPPR > 0) ? (60000000UL / motorPPR) : 0;

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
        digitalWriteFast(MotorIN_A, HIGH);
        digitalWriteFast(MotorIN_B, LOW);
        analogWrite(MotorPWM, speed);
    }
    else if (speed < 0)
    {
        digitalWriteFast(MotorIN_A, LOW);
        digitalWriteFast(MotorIN_B, HIGH);
        analogWrite(MotorPWM, -speed);
    }
    else
    {
        digitalWriteFast(MotorIN_A, LOW);
        digitalWriteFast(MotorIN_B, LOW);
        analogWrite(MotorPWM, 0);
    }
}

// Optimize 2 & 6: รันใน ITCM RAM และอัปเดตตัวแปรให้น้อยที่สุด
FASTRUN void Motor::ISREncoder()
{
    uint32_t CurrentTime = micros();
    DeltaTime = CurrentTime - PrevTime;
    PrevTime = CurrentTime;

    if (digitalReadFast(EncoderIN_B) == LOW)
        EncoderCount++;
    else
        EncoderCount--;
}

// Optimize 3 & 5: ดึงค่า volatile มาพักใน Register ครั้งเดียว และตัดการคูณซ้ำซ้อน
void Motor::ReadRPM()
{
    int32_t count = EncoderCount; // อ่านจาก RAM แค่ครั้งเดียว
    int32_t diff = count - PrevEncoderCount;

    if (diff == 0)
    {
        if (Counter < MaxCounter)
            Counter++;
    }
    else
    {
        Counter = 0;
        PrevEncoderCount = count;
        LastDirection = (diff > 0) ? 1 : -1;
    }

    uint32_t dt = DeltaTime; // อ่านจาก RAM แค่ครั้งเดียว
    if (Counter < MaxCounter && dt > 0)
    {
        int16_t rpm = (int16_t)(rpmConstant / dt);
        CurrentRPM = (LastDirection > 0) ? rpm : -rpm;
    }
    else
    {
        CurrentRPM = 0;
    }
}

void updateAllRPM()
{
    MotorA.ReadRPM();
    MotorB.ReadRPM();
    MotorC.ReadRPM();
    MotorD.ReadRPM();
}
```

---

## 💡 3. เกร็ดเพิ่มเติม: Hardware QuadEncoder ของ Teensy 4.1 (Zero CPU Load)

หากในอนาคตต้องการให้การนับ Pulse ของ Encoder **กินแรง CPU = 0% (ไม่ต้องใช้ `attachInterrupt` เลยแม้แต่ครั้งเดียว)**:
- ชิป NXP i.MX RT1062 บน **Teensy 4.1** มีวงจร **Hardware Quadrature Decoder (ENC1 - ENC4)** อยู่ในตัวชิปถึง 4 ช่องพอดีสำหรับมอเตอร์ 4 ล้อ!
- ขาที่คุณใช้อยู่ตอนนี้หลายขาตรงกับ Hardware Encoder ของ Teensy 4.1 อยู่แล้ว (เช่น ขา `0, 1, 2, 3, 4` หรือใช้งานผ่าน Library `QuadEncoder.h` ที่ติดมากับ Teensyduino)
- ข้อดีคือตัวนับจะทำงานด้วยวงจรอิเล็กทรอนิกส์ล้วนๆ โดยไม่รบกวน CPU เลยแม้แต่นาโนวินาทีเดียว
