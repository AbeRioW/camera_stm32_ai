#include "ov2640.h"
#include "lvgl.h"
#include "stdlib.h"
#include "string.h"
#include "stdio.h"

// 全局变量
static OV2640_StatusTypeDef ov2640_status = OV2640_STATUS_IDLE;
static OV2640_ConfigTypeDef ov2640_config;

// 图像缓冲区 - 用于DCMI DMA传输
static uint8_t image_buffer[40 * 40 * 2]; // 40x40 RGB565 全局缓冲区
static volatile uint8_t frame_ready = 0;

// 外部变量声明
extern DCMI_HandleTypeDef hdcmi;

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

// 外部声明
extern DCMI_HandleTypeDef hdcmi;
extern DMA_HandleTypeDef hdma_dcmi;

// 延时函数
static void delay_us(uint32_t us) {
    for (volatile uint32_t i = 0; i < us * 20; i++);
}

// OV2640初始化寄存器配置 - 简化配置，确保基本功能
static const uint8_t ov2640_init_regs[][2] = {
    {0xFF, 0x00},  // 切换到DSP bank
    {0x2C, 0xFF},  // 复位
    {0x2E, 0xDF},  // 复位
    {0xFF, 0x01},  // 切换到Sensor bank
    {0x12, 0x80},  // 软件复位
    {0xFF, 0x01},  // 切换到Sensor bank
    {0x12, 0x04},  // RGB565格式
    {0x0C, 0x00},  // 输出驱动能力
    {0x11, 0x00},  // 时钟分频 (12MHz输入时钟)
    {0x3C, 0x32},  // 时钟设置
    {0x03, 0x1F},  // 同步信号配置 - 启用所有同步信号
    {0x04, 0x00},  // 禁用镜像/翻转
    {0x16, 0x00},  // 禁用水平翻转
    {0x17, 0x11},  // 水平起始
    {0x18, 0x43},  // 水平结束
    {0x19, 0x00},  // 垂直起始
    {0x1A, 0x4B},  // 垂直结束
    {0x32, 0x0A},  // 水平输出大小 40
    {0x37, 0x0A},  // 垂直输出大小 40
    {0xFF, 0x00},  // 切换到DSP bank
    {0x8C, 0x00},  // 输出格式
    {0x41, 0x24},  // 时钟设置
    {0x70, 0x33},  // 时钟配置
    {0x71, 0x33},  // 时钟配置
    {0x72, 0x11},  // 时钟配置
    {0x73, 0xF0},  // 时钟配置
    {0x00, 0x00}   // 结束标记
};

// OV2640 320x240分辨率配置
static const uint8_t ov2640_qvga_regs[][2] = {
    {0xFF, 0x01},  // 切换到Sensor bank
    {0x12, 0x40},  // QVGA RGB
    {0x17, 0x11},  // 水平起始
    {0x18, 0x43},  // 水平结束
    {0x19, 0x00},  // 垂直起始
    {0x1A, 0x4B},  // 垂直结束
    {0x32, 0x09},  // 水平输出大小
    {0x37, 0xC0},  // 垂直输出大小
    {0xFF, 0x00},  // 切换到DSP bank
    {0xE0, 0x04},  // 复位
    {0xC0, 0x64},  // HSIZE8 0x64 = 100 * 8 = 800 -> 640
    {0xC1, 0x4B},  // VSIZE8 0x4B = 75 * 8 = 600 -> 480
    {0x8C, 0x00},  // 输出格式
    {0x86, 0x3D},  // SDE, UV, 缩放使能
    {0xD5, 0x40},  // 缩放输出宽度
    {0xD6, 0x40},  // 缩放输出高度
    {0xD7, 0x03},  // 缩放设置
    {0xD8, 0x90},  // 缩放设置
    {0xD9, 0x12},  // 缩放设置
    {0xDA, 0x40},  // 缩放设置
    {0xDB, 0xF0},  // 缩放设置
    {0xDC, 0xF0},  // 缩放设置
    {0xDD, 0xF0},  // 缩放设置
    {0xDE, 0xF0},  // 缩放设置
    {0xE0, 0x00},  // 缩放设置
    {0x71, 0x00},  // 时钟设置
    {0xFF, 0x00},  // 切换到DSP bank
    {0xDA, 0x08},  // 图像水平输出大小 320
    {0xD7, 0x03},  // 图像垂直输出大小 240
    {0xE0, 0x00},  // 复位完成
    {0x00, 0x00}   // 结束标记
};

// OV2640 40x40分辨率配置
static const uint8_t ov2640_40x40_regs[][2] = {
    {0xFF, 0x01},  // 切换到Sensor bank
    {0x12, 0x04},  // RGB565格式
    {0x17, 0x11},  // 水平起始
    {0x18, 0x43},  // 水平结束
    {0x19, 0x00},  // 垂直起始
    {0x1A, 0x4B},  // 垂直结束
    {0x32, 0x0A},  // 水平输出大小 40 (0x0A = 10 * 4)
    {0x37, 0x0A},  // 垂直输出大小 40 (0x0A = 10 * 4)
    {0x03, 0x0F},  // 同步信号配置
    {0x3C, 0x32},  // 时钟设置
    {0xFF, 0x00},  // 切换到DSP bank
    {0xE0, 0x04},  // 复位
    {0xC0, 0x05},  // HSIZE8 40 (0x05 = 5 * 8)
    {0xC1, 0x05},  // VSIZE8 40 (0x05 = 5 * 8)
    {0x8C, 0x00},  // 输出格式
    {0x86, 0x3D},  // SDE, UV, 缩放使能
    {0xD5, 0x28},  // 缩放输出宽度 40 (0x28 = 40)
    {0xD6, 0x28},  // 缩放输出高度 40 (0x28 = 40)
    {0xD7, 0x03},  // 缩放设置
    {0xD8, 0x90},  // 缩放设置
    {0xD9, 0x12},  // 缩放设置
    {0xDA, 0x28},  // 缩放设置
    {0xDB, 0x1E},  // 缩放设置
    {0xDC, 0x1E},  // 缩放设置
    {0xDD, 0x1E},  // 缩放设置
    {0xDE, 0x1E},  // 缩放设置
    {0xE0, 0x00},  // 缩放设置
    {0x71, 0x00},  // 时钟设置
    {0x41, 0x24},  // 时钟设置
    {0xDA, 0x05},  // 图像水平输出大小 40 (0x05 = 5 * 8)
    {0xD7, 0x05},  // 图像垂直输出大小 40 (0x05 = 5 * 8)
    {0xE0, 0x00},  // 复位完成
    {0x00, 0x00}   // 结束标记
};

// OV2640 160x120分辨率配置
static const uint8_t ov2640_qqvga_regs[][2] = {
    {0xFF, 0x01},  // 切换到Sensor bank
    {0x12, 0x40},  // QQVGA RGB
    {0x17, 0x11},  // 水平起始
    {0x18, 0x43},  // 水平结束
    {0x19, 0x00},  // 垂直起始
    {0x1A, 0x4B},  // 垂直结束
    {0x32, 0x09},  // 水平输出大小
    {0x37, 0xC0},  // 垂直输出大小
    {0xFF, 0x00},  // 切换到DSP bank
    {0xE0, 0x04},  // 复位
    {0xC0, 0x64},  // HSIZE8
    {0xC1, 0x4B},  // VSIZE8
    {0x8C, 0x00},  // 输出格式
    {0x86, 0x3D},  // SDE, UV, 缩放使能
    {0xD5, 0x20},  // 缩放输出宽度 160
    {0xD6, 0x20},  // 缩放输出高度 120
    {0xD7, 0x03},  // 缩放设置
    {0xD8, 0x90},  // 缩放设置
    {0xD9, 0x12},  // 缩放设置
    {0xDA, 0x20},  // 缩放设置
    {0xDB, 0x78},  // 缩放设置
    {0xDC, 0x78},  // 缩放设置
    {0xDD, 0x78},  // 缩放设置
    {0xDE, 0x78},  // 缩放设置
    {0xE0, 0x00},  // 缩放设置
    {0x71, 0x00},  // 时钟设置
    {0xFF, 0x00},  // 切换到DSP bank
    {0xDA, 0x04},  // 图像水平输出大小 160
    {0xD7, 0x01},  // 图像垂直输出大小 120
    {0xE0, 0x00},  // 复位完成
    {0x00, 0x00}   // 结束标记
};

// 初始化OV2640
void OV2640_Init(void) {
    ov2640_status = OV2640_STATUS_INITIALIZING;
    
    // 初始化SCCB
    SCCB_Init();
    
    // 初始化PWDN引脚为低电平，启用摄像头
    HAL_GPIO_WritePin(OV2640_PWDN_GPIO_Port, OV2640_PWDN_Pin, GPIO_PIN_RESET);
    HAL_Delay(100); // 增加延时
    
    // 复位摄像头
    HAL_GPIO_WritePin(OV2640_RST_GPIO_Port, OV2640_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(100); // 增加延时
    HAL_GPIO_WritePin(OV2640_RST_GPIO_Port, OV2640_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(200); // 增加延时以确保摄像头稳定
    
    // 软件复位
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0x12, 0x80); // COM7寄存器，软件复位
    HAL_Delay(100); // 增加延时
    
    // 检查传感器ID
    uint8_t id_h = OV2640_I2C_Read(OV2640_REG_SENSOR_ID_H);
    uint8_t id_l = OV2640_I2C_Read(OV2640_REG_SENSOR_ID_L);
    
    // 调试信息：打印读取到的ID
    printf("OV2640 Sensor ID: 0x%02X%02X\r\n", id_h, id_l);
    
    if ((id_h != 0x26) || (id_l != 0x42)) {
        // 传感器ID错误
        printf("OV2640 Init Failed: Invalid ID\r\n");
        ov2640_status = OV2640_STATUS_ERROR;
        return;
    }
    
    // 初始化默认配置
    ov2640_config.resolution = OV2640_RES_320x240;
    ov2640_config.format = OV2640_FORMAT_RGB565;
    ov2640_config.brightness = 0x80;
    ov2640_config.contrast = 0x80;
    ov2640_config.saturation = 0x80;
    
    // 写入初始化寄存器配置
    for (int i = 0; ov2640_init_regs[i][0] != 0x00 || ov2640_init_regs[i][1] != 0x00; i++) {
        OV2640_I2C_Write(ov2640_init_regs[i][0], ov2640_init_regs[i][1]);
        HAL_Delay(2); // 增加延时以确保寄存器写入完成
    }
    
    // 设置输出格式为RGB565（参考探索者开发板的配置）
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0xDA, 0x09); // 图像水平输出大小
    OV2640_I2C_Write(0xD7, 0x03); // 图像垂直输出大小
    OV2640_I2C_Write(0xDF, 0x02); // 图像格式设置
    OV2640_I2C_Write(0x33, 0xA0); // 图像格式设置
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0x3C, 0x00); // 时钟设置
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0xE1, 0x67); // 图像格式设置
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0xE0, 0x00); // 图像格式设置
    OV2640_I2C_Write(0xE1, 0x00); // 图像格式设置
    OV2640_I2C_Write(0xE5, 0x00); // 图像格式设置
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0xD7, 0x00); // 图像垂直输出大小
    OV2640_I2C_Write(0xDA, 0x00); // 图像水平输出大小
    OV2640_I2C_Write(0xE0, 0x00); // 图像格式设置
    
    // 再次设置输出格式为RGB565，确保不会被初始化配置覆盖
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0x12, 0x04); // RGB565格式
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0x8C, 0x00); // 输出格式设置为RGB
    
    // 再次设置时钟，确保不会被初始化配置覆盖
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0x11, 0x00); // 时钟分频
    OV2640_I2C_Write(0x3C, 0x32); // 时钟设置
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0x41, 0x24); // 时钟设置
    
    // 设置默认分辨率
    OV2640_SetResolution(OV2640_RES_40x40);
    
    // 再次设置输出格式为RGB565，确保不会被分辨率配置覆盖
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0x12, 0x04); // RGB565格式
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0x8C, 0x00); // 输出格式设置为RGB
    
    // 设置默认图像参数
    OV2640_SetBrightness(0x80);
    OV2640_SetContrast(0x80);
    OV2640_SetSaturation(0x80);
    
    // 再次设置输出格式为RGB565，确保不会被图像参数配置覆盖
    OV2640_SelectBank(1); // 切换到Sensor bank
    OV2640_I2C_Write(0x12, 0x04); // RGB565格式
    OV2640_SelectBank(0); // 切换到DSP bank
    OV2640_I2C_Write(0x8C, 0x00); // 输出格式设置为RGB
    
    // 启用颜色条测试模式，用于验证传感器是否正常输出数据
    OV2640_SelectBank(1); // 切换到Sensor bank
    uint8_t com7 = OV2640_I2C_Read(0x12);
    com7 |= 0x02; // 启用颜色条
    OV2640_I2C_Write(0x12, com7);
    
    // 确保同步信号配置正确
    OV2640_I2C_Write(0x03, 0x1F); // 启用所有同步信号
    
    // 确保RGB565格式
    OV2640_I2C_Write(0x12, 0x04); // RGB565格式
    
    // 读取并打印更多寄存器值，用于调试
    OV2640_SelectBank(1); // 切换到Sensor bank
    com7 = OV2640_I2C_Read(0x12);
    uint8_t clkrc = OV2640_I2C_Read(0x11);
    uint8_t com3 = OV2640_I2C_Read(0x03);
    uint8_t com14 = OV2640_I2C_Read(0x3C);
    OV2640_SelectBank(0); // 切换到DSP bank
    uint8_t ctrl0 = OV2640_I2C_Read(0xC2);
    uint8_t ctrl1 = OV2640_I2C_Read(0xC3);
    uint8_t sizel = OV2640_I2C_Read(0x8C);
    uint8_t clkdiv = OV2640_I2C_Read(0x41);
    printf("OV2640 Registers: COM7=0x%02X, CLKRC=0x%02X, COM3=0x%02X, COM14=0x%02X, CTRL0=0x%02X, CTRL1=0x%02X, SIZE=0x%02X, CLKDIV=0x%02X\r\n", 
           com7, clkrc, com3, com14, ctrl0, ctrl1, sizel, clkdiv);
    
    ov2640_status = OV2640_STATUS_READY;
}

// 获取摄像头状态
OV2640_StatusTypeDef OV2640_GetStatus(void) {
    return ov2640_status;
}

// 设置分辨率
void OV2640_SetResolution(uint8_t resolution) {
    if (resolution > OV2640_RES_1600x1200) {
        return;
    }
    
    ov2640_config.resolution = resolution;
    
    // 根据分辨率写入不同的寄存器配置
    const uint8_t (*regs)[2] = NULL;
    
    switch (resolution) {
        case OV2640_RES_40x40:
            regs = ov2640_40x40_regs;
            break;
        case OV2640_RES_160x120:
            regs = ov2640_qqvga_regs;
            break;
        case OV2640_RES_320x240:
            regs = ov2640_qvga_regs;
            break;
        default:
            regs = ov2640_qvga_regs;
            break;
    }
    
    if (regs != NULL) {
        for (int i = 0; regs[i][0] != 0x00 || regs[i][1] != 0x00; i++) {
            OV2640_I2C_Write(regs[i][0], regs[i][1]);
            HAL_Delay(1); // 增加延时以确保寄存器写入完成
        }
    }
}

// 设置图像格式
void OV2640_SetFormat(uint8_t format) {
    if (format > OV2640_FORMAT_JPEG) {
        return;
    }
    
    ov2640_config.format = format;
    
    // 选择图像格式
    switch (format) {
        case OV2640_FORMAT_YUV422:
            OV2640_SelectBank(1); // 切换到Sensor bank
            OV2640_I2C_Write(0x12, 0x00);  // YUV422格式
            break;
        case OV2640_FORMAT_RGB565:
            OV2640_SelectBank(1); // 切换到Sensor bank
            OV2640_I2C_Write(0x12, 0x04);  // RGB565格式
            // 确保输出格式正确
            OV2640_SelectBank(0); // 切换到DSP bank
            OV2640_I2C_Write(0x8C, 0x00); // 输出格式设置为RGB
            break;
        case OV2640_FORMAT_JPEG:
            OV2640_SelectBank(1); // 切换到Sensor bank
            OV2640_I2C_Write(0x12, 0x08);  // JPEG格式
            break;
    }
}

// 设置亮度
void OV2640_SetBrightness(uint8_t brightness) {
    ov2640_config.brightness = brightness;
    OV2640_SelectBank(0);
    OV2640_I2C_Write(0x9B, brightness);
}

// 设置对比度
void OV2640_SetContrast(uint8_t contrast) {
    ov2640_config.contrast = contrast;
    OV2640_SelectBank(0);
    OV2640_I2C_Write(0x9C, contrast);
}

// 设置饱和度
void OV2640_SetSaturation(uint8_t saturation) {
    ov2640_config.saturation = saturation;
    OV2640_SelectBank(0);
    OV2640_I2C_Write(0x9D, saturation);
}

// 获取图像尺寸
static void OV2640_GetImageSize(uint16_t *width, uint16_t *height) {
    switch (ov2640_config.resolution) {
        case OV2640_RES_40x40:
            *width = 40;
            *height = 40;
            break;
        case OV2640_RES_160x120:
            *width = 160;
            *height = 120;
            break;
        case OV2640_RES_320x240:
            *width = 320;
            *height = 240;
            break;
        case OV2640_RES_640x480:
            *width = 640;
            *height = 480;
            break;
        default:
            *width = 320;
            *height = 240;
            break;
    }
}

// 开始捕获图像
void OV2640_StartCapture(void) {
    if (ov2640_status == OV2640_STATUS_READY) {
        ov2640_status = OV2640_STATUS_CAPTURING;
        frame_ready = 0;
        
        // 获取图像尺寸
        uint16_t width, height;
        OV2640_GetImageSize(&width, &height);
        
        printf("Starting capture: %dx%d\r\n", width, height);
        
        // 计算缓冲区大小 (RGB565 = 2 bytes per pixel)
        uint32_t buffer_size = width * height * 2;
        
        printf("Buffer size: %d bytes\r\n", buffer_size);
        printf("Using global buffer at: 0x%p\r\n", image_buffer);
        
        // 确保DCMI和DMA已正确初始化
        if (hdcmi.Instance == NULL) {
            printf("DCMI not initialized\r\n");
            return;
        }
        
        // 启用DCMI的所有中断
        __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_FRAME);
        __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_VSYNC);
        __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_LINE);
        __HAL_DCMI_ENABLE_IT(&hdcmi, DCMI_IT_ERR);
        
        // 启动DCMI DMA传输 - 使用连续模式
        HAL_StatusTypeDef status = HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)image_buffer, width * height);
        printf("DCMI Start DMA status: %d\r\n", status);
        
        // 等待帧就绪标志
        uint32_t timeout = 1000000; // 设置超时
        uint32_t counter = 0;
        while (frame_ready == 0 && counter < timeout) {
            counter++;
        }
        printf("Waited %d cycles for frame ready\r\n", counter);
        printf("Frame ready flag: %d\r\n", frame_ready);
        
        // 打印缓冲区数据的前20个字节，以验证数据是否正确
        printf("Buffer data: ");
        for (int i = 0; i < 20; i++) {
            printf("%02X ", image_buffer[i]);
        }
        printf("\r\n");
        
        // 检查缓冲区数据是否全为0
        uint8_t all_zero = 1;
        for (int i = 0; i < buffer_size; i++) {
            if (image_buffer[i] != 0) {
                all_zero = 0;
                break;
            }
        }
        printf("Buffer data all zero: %d\r\n", all_zero);
        
        // 停止DCMI
        HAL_DCMI_Stop(&hdcmi);
        
        ov2640_status = OV2640_STATUS_READY;
    } else {
        printf("Camera not ready for capture, status: %d\r\n", ov2640_status);
    }
}

// 停止捕获图像
void OV2640_StopCapture(void) {
    if (ov2640_status == OV2640_STATUS_CAPTURING) {
        HAL_DCMI_Stop(&hdcmi);
        ov2640_status = OV2640_STATUS_READY;
    }
}

// 读取图像数据
void OV2640_ReadFrame(uint8_t *buffer, uint32_t size) {
    if (image_buffer != NULL && buffer != NULL) {
        memcpy(buffer, image_buffer, size);
    }
}

// 检查帧是否准备好
uint8_t OV2640_IsFrameReady(void) {
    return frame_ready;
}

// 清除帧就绪标志
void OV2640_ClearFrameReady(void) {
    frame_ready = 0;
}

// 获取图像缓冲区
uint8_t* OV2640_GetBuffer(void) {
    return image_buffer;
}

// 帧就绪处理函数
void OV2640_OnFrameReady(void) {
	printf("OV2640_OnFrameReady called\r\n");
	frame_ready = 1;
}

// 选择寄存器 bank
static void OV2640_SelectBank(uint8_t bank) {
    OV2640_I2C_Write(OV2640_REG_BANK_SELECT, bank);
}

// I2C写入函数
static void OV2640_I2C_Write(uint8_t reg, uint8_t value) {
    SCCB_Start();
    SCCB_SendByte((OV2640_I2C_ADDR << 1) | 0x00); // 写入模式，7位地址左移一位
    SCCB_SendByte(reg); // 发送寄存器地址
    SCCB_SendByte(value); // 发送数据
    SCCB_Stop();
}

// I2C读取函数
static uint8_t OV2640_I2C_Read(uint8_t reg) {
    uint8_t value = 0;
    
    // 先写入寄存器地址
    SCCB_Start();
    SCCB_SendByte((OV2640_I2C_ADDR << 1) | 0x00); // 写入模式，7位地址左移一位
    SCCB_SendByte(reg); // 发送寄存器地址
    SCCB_Stop();
    
    // 然后读取数据
    SCCB_Start();
    SCCB_SendByte((OV2640_I2C_ADDR << 1) | 0x01); // 读取模式，7位地址左移一位
    value = SCCB_ReceiveByte();
    SCCB_Stop();
    
    return value;
}

// SCCB初始化
static void SCCB_Init(void) {
    // 初始化SCCB引脚
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    
    // 启用GPIO时钟
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    // 初始化SCL引脚
    GPIO_InitStruct.Pin = SCCB_SCL_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SCCB_SCL_PORT, &GPIO_InitStruct);
    
    // 初始化SDA引脚
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 初始化复位和电源引脚
    __HAL_RCC_GPIOD_CLK_ENABLE();
    
    // 初始化RST引脚
    GPIO_InitStruct.Pin = OV2640_RST_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OV2640_RST_GPIO_Port, &GPIO_InitStruct);
    
    // 初始化PWDN引脚
    GPIO_InitStruct.Pin = OV2640_PWDN_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(OV2640_PWDN_GPIO_Port, &GPIO_InitStruct);
    
    // 初始状态
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET);
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
    delay_us(5);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_RESET);
    delay_us(5);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    delay_us(5);
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
    delay_us(5);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET);
    delay_us(5);
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
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, ack ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    delay_us(5);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    delay_us(5);
}

// SCCB接收应答信号
static uint8_t SCCB_ReceiveAck(void) {
    uint8_t ack = 1;
    
    // SDA设置为输入
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 接收应答信号
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    delay_us(5);
    ack = HAL_GPIO_ReadPin(SCCB_SDA_PORT, SCCB_SDA_PIN);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    delay_us(5);
    
    return ack;
}

// SCCB发送一个字节
static void SCCB_SendByte(uint8_t byte) {
    // SDA设置为输出
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 发送8位数据
    for (int8_t i = 7; i >= 0; i--) {
        uint8_t bit = (byte >> i) & 0x01;
        HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, bit ? GPIO_PIN_SET : GPIO_PIN_RESET);
        delay_us(10); // 增加延时
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
        delay_us(10); // 增加延时
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    }
    
    // 接收应答
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET);
    delay_us(10); // 增加延时
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    delay_us(10); // 增加延时
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
}

// SCCB接收一个字节
static uint8_t SCCB_ReceiveByte(void) {
    uint8_t byte = 0;
    
    // SDA设置为输入
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = SCCB_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    
    // 接收8位数据
    for (uint8_t i = 0; i < 8; i++) {
        delay_us(10); // 增加延时
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
        delay_us(10); // 增加延时
        byte = (byte << 1) | HAL_GPIO_ReadPin(SCCB_SDA_PORT, SCCB_SDA_PIN);
        HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    }
    
    // 发送非应答信号
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    HAL_GPIO_Init(SCCB_SDA_PORT, &GPIO_InitStruct);
    HAL_GPIO_WritePin(SCCB_SDA_PORT, SCCB_SDA_PIN, GPIO_PIN_SET); // 非应答
    delay_us(10);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(SCCB_SCL_PORT, SCCB_SCL_PIN, GPIO_PIN_RESET);
    
    return byte;
}

// 显示图像到LVGL - 使用Canvas方式
void OV2640_DisplayImage(void) {
    uint16_t width, height;
    OV2640_GetImageSize(&width, &height);
    
    // 计算缓冲区大小
    uint32_t buffer_size = width * height * 2; // RGB565格式
    
    // 启动捕获
    OV2640_StartCapture();
    
    // 等待帧捕获完成
    while (!OV2640_IsFrameReady()) {
        // 可以在这里添加超时处理
    }
    
    // 获取图像缓冲区
    uint8_t *buffer = OV2640_GetBuffer();
    if (buffer == NULL) {
        return;
    }
    
    // 在LVGL中显示图像
    // 方法1: 使用lv_img显示静态图像
    if (ov2640_config.format == OV2640_FORMAT_RGB565) {
        // 创建LVGL图像描述符
        static lv_img_dsc_t img_dsc;
        img_dsc.header.always_zero = 0;
        img_dsc.header.w = width;
        img_dsc.header.h = height;
        img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR; // RGB565格式
        img_dsc.data_size = buffer_size;
        img_dsc.data = buffer;
        
        // 创建或更新图像对象
        static lv_obj_t *img_obj = NULL;
        if (img_obj == NULL) {
            img_obj = lv_img_create(lv_scr_act());
            lv_obj_set_pos(img_obj, 0, 0);
        }
        
        // 设置图像源
        lv_img_set_src(img_obj, &img_dsc);
        
        // 刷新显示
        lv_obj_invalidate(img_obj);
    }
    
    // 清除帧就绪标志
    OV2640_ClearFrameReady();
}

// 获取当前配置
void OV2640_GetConfig(OV2640_ConfigTypeDef *config) {
    if (config != NULL) {
        *config = ov2640_config;
    }
}

// 获取传感器ID
void OV2640_GetSensorID(uint8_t *id_h, uint8_t *id_l) {
    if (id_h != NULL && id_l != NULL) {
        OV2640_SelectBank(1); // 切换到Sensor bank
        *id_h = OV2640_I2C_Read(OV2640_REG_SENSOR_ID_H);
        *id_l = OV2640_I2C_Read(OV2640_REG_SENSOR_ID_L);
    }
}
