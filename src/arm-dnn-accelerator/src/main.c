#include "dnn.h"
#include "main.h"

#define DVFS_SETTING_7

#ifdef DVFS_SETTING_1
#define PLLM_VAL RCC_PLLM_DIV8 
#define PLLN_VAL 8
#define FLASH_WAITS FLASH_LATENCY_0
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE2

#elif defined(DVFS_SETTING_2)
#define PLLM_VAL RCC_PLLM_DIV8 
#define PLLN_VAL 16
#define FLASH_WAITS FLASH_LATENCY_1
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE2

#elif defined(DVFS_SETTING_3)
#define PLLM_VAL RCC_PLLM_DIV8 
#define PLLN_VAL 24
#define FLASH_WAITS FLASH_LATENCY_1
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE2

#elif defined(DVFS_SETTING_4)
#define PLLM_VAL RCC_PLLM_DIV8 
#define PLLN_VAL 48
#define FLASH_WAITS FLASH_LATENCY_1
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE1

#elif defined(DVFS_SETTING_5)
#define PLLM_VAL RCC_PLLM_DIV8
#define PLLN_VAL 64
#define FLASH_WAITS FLASH_LATENCY_2
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE1

#elif defined(DVFS_SETTING_6)
#define PLLM_VAL RCC_PLLM_DIV4
#define PLLN_VAL 75
#define FLASH_WAITS FLASH_LATENCY_4
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE1

#elif defined(DVFS_SETTING_7)
#define PLLM_VAL RCC_PLLM_DIV4
#define PLLN_VAL 85
#define FLASH_WAITS FLASH_LATENCY_4
#define REG_VOLTAGE PWR_REGULATOR_VOLTAGE_SCALE1_BOOST

#endif

void SystemClock_Config(void);
void SystemClock_Config2(void);
void SystemClockHSE_Config(void);
void SystemClock_Decrease(void);
static GPIO_InitTypeDef  GPIO_InitStruct;

/*
int main(void){
	volatile uint64_t a, b, c;
	a = 0x1234;
	b = 0xabcd;
	c = a*b;

	return c;

}
*/

uint8_t __ro_flash frame[19200] = {0};

uint8_t dnn_done = 0;
uint8_t result = 0;

int main(void)
{
		HAL_Init();

		SystemClock_Config();
		__HAL_RCC_PWR_CLK_ENABLE();
		
		BSP_LED_Init(LED2);
		BSP_LED_Off(LED2);
		
		__HAL_RCC_GPIOA_CLK_ENABLE();
		GPIO_InitStruct.Pin = GPIO_PIN_8;
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		//HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_SYSCLK, RCC_MCODIV_1);
		
		GPIO_InitStruct.Pin = GPIO_PIN_9;
		GPIO_InitStruct.Mode  = GPIO_MODE_OUTPUT_PP;
		GPIO_InitStruct.Pull  = GPIO_PULLUP;
		GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
		HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
		
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);	
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
		
		for( uint16_t i = 0; i < 19200; i++ )
			frame[i] = i % 256;

		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);	
		while(dnn_done == 0){
			//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);	
			run_DNN();
			//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_RESET);
		}
		HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);	
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);	
		//SystemClock_Config2();
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);

		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);	
		//result = workload_run();	
		//HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);	
		while (1)
		{
			__HAL_FLASH_SLEEP_POWERDOWN_ENABLE();

			/* Reset all RCC Sleep and Stop modes register to */
			/* improve power consumption                      */
			RCC->AHB1SMENR  = 0x0;
			RCC->AHB2SMENR  = 0x0;
			RCC->AHB3SMENR  = 0x0;

			RCC->APB1SMENR1 = 0x0;
			RCC->APB1SMENR2 = 0x0;
			RCC->APB2SMENR  = 0x0;
			
			//SystemClock_Config();
			SystemClock_Decrease();
			HAL_SuspendTick();

			/* De-init LED2 */
			BSP_LED_DeInit(LED2);

			/* Enter Sleep Mode, wake up is done once User push-button is pressed */
			HAL_PWR_EnterSLEEPMode(PWR_LOWPOWERREGULATOR_ON, PWR_SLEEPENTRY_WFI);
			
			//HAL_Delay(1000);
			//BSP_LED_Toggle(LED2);
		}
	return result;
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  // HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
  // HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  // HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
  HAL_PWREx_ControlVoltageScaling(REG_VOLTAGE);


  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI; 		// HSI is set to 16MHz
  RCC_OscInitStruct.PLL.PLLM = PLLM_VAL; 				// PLLM sets VCO input to 2MHz
  RCC_OscInitStruct.PLL.PLLN = PLLN_VAL; 							// VCO output = 4MHz (VCO input) * PLLN; PLLN = main sys * 2 / 4MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2; 				// Main system clock = VCO output/2
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAITS) != HAL_OK)
  {
    Error_Handler();
  }
  
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLPEN);
  CLEAR_BIT(RCC->PLLCFGR, RCC_PLLCFGR_PLLQEN);
	
  LL_APB1_GRP1_DisableClock(0xFFFFFFFF);
  LL_APB2_GRP1_DisableClock(0xFFFFFFFF);
}

void SystemClock_Config2(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  // HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1_BOOST);
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);
  // HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE2);
  
  /* -1- Select HSI as system clock source to allow modification of the PLL configuration */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI; 		// HSI is set to 16MHz
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV4; 				// PLLM sets VCO input to 4MHz
  RCC_OscInitStruct.PLL.PLLN = 45; 							// VCO output = 4MHz (VCO input) * PLLN; PLLN = main sys * 2 / 4MHz
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2; 				// Main system clock = VCO output/2
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

void SystemClockHSE_Config(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* -1- Select HSI as system clock source to allow modification of the PLL configuration */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* -2- Enable HSE  Oscillator, select it as PLL source and finally activate the PLL */
  RCC_OscInitStruct.OscillatorType        = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState              = RCC_HSE_ON;

  RCC_OscInitStruct.PLL.PLLSource         = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLState          = RCC_PLL_OFF;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV6;
  RCC_OscInitStruct.PLL.PLLN = 13;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* -3- Select the PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 clocks dividers */
  //RCC_ClkInitStruct.ClockType       = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
  RCC_ClkInitStruct.ClockType       = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK);
  RCC_ClkInitStruct.SYSCLKSource    = RCC_SYSCLKSOURCE_HSE;
  RCC_ClkInitStruct.AHBCLKDivider   = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider  = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider  = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_WAITS) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* -4- Optional: Disable HSI Oscillator (if the HSI is no more needed by the application)*/
  RCC_OscInitStruct.OscillatorType  = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState        = RCC_HSI_OFF;
  RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void SystemClock_Decrease(void)
{
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};

  /* Select HSI as system clock source */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }

  /* Modify HSI to HSI DIV8 */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    /* Initialization Error */
    Error_Handler();
  }
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
  while(1) 
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
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
  /* Infinite loop */
  while(1) 
  {
  }
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
