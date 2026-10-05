/**
  ******************************************************************************
  * @file    esu_selftest.c
  * @brief   Açılış kendi kendini test (Power Supply, Monopolar kart, Bipolar kart)
  ******************************************************************************
  */
#include "core.h"
#include "modules.h"

/**
  * @brief Üç aşamalı açılış testini çalıştırır; hata olursa hata kodunu yazar ve sonsuz döngüye girer
  * @retval None
  * @note Sırasıyla: güç kaynağı voltajı, monopolar kart, bipolar kart. Hepsi geçerse geri döner.
  */
void esuSelftest_run(void) {
    Bist_State_e durum = BIST_PWR_SUPPLY;
    uint8_t gecikme = 0U;
    bool basladi = false;
    uint32_t baslangicTick = HAL_GetTick();

    while (BIST_OK != durum) {
        if ((HAL_GetTick() - baslangicTick) >= 1U) {
            gHighVolt = gAdc1DmaBuff[ADC_CH_PWR];
            basladi = true;
        }

        HAL_Delay(50U);

        if (true == basladi) {
            uint32_t esik = 10U;
            const char *pHataKomut = "S0_acilma.t1.txt=\"DEFAULT E-15\"";

            if (BIST_PWR_SUPPLY == durum) {
                esik = 2U;
                pHataKomut = "S0_acilma.t1.txt=\"DEFAULT E-14\"";
            } else if (BIST_MONO_BOARD == durum) {
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_8, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);
            } else {
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11 | GPIO_PIN_8, GPIO_PIN_SET);
                pHataKomut = "S0_acilma.t1.txt=\"DEFAULT E-16\"";
            }

            gecikme++;
            if (gecikme >= 10U) {
                gecikme = 0U;

                if (gHighVolt <= esik) {
                    gDusukOncelikAlarm = 1U;
                    gAcikSayfa = UI_PAGE_ERROR;
                    (void)NEXTION_sendCmdRetry(pHataKomut);
                    HAL_Delay(100U);
                    esuAlarm_run();
                    HAL_Delay(500U);
                    gDusukOncelikAlarm = 0U;
                    gDusukAlarmSay = 0U;
                    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_2, GPIO_PIN_RESET);
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
                    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11 | GPIO_PIN_8, GPIO_PIN_RESET);
                    esuCore_dacSet((float)0.0f);
                    durum = BIST_OK;
                }
            }
        }
    }
}