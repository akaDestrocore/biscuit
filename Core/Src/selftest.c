/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    selftest.c
 * @brief   Açılış kendi kendini test (Power Supply, Monopolar kart, Bipolar kart)
 *
 * @author  destrocore
 * @date    2026
 */

#include "genx.h"


// -- Built-in Self-test tanımları -----------------------------
typedef enum {
    BIST_PWR_SUPPLY     = 0U,
    BIST_MONO_BOARD     = 1U,
    BIST_BIPOLAR_BOARD  = 2U,
    BIST_OK             = 3U
} Bist_State_e;

/**
  * @brief Üç aşamalı açılış testini çalıştırır. Hata olursa hata kodunu yazar ve sonsuz döngüye girer
  * @retval None
  * @note Sırasıyla güç kaynağı voltajı, monopolar kart, bipolar kart. Hepsi geçerse geri döner.
  */
void selftest(void) {

    Bist_State_e durum = BIST_PWR_SUPPLY;
    uint8_t gecikme = 0U;
    bool basladi = false;
    uint32_t baslangicTick = HAL_GetTick();

    while (BIST_OK != durum) {
        if ((HAL_GetTick() - baslangicTick) >= 1U) {
            gHighVolt = gAdc1DmaBuff[ADC_CH_PWR];
            basladi = true;
        }

        genx_delay(50U);

        if (true == basladi) {
            uint32_t esik = 10U;
            const char *pHataKomut = "S0_acilma.t1.txt=\"DEFAULT E-15\"";

            if (BIST_PWR_SUPPLY == durum) {
                esik = 2U;
                pHataKomut = "S0_acilma.t1.txt=\"DEFAULT E-14\"";
            } else if (BIST_MONO_BOARD == durum) {
                HAL_GPIO_WritePin(PWR_ON_R_GPIO_Port, PWR_ON_R_Pin, GPIO_PIN_SET);
                HAL_GPIO_WritePin(BIPOLAR_ON_R_GPIO_Port, BIPOLAR_ON_R_Pin, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(BIPOLAR_ON_R_GPIO_Port, BIPOLAR_ON_R_Pin | PWR_ON_R_Pin, GPIO_PIN_SET);
                pHataKomut = "S0_acilma.t1.txt=\"DEFAULT E-16\"";
            }

            gecikme++;
            if (gecikme >= 10U) {
                gecikme = 0U;

                if (gHighVolt <= esik) {
                    gDusukOncelikAlarm = 1U;
                    gAcikSayfa = UI_PAGE_ERROR;
                    (void)genx_sendCmd(pHataKomut);
                    genx_delay(100U);
                    alarm_yonet();
                    genx_delay(500U);
                    gDusukOncelikAlarm = 0U;
                    gDusukAlarmSay = 0U;
                    HAL_GPIO_WritePin(ALARM_SOUND_R_GPIO_Port, ALARM_SOUND_R_Pin, GPIO_PIN_RESET);
                    HAL_TIM_PWM_Stop(&htim9, TIM_CHANNEL_2);

                    while (1U) {
                        __NOP();
                    }
                }

                if (BIST_PWR_SUPPLY == durum) {
                    durum = BIST_MONO_BOARD;
                } else if (BIST_MONO_BOARD == durum) {
                    durum = BIST_BIPOLAR_BOARD;
                } else {
                    gHighVolt = 0U;
                    HAL_GPIO_WritePin(BIPOLAR_ON_R_GPIO_Port, BIPOLAR_ON_R_Pin | PWR_ON_R_Pin, GPIO_PIN_RESET);
                    genx_dacUygula((float)0.0f);
                    durum = BIST_OK;
                }
            }
        }
    }
}