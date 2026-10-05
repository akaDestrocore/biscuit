/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    alarm.c
 * @brief   Çalışma sesleri ve öncelikli alarm sesleri
 *
 * @author  destrocore
 * @date    2026
 */

#include "core.h"
#include "modules.h"

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void alarm_tonDegistir(Alarm_Ses_e sesTonu, uint8_t *pDurum);


/* ============================================================
 * Genel API
 * ============================================================*/

 /**
  * @brief Çalışma seslerini ve öncelikli alarm seslerini yönetir
  * @retval None
  */
void alarm_run(void) {
    
    alarm_tonDegistir(ALARM_SES_MONO_CUT, &gMonopolarCutSes);
    alarm_tonDegistir(ALARM_SES_MONO_COAG, &gMonopolarCoagSes);
    alarm_tonDegistir(ALARM_SES_BIPOLAR_CUT, &gBipolarCutSes);
    alarm_tonDegistir(ALARM_SES_BIPOLAR_COAG, &gBipolarCoagSes);

    if (1U == gAlarmPlate) {

        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
        htim9.Instance->PSC = 150U;
        HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
        HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    } else if (2U == gAlarmPlate) {

        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);
        HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_1);
        gAlarmPlate = 0U;
    }

    if (1U == gDusukOncelikAlarm) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_SET);
        htim9.Instance->PSC = 150U;
        HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);

        gDusukAlarmSay++;
        if (17000U == gDusukAlarmSay) {
            gDusukOncelikAlarm = 0U;
            gDusukAlarmSay = 0U;
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
            HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);
        }
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Belirtilen ses tonu için hoparlör durumunu günceller
  * @param sesTonu Güncellenecek tonun ID
  * @param pDurum Durum değişkenine işaretçi
  * @retval None
  */
static void alarm_tonDegistir(Alarm_Ses_e sesTonu, uint8_t *pDurum) {
    
    if (1U == *pDurum) {
        
        if (0U == gAlarmPlate) {
            
            for (uint8_t i = 0U; i < ALARM_COUNT; i++) {
                
                if (sesTonu == gAlarmConfigTable[i].sesTonu) {
                    
                    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_SET);
                    htim9.Instance->PSC = gAlarmConfigTable[i].PSC;
                    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
                    break;
                }
            }
        }
    } else if (2U == *pDurum) {
        
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_2, GPIO_PIN_RESET);
        HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);
        *pDurum = 0U;
    }
}