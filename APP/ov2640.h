#ifndef __OV2640_H__
#define __OV2640_H__

#include "main.h"

// OV2640 I2C地址
#define OV2640_I2C_ADDR 0x30

// OV2640寄存器地址
#define OV2640_REG_BANK_SELECT 0xFF
#define OV2640_REG_SENSOR_ID_H 0x0A
#define OV2640_REG_SENSOR_ID_L 0x0B
#define OV2640_REG_AUTO_EXPOSURE 0x10
#define OV2640_REG_BRIGHTNESS 0x11
#define OV2640_REG_CONTRAST 0x12
#define OV2640_REG_SATURATION 0x13

// 图像分辨率
#define OV2640_RES_160x120 0
#define OV2640_RES_320x240 1
#define OV2640_RES_640x480 2
#define OV2640_RES_800x600 3
#define OV2640_RES_1024x768 4
#define OV2640_RES_1280x960 5
#define OV2640_RES_1600x1200 6

// 图像格式
#define OV2640_FORMAT_YUV422 0
#define OV2640_FORMAT_RGB565 1
#define OV2640_FORMAT_JPEG 2

// 摄像头状态
typedef enum {
    OV2640_STATUS_IDLE,
    OV2640_STATUS_INITIALIZING,
    OV2640_STATUS_READY,
    OV2640_STATUS_CAPTURING,
    OV2640_STATUS_ERROR
} OV2640_StatusTypeDef;

// 摄像头配置
typedef struct {
    uint8_t resolution;
    uint8_t format;
    uint8_t brightness;
    uint8_t contrast;
    uint8_t saturation;
} OV2640_ConfigTypeDef;

// 函数声明
void OV2640_Init(void);
OV2640_StatusTypeDef OV2640_GetStatus(void);
void OV2640_SetResolution(uint8_t resolution);
void OV2640_SetFormat(uint8_t format);
void OV2640_SetBrightness(uint8_t brightness);
void OV2640_SetContrast(uint8_t contrast);
void OV2640_SetSaturation(uint8_t saturation);
void OV2640_StartCapture(void);
void OV2640_StopCapture(void);
void OV2640_ReadFrame(uint8_t *buffer, uint32_t size);

#endif /* __OV2640_H__ */