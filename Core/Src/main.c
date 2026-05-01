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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct {
  uint8_t note;
  uint16_t duration_ms;
  uint16_t led_pin;
} BeepStep;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define CS43L22_I2C_ADDR              0x94

#define CS43L22_REG_POWER_CTL1        0x02
#define CS43L22_REG_POWER_CTL2        0x04
#define CS43L22_REG_CLOCKING_CTL      0x05
#define CS43L22_REG_INTERFACE_CTL1    0x06
#define CS43L22_REG_MASTER_A_VOL      0x20
#define CS43L22_REG_MASTER_B_VOL      0x21
#define CS43L22_REG_BEEP_FREQ_ONTIME  0x1C
#define CS43L22_REG_BEEP_VOL_OFFTIME  0x1D
#define CS43L22_REG_BEEP_TONE_CFG     0x1E

#define NOTE_C4   0x00
#define NOTE_C5   0x10
#define NOTE_D5   0x20
#define NOTE_E5   0x30
#define NOTE_F5   0x40
#define NOTE_G5   0x50
#define NOTE_A5   0x60
#define NOTE_B5   0x70
#define NOTE_C6   0x80
#define NOTE_D6   0x90
#define NOTE_E6   0xA0
#define NOTE_F6   0xB0
#define NOTE_G6   0xC0
#define NOTE_A6   0xD0
#define NOTE_B6   0xE0
#define NOTE_C7   0xF0

#define MELODY_REPEAT_COUNT 3
#define I2S_BUFFER_SIZE     16
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

I2S_HandleTypeDef hi2s3;
DMA_HandleTypeDef hdma_spi3_tx;

/* USER CODE BEGIN PV */
static uint16_t i2sBuffer[I2S_BUFFER_SIZE] = {0};

static const BeepStep melody[] = {
  {NOTE_C5, 180, GPIO_PIN_12},
  {NOTE_D5, 180, GPIO_PIN_13},
  {NOTE_E5, 220, GPIO_PIN_14},
  {NOTE_G5, 320, GPIO_PIN_15},

  {NOTE_E5, 180, GPIO_PIN_14},
  {NOTE_D5, 180, GPIO_PIN_13},
  {NOTE_C5, 300, GPIO_PIN_12},

  {NOTE_G5, 180, GPIO_PIN_15},
  {NOTE_A5, 180, GPIO_PIN_12},
  {NOTE_G5, 220, GPIO_PIN_13},
  {NOTE_E5, 320, GPIO_PIN_14},

  {NOTE_F5, 180, GPIO_PIN_15},
  {NOTE_E5, 180, GPIO_PIN_14},
  {NOTE_D5, 300, GPIO_PIN_13},

  {NOTE_C5, 180, GPIO_PIN_12},
  {NOTE_E5, 180, GPIO_PIN_14},
  {NOTE_G5, 220, GPIO_PIN_15},
  {NOTE_C6, 360, GPIO_PIN_12},

  {NOTE_G5, 180, GPIO_PIN_15},
  {NOTE_E5, 180, GPIO_PIN_14},
  {NOTE_C5, 420, GPIO_PIN_12}
};

static const uint32_t melodyLength = sizeof(melody) / sizeof(melody[0]);
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_I2C1_Init(void);
static void MX_I2S3_Init(void);
/* USER CODE BEGIN PFP */
static void CS43L22_WriteReg(uint8_t reg, uint8_t value);
static void CS43L22_Init(void);
static void CS43L22_PlayBeep(uint8_t note, uint16_t duration_ms, uint16_t led_pin);
static void PlayMelodyOnce(void);
static void PlayMelodyThreeTimes(void);
static void StopAllLeds(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void CS43L22_WriteReg(uint8_t reg, uint8_t value)
{
  uint8_t data[2] = {reg, value};
  HAL_I2C_Master_Transmit(&hi2c1, CS43L22_I2C_ADDR, data, 2, HAL_MAX_DELAY);
}

static void CS43L22_Init(void)
{
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_Delay(10);
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_4, GPIO_PIN_SET);
  HAL_Delay(10);

  CS43L22_WriteReg(CS43L22_REG_POWER_CTL1, 0x01);
  CS43L22_WriteReg(CS43L22_REG_POWER_CTL2, 0xAF);
  CS43L22_WriteReg(CS43L22_REG_CLOCKING_CTL, 0x81);
  CS43L22_WriteReg(CS43L22_REG_INTERFACE_CTL1, 0x04);
  CS43L22_WriteReg(CS43L22_REG_MASTER_A_VOL, 0x18);
  CS43L22_WriteReg(CS43L22_REG_MASTER_B_VOL, 0x18);
  CS43L22_WriteReg(CS43L22_REG_POWER_CTL1, 0x9E);
}

static void StopAllLeds(void)
{
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);
}

static void CS43L22_PlayBeep(uint8_t note, uint16_t duration_ms, uint16_t led_pin)
{
  if (duration_ms == 0) {
    return;
  }

  StopAllLeds();

  CS43L22_WriteReg(CS43L22_REG_BEEP_FREQ_ONTIME, note | 0x0F);
  CS43L22_WriteReg(CS43L22_REG_BEEP_VOL_OFFTIME, 0x00);
  CS43L22_WriteReg(CS43L22_REG_BEEP_TONE_CFG, 0xC0);

  HAL_GPIO_WritePin(GPIOD, led_pin, GPIO_PIN_SET);
  HAL_Delay(duration_ms);

  CS43L22_WriteReg(CS43L22_REG_BEEP_TONE_CFG, 0x00);
  StopAllLeds();
}

static void PlayMelodyOnce(void)
{
  for (uint32_t i = 0; i < melodyLength; i++) {
    CS43L22_PlayBeep(melody[i].note, melody[i].duration_ms, melody[i].led_pin);
    HAL_Delay(45);
  }
}

static void PlayMelodyThreeTimes(void)
{
  for (uint32_t round = 0; round < MELODY_REPEAT_COUNT; round++) {
    PlayMelodyOnce();
    HAL_Delay(500);
  }

  StopAllLeds();
}
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
  MX_I2C1_Init();
  MX_I2S3_Init();
  /* USER CODE BEGIN 2 */
  if (HAL_I2S_Transmit_DMA(&hi2s3, i2sBuffer, I2S_BUFFER_SIZE) != HAL_OK)
  {
    Error_Handler();
  }

  CS43L22_Init();
  PlayMelodyThreeTimes();
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Macro to configure the PLL multiplication factor
  */
  __HAL_RCC_PLL_PLLM_CONFIG(16);

  /** Macro to configure the PLL clock source
  */
  __HAL_RCC_PLL_PLLSOURCE_CONFIG(RCC_PLLSOURCE_HSI);

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief I2S3 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2S3_Init(void)
{

  /* USER CODE BEGIN I2S3_Init 0 */

  /* USER CODE END I2S3_Init 0 */

  /* USER CODE BEGIN I2S3_Init 1 */

  /* USER CODE END I2S3_Init 1 */
  hi2s3.Instance = SPI3;
  hi2s3.Init.Mode = I2S_MODE_MASTER_TX;
  hi2s3.Init.Standard = I2S_STANDARD_PHILIPS;
  hi2s3.Init.DataFormat = I2S_DATAFORMAT_16B;
  hi2s3.Init.MCLKOutput = I2S_MCLKOUTPUT_ENABLE;
  hi2s3.Init.AudioFreq = I2S_AUDIOFREQ_48K;
  hi2s3.Init.CPOL = I2S_CPOL_LOW;
  hi2s3.Init.ClockSource = I2S_CLOCK_PLL;
  hi2s3.Init.FullDuplexMode = I2S_FULLDUPLEXMODE_DISABLE;
  if (HAL_I2S_Init(&hi2s3) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2S3_Init 2 */

  /* USER CODE END I2S3_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Stream5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Stream5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOD, GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pins : PD12 PD13 PD14 PD15
                           PD4 */
  GPIO_InitStruct.Pin = GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14|GPIO_PIN_15
                          |GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

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
