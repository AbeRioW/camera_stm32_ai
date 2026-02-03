#ifndef __FT6336_TOUCH_H__
#define __FT6336_TOUCH_H__

#include "main.h"

// FT6336 I2C地址
#define FT6336_I2C_ADDR 0x38

// FT6336寄存器地址
#define FT6336_REG_DEV_MODE 0x00
#define FT6336_REG_GEST_ID 0x01
#define FT6336_REG_TD_STATUS 0x02
#define FT6336_REG_TOUCH1_XH 0x03
#define FT6336_REG_TOUCH1_XL 0x04
#define FT6336_REG_TOUCH1_YH 0x05
#define FT6336_REG_TOUCH1_YL 0x06
#define FT6336_REG_TOUCH2_XH 0x09
#define FT6336_REG_TOUCH2_XL 0x0A
#define FT6336_REG_TOUCH2_YH 0x0B
#define FT6336_REG_TOUCH2_YL 0x0C
#define FT6336_REG_CHIP_ID 0xA3
#define FT6336_REG_FIRMWARE_ID 0xA6
#define FT6336_REG_VENDOR_ID 0xA8

// 触摸点结构体
typedef struct {
    uint8_t status;      // 触摸状态
    uint16_t x;          // X坐标
    uint16_t y;          // Y坐标
} FT6336_TouchPoint;

// 函数声明
void FT6336_Init(void);
uint8_t FT6336_ReadReg(uint8_t reg);
void FT6336_WriteReg(uint8_t reg, uint8_t value);
uint8_t FT6336_GetTouchStatus(void);
void FT6336_GetTouchPoint(FT6336_TouchPoint *point);

#endif

