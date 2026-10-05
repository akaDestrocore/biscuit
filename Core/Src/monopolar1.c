/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    monopolar1.c
 * @brief   Mono1 kanalı. Her mod kendi başlatma ve izleme adımlarını içerir, 
 *          durdurma kanal başına tek fonksiyondur.
 *
 * @author  destrocore
 * @date    2026
 */

#include <math.h>
#include "core.h"
#include "modules.h"

/* ============================================================
 * Özel: modül sabitleri
 * ============================================================*/
static volatile Mono1Cut_State_e gMono1CutDurum = MONO1_CUT_DURUM_BOSTA;
static Mono1Coag_State_e gMono1CoagDurum = MONO1_COAG_DURUM_BOSTA;
static uint16_t gGucGecikme = 0U;
static uint8_t gYuzStart = 0U;
static volatile uint8_t gEndoCut1 = 0U;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void mono1Cut_cut(void);
static void mono1Cut_blend1(void);
static void mono1Cut_blend2(void);
static void mono1Cut_blend3(void);
static void mono1Cut_endo(void);
static void mono1Cut_stop(void);
static void mono1Cut_run(void);
static void mono1Coag_contact(void);
static void mono1Coag_spray1(void);
static void mono1Coag_spray2(void);
static void mono1Coag_spray3(void);
static void mono1Coag_stop(void);
static void mono1Coag_run(void);

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief Mono1 kanalının (CUT + COAG) tek giriş noktası
  * @retval None
  */
void mono1_run(void) {
    
    mono1Cut_run();
    mono1Coag_run();
}

/**
  * @brief TIM5 periyot kesmesi: Mono1 Endo
  * @param htim Kesmeyi üreten zamanlayıcı
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    
    if (TIM5 == htim->Instance) {
        
        if (((MONO1_CUT_MODE_POLYPECTOMY == gMono1CutMode) || (MONO1_CUT_MODE_PAPILLOTOMES == gMono1CutMode)) && (GUCU_ACIK == gMono1CutStart)) {
            gEndoCut1 = 1U;
            gEndoCutSay++;
            if (gEndoCutSay <= gEndoCutSure) {          // 1 == 0.01 saniye
                
                gCut1WattPol = ((0.000022f * gEndoCutWattPol * gEndoCutWattPol * gEndoCutWattPol) - (0.0205f * gEndoCutWattPol * gEndoCutWattPol) \
					+ (10.25f * gEndoCutWattPol) + (287U));
                gCut1WattPol /= 10.0f; 

                genx_dacSet(gCut1WattPol);
            }

            if (gEndoCutSay > gEndoCutSure) {

                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 108U);
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

                gCoag1WattPol = ((0.000075f * gEndoCoagWattPol * gEndoCoagWattPol * gEndoCoagWattPol) \
					- (0.0212f * gEndoCoagWattPol * gEndoCoagWattPol) + (3.2617f * gEndoCoagWattPol) + (34.4f));
                
                genx_dacSet(gEndoCoagWattPol);
            }

            if (gEndoCutSay >= (gEndoCoagTimer + gEndoCutSure)) {

                gEndoCutSay = 0U;
                HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
            }
        }

        if ((GUCU_KAPALI == gMono1CutStart) && (MONO1_CUT_DURUM_BOSTA == gMono1CutDurum) && (1U == gEndoCut1)) {
            
            gEndoCut1 = 0U;
            genx_dacSet(0.0f);
        }
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Mono1 CUT / RESECTION1 / RESECTION2 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono1Cut_cut(void) {

    if (MONO1_CUT_DURUM_BOSTA == gMono1CutDurum) {
        if (0U == gPlate100Overresistance) {
            float x = gCut1WattPol;

            (void)NEXTION_sendCmdRetry("tsw 255,0");
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
            genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_CUT);
            gMonopolarCutSes = 1U;
            gGucGecikme = 0U;
            HAL_Delay(200U);
            if (x >= 100.0f) {
                gYuzStart = 1U;
                x = 100.0f;
            }
            
            x = (0.000022f * x * x * x) - (0.0205f * x * x) + (10.25f * x) + 287.0f;
            x /= 10.0f;

            genx_dacSet(x);

            gMono1CutDurum = MONO1_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {
            
            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);

            if ((1U == gYuzStart) && (guc > 70U)) {

                gYuzStart = 0U;
                gCut1WattPol = ((0.000021f * gCut1WattPol * gCut1WattPol * gCut1WattPol) - (0.0205f * gCut1WattPol * gCut1WattPol) \
									+ (10.1f * gCut1WattPol) + (287U));
                gCut1WattPol /= 10.0f;

                genx_dacSet(gCut1WattPol);
            }

            uint16_t monoCut1KorumaVeri = ((0.0000278f * gCut1WattPol * gCut1WattPol * gCut1WattPol) \
										- (0.0383f * gCut1WattPol * gCut1WattPol) + (19.136f * gCut1WattPol) + (683.43f));

            if ((gCut1WattPol >= 50U) && (guc > (uint32_t)monoCut1KorumaVeri)) {
                gMono1Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono1 BLEND1 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono1Cut_blend1(void) {

    if (MONO1_CUT_DURUM_BOSTA == gMono1CutDurum) {
        if (0U == gPlate100Overresistance) {
            
            float x = gCut1WattPol;

            (void)NEXTION_sendCmdRetry("tsw 255,0");
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 212U);     // %80
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
            genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_CUT);
            gMonopolarCutSes = 1U;
            gGucGecikme = 0U;
            HAL_Delay(200U);
            if (x >= 100.0f) {
                gYuzStart = 1U;
                x = 100.0f;
            }

            x = (0.0000126f * x * x * x) - (0.0063f * x * x) + (1.6669f * x) + 31.0f;

            genx_dacSet(x);

            gMono1CutDurum = MONO1_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);
            if ((1U == gYuzStart) && (guc > 120U)) {
                gYuzStart = 0U;
                gCut1WattPol = (0.0000126f * gCut1WattPol * gCut1WattPol * gCut1WattPol) \
                            - (0.0063f * gCut1WattPol * gCut1WattPol) + (1.6669f * gCut1WattPol) + 31.0f;

                genx_dacSet(gCut1WattPol);
            }

            uint16_t monoCut1KorumaVeri = ((0.000186f * gCut1WattPol * gCut1WattPol * gCut1WattPol) \
                                        - (0.0928f * gCut1WattPol * gCut1WattPol) + (23.777f * gCut1WattPol) + (548.8f));

            if ((gCut1WattPol >= 30.0f) && (guc > monoCut1KorumaVeri)) {
                gMono1Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono1 BLEND2 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono1Cut_blend2(void) {

    if (MONO1_CUT_DURUM_BOSTA == gMono1CutDurum) {
        if (0U == gPlate100Overresistance) {
            
            float x = gCut1WattPol;

            (void)NEXTION_sendCmdRetry("tsw 255,0");
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 160U);     // %60
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
            genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_CUT);
            gMonopolarCutSes = 1U;
            gGucGecikme = 0U;
            HAL_Delay(200U);
            if (x >= 100.0f) {
                gYuzStart = 1U;
                x = 100.0f;
            }

            x = (0.0000173f * x * x * x) - (0.00755f * x * x) + (1.9052f * x) + 33.0f;

            genx_dacSet(x);

            gMono1CutDurum = MONO1_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {
            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);
            if ((1U == gYuzStart) && (guc > 130U)) {
                gYuzStart = 0U;
                gCut1WattPol = (0.0000173f * gCut1WattPol * gCut1WattPol * gCut1WattPol) - (0.00755f * gCut1WattPol * gCut1WattPol) \
                            + (1.9052f * gCut1WattPol) + 33.0f;

                genx_dacSet(gCut1WattPol);
            }

            uint16_t monoCut1KorumaVeri = ((0.000115f * gCut1WattPol * gCut1WattPol * gCut1WattPol) \
									- (0.058f * gCut1WattPol * gCut1WattPol) + (18.493f * gCut1WattPol) + (704U));

            if ((gCut1WattPol >= 30.0f) && (guc > (uint32_t)monoCut1KorumaVeri)) {
                gMono1Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono1 BLEND3 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono1Cut_blend3(void) {

    if (MONO1_CUT_DURUM_BOSTA == gMono1CutDurum) {
        if (0U == gPlate100Overresistance) {
            
            float x = gCut1WattPol;

            (void)NEXTION_sendCmdRetry("tsw 255,0");
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 127U);     // %50
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
            genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_CUT);
            gMonopolarCutSes = 1U;
            gGucGecikme = 0U;
            HAL_Delay(200U);
            if (x >= 100.0f) {
                gYuzStart = 1U;
                x = 100.0f;
            }

            x = (0.000039f * x * x * x) - (0.0135f * x * x) + (2.5977f * x) + 33.355f;

            genx_dacSet(x);

            gMono1CutDurum = MONO1_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {
            
            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);
            if ((1U == gYuzStart) && (guc > 130U)) {
                gYuzStart = 0U;

                gCut1WattPol = (0.000039f * gCut1WattPol * gCut1WattPol * gCut1WattPol) - (0.0135f * gCut1WattPol * gCut1WattPol) + (2.5977f * gCut1WattPol) + 33.355f;
                
                genx_dacSet(gCut1WattPol);
            }

            uint16_t monoCut1KorumaVeri = ((0.000465f * gCut1WattPol * gCut1WattPol * gCut1WattPol) \
										- (0.1818f * gCut1WattPol * gCut1WattPol) + (34.623f * gCut1WattPol) + (319.0f)); 

            if ((gCut1WattPol >= 30.0f) && (guc > (uint32_t)monoCut1KorumaVeri)) {
                gMono1Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono1 endocut başlatma. DAC'ı TIM5 kesmesi sürer.
  * @retval None
  */
static void mono1Cut_endo(void) {
    
    if ((MONO1_CUT_DURUM_BOSTA == gMono1CutDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_CUT);
        gMonopolarCutSes = 1U;
        HAL_Delay(200U);
        genx_dacSet(0.0f);
        gEndoCutSay = 0U;
        HAL_TIM_Base_Start_IT(&htim5);

        gMono1CutDurum = MONO1_CUT_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono1 CUT çıkışını kapatır
  * @retval None
  */
static void mono1Cut_stop(void) {

    genx_dacSet(0.0f);
    gMonopolarCutSes = 2U;
    HAL_Delay(100U);
    gEndoCutSay = 11U;
    genx_muxSelect(0U);
    HAL_TIM_Base_Stop_IT(&htim5);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, MONO_RELAY_Pin | GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET); // FAN
    gMono1CutDurum = MONO1_CUT_DURUM_BOSTA;
    gYuzStart = 0U;
    HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_SET);
    HAL_Delay(300U);
    (void)NEXTION_sendCmdRetry("tsw 255,1");
}

/**
  * @brief Mono1 CUT kanalı: Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @retval None
  */
static void mono1Cut_run(void) {

    if (GUCU_ACIK == gMono1CutStart) {
        
        bool pwrKontrol = false;

        switch (gMono1CutMode) {

            case MONO1_CUT_MODE_CUT:
            case MONO1_CUT_MODE_RESECTION1:
            case MONO1_CUT_MODE_RESECTION2: { 
                mono1Cut_cut();
                pwrKontrol = true;
                break;
            }

            case MONO1_CUT_MODE_BLEND1: { 
                mono1Cut_blend1(); 
                pwrKontrol = true; 
                break; 
            }

            case MONO1_CUT_MODE_BLEND2: { 
                mono1Cut_blend2(); 
                pwrKontrol = true; 
                break; 
            }

            case MONO1_CUT_MODE_BLEND3: { 
                mono1Cut_blend3(); 
                pwrKontrol = true; 
                break; 
            }

            case MONO1_CUT_MODE_POLYPECTOMY:
            case MONO1_CUT_MODE_PAPILLOTOMES: { 
                mono1Cut_endo(); 
                break; 
            }

            default: { 
                break; 
            }
        }

        // Güç kaynağı kontrolü
        if ((true == pwrKontrol) && (true == gPwrYeni)) {

            if ((gMono1CutWatt >= 10U) && (gMono1CutWatt <= 400U) && (gHighVolt <= 2U)) {
                
                gPwrKoruma = 1U;
            }

            if ((((gMono1CutWatt >= 10U) && (gMono1CutWatt <= 45U)) || ((gMono1CutWatt >= 50U) && (gMono1CutWatt <= 400U))) \
            && (gHighVolt < 200U)) {
                
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONO1_CUT_DURUM_AKTIF == gMono1CutDurum) {
        
        mono1Cut_stop();
    }
}

/**
  * @brief Mono1 COAG CONTACT başlatma
  * @retval None
  */
static void mono1Coag_contact(void) {
    if ((MONO1_COAG_DURUM_BOSTA == gMono1CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 108U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);
        gCoag1WattPol = (0.000087f * gCoag1WattPol * gCoag1WattPol * gCoag1WattPol) - (0.021f * gCoag1WattPol * gCoag1WattPol) \
							+ (3.0f * gCoag1WattPol) + 29.0f;
        
        genx_dacSet(gCoag1WattPol);
        
        gMono1CoagDurum = MONO1_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono1 COAG SPRAY1 başlatma
  * @retval None
  */
static void mono1Coag_spray1(void) {

    if ((MONO1_COAG_DURUM_BOSTA == gMono1CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 20U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        //spray Relay
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);

        gCoag1WattPol = (0.000007f * gCoag1WattPol * gCoag1WattPol * gCoag1WattPol) - (0.0153f * gCoag1WattPol * gCoag1WattPol) \
					+ (3.5f * gCoag1WattPol) + 50.7f;

        genx_dacSet(gCoag1WattPol);

        gMono1CoagDurum = MONO1_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono1 COAG SPRAY2 başlatma
  * @retval None
  */
static void mono1Coag_spray2(void) {

    if ((MONO1_COAG_DURUM_BOSTA == gMono1CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 22U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        //spray Relay
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);
        gCoag1WattPol = (0.00007f * gCoag1WattPol * gCoag1WattPol * gCoag1WattPol) - (0.0234f * gCoag1WattPol * gCoag1WattPol) \
					+ (3.7182f * gCoag1WattPol) + 42.463f;
        genx_dacSet(gCoag1WattPol);

        gMono1CoagDurum = MONO1_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono1 COAG SPRAY3 ve FULGURATION başlatma
  * @retval None
  */
static void mono1Coag_spray3(void) {
    
    if ((MONO1_COAG_DURUM_BOSTA == gMono1CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 24U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        //spray Relay
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);
        gCoag1WattPol = (0.00009f * gCoag1WattPol * gCoag1WattPol * gCoag1WattPol) - (0.0244f * gCoag1WattPol * gCoag1WattPol) \
					+ (3.6567f * gCoag1WattPol) + 40.692f;
        genx_dacSet(gCoag1WattPol);

        gMono1CoagDurum = MONO1_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono1 COAG çıkışını kapatır
  * @retval None
  */
static void mono1Coag_stop(void) {

    genx_dacSet((float)0.0f);
    gMonopolarCoagSes = 2U;
    HAL_Delay(100U);
    genx_muxSelect(0U);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);      //spray Relay
    HAL_GPIO_WritePin(GPIOD, MONO_RELAY_Pin | GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);       // FAN
    HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    gMono1CoagDurum = MONO1_COAG_DURUM_BOSTA;
    HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin, GPIO_PIN_SET);
    HAL_Delay(300U);
    (void)NEXTION_sendCmdRetry("tsw 255,1");
}

/**
  * @brief Mono1 COAG kanalı: Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @retval None
  */
static void mono1Coag_run(void) {

    if (GUCU_ACIK == gMono1CoagStart) {

        switch (gMono1CoagMode) {
            
            case MONO1_COAG_MODE_CONTACT: { 
                mono1Coag_contact(); 
                break; 
            }

            case MONO1_COAG_MODE_SPRAY1: { 
                mono1Coag_spray1(); 
                break; 
            }
            
            case MONO1_COAG_MODE_SPRAY2: { 
                mono1Coag_spray2(); 
                break; 
            }

            case MONO1_COAG_MODE_SPRAY3:
            case MONO1_COAG_MODE_FULGURATION: { 
                mono1Coag_spray3(); 
                break; 
            }

            default: { 
                break; 
            }
        }

        if ((true == gPwrYeni) && (gMono1CoagWatt >= 10U) && (gMono1CoagWatt <= 120U)) {
            
            if (gHighVolt <= 2U) {
                gPwrKoruma = 1U;
            }

            if (gHighVolt < 200U) {
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONO1_COAG_DURUM_AKTIF == gMono1CoagDurum) {
        
        mono1Coag_stop();
    }
}