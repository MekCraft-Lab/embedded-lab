/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
 ******************************************************************************
 * @attention
 *
 * Copyright (c) 2025 STMicroelectronics.
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
#include "gpio.h"
#include "main.h"
#include "usb_device.h"
#include "usbd_hid.h"


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
#define GET_HID_CODE(x) (x - 'a' + 4)
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
extern USBD_HandleTypeDef hUsbDeviceFS;
uint8_t hidBuffer[17];
uint8_t mouseBuffer[8] = {0x00};
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

void keyboard(uint8_t modify, uint8_t keyValue) {
    memset(hidBuffer, 0, 9);
    hidBuffer[0] = 0x01; // 报文ID
    hidBuffer[1] = modify;
    hidBuffer[3] = keyValue;
    USBD_HID_SendReport(&hUsbDeviceFS, hidBuffer, 9);
    HAL_Delay(10); // 添加短延迟，让主机处理这个按键状态

    // 释放所有按键（发送 0 报文）
    memset(hidBuffer + 1, 0, 8);
    USBD_HID_SendReport(&hUsbDeviceFS, hidBuffer, 9);
    HAL_Delay(10);
}


/* USER CODE END 0 */

/**
 * @brief  The application entry point.
 * @retval int
 */
int main(void) {

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
    MX_USB_DEVICE_Init();
    /* USER CODE BEGIN 2 */
	mouseBuffer[0] = 0x02;
	
    HAL_Delay(1000);
    // keyboard(0x08, 0x00); // Win + R
    HAL_Delay(10);
    keyboard(0x08, 0x15);
    HAL_Delay(500);
    keyboard(0x00, 0x11); // n
    HAL_Delay(10);
    keyboard(0x00, 0x12); // o
    HAL_Delay(10);
    keyboard(0x00, 0x17); // t
    HAL_Delay(10);
    keyboard(0x00, 0x08); // e
    HAL_Delay(10);
    keyboard(0x00, 0x13); // p
    HAL_Delay(10);
    keyboard(0x00, 0x04); // a
    HAL_Delay(10);
    keyboard(0x00, 0x07); // d
    HAL_Delay(10);
    keyboard(0x00, 0x28); // Enter
    HAL_Delay(10);
    keyboard(0x00, 0x28); // Enter
	HAL_Delay(10);
	mouseBuffer[2] = 0x5;
	mouseBuffer[3] = 0x5;
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	HAL_Delay(500);
	keyboard(0x00, GET_HID_CODE('w'));
	keyboard(0x00, GET_HID_CODE('o'));
	keyboard(0x00, GET_HID_CODE('s'));
	keyboard(0x00, GET_HID_CODE('h'));
	keyboard(0x00, GET_HID_CODE('i'));
	keyboard(0x00, GET_HID_CODE('c'));
	keyboard(0x00, GET_HID_CODE('h'));
	keyboard(0x00, GET_HID_CODE('e'));
	keyboard(0x00, GET_HID_CODE('n'));
	keyboard(0x00, GET_HID_CODE('j'));
	keyboard(0x00, GET_HID_CODE('i'));
	keyboard(0x00, GET_HID_CODE('a'));
	keyboard(0x00, GET_HID_CODE('x'));
	keyboard(0x00, GET_HID_CODE('i'));
	keyboard(0x00, GET_HID_CODE('n'));
	keyboard(0x00, GET_HID_CODE('g'));
	keyboard(0x00, 0x2C);
	keyboard(0x00, 0x36);

	keyboard(0x00, GET_HID_CODE('w'));
	keyboard(0x00, GET_HID_CODE('o'));
	keyboard(0x00, GET_HID_CODE('s'));
	keyboard(0x00, GET_HID_CODE('h'));
	keyboard(0x00, GET_HID_CODE('i'));
	keyboard(0x00, GET_HID_CODE('g'));
	keyboard(0x00, GET_HID_CODE('e'));
	keyboard(0x00, GET_HID_CODE('s'));
	keyboard(0x00, GET_HID_CODE('h'));
	keyboard(0x00, GET_HID_CODE('a'));
	keyboard(0x00, GET_HID_CODE('b'));
	keyboard(0x00, GET_HID_CODE('i'));
	keyboard(0x00, 0x2C);

    HAL_Delay(1000);
    keyboard(0x04, 0x3D); // Alt + F4
	keyboard(0x00, 0x28); // Enter
		mouseBuffer[2] =0 - 0x20;
	mouseBuffer[3] =0 - 0x10;
	uint8_t count;



    /* USER CODE END 2 */

    /* Infinite loop */
    /* USER CODE BEGIN WHILE */
    while (1) {
	mouseBuffer[2] =0 - mouseBuffer[2];
	mouseBuffer[3] =0 - mouseBuffer[3];
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);
	USBD_HID_SendReport(&hUsbDeviceFS, mouseBuffer, 5);
	HAL_Delay(10);

	HAL_Delay(500);
	if(count % 3 == 0) {
		count++;
		
	
	keyboard(0x00, 0x28); // Enter
	}

        /* USER CODE END WHILE */

        /* USER CODE BEGIN 3 */
    }
    /* USER CODE END 3 */
}

/**
 * @brief System Clock Configuration
 * @retval None
 */
void SystemClock_Config(void) {
    RCC_OscInitTypeDef RCC_OscInitStruct   = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct   = {0};
    RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

    /** Initializes the RCC Oscillators according to the specified parameters
     * in the RCC_OscInitTypeDef structure.
     */
    RCC_OscInitStruct.OscillatorType       = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState             = RCC_HSE_ON;
    RCC_OscInitStruct.HSEPredivValue       = RCC_HSE_PREDIV_DIV1;
    RCC_OscInitStruct.HSIState             = RCC_HSI_ON;
    RCC_OscInitStruct.PLL.PLLState         = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource        = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLMUL           = RCC_PLL_MUL9;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        Error_Handler();
    }

    /** Initializes the CPU, AHB and APB buses clocks
     */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK) {
        Error_Handler();
    }
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    PeriphClkInit.UsbClockSelection    = RCC_USBCLKSOURCE_PLL_DIV1_5;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK) {
        Error_Handler();
    }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
 * @brief  This function is executed in case of error occurrence.
 * @retval None
 */
void Error_Handler(void) {
    /* USER CODE BEGIN Error_Handler_Debug */
    /* User can add his own implementation to report the HAL error return state */
    __disable_irq();
    while (1) {
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
void assert_failed(uint8_t* file, uint32_t line) {
    /* USER CODE BEGIN 6 */
    /* User can add his own implementation to report the file name and line number,
       ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
    /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
