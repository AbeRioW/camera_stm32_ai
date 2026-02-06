#include "ov2640.h"
#include "lvgl.h"

// 全局变量
static OV2640_StatusTypeDef ov2640_status = OV2640_STATUS_IDLE;
static OV2640_ConfigTypeDef ov2640_config;

// SCCB引脚定义
#define SCCB_SCL_PIN SCCB_SCL_Pin
#define SCCB_SCL_PORT SCCB_SCL_GPIO_Port
#define SCCB_SDA_PIN SCCB_SDA_Pin
#define SCCB_SDA_PORT SCCB_SDA_GPIO_Port

// SCCB通信函数
static void SCCB_Init(void);
static void SCCB_Start(void);
static void SCCB_Stop(void);
static void SCCB_SendAck(uint8_t ack);
static uint8_t SCCB_ReceiveAck(void);
static void SCCB_SendByte(uint8_t byte);
static uint8_t SCCB_ReceiveByte(void);

// I2C通信函数
static void OV2640_I2C_Write(uint8_t reg, uint8_t value);
static uint8_t OV2640_I2C_Read(uint8_t reg);
static void OV2640_SelectBank(uint8_t bank);

// 分辨率配置表
static const uint8_t resolution_config[][2] = {
    {0x00, 0x00}, // 160x120
    {0x01, 0x00}, // 320x240
    {0x02, 0x00}, // 640x480
    {0x03, 0x00}, // 800x600
    {0x04, 0x00}, // 1024x768
    {0x05, 0x00}, // 1280x960
    {0x06, 0x00}  // 1600x1200
};

// 初始化OV2640
void OV2640_Init(void) {
    ov2640_status = OV2640_STATUS_INITIALIZING;
    
    // 初始化SCCB
    SCCB_Init();
    
    // 复位摄像头
    // 这里需要根据硬件连接实现复位操作
    HAL_GPIO_WritePin(OV2640_RST_GPIO_Port, OV2640_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(OV2640_RST_GPIO_Port, OV2640_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    
    // 检查传感器ID
    uint8_t id_h = OV2640_I2C_Read(OV2640_REG_SENSOR_ID_H);
    uint8_t id_l = OV2640_I2C_Read(OV2640_REG_SENSOR_ID_L);
    
    if ((id_h != 0x26) || (id_l != 0x40)) {
        // 传感器ID错误
        ov2640_status = OV2640_STATUS_ERROR;
        return;
    }
    
    // 初始化默认配置
    ov2640_config.resolution = OV2640_RES_640x480;
    ov2640_config.format = OV2640_FORMAT_RGB565;
    ov2640_config.brightness = 0x80;
    ov2640_config.contrast = 0x80;
    ov2640_config.saturation = 0x80;
    
    // 设置默认分辨率
    OV2640_SetResolution(OV2640_RES_640x480);
    
    // 设置默认格式
    OV2640_SetFormat(OV2640_FORMAT_RGB565);
    
    // 设置默认图像参数
    OV2640_SetBrightness(0x80);
    OV2640_SetContrast(0x80);
    OV2640_SetSaturation(0x80);
    
    ov2640_status = OV2640_STATUS_READY;
}

// 获取摄像头状态
OV2640_StatusTypeDef OV2640_GetStatus(void) {
    return ov2640_status;
}

// 设置分辨率
void OV2640_SetResolution(uint8_t resolution) {
    if (resolution >= OV2640_RES_1600x1200) {
        return;
    }
    
    ov2640_config.resolution = resolution;
    
    // 选择分辨率配置
    // 这里需要根据OV2640的数据手册实现具体的寄存器配置
    // 示例：设置为640x480分辨率
    OV2640_SelectBank(0);
    OV2640_I2C_Write(0x3A, 0x04); // 设置水平分辨率
    OV2640_I2C_Write(0x3B, 0x03); // 设置垂直分辨率
}

// 设置图像格式
void OV2640_SetFormat(uint8_t format) {
    if (format > OV2640_FORMAT_JPEG) {
        return;
    }
    
    ov2640_config.format = format;
    
    // 选择图像格式
    // 这里需要根据OV2640的数据手册实现具体的寄存器配置
    switch (format) {
        case OV2640_FORMAT_YUV422:
            // 设置为YUV422格式
            break;
        case OV2640_FORMAT_RGB565:
            // 设置为RGB565格式
            break;
        case OV2640_FORMAT_JPEG:
            // 设置为JPEG格式
            break;
    }
}

// 设置亮度
void OV2640_SetBrightness(uint8_t brightness) {
    ov2640_config.brightness = brightness;
    OV2640_I2C_Write(OV2640_REG_BRIGHTNESS, brightness);
}

// 设置对比度
void OV2640_SetContrast(uint8_t contrast) {
    ov2640_config.contrast = contrast;
    OV2640_I2C_Write(OV2640_REG_CONTRAST, contrast);
}

// 设置饱和度
void OV2640_SetSaturation(uint8_t saturation) {
    ov2640_config.saturation = saturation;
    OV2640_I2C_Write(OV2640_REG_SATURATION, saturation);
}

// 开始捕获图像
void OV2640_StartCapture(void) {
    if (ov2640_status == OV2640_STATUS_READY) {
        ov2640_status = OV2640_STATUS_CAPTURING;
        // 这里需要实现开始捕获的具体操作
    }
}

// 停止捕获图像
void OV2640_StopCapture(void) {
    if (ov2640_status == OV2640_STATUS_CAPTURING) {
        ov2640_status = OV2640_STATUS_READY;
        // 这里需要实现停止捕获的具体操作
    }
}

// 读取图像数据
void OV2640_ReadFrame(uint8_t *buffer, uint32_t size) {
    // 这里需要实现从摄像头读取图像数据的具体操作
    // 示例：读取JPEG格式图像
    if (ov2640_config.format == OV2640_FORMAT_JPEG) {
        // 读取JPEG数据
    } else {
        // 读取其他格式数据
    }
}

// 选择寄存器 bank
static void OV2640_SelectBank(uint8_t bank) {
    OV2640_I2C_Write(OV2640_REG_BANK_SELECT, bank);
}

// I2C写入函数
static void OV2640_I2C_Write(uint8_t reg, uint8_t value) {
    SCCB_Start();
    SCCB_SendByte(OV2640_I2C_ADDR << 1); // 写入模式
    if (SCCB_ReceiveAck() == 0) {
        SCCB_SendByte(reg);
        if (SCCB_ReceiveAck() == 0) {
            SCCB_SendByte(value);
            SCCB_ReceiveAck();
        }
    }
    SCCB_Stop();
}

// I2C读取函数
static uint8_t OV2640_I2C_Read(uint8_t reg) {
    uint8_t value = 0;
    
    // 先写入寄存器地址
    SCCB_Start();
    SCCB_SendByte(OV2640_I2C_ADDR << 1); // 写入模式
    if (SCCB_ReceiveAck() == 0) {
        SCCB_SendByte(reg);
        if (SCCB_ReceiveAck() == 0) {
            // 然后读取数据
            SCCB_Start();
            SCCB_SendByte((OV2640_I2C_ADDR << 1) | 0x01); // 读取模式
            if (SCCB_ReceiveAck() == 0) {
                value = SCCB_ReceiveByte();
                SCCB_SendAck(1); // 发送非应答
            }
        }
    }
    SCCB_Stop();
    
    return value;
}

// SCCB初始化
static void SCCB_Init(void) {
    // SCCB引脚已经在main.h中定义，这里不需要重复初始化
    // 如果需要额外的初始化操作，可以在这里添加
}

// SCCB发送起始信号
static void SCCB_Start(void) {
    // SDA设置为输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 起始信号：SDA从高到低，同时SCL为高
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
}

// SCCB发送停止信号
static void SCCB_Stop(void) {
    // SDA设置为输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 停止信号：SDA从低到高，同时SCL为高
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
}

// SCCB发送应答信号
static void SCCB_SendAck(uint8_t ack) {
    // SDA设置为输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 发送应答信号
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, ack);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
}

// SCCB接收应答信号
static uint8_t SCCB_ReceiveAck(void) {
    uint8_t ack;
    
    // SDA设置为输入
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 接收应答信号
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    HAL_Delay(1);
    ack = HAL_GPIO_ReadPin(SCCB_SDA_PORT, SCCB_SDA_PIN);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    HAL_Delay(1);
    
    return ack;
}

// SCCB发送一个字节
static void SCCB_SendByte(uint8_t byte) {
    // SDA设置为输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 发送8位数据
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, (byte & 0x80) >> 7);
        byte <<= 1;
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
        HAL_Delay(1);
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
}

// SCCB接收一个字节
static uint8_t SCCB_ReceiveByte(void) {
    uint8_t byte = 0;
    
    // SDA设置为输入
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 接收8位数据
    for (uint8_t i = 0; i < 8; i++) {
        byte <<= 1;
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
        HAL_Delay(1);
        byte |= HAL_GPIO_ReadPin(SCCB_SDA_PORT, SCCB_SDA_PIN);
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
        HAL_Delay(1);
    }
    
    return byte;
}

// 显示图像到LVGL
void OV2640_DisplayImage(void) {
    uint8_t *image_buffer = NULL;
    uint32_t buffer_size = 0;
    uint16_t width = 0, height = 0;
    
    // 根据分辨率确定缓冲区大小和图像尺寸
    switch (ov2640_config.resolution) {
        case OV2640_RES_160x120:
            width = 160;
            height = 120;
            buffer_size = width * height * 2; // RGB565格式
            break;
        case OV2640_RES_320x240:
            width = 320;
            height = 240;
            buffer_size = width * height * 2; // RGB565格式
            break;
        case OV2640_RES_640x480:
            width = 640;
            height = 480;
            buffer_size = width * height * 2; // RGB565格式
            break;
        default:
            width = 320;
            height = 240;
            buffer_size = width * height * 2; // 默认使用320x240
            break;
    }
    
    // 分配缓冲区
    image_buffer = (uint8_t *)malloc(buffer_size);
    if (image_buffer == NULL) {
        return;
    }
    
    // 启动捕获
    OV2640_StartCapture();
    
    // 读取图像数据
    OV2640_ReadFrame(image_buffer, buffer_size);
    
    // 将图像显示到LVGL
    if (ov2640_config.format == OV2640_FORMAT_RGB565) {
        // 创建LVGL图像缓冲区
        static lv_img_dsc_t img_dsc;
        img_dsc.header.always_zero = 0;
        img_dsc.header.w = width;
        img_dsc.header.h = height;
        img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR; // RGB565格式
        img_dsc.data_size = buffer_size;
        img_dsc.data = (uint8_t *)image_buffer;
        
        // 创建或获取画布对象
        static lv_obj_t *canvas = NULL;
        if (canvas == NULL) {
            // 如果画布不存在，创建一个新的
            canvas = lv_img_create(lv_scr_act());
            lv_obj_center(canvas);
        }
        
        // 显示图像
        lv_img_set_src(canvas, &img_dsc);
    }
    
    // 停止捕获
    OV2640_StopCapture();
    
    // 释放缓冲区
    // 注意：如果LVGL仍然在使用这个缓冲区，需要确保在释放前图像已经显示完成
    // 这里简化处理，直接释放
    free(image_buffer);
}
