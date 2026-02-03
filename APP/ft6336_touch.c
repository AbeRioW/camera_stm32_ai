#include "ft6336_touch.h"

// 引脚定义
#define I2C_SDA_PIN GPIO_PIN_7
#define I2C_SDA_PORT GPIOB
#define I2C_SCL_PIN GPIO_PIN_6
#define I2C_SCL_PORT GPIOB
#define I2C_RST_PIN GPIO_PIN_5
#define I2C_RST_PORT GPIOB
#define I2C_INT_PIN GPIO_PIN_3
#define I2C_INT_PORT GPIOB

// 延时函数
void I2C_Delay(void) {
    for (uint8_t i = 0; i < 10; i++);
}

// SDA设置为输出
void I2C_SDA_Output(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);
}

// SDA设置为输入
void I2C_SDA_Input(void) {
    GPIO_InitTypeDef GPIO_InitStruct = {0};
    GPIO_InitStruct.Pin = I2C_SDA_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(I2C_SDA_PORT, &GPIO_InitStruct);
}

// 发送起始信号
void I2C_Start(void) {
    I2C_SDA_Output();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    I2C_Delay();
}

// 发送停止信号
void I2C_Stop(void) {
    I2C_SDA_Output();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, GPIO_PIN_SET);
    I2C_Delay();
}

// 发送应答信号
void I2C_SendAck(uint8_t ack) {
    I2C_SDA_Output();
    HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, ack);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    I2C_Delay();
}

// 接收应答信号
uint8_t I2C_ReceiveAck(void) {
    uint8_t ack;
    I2C_SDA_Input();
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
    I2C_Delay();
    ack = HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN);
    HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
    I2C_Delay();
    return ack;
}

// 发送一个字节
void I2C_SendByte(uint8_t byte) {
    I2C_SDA_Output();
    for (uint8_t i = 0; i < 8; i++) {
        HAL_GPIO_WritePin(I2C_SDA_PORT, I2C_SDA_PIN, (byte & 0x80) >> 7);
        byte <<= 1;
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        I2C_Delay();
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        I2C_Delay();
    }
    I2C_ReceiveAck();
}

// 接收一个字节
uint8_t I2C_ReceiveByte(void) {
    uint8_t byte = 0;
    I2C_SDA_Input();
    for (uint8_t i = 0; i < 8; i++) {
        byte <<= 1;
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_SET);
        I2C_Delay();
        byte |= HAL_GPIO_ReadPin(I2C_SDA_PORT, I2C_SDA_PIN);
        HAL_GPIO_WritePin(I2C_SCL_PORT, I2C_SCL_PIN, GPIO_PIN_RESET);
        I2C_Delay();
    }
    return byte;
}

// 读取寄存器
uint8_t FT6336_ReadReg(uint8_t reg) {
    uint8_t value;
    I2C_Start();
    I2C_SendByte(FT6336_I2C_ADDR << 1);
    I2C_SendByte(reg);
    I2C_Start();
    I2C_SendByte((FT6336_I2C_ADDR << 1) | 0x01);
    value = I2C_ReceiveByte();
    I2C_SendAck(1);
    I2C_Stop();
    return value;
}

// 写入寄存器
void FT6336_WriteReg(uint8_t reg, uint8_t value) {
    I2C_Start();
    I2C_SendByte(FT6336_I2C_ADDR << 1);
    I2C_SendByte(reg);
    I2C_SendByte(value);
    I2C_Stop();
}

// 初始化FT6336
void FT6336_Init(void) {
    // 复位触摸芯片
    HAL_GPIO_WritePin(I2C_RST_PORT, I2C_RST_PIN, GPIO_PIN_RESET);
    HAL_Delay(50);
    HAL_GPIO_WritePin(I2C_RST_PORT, I2C_RST_PIN, GPIO_PIN_SET);
    HAL_Delay(50);
    
    // 检查芯片ID
    uint8_t chip_id = FT6336_ReadReg(FT6336_REG_CHIP_ID);
    if (chip_id != 0x36) {
        // 芯片ID错误
        return;
    }
    
    // 初始化完成
}

// 获取触摸状态
uint8_t FT6336_GetTouchStatus(void) {
    return FT6336_ReadReg(FT6336_REG_TD_STATUS);
}

// 获取触摸点坐标
void FT6336_GetTouchPoint(FT6336_TouchPoint *point) {
    uint8_t status = FT6336_GetTouchStatus();
    point->status = status;
    
    if (status > 0) {
        uint8_t xh = FT6336_ReadReg(FT6336_REG_TOUCH1_XH);
        uint8_t xl = FT6336_ReadReg(FT6336_REG_TOUCH1_XL);
        uint8_t yh = FT6336_ReadReg(FT6336_REG_TOUCH1_YH);
        uint8_t yl = FT6336_ReadReg(FT6336_REG_TOUCH1_YL);
        
        point->x = ((xh & 0x0F) << 8) | xl;
        point->y = ((yh & 0x0F) << 8) | yl;
    } else {
        point->x = 0;
        point->y = 0;
    }
}


