/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "spi.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd_ILI9341V.h"
#include "lv_port_disp_template.h"
#include "lv_port_indev_template.h"
#include "ov2640.h"
#include "stdio.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
// 摄像头图像显示对象
static lv_obj_t *camera_img = NULL;
static lv_timer_t *camera_timer = NULL;
static uint8_t camera_running = 0;

// 图像描述符
static lv_img_dsc_t camera_img_dsc;
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void Camera_InitUI(void);
static void Camera_UpdateFrame(lv_timer_t *timer);
static void Camera_Start(void);
static void Camera_Stop(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// 重定向printf到串口2
//int fputc(int ch, FILE *f) {
//    HAL_UART_Transmit(&huart2, (uint8_t *)&ch, 1, 0xffff);
//    return ch;
//}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  
  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_DCMI_Init();
  MX_SPI2_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */
	    printf("goX\r\n");
	// 初始化LVGL
	lv_init();
	lv_port_disp_init();
	
	// 初始化OV2640摄像头
	OV2640_Init();
	
	// 检查摄像头状态
	if (OV2640_GetStatus() == OV2640_STATUS_READY) {
		// 获取并显示传感器ID
		uint8_t id_h, id_l;
		OV2640_GetSensorID(&id_h, &id_l);
		
		// 创建ID显示标签
		lv_obj_t *id_label = lv_label_create(lv_scr_act());
		char id_str[32];
		sprintf(id_str, "Camera ID: 0x%02X%02X", id_h, id_l);
		lv_label_set_text(id_label, id_str);
		lv_obj_align(id_label, LV_ALIGN_TOP_LEFT, 10, 10);
		
		// 创建摄像头显示界面
		Camera_InitUI();
		// 启动摄像头显示
		Camera_Start();
	} else {
		// 摄像头初始化失败，显示错误信息
		lv_obj_t *label = lv_label_create(lv_scr_act());
		lv_label_set_text(label, "Camera Init Failed!");
		lv_obj_center(label);
		
		// 显示读取到的ID
		uint8_t id_h, id_l;
		OV2640_GetSensorID(&id_h, &id_l);
		lv_obj_t *id_label = lv_label_create(lv_scr_act());
		char id_str[32];
		sprintf(id_str, "Read ID: 0x%02X%02X", id_h, id_l);
		lv_label_set_text(id_label, id_str);
		lv_obj_align(id_label, LV_ALIGN_TOP_LEFT, 10, 10);
	}
	
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
		lv_timer_handler();
		HAL_Delay(5);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

// 初始化摄像头显示界面
static void Camera_InitUI(void) {
	// 设置背景为黑色
	lv_obj_set_style_bg_color(lv_scr_act(), lv_color_black(), 0);
	
	// 获取摄像头配置
	OV2640_ConfigTypeDef config;
	OV2640_GetConfig(&config);
	
	// 计算图像尺寸
	uint16_t width, height;
	switch (config.resolution) {
		case OV2640_RES_160x120:
			width = 160;
			height = 120;
			break;
		case OV2640_RES_320x240:
			width = 320;
			height = 240;
			break;
		default:
			width = 320;
			height = 240;
			break;
	}
	
	// 初始化图像描述符
	camera_img_dsc.header.always_zero = 0;
	camera_img_dsc.header.w = width;
	camera_img_dsc.header.h = height;
	camera_img_dsc.header.cf = LV_IMG_CF_TRUE_COLOR;
	camera_img_dsc.data_size = width * height * 2;
	camera_img_dsc.data = NULL; // 将在捕获后设置
	
	// 创建图像对象
	camera_img = lv_img_create(lv_scr_act());
	lv_obj_set_pos(camera_img, 0, 0);
	
	// 如果图像比屏幕大，进行缩放
	if (width > 240 || height > 320) {
		// 计算缩放比例以适应屏幕
		float scale_x = 240.0f / width;
		float scale_y = 320.0f / height;
		float scale = (scale_x < scale_y) ? scale_x : scale_y;
		lv_img_set_zoom(camera_img, (uint16_t)(scale * 256)); // LVGL缩放系数是256为1倍
	}
}

// 更新摄像头帧
static void Camera_UpdateFrame(lv_timer_t *timer) {
	if (!camera_running) {
		return;
	}
	
	// 检查是否有新的帧准备好
	if (OV2640_IsFrameReady()) {
		// 获取图像缓冲区
		uint8_t *buffer = OV2640_GetBuffer();
		if (buffer != NULL) {
			// 更新图像描述符的数据指针
			camera_img_dsc.data = buffer;
			
			// 设置图像源
			lv_img_set_src(camera_img, &camera_img_dsc);
			
			// 刷新显示
			lv_obj_invalidate(camera_img);
		}
		
		// 清除帧就绪标志
		OV2640_ClearFrameReady();
		
		// 启动下一次捕获
		OV2640_StartCapture();
	}
}

// 启动摄像头显示
static void Camera_Start(void) {
	if (camera_running) {
		return;
	}
	
	camera_running = 1;
	
	// 启动第一次捕获
	OV2640_StartCapture();
	
	// 创建定时器用于更新显示 (30fps)
	camera_timer = lv_timer_create(Camera_UpdateFrame, 33, NULL);
}

// 停止摄像头显示
static void Camera_Stop(void) {
	camera_running = 0;
	
	// 删除定时器
	if (camera_timer != NULL) {
		lv_timer_del(camera_timer);
		camera_timer = NULL;
	}
	
	// 停止摄像头捕获
	OV2640_StopCapture();
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
