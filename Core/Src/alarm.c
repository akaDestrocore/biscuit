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

#include "genx.h"

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
void alarm_yonet(void) {
    
    alarm_tonDegistir(ALARM_SES_MONO_CUT, &gMonopolarCutSes);
    alarm_tonDegistir(ALARM_SES_MONO_COAG, &gMonopolarCoagSes);
    alarm_tonDegistir(ALARM_SES_BIPOLAR_CUT, &gBipolarCutSes);
    alarm_tonDegistir(ALARM_SES_BIPOLAR_COAG, &gBipolarCoagSes);

    if (1U == gAlarmPlate) {

        HAL_GPIO_WritePin(SOUND_RELAY_GPIO_Port, SOUND_RELAY_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ALARM_SOUND_R_GPIO_Port, ALARM_SOUND_R_Pin, GPIO_PIN_SET);
        htim9.Instance->PSC = 160U;
        HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
        alarm_beepBaslat();
    } else if (2U == gAlarmPlate) {

        HAL_GPIO_WritePin(ALARM_SOUND_R_GPIO_Port, ALARM_SOUND_R_Pin, GPIO_PIN_RESET);
        HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);
        alarm_beepDurdur();
        gAlarmPlate = 0U;
    }

    if (1U == gDusukOncelikAlarm) {
        HAL_GPIO_WritePin(ALARM_PWM_S_GPIO_Port, ALARM_PWM_S_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(SOUND_RELAY_GPIO_Port, SOUND_RELAY_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(ALARM_SOUND_R_GPIO_Port, ALARM_SOUND_R_Pin, GPIO_PIN_SET);
        htim9.Instance->PSC = 160U;
        HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
        
        if (0U == gDusukAlarmBasla) {
            gDusukAlarmBasla = HAL_GetTick();   // 0 = başlamadı; tick==0 ise 1 ms sonra yeniden alınır
        }

        if ((HAL_GetTick() - gDusukAlarmBasla) >= 750U) {
            gDusukOncelikAlarm = 0U;
            gDusukAlarmBasla = 0U;
            HAL_GPIO_WritePin(ALARM_PWM_S_GPIO_Port, ALARM_PWM_S_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(ALARM_SOUND_R_GPIO_Port, ALARM_SOUND_R_Pin, GPIO_PIN_RESET);
            HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);
        }
    }
}

/**
  * @brief Bip / sessizlik desenini başlatır, desen çalışıyorsa bir şey yapmaz
  * @retval None
  */
void alarm_beepBaslat(void) {

    if (false == gBeepAktif)
    {
        HAL_TIM_Base_Start_IT(&htim5);
        gBeepTick = 0U;
        gBeepAktif = true;
        HAL_GPIO_WritePin(ALARM_PWM_S_GPIO_Port, ALARM_PWM_S_Pin, GPIO_PIN_SET);
    }
}

/**
  * @brief Deseni durdurur
  * @retval None
  */
void alarm_beepDurdur(void) {
    
    gBeepAktif = false;
    gBeepTick = 0U;
    HAL_GPIO_WritePin(ALARM_PWM_S_GPIO_Port, ALARM_PWM_S_Pin, GPIO_PIN_RESET);
    HAL_TIM_Base_Stop_IT(&htim5);
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Belirtilen ses tonu için hoparlör durumunu günceller
  * @param sesTonu Güncellenecek tonun ID'si
  * @param pDurum Durum değişkenine işaretçi
  * @retval None
  */
static void alarm_tonDegistir(Alarm_Ses_e sesTonu, uint8_t *pDurum) {
    
    if (1U == *pDurum) {
        
        if (0U == gAlarmPlate) {
            
            for (uint8_t i = 0U; i < ALARM_COUNT; i++) {
                
                if (sesTonu == gAlarmConfigTable[i].sesTonu) {
                    
                    HAL_GPIO_WritePin(SOUND_RELAY_GPIO_Port, SOUND_RELAY_Pin, GPIO_PIN_SET);
                    htim9.Instance->PSC = gAlarmConfigTable[i].PSC;
                    HAL_TIM_PWM_Start(&htim9, TIM_CHANNEL_2);
                    break;
                }
            }
        }
    } else if (2U == *pDurum) {
        
        HAL_GPIO_WritePin(SOUND_RELAY_GPIO_Port, SOUND_RELAY_Pin, GPIO_PIN_RESET);
        HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);
        *pDurum = 0U;
    }
}