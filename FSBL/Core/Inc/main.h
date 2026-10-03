/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#if defined ( __ICCARM__ )
#  define CMSE_NS_CALL  __cmse_nonsecure_call
#  define CMSE_NS_ENTRY __cmse_nonsecure_entry
#else
#  define CMSE_NS_CALL  __attribute((cmse_nonsecure_call))
#  define CMSE_NS_ENTRY __attribute((cmse_nonsecure_entry))
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32n6xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "stm32n6570_discovery.h"
#include "stm32n6570_discovery_bus.h"
#include "imx335.h"
#include "camera.h"
#define CAMERA_IMX335_ADDRESS 0x34U
#define FRAME_WIDTH  800
#define FRAME_HEIGHT 480
#define FRAME_BUFFER_SIZE (FRAME_WIDTH * FRAME_HEIGHT*2)
#define BUFFER_ADDRESS  0x34200000

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* Function pointer declaration in non-secure*/
#if defined ( __ICCARM__ )
typedef void (CMSE_NS_CALL *funcptr)(void);
#else
typedef void CMSE_NS_CALL (*funcptr)(void);
#endif

/* typedef for non-secure callback functions */
typedef funcptr funcptr_NS;

/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */
//#define INTA_FRM_SYSCTL_Pin GPIO_PIN_14
//#define INTA_FRM_SYSCTL_GPIO_Port GPIOB
#define CTS_FROM_SYSCNTR_Pin GPIO_PIN_13
#define CTS_FROM_SYSCNTR_GPIO_Port GPIOB
#define SENSOR_SDA_Pin GPIO_PIN_1
#define SENSOR_SDA_GPIO_Port GPIOC
#define SENSOR_SCL_Pin GPIO_PIN_9
#define SENSOR_SCL_GPIO_Port GPIOH
#define CAM_FLASH_Pin GPIO_PIN_10
#define CAM_FLASH_GPIO_Port GPIOC
#define N6_WKUP_CLK_Pin GPIO_PIN_0
#define N6_WKUP_CLK_GPIO_Port GPIOC
#define OSC_IN_Pin GPIO_PIN_0
#define OSC_IN_GPIO_Port GPIOH
#define I2C1_SDA_Pin GPIO_PIN_1
#define I2C1_SDA_GPIO_Port GPIOC
#define MIC_D1_Pin GPIO_PIN_8
#define MIC_D1_GPIO_Port GPIOE
#define OSC_OUT_Pin GPIO_PIN_1
#define OSC_OUT_GPIO_Port GPIOH
#define I2C2_SDA_Pin GPIO_PIN_4
#define I2C2_SDA_GPIO_Port GPIOD
#define VCP_TX_Pin GPIO_PIN_5
#define VCP_TX_GPIO_Port GPIOE
#define CAM_PWRC_Pin GPIO_PIN_2
#define CAM_PWRC_GPIO_Port GPIOD
#define TX_TO_SYSCTLR_ENABLE_Pin GPIO_PIN_13
#define TX_TO_SYSCTLR_ENABLE_GPIO_Port GPIOD
#define I2C1_SCL_Pin GPIO_PIN_9
#define I2C1_SCL_GPIO_Port GPIOH
#define LED_Pin GPIO_PIN_2
#define LED_GPIO_Port GPIOC
#define VCP_RX_Pin GPIO_PIN_6
#define VCP_RX_GPIO_Port GPIOE
#define SAI1_FS_A_Pin GPIO_PIN_0
#define SAI1_FS_A_GPIO_Port GPIOB
#define SAI1_SD_A_Pin GPIO_PIN_7
#define SAI1_SD_A_GPIO_Port GPIOB
#define SAI1_CLK_A_Pin GPIO_PIN_6
#define SAI1_CLK_A_GPIO_Port GPIOB
#define User_Pin GPIO_PIN_13
#define User_GPIO_Port GPIOC
#define SAI1_SD_B_Pin GPIO_PIN_3
#define SAI1_SD_B_GPIO_Port GPIOE
#define MIC_CK_Pin GPIO_PIN_2
#define MIC_CK_GPIO_Port GPIOE
#define TAMP_Pin GPIO_PIN_0
#define TAMP_GPIO_Port GPIOE
#define HEXASPI_IO_7_Pin GPIO_PIN_7
#define HEXASPI_IO_7_GPIO_Port GPIOP
#define HEXASPI_IO_6_Pin GPIO_PIN_6
#define HEXASPI_IO_6_GPIO_Port GPIOP
#define HEXASPI_IO_0_Pin GPIO_PIN_0
#define HEXASPI_IO_0_GPIO_Port GPIOP
#define HEXASPI_IO_4_Pin GPIO_PIN_4
#define HEXASPI_IO_4_GPIO_Port GPIOP
#define HEXASPI_IO_1_Pin GPIO_PIN_1
#define HEXASPI_IO_1_GPIO_Port GPIOP
#define HEXASPI_IO_15_Pin GPIO_PIN_15
#define HEXASPI_IO_15_GPIO_Port GPIOP
#define HEXASPI_IO_5_Pin GPIO_PIN_5
#define HEXASPI_IO_5_GPIO_Port GPIOP
#define HEXASPI_IO_12_Pin GPIO_PIN_12
#define HEXASPI_IO_12_GPIO_Port GPIOP
#define HEXASPI_IO_3_Pin GPIO_PIN_3
#define HEXASPI_IO_3_GPIO_Port GPIOP
#define HEXASPI_IO_2_Pin GPIO_PIN_2
#define HEXASPI_IO_2_GPIO_Port GPIOP
#define HEXASPI_IO_13_Pin GPIO_PIN_13
#define HEXASPI_IO_13_GPIO_Port GPIOP
#define HEXASPI_DQS0_Pin GPIO_PIN_2
#define HEXASPI_DQS0_GPIO_Port GPIOO
#define SAI1_MCLK_A_Pin GPIO_PIN_7
#define SAI1_MCLK_A_GPIO_Port GPIOG
#define HEXASPI_IO_11_Pin GPIO_PIN_11
#define HEXASPI_IO_11_GPIO_Port GPIOP
#define HEXASPI_IO_8_Pin GPIO_PIN_8
#define HEXASPI_IO_8_GPIO_Port GPIOP
#define HEXASPI_IO_14_Pin GPIO_PIN_14
#define HEXASPI_IO_14_GPIO_Port GPIOP
#define HEXASPI_DQS1_Pin GPIO_PIN_3
#define HEXASPI_DQS1_GPIO_Port GPIOO
#define HEXASPI_NCS_Pin GPIO_PIN_0
#define HEXASPI_NCS_GPIO_Port GPIOO
#define HEXASPI_IO_9_Pin GPIO_PIN_9
#define HEXASPI_IO_9_GPIO_Port GPIOP
#define HEXASPI_IO_10_Pin GPIO_PIN_10
#define HEXASPI_IO_10_GPIO_Port GPIOP
#define HEXASPI_CLK_Pin GPIO_PIN_4
#define HEXASPI_CLK_GPIO_Port GPIOO
#define OCTOSPI_IO2_Pin GPIO_PIN_4
#define OCTOSPI_IO2_GPIO_Port GPION
#define OCTOSPI_IO4_Pin GPIO_PIN_8
#define OCTOSPI_IO4_GPIO_Port GPION
#define OCTOSPI_DQS_Pin GPIO_PIN_0
#define OCTOSPI_DQS_GPIO_Port GPION
#define OCTOSPI_IO1_Pin GPIO_PIN_3
#define OCTOSPI_IO1_GPIO_Port GPION
#define OCTOSPI_IO3_Pin GPIO_PIN_5
#define OCTOSPI_IO3_GPIO_Port GPION
#define OCTOSPI_NCS_Pin GPIO_PIN_1
#define OCTOSPI_NCS_GPIO_Port GPION
#define SWCLK_Pin GPIO_PIN_14
#define SWCLK_GPIO_Port GPIOA
#define UCPD1_VSENSE_Pin GPIO_PIN_11
#define UCPD1_VSENSE_GPIO_Port GPIOA
#define OCTOSPI_IO5_Pin GPIO_PIN_9
#define OCTOSPI_IO5_GPIO_Port GPION
#define OCTOSPI_IO0_Pin GPIO_PIN_2
#define OCTOSPI_IO0_GPIO_Port GPION
#define SWDIO_Pin GPIO_PIN_13
#define SWDIO_GPIO_Port GPIOA
#define SWO_Pin GPIO_PIN_5
#define SWO_GPIO_Port GPIOB
#define OCTOSPI_IO6_Pin GPIO_PIN_10
#define OCTOSPI_IO6_GPIO_Port GPION
#define OCTOSPI_IO7_Pin GPIO_PIN_11
#define OCTOSPI_IO7_GPIO_Port GPION

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define TX_EN_Pin GPIO_PIN_13
#define TX_EN_GPIO_Port GPIOD
#define NRST_CAM_Pin GPIO_PIN_8
#define NRST_CAM_GPIO_Port GPIOC
#define WKUP2_Pin GPIO_PIN_2
#define WKUP2_GPIO_Port GPIOA

/* USER CODE BEGIN Private defines */
/* Definition for HSPI clock resources */
#define XSPI1_CLK_ENABLE()                 __HAL_RCC_XSPI1_CLK_ENABLE()
#define XSPI1_CLK_DISABLE()                __HAL_RCC_XSPI1_CLK_DISABLE()

#define XSPI1_CLK_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI1_DQS0_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI1_DQS1_GPIO_CLK_ENABLE()       __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI1_CS_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOO_CLK_ENABLE()
#define XSPI1_D0_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D1_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D2_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D3_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D4_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D5_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D6_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D7_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D8_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D9_GPIO_CLK_ENABLE()         __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D10_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D11_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D12_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D13_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D14_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOP_CLK_ENABLE()
#define XSPI1_D15_GPIO_CLK_ENABLE()        __HAL_RCC_GPIOP_CLK_ENABLE()

#define XSPI1_CLK_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_DQS0_GPIO_CLK_DISABLE()       __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_DQS1_GPIO_CLK_DISABLE()       __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_CS_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D0_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D1_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D2_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D3_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D4_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D5_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D6_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D7_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D8_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D9_GPIO_CLK_DISABLE()         __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D10_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D11_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D12_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D13_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D14_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()
#define XSPI1_D15_GPIO_CLK_DISABLE()        __HAL_RCC_GPIOP_CLK_DISABLE()

#define XSPI1_FORCE_RESET()                __HAL_RCC_XSPI1_FORCE_RESET()
#define XSPI1_RELEASE_RESET()              __HAL_RCC_XSPI1_RELEASE_RESET()

/* Definition for HSPI Pins */
/* HSPI_CLK */
#define XSPI1_CLK_PIN                      GPIO_PIN_4
#define XSPI1_CLK_GPIO_PORT                GPIOO
#define XSPI1_CLK_PIN_AF                   GPIO_AF9_XSPIM_P1
/* HSPI_DQS0 */
#define XSPI1_DQS0_PIN                     GPIO_PIN_2
#define XSPI1_DQS0_GPIO_PORT               GPIOO
#define XSPI1_DQS0_PIN_AF                  GPIO_AF9_XSPIM_P1
/* HSPI_DQS1 */
#define XSPI1_DQS1_PIN                     GPIO_PIN_3
#define XSPI1_DQS1_GPIO_PORT               GPIOO
#define XSPI1_DQS1_PIN_AF                  GPIO_AF9_XSPIM_P1
/* HSPI_CS */
#define XSPI1_CS_PIN                       GPIO_PIN_0
#define XSPI1_CS_GPIO_PORT                 GPIOO
#define XSPI1_CS_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D0 */
#define XSPI1_D0_PIN                       GPIO_PIN_0
#define XSPI1_D0_GPIO_PORT                 GPIOP
#define XSPI1_D0_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D1 */
#define XSPI1_D1_PIN                       GPIO_PIN_1
#define XSPI1_D1_GPIO_PORT                 GPIOP
#define XSPI1_D1_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D2 */
#define XSPI1_D2_PIN                       GPIO_PIN_2
#define XSPI1_D2_GPIO_PORT                 GPIOP
#define XSPI1_D2_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D3 */
#define XSPI1_D3_PIN                       GPIO_PIN_3
#define XSPI1_D3_GPIO_PORT                 GPIOP
#define XSPI1_D3_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D4 */
#define XSPI1_D4_PIN                       GPIO_PIN_4
#define XSPI1_D4_GPIO_PORT                 GPIOP
#define XSPI1_D4_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D5 */
#define XSPI1_D5_PIN                       GPIO_PIN_5
#define XSPI1_D5_GPIO_PORT                 GPIOP
#define XSPI1_D5_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D6 */
#define XSPI1_D6_PIN                       GPIO_PIN_6
#define XSPI1_D6_GPIO_PORT                 GPIOP
#define XSPI1_D6_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D7 */
#define XSPI1_D7_PIN                       GPIO_PIN_7
#define XSPI1_D7_GPIO_PORT                 GPIOP
#define XSPI1_D7_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D8 */
#define XSPI1_D8_PIN                       GPIO_PIN_8
#define XSPI1_D8_GPIO_PORT                 GPIOP
#define XSPI1_D8_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D9 */
#define XSPI1_D9_PIN                       GPIO_PIN_9
#define XSPI1_D9_GPIO_PORT                 GPIOP
#define XSPI1_D9_PIN_AF                    GPIO_AF9_XSPIM_P1
/* HSPI_D10 */
#define XSPI1_D10_PIN                      GPIO_PIN_10
#define XSPI1_D10_GPIO_PORT                GPIOP
#define XSPI1_D10_PIN_AF                   GPIO_AF9_XSPIM_P1
/* HSPI_D11 */
#define XSPI1_D11_PIN                      GPIO_PIN_11
#define XSPI1_D11_GPIO_PORT                GPIOP
#define XSPI1_D11_PIN_AF                   GPIO_AF9_XSPIM_P1
/* HSPI_D12 */
#define XSPI1_D12_PIN                      GPIO_PIN_12
#define XSPI1_D12_GPIO_PORT                GPIOP
#define XSPI1_D12_PIN_AF                   GPIO_AF9_XSPIM_P1
/* HSPI_D13 */
#define XSPI1_D13_PIN                      GPIO_PIN_13
#define XSPI1_D13_GPIO_PORT                GPIOP
#define XSPI1_D13_PIN_AF                   GPIO_AF9_XSPIM_P1
/* HSPI_D14 */
#define XSPI1_D14_PIN                      GPIO_PIN_14
#define XSPI1_D14_GPIO_PORT                GPIOP
#define XSPI1_D14_PIN_AF                   GPIO_AF9_XSPIM_P1
/* HSPI_D15 */
#define XSPI1_D15_PIN                      GPIO_PIN_15
#define XSPI1_D15_GPIO_PORT                GPIOP
#define XSPI1_D15_PIN_AF                   GPIO_AF9_XSPIM_P1

/* Aps256xx APMemory memory */

/* Read Operations */
#define READ_CMD                                0x00
#define READ_LINEAR_BURST_CMD                   0x20
#define READ_HYBRID_BURST_CMD                   0x3F

/* Write Operations */
#define WRITE_CMD                               0x80
#define WRITE_LINEAR_BURST_CMD                  0xA0
#define WRITE_HYBRID_BURST_CMD                  0xBF

/* Reset Operations */
#define RESET_CMD                               0xFF

/* Registers definition */
#define MR0                                     0x00000000
#define MR1                                     0x00000001
#define MR2                                     0x00000002
#define MR3                                     0x00000003
#define MR4                                     0x00000004
#define MR8                                     0x00000008

/* Register Operations */
#define READ_REG_CMD                            0x40
#define WRITE_REG_CMD                           0xC0

/* Default dummy clocks cycles, 7(6+1) to support up to 200MHz CLK */
#define DUMMY_CLOCK_CYCLES_READ                 6
#define DUMMY_CLOCK_CYCLES_WRITE                6

/* Size of buffers */
#define BUFFERSIZE                              10240
#define KByte                                   1024

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
