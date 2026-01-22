#include "lcd_ILI9341V.h"

 void ILI9341_Select() {
    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
}


void ILI9341_Unselect() {
    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}

static void ILI9341_Reset() {
    HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_SET);
}


static void ILI9341_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ILI9341_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}


static void ILI9341_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_SET);

    // split data in small chunks because HAL can't send more then 64K at once
    while(buff_size > 0) {
        uint16_t chunk_size = buff_size > 32768 ? 32768 : buff_size;
        HAL_SPI_Transmit(&ILI9341_SPI_PORT, buff, chunk_size, HAL_MAX_DELAY);
        buff += chunk_size;
        buff_size -= chunk_size;
    }
}


 void ILI9341_SetAddressWindow(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    // column address set
    ILI9341_WriteCommand(0x2A); // CASET
    {
        uint8_t data[] = { (x0 >> 8) & 0xFF, x0 & 0xFF, (x1 >> 8) & 0xFF, x1 & 0xFF };
        ILI9341_WriteData(data, sizeof(data));
    }

    // row address set
    ILI9341_WriteCommand(0x2B); // RASET
    {
        uint8_t data[] = { (y0 >> 8) & 0xFF, y0 & 0xFF, (y1 >> 8) & 0xFF, y1 & 0xFF };
        ILI9341_WriteData(data, sizeof(data));
    }

    // write to RAM
    ILI9341_WriteCommand(0x2C); // RAMWR
}


void ILI9341_Init() {
    ILI9341_Select();
    ILI9341_Reset();

    // command list is based on https://github.com/martnak/STM32-ILI9341

    // SOFTWARE RESET
    ILI9341_WriteCommand(0x01);
    HAL_Delay(1000);
        
    // POWER CONTROL A
    ILI9341_WriteCommand(0xCB);
    {
        uint8_t data[] = { 0x39, 0x2C, 0x00, 0x34, 0x02 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL B
    ILI9341_WriteCommand(0xCF);
    {
        uint8_t data[] = { 0x00, 0xC1, 0x30 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL A
    ILI9341_WriteCommand(0xE8);
    {
        uint8_t data[] = { 0x85, 0x00, 0x78 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DRIVER TIMING CONTROL B
    ILI9341_WriteCommand(0xEA);
    {
        uint8_t data[] = { 0x00, 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER ON SEQUENCE CONTROL
    ILI9341_WriteCommand(0xED);
    {
        uint8_t data[] = { 0x64, 0x03, 0x12, 0x81 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // PUMP RATIO CONTROL
    ILI9341_WriteCommand(0xF7);
    {
        uint8_t data[] = { 0x20 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL,VRH[5:0]
    ILI9341_WriteCommand(0xC0);
    {
        uint8_t data[] = { 0x23 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POWER CONTROL,SAP[2:0];BT[3:0]
    ILI9341_WriteCommand(0xC1);
    {
        uint8_t data[] = { 0x10 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // VCM CONTROL
    ILI9341_WriteCommand(0xC5);
    {
        uint8_t data[] = { 0x3E, 0x28 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // VCM CONTROL 2
    ILI9341_WriteCommand(0xC7);
    {
        uint8_t data[] = { 0x86 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // MEMORY ACCESS CONTROL
    ILI9341_WriteCommand(0x36);
    {
        uint8_t data[] = { ILI9341_ROTATION };
        ILI9341_WriteData(data, sizeof(data));
    }

    // PIXEL FORMAT
    ILI9341_WriteCommand(0x3A);
    {
        uint8_t data[] = { 0x55 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // FRAME RATIO CONTROL, STANDARD RGB COLOR
    ILI9341_WriteCommand(0xB1);
    {
        uint8_t data[] = { 0x00, 0x18 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // DISPLAY FUNCTION CONTROL
    ILI9341_WriteCommand(0xB6);
    {
        uint8_t data[] = { 0x08, 0x82, 0x27 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // 3GAMMA FUNCTION DISABLE
    ILI9341_WriteCommand(0xF2);
    {
        uint8_t data[] = { 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // GAMMA CURVE SELECTED
    ILI9341_WriteCommand(0x26);
    {
        uint8_t data[] = { 0x01 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // POSITIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE0);
    {
        uint8_t data[] = { 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, 0x4E, 0xF1,
                           0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00 };
        ILI9341_WriteData(data, sizeof(data));
    }

    // NEGATIVE GAMMA CORRECTION
    ILI9341_WriteCommand(0xE1);
    {
        uint8_t data[] = { 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, 0x31, 0xC1,
                           0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F };
        ILI9341_WriteData(data, sizeof(data));
    }

    // EXIT SLEEP
    ILI9341_WriteCommand(0x11);
    HAL_Delay(120);

    // TURN ON DISPLAY
    ILI9341_WriteCommand(0x29);

    // MADCTL
    ILI9341_WriteCommand(0x36);
    {
        uint8_t data[] = { ILI9341_ROTATION };
        ILI9341_WriteData(data, sizeof(data));
    }

    ILI9341_Unselect();
}

void ILI9341_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT))
        return;

    ILI9341_Select();

    ILI9341_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color & 0xFF, color >> 8 };
    ILI9341_WriteData(data, sizeof(data));

    ILI9341_Unselect();
}

//static void ILI9341_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
//    uint32_t i, b, j;

//    ILI9341_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

//    for(i = 0; i < font.height; i++) {
//        b = font.data[(ch - 32) * font.height + i];
//        for(j = 0; j < font.width; j++) {
//            if((b << j) & 0x8000)  {
//                uint8_t data[] = { color >> 8, color & 0xFF };
//                ILI9341_WriteData(data, sizeof(data));
//            } else {
//                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
//                ILI9341_WriteData(data, sizeof(data));
//            }
//        }
//    }
//}

void ILI9341_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ILI9341_WIDTH) || (y >= ILI9341_HEIGHT)) return;
    if((x + w - 1) >= ILI9341_WIDTH) w = ILI9341_WIDTH - x;
    if((y + h - 1) >= ILI9341_HEIGHT) h = ILI9341_HEIGHT - y;

    ILI9341_Select();
    ILI9341_SetAddressWindow(x, y, x+w-1, y+h-1);

    // Set RS to data mode
    HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_SET);
    
    // Calculate total pixels
    uint32_t total_pixels = (uint32_t)w * h;
   // uint8_t color_data[2] = { color >> 8, color & 0xFF };
   uint8_t color_data[2] = { color & 0xFF, color >> 8 };
    // Send color data in chunks
    while(total_pixels > 0) {
        uint16_t chunk_size = total_pixels > 16384 ? 16384 : total_pixels;
        for(uint16_t i = 0; i < chunk_size; i++) {
            HAL_SPI_Transmit(&ILI9341_SPI_PORT, color_data, sizeof(color_data), HAL_MAX_DELAY);
        }
        total_pixels -= chunk_size;
    }

    ILI9341_Unselect();
}

void ILI9341_FillScreen(uint16_t color) {
    ILI9341_FillRectangle(0, 0, ILI9341_WIDTH, ILI9341_HEIGHT, color);
}

void ILI9341_InvertColors(bool invert) {
    ILI9341_Select();
    ILI9341_WriteCommand(invert ? 0x21 /* INVON */ : 0x20 /* INVOFF */);
    ILI9341_Unselect();
}


#if 0
//����LCD��Ҫ����
//Ĭ��Ϊ����
_lcd_dev lcddev;

//������ɫ,������ɫ
uint16_t POINT_COLOR = 0x0000,BACK_COLOR = 0xFFFF;  
uint16_t DeviceCode;	 


static void LCD_WR_REG(uint8_t data)
{
   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
   HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_RESET);
   HAL_SPI_Transmit(&hspi1,&data,1,HAL_MAX_DELAY);
   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}

void LCD_WR_DATA(uint8_t data)
{
   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
   HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_SET);
   HAL_SPI_Transmit(&hspi1,&data,1,0xffff);
   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}


uint8_t LCD_RD_DATA(void)
{
	 uint8_t data;
	 uint8_t tx = 0xff;
   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);      
	 HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_SET);
	
	 HAL_SPI_TransmitReceive(&hspi1,&tx,&data,1,0xffff);
	    HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
	 return data;
}


void LCD_WriteReg(uint8_t LCD_Reg, uint16_t LCD_RegValue)
{	
	LCD_WR_REG(LCD_Reg);  
	LCD_WR_DATA(LCD_RegValue);	    		 
}	   

uint8_t LCD_ReadReg(uint8_t LCD_Reg)
{
	LCD_WR_REG(LCD_Reg);
  return LCD_RD_DATA();
}


void LCD_WriteRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.wramcmd);
}	 


void LCD_ReadRAM_Prepare(void)
{
	LCD_WR_REG(lcddev.rramcmd);
}	 


/*****************************************************************************
 * @name       :void Lcd_WriteData_16Bit(u16 Data)
 * @date       :2018-08-09 
 * @function   :Write an 16-bit command to the LCD screen
 * @parameters :Data:Data to be written
 * @retvalue   :None
******************************************************************************/	 
void Lcd_WriteData_16Bit(uint16_t Data)
{	
	 uint8_t high = Data>>8;
	 uint8_t low  = (uint8_t)Data;
   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);      
	 HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_SET);
   HAL_SPI_Transmit(&hspi1,&high,1,0xffff);
   HAL_SPI_Transmit(&hspi1,&low,1,0xffff);
	  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
}

uint16_t Color_To_565(uint8_t r, uint8_t g, uint8_t b)
{
	return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3);
}

//uint16_t Lcd_ReadData_16Bit(void)
//{
//	uint8_t r,g,b;
//   HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);      
//	 HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_RESET);
//	SPI_SetSpeed(SPI1,0);
//	//dummy data
//	SPI_WriteByte(SPI1,0xFF);
//	//8bit:red data
//	//16bit:red and green data
//	r=SPI_WriteByte(SPI1,0xFF);
//	
//	//8bit:green data
//	//16bit:blue data
//	g=SPI_WriteByte(SPI1,0xFF);
//	
//	//blue data
//	b=SPI_WriteByte(SPI1,0xFF);
//	//r >>= 8;
//	//g >>= 8;
//	//b >>= 8;
//	SPI_SetSpeed(SPI1,1);
//	LCD_CS_SET;
//	return Color_To_565(r, g, b);
//}


void LCD_DrawPoint(uint16_t x,uint16_t y)
{
	LCD_SetCursor(x,y);//���ù��λ�� 
	Lcd_WriteData_16Bit(POINT_COLOR); 
}

//uint16_t LCD_ReadPoint(uint16_t x,uint16_t y)
//{
//	LCD_SetCursor(x,y);//���ù��λ�� 
//	LCD_ReadRAM_Prepare();
//	return Lcd_ReadData_16Bit();
//}

void LCD_Clear(uint16_t Color)
{
  unsigned int i,m;
  uint8_t data[2];
  data[0] = (uint8_t)(Color>>8);
  data[1] = (uint8_t)Color;
  LCD_SetWindows(0,0,lcddev.width-1,lcddev.height-1);
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, LCD_RS_Pin, GPIO_PIN_SET);
  for(i=0;i<lcddev.height;i++)
  {
    for(m=0;m<lcddev.width;m++)
    {
      HAL_SPI_Transmit(&hspi1,data,2,0xffff);
    }
  }
  HAL_GPIO_WritePin(GPIOB, LCD_CS_Pin, GPIO_PIN_SET);
} 

void LCD_Init(void)
{
	 LCD_RESET(); //LCD ��λ
	
	//*************2.8 ILI9341 IPS��ʼ��**********//	
	LCD_WR_REG(0xCF);  
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0xC1); 
	LCD_WR_DATA(0x30); 
 
	LCD_WR_REG(0xED);  
	LCD_WR_DATA(0x64); 
	LCD_WR_DATA(0x03); 
	LCD_WR_DATA(0X12); 
	LCD_WR_DATA(0X81); 
 
	LCD_WR_REG(0xE8);  
	LCD_WR_DATA(0x85); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x78); 

	LCD_WR_REG(0xCB);  
	LCD_WR_DATA(0x39); 
	LCD_WR_DATA(0x2C); 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x34); 
	LCD_WR_DATA(0x02); 
	
	LCD_WR_REG(0xF7);  
	LCD_WR_DATA(0x20); 
 
	LCD_WR_REG(0xEA);  
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x00); 

	LCD_WR_REG(0xC0);       //Power control 
	LCD_WR_DATA(0x13);     //VRH[5:0] 
 
	LCD_WR_REG(0xC1);       //Power control 
	LCD_WR_DATA(0x13);     //SAP[2:0];BT[3:0] 
 
	LCD_WR_REG(0xC5);       //VCM control 
	LCD_WR_DATA(0x22);   //22
	LCD_WR_DATA(0x35);   //35
 
	LCD_WR_REG(0xC7);       //VCM control2 
	LCD_WR_DATA(0xBD);  //AF

	LCD_WR_REG(0x21);

	LCD_WR_REG(0x36);       // Memory Access Control 
	LCD_WR_DATA(0x08); 

	LCD_WR_REG(0xB6);  
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0xA2); 

	LCD_WR_REG(0x3A);       
	LCD_WR_DATA(0x55); 

	LCD_WR_REG(0xF6);  //Interface Control
	LCD_WR_DATA(0x01); 
	LCD_WR_DATA(0x30);  //MCU

	LCD_WR_REG(0xB1);       //VCM control 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x1B); 
 
	LCD_WR_REG(0xF2);       // 3Gamma Function Disable 
	LCD_WR_DATA(0x00); 
 
	LCD_WR_REG(0x26);       //Gamma curve selected 
	LCD_WR_DATA(0x01); 
 
	LCD_WR_REG(0xE0);       //Set Gamma 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x35); 
	LCD_WR_DATA(0x31); 
	LCD_WR_DATA(0x0B); 
	LCD_WR_DATA(0x0E); 
	LCD_WR_DATA(0x06); 
	LCD_WR_DATA(0x49); 
	LCD_WR_DATA(0xA7); 
	LCD_WR_DATA(0x33); 
	LCD_WR_DATA(0x07); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x03); 
	LCD_WR_DATA(0x0C); 
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0x00); 
 
	LCD_WR_REG(0XE1);       //Set Gamma 
	LCD_WR_DATA(0x00); 
	LCD_WR_DATA(0x0A); 
	LCD_WR_DATA(0x0F); 
	LCD_WR_DATA(0x04); 
	LCD_WR_DATA(0x11); 
	LCD_WR_DATA(0x08); 
	LCD_WR_DATA(0x36); 
	LCD_WR_DATA(0x58); 
	LCD_WR_DATA(0x4D); 
	LCD_WR_DATA(0x07); 
	LCD_WR_DATA(0x10); 
	LCD_WR_DATA(0x0C); 
	LCD_WR_DATA(0x32); 
	LCD_WR_DATA(0x34); 
	LCD_WR_DATA(0x0F); 

	LCD_WR_REG(0x11);       //Exit Sleep 
	HAL_Delay(120); 
	LCD_WR_REG(0x29);       //Display on 

  LCD_direction(USE_HORIZONTAL);//����LCD��ʾ���� 
	LCD_Clear(RED);//��ȫ����ɫ
}


void LCD_RESET(void)
{
	HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(50);
	HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_RESET);
	HAL_Delay(100);
	HAL_GPIO_WritePin(GPIOB, LCD_RST_Pin, GPIO_PIN_SET);
	HAL_Delay(50);
}

void LCD_SetWindows(uint16_t xStar, uint16_t yStar,uint16_t xEnd,uint16_t yEnd)
{	
	LCD_WR_REG(lcddev.setxcmd);	
	LCD_WR_DATA(xStar>>8);
	LCD_WR_DATA(0x00FF&xStar);		
	LCD_WR_DATA(xEnd>>8);
	LCD_WR_DATA(0x00FF&xEnd);

	LCD_WR_REG(lcddev.setycmd);	
	LCD_WR_DATA(yStar>>8);
	LCD_WR_DATA(0x00FF&yStar);		
	LCD_WR_DATA(yEnd>>8);
	LCD_WR_DATA(0x00FF&yEnd);

	LCD_WriteRAM_Prepare();	//��ʼд��GRAM			
}   

/*****************************************************************************
 * @name       :void LCD_SetCursor(u16 Xpos, u16 Ypos)
 * @date       :2018-08-09 
 * @function   :Set coordinate value
 * @parameters :Xpos:the  x coordinate of the pixel
								Ypos:the  y coordinate of the pixel
 * @retvalue   :None
******************************************************************************/ 
void LCD_SetCursor(uint16_t Xpos, uint16_t Ypos)
{	  	    			
	LCD_SetWindows(Xpos,Ypos,Xpos,Ypos);	
} 

/*****************************************************************************
 * @name       :void LCD_direction(u8 direction)
 * @date       :2018-08-09 
 * @function   :Setting the display direction of LCD screen
 * @parameters :direction:0-0 degree
                          1-90 degree
													2-180 degree
													3-270 degree
 * @retvalue   :None
******************************************************************************/ 
void LCD_direction(uint8_t direction)
{ 
	lcddev.setxcmd=0x2A;
	lcddev.setycmd=0x2B;
	lcddev.wramcmd=0x2C;
	lcddev.rramcmd=0x2E;
			lcddev.dir = direction%4;
	switch(lcddev.dir){		  
		case 0:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;		
			LCD_WriteReg(0x36,(1<<3)|(0<<6)|(0<<7));//BGR==1,MY==0,MX==0,MV==0
		break;
		case 1:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(0<<7)|(1<<6)|(1<<5));//BGR==1,MY==1,MX==0,MV==1
		break;
		case 2:						 	 		
			lcddev.width=LCD_W;
			lcddev.height=LCD_H;	
			LCD_WriteReg(0x36,(1<<3)|(1<<6)|(1<<7));//BGR==1,MY==0,MX==0,MV==0
		break;
		case 3:
			lcddev.width=LCD_H;
			lcddev.height=LCD_W;
			LCD_WriteReg(0x36,(1<<3)|(1<<7)|(1<<5));//BGR==1,MY==1,MX==0,MV==1
		break;	
		default:break;
	}		
}	 


uint16_t LCD_Read_ID(void)
{
uint8_t i,val[3] = {0};
	for(i=1;i<4;i++)
	{
		LCD_WR_REG(0xD9);
		LCD_WR_DATA(0x10+i);
		LCD_WR_REG(0xD3);
		val[i-1] = LCD_RD_DATA();
	}
	lcddev.id=val[1];
	lcddev.id<<=8;
	lcddev.id|=val[2];
	return lcddev.id;
}
#endif




