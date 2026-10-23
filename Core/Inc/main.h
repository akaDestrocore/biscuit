/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define SOUND_RELAY_Pin GPIO_PIN_2
#define SOUND_RELAY_GPIO_Port GPIOE
#define VOLUME_1_Pin GPIO_PIN_3
#define VOLUME_1_GPIO_Port GPIOE
#define BIPOLAR_CUT_R_Pin GPIO_PIN_13
#define BIPOLAR_CUT_R_GPIO_Port GPIOC
#define BIPOLAR_TEK_PEDAL_Pin GPIO_PIN_1
#define BIPOLAR_TEK_PEDAL_GPIO_Port GPIOC
#define ALARM_SOUND_R_Pin GPIO_PIN_2
#define ALARM_SOUND_R_GPIO_Port GPIOC
#define MONO_KORUMA_Pin GPIO_PIN_4
#define MONO_KORUMA_GPIO_Port GPIOC
#define MAIN_BOARD_PWM_R_Pin GPIO_PIN_5
#define MAIN_BOARD_PWM_R_GPIO_Port GPIOC
#define VOLUME_2_Pin GPIO_PIN_1
#define VOLUME_2_GPIO_Port GPIOB
#define VOLUME_3_Pin GPIO_PIN_7
#define VOLUME_3_GPIO_Port GPIOE
#define VOLUME_4_Pin GPIO_PIN_8
#define VOLUME_4_GPIO_Port GPIOE
#define CIFT_PEDAL_CUT_Pin GPIO_PIN_9
#define CIFT_PEDAL_CUT_GPIO_Port GPIOE
#define CIFT_PEDAL_COAG_Pin GPIO_PIN_10
#define CIFT_PEDAL_COAG_GPIO_Port GPIOE
#define PLATE_LED_YESIL_Pin GPIO_PIN_11
#define PLATE_LED_YESIL_GPIO_Port GPIOE
#define PLATE_LED_KIRMIZI_Pin GPIO_PIN_12
#define PLATE_LED_KIRMIZI_GPIO_Port GPIOE
#define BIPOLAR_LED_MAVI_Pin GPIO_PIN_13
#define BIPOLAR_LED_MAVI_GPIO_Port GPIOE
#define BIPOLAR_LED_BEYAZ_Pin GPIO_PIN_14
#define BIPOLAR_LED_BEYAZ_GPIO_Port GPIOE
#define BIPOLAR_LED_SARI_Pin GPIO_PIN_15
#define BIPOLAR_LED_SARI_GPIO_Port GPIOE
#define MONO2_LED_MAVI_Pin GPIO_PIN_12
#define MONO2_LED_MAVI_GPIO_Port GPIOB
#define MONO1_LED_MAVI_Pin GPIO_PIN_13
#define MONO1_LED_MAVI_GPIO_Port GPIOB
#define _74LS_R1_Pin GPIO_PIN_14
#define _74LS_R1_GPIO_Port GPIOB
#define MONO2_LED_SARI_Pin GPIO_PIN_15
#define MONO2_LED_SARI_GPIO_Port GPIOB
#define PWR_ON_R_Pin GPIO_PIN_8
#define PWR_ON_R_GPIO_Port GPIOD
#define FAN_R_Pin GPIO_PIN_9
#define FAN_R_GPIO_Port GPIOD
#define _74LS_R2_Pin GPIO_PIN_10
#define _74LS_R2_GPIO_Port GPIOD
#define BIPOLAR_ON_R_Pin GPIO_PIN_11
#define BIPOLAR_ON_R_GPIO_Port GPIOD
#define MONO2_LED_BEYAZ_Pin GPIO_PIN_13
#define MONO2_LED_BEYAZ_GPIO_Port GPIOD
#define MONO1_LED_SARI_Pin GPIO_PIN_14
#define MONO1_LED_SARI_GPIO_Port GPIOD
#define RF_GND_EARTH_R_Pin GPIO_PIN_15
#define RF_GND_EARTH_R_GPIO_Port GPIOD
#define ALARM_PWM_S_Pin GPIO_PIN_6
#define ALARM_PWM_S_GPIO_Port GPIOC
#define MONO1_LED_BEYAZ_Pin GPIO_PIN_7
#define MONO1_LED_BEYAZ_GPIO_Port GPIOC
#define RF_KALEM_Pin GPIO_PIN_8
#define RF_KALEM_GPIO_Port GPIOC
#define CD4051_A_Pin GPIO_PIN_8
#define CD4051_A_GPIO_Port GPIOA
#define CD4051_B_Pin GPIO_PIN_11
#define CD4051_B_GPIO_Port GPIOA
#define CD4051_C_Pin GPIO_PIN_12
#define CD4051_C_GPIO_Port GPIOA
#define RF_BIPOLAR_R2_Pin GPIO_PIN_10
#define RF_BIPOLAR_R2_GPIO_Port GPIOC
#define SPRAY_RELAY_Pin GPIO_PIN_11
#define SPRAY_RELAY_GPIO_Port GPIOC
#define PLATE_RELAY_Pin GPIO_PIN_12
#define PLATE_RELAY_GPIO_Port GPIOC
#define HAND_RELAY_Pin GPIO_PIN_0
#define HAND_RELAY_GPIO_Port GPIOD
#define MONO1_RELAY_Pin GPIO_PIN_1
#define MONO1_RELAY_GPIO_Port GPIOD
#define LIGASURE_R1_Pin GPIO_PIN_2
#define LIGASURE_R1_GPIO_Port GPIOD
#define MONO2_RELAY_Pin GPIO_PIN_3
#define MONO2_RELAY_GPIO_Port GPIOD
#define RF_PWR_RELAY_Pin GPIO_PIN_4
#define RF_PWR_RELAY_GPIO_Port GPIOD
#define RF_BIPOLAR_R_Pin GPIO_PIN_5
#define RF_BIPOLAR_R_GPIO_Port GPIOD
#define LIGASURE_R2_Pin GPIO_PIN_6
#define LIGASURE_R2_GPIO_Port GPIOD
#define LIGASURE_R3_Pin GPIO_PIN_7
#define LIGASURE_R3_GPIO_Port GPIOD
#define MONO2_KALEM_COAG_Pin GPIO_PIN_5
#define MONO2_KALEM_COAG_GPIO_Port GPIOB
#define MONO1_KALEM_COAG_Pin GPIO_PIN_4
#define MONO1_KALEM_COAG_GPIO_Port GPIOB
#define MONO2_KALEM_CUT_Pin GPIO_PIN_7
#define MONO2_KALEM_CUT_GPIO_Port GPIOB
#define MONO1_KALEM_CUT_Pin GPIO_PIN_8
#define MONO1_KALEM_CUT_GPIO_Port GPIOB
#define MULTIPLEXER_S_Pin GPIO_PIN_9
#define MULTIPLEXER_S_GPIO_Port GPIOB
#define RF_MONO_X_FAN_Pin GPIO_PIN_0
#define RF_MONO_X_FAN_GPIO_Port GPIOE
#define RF_MONO_RELAY_Pin GPIO_PIN_1
#define RF_MONO_RELAY_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
