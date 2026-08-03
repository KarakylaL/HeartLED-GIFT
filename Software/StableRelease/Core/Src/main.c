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

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static uint8_t previous = 0;
static uint8_t mode = 1;


typedef struct
{
    int8_t row;
    uint8_t column;
    uint8_t speed_div;
    uint8_t tick;
} Star;

static void Frame_SetPixel(
    uint8_t frame[7],
    int8_t row,
    uint8_t column)
{
    if ((row < 0) || (row >= 7) || (column >= 7))
    {
        return;
    }

    frame[row] |= (uint8_t)(1U << (column + 1U));
}




static void Delay_us(uint16_t us)
{
    __HAL_TIM_SET_COUNTER(&htim2, 0U);

    while (__HAL_TIM_GET_COUNTER(&htim2) < us)
    {
    }
}

static void Shift_Write(uint8_t rows, uint8_t columns)
{
    uint8_t tx[2] = {rows, columns};

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

    HAL_SPI_Transmit(&hspi1, tx, 2, HAL_MAX_DELAY);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
}



static void Button_Update(void)
{
    uint8_t current =
        (uint8_t)HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2);

    if ((previous == 0U) && (current == 1U))
    {
        Delay_us(20000);

        if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_2) == GPIO_PIN_SET)
        {
            mode++;

            if (mode > 4U)
            {
                mode = 1U;
            }
        }
    }

    previous = current;
}


static void DisplayFrames(
    const uint8_t (*frame)[7],
    uint32_t duration_ms,
    uint16_t frame_count,
    uint8_t brightness)
{
    uint8_t mode_at_start = mode;

    const uint16_t row_period_us = 1000U;

    if (brightness > 100U)
    {
        brightness = 100U;
    }

    uint16_t on_time_us =
        (uint16_t)(((uint32_t)row_period_us * brightness) / 100U);

    uint16_t off_time_us =
        (uint16_t)(row_period_us - on_time_us);

    for (uint16_t i = 0; i < frame_count; i++)
    {
        uint32_t start = HAL_GetTick();

        while ((HAL_GetTick() - start) < duration_ms)
        {
            Button_Update();

            if (mode != mode_at_start)
            {
                Shift_Write(0x00, 0x00);
                return;
            }

            for (uint8_t row = 0; row < 7; row++)
            {
                uint8_t row_mask =
                    (uint8_t)(1U << (7U - row));


                Shift_Write(0x00, 0x00);


                if ((on_time_us > 0U) && (frame[i][row] != 0U))
                {
                    Shift_Write(row_mask, frame[i][row]);
                    Delay_us(on_time_us);
                }


                Shift_Write(0x00, 0x00);

                if (off_time_us > 0U)
                {
                    Delay_us(off_time_us);
                }
            }
        }
    }

    Shift_Write(0x00, 0x00);
}

static void Starfall(void)
{
    static Star stars[3U] =
    {
        { .row = -1, .column = 1U, .speed_div = 1U, .tick = 0U },
        { .row = -4, .column = 4U, .speed_div = 2U, .tick = 0U },
        { .row = -7, .column = 6U, .speed_div = 3U, .tick = 0U }
    };

    uint8_t frame[7] = {0};

    for (uint8_t i = 0U; i < 3U; i++)
    {
        Frame_SetPixel(frame, stars[i].row, stars[i].column);
        Frame_SetPixel(frame, stars[i].row - 1, stars[i].column);
        Frame_SetPixel(frame, stars[i].row - 2, stars[i].column);

        stars[i].tick++;

        if (stars[i].tick >= stars[i].speed_div)
        {
            stars[i].tick = 0U;
            stars[i].row++;
        }

        if (stars[i].row > 8)
        {
            stars[i].row = -2;

            stars[i].column =
                (uint8_t)((stars[i].column + 2U + i) % 7U);

            stars[i].speed_div =
                (uint8_t)(1U + ((stars[i].speed_div + i) % 3U));
        }
    }

    DisplayFrames(
        &frame,
        70U,
        1U,
        100U
    );
}



uint8_t love_frames[][7] = {
		{0b00000000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00010000, 0b00010000}, //i
		{0b01101100, 0b11111110, 0b11111110, 0b11111110, 0b01111100, 0b00111000, 0b00010000}, //heart
	    {0b01000100, 0b01000100, 0b00101000, 0b00010000, 0b00010000, 0b00010000, 0b00010000}, //y
	    {0b00000000, 0b00111000, 0b01000100, 0b01000100, 0b01000100, 0b00111000, 0b00000000}, //o
		{0b01000100, 0b01000100, 0b01000100, 0b01000100, 0b01000100, 0b00111000, 0b00000000} //u
};


uint8_t heart_frames[][7] = {
		{0b01101100, 0b11111110, 0b11111110, 0b11111110, 0b01111100, 0b00111000, 0b00010000}
};


static uint8_t IsHeartPixel(int8_t row, int8_t column)
{
    static const uint8_t heart_shape[7] =
    {
        0b01101100,
        0b11111110,
        0b11111110,
        0b11111110,
        0b01111100,
        0b00111000,
        0b00010000
    };

    if ((row < 0) || (row >= 7) ||
        (column < 0) || (column >= 7))
    {
        return 0U;
    }

    uint8_t column_mask =
        (uint8_t)(1U << ((uint8_t)column + 1U));

    return ((heart_shape[row] & column_mask) != 0U);
}

static uint32_t random_state = 0xA341316CU;

static uint32_t Random_Get(void)
{
    random_state ^= random_state << 13;
    random_state ^= random_state >> 17;
    random_state ^= random_state << 5;

    return random_state;
}

static void RandomComet(void)
{
    static Star comet =
    {
        .row = 3,
        .column = 3U,
        .speed_div = 0U,
        .tick = 0U
    };

    static int8_t previous_row = 3;
    static int8_t previous_column = 3;

    static int8_t tail_2_row = 3;
    static int8_t tail_2_column = 3;


    static uint32_t last_move_ms = 0U;


    const uint32_t move_period_ms = 150U;

    uint8_t frame[7] = {0};


    Frame_SetPixel(
        frame,
        comet.row,
        comet.column
    );


    if ((comet.tick % 2U) == 0U)
    {
        Frame_SetPixel(
            frame,
            previous_row,
            (uint8_t)previous_column
        );
    }


    if ((comet.tick % 4U) == 0U)
    {
        Frame_SetPixel(
            frame,
            tail_2_row,
            (uint8_t)tail_2_column
        );
    }

    DisplayFrames(
        &frame,
        10U,
        1U,
        100U
    );


    if ((HAL_GetTick() - last_move_ms) < move_period_ms)
    {
        return;
    }

    last_move_ms = HAL_GetTick();


    static const int8_t row_step[4] =
    {
        -1, 1, 0, 0
    };

    static const int8_t column_step[4] =
    {
        0, 0, -1, 1
    };


    uint8_t valid_directions[4];
    uint8_t valid_count = 0U;


    for (uint8_t direction = 0U;
         direction < 4U;
         direction++)
    {
        int8_t next_row =
            comet.row + row_step[direction];

        int8_t next_column =
            (int8_t)comet.column + column_step[direction];


        if (!IsHeartPixel(next_row, next_column))
        {
            continue;
        }


        if ((next_row == previous_row) &&
            (next_column == previous_column))
        {
            continue;
        }

        valid_directions[valid_count] = direction;
        valid_count++;
    }


    if (valid_count == 0U)
    {
        for (uint8_t direction = 0U;
             direction < 4U;
             direction++)
        {
            int8_t next_row =
                comet.row + row_step[direction];

            int8_t next_column =
                (int8_t)comet.column +
                column_step[direction];

            if (IsHeartPixel(next_row, next_column))
            {
                valid_directions[valid_count] = direction;
                valid_count++;
            }
        }
    }


    if (valid_count == 0U)
    {
        return;
    }


    uint8_t selected_direction =
        valid_directions[Random_Get() % valid_count];

    int8_t next_row =
        comet.row + row_step[selected_direction];

    int8_t next_column =
        (int8_t)comet.column +
        column_step[selected_direction];


    tail_2_row = previous_row;
    tail_2_column = previous_column;

    previous_row = comet.row;
    previous_column = (int8_t)comet.column;


    comet.row = next_row;
    comet.column = (uint8_t)next_column;

    comet.tick++;
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
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  HAL_TIM_Base_Start(&htim2);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      Button_Update();

      if (mode == 1U)
      {
          DisplayFrames(
              love_frames,
              1000,
              sizeof(love_frames) / sizeof(love_frames[0]),
			  100U
          );
      }
      else if(mode == 2U)
      {
          DisplayFrames(
              heart_frames,
              1000,
              sizeof(heart_frames) / sizeof(heart_frames[0]),
			  100U
          );
      }

      else if (mode == 3U){
    	  Starfall();
      }

      else if (mode == 4U){

    	  RandomComet();
      }

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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 65535;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

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

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin : PA2 */
  GPIO_InitStruct.Pin = GPIO_PIN_2;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
