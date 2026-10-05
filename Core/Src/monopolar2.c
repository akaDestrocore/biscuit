/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    monopolar2.c
 * @brief   Mono2 kanalı. Her mod kendi başlatma ve izleme adımlarını içerir, 
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
static volatile Mono2Cut_State_e gMono2CutDurum = MONO2_CUT_DURUM_BOSTA;
static Mono2Coag_State_e gMono2CoagDurum = MONO2_COAG_DURUM_BOSTA;
static uint16_t gGucGecikme = 0U;
static uint8_t gYuzStart = 0U;
static volatile uint8_t gEndoCut2 = 0U;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void mono2Cut_cut(void);
static void mono2Cut_blend1(void);
static void mono2Cut_blend2(void);
static void mono2Cut_blend3(void);
static void mono2Cut_endo(void);
static void mono2Cut_stop(void);
static void mono2Cut_run(void);
static void mono2Coag_contact(void);
static void mono2Coag_spray1(void);
static void mono2Coag_spray2(void);
static void mono2Coag_spray3(void);
static void mono2Coag_stop(void);
static void mono2Coag_run(void);

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief Mono2 kanalının (CUT + COAG) tek giriş noktası
  * @retval None
  */
void mono2_run(void) {

    mono2Cut_run();
    mono2Coag_run();
}

/**
  * @brief TIM5 periyot kesmesi: Mono2 Endo (POLYPECTOMY/PAPILLOTOMES) cut/coag darbe dizisi
  * @param pHtim Kesmeyi üreten zamanlayıcı
  * @retval None
  */
// void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *pHtim) {
//     if (TIM5 == pHtim->Instance) {
//         if (((MONO2_CUT_MODE_POLYPECTOMY == gMono2CutMode) || (MONO2_CUT_MODE_PAPILLOTOMES == gMono2CutMode)) \
//             && (GUCU_ACIK == gMono2CutStart)) {
//             gEndoCut2 = 1U;
//             gEndoCutSay++;
//             if (gEndoCutSay <= gEndoCutSure) {          // 1 == 0.01 saniye
//                 gCut2WattPol = ((0.000022f * gEndoCutWattPol * gEndoCutWattPol * gEndoCutWattPol) - (0.0205f * gEndoCutWattPol * gEndoCutWattPol) \
// 					+ (10.25f * gEndoCutWattPol) + (287U));
//                 gCut2WattPol /= 10.0f;

//                 genx_dacSet(gCut2WattPol);
//             }
//             if (gEndoCutSay > gEndoCutSure) {
//                 __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 108U);
//                 HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

//                 gCoag2WattPol = ((0.000075f * gEndoCoagWattPol * gEndoCoagWattPol * gEndoCoagWattPol) \
// 					- (0.0212f * gEndoCoagWattPol * gEndoCoagWattPol) + (3.2617f * gEndoCoagWattPol) + (34.4f));
                
//                 genx_dacSet(gEndoCoagWattPol);
//             }
//             if (gEndoCutSay >= (gEndoCoagTimer + gEndoCutSure)) {
                
//                 gEndoCutSay = 0U;
//                 HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
//             }
//         }

//         if ((GUCU_KAPALI == gMono2CutStart) && (MONO2_CUT_DURUM_BOSTA == gMono2CutDurum) && (1U == gEndoCut2)) {
//             gEndoCut2 = 0U;
//             genx_dacSet(0.0f);
//         }
//     }
// }

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Mono2 CUT / RESECTION1 / RESECTION2 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono2Cut_cut(void) {

    if (MONO2_CUT_DURUM_BOSTA == gMono2CutDurum) {
        if (0U == gPlate100Overresistance) {
            float x = gCut2WattPol;

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

            gMono2CutDurum = MONO2_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {
            
            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);

            if ((1U == gYuzStart) && (guc > 70U)) {

                gYuzStart = 0U;
                gCut2WattPol = ((0.000021f * gCut2WattPol * gCut2WattPol * gCut2WattPol) - (0.0205f * gCut2WattPol * gCut2WattPol) \
									+ (10.1f * gCut2WattPol) + (287U));
                gCut2WattPol /= 10.0f;

                genx_dacSet(gCut2WattPol);
            }

            uint16_t monoCut2KorumaVeri = ((0.0000278f * gCut2WattPol * gCut2WattPol * gCut2WattPol) \
										- (0.0383f * gCut2WattPol * gCut2WattPol) + (19.136f * gCut2WattPol) + (683.43f));

            if ((gCut2WattPol >= 50U) && (guc > (uint32_t)monoCut2KorumaVeri)) {
                gMono2Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono2 BLEND1 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono2Cut_blend1(void) {

    if (MONO2_CUT_DURUM_BOSTA == gMono2CutDurum) {
        if (0U == gPlate100Overresistance) {
            
            float x = gCut2WattPol;

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

            gMono2CutDurum = MONO2_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);
            if ((1U == gYuzStart) && (guc > 120U)) {
                gYuzStart = 0U;
                gCut2WattPol = (0.0000126f * gCut2WattPol * gCut2WattPol * gCut2WattPol) \
                            - (0.0063f * gCut2WattPol * gCut2WattPol) + (1.6669f * gCut2WattPol) + 31.0f;

                genx_dacSet(gCut2WattPol);
            }

            uint16_t monoCut2KorumaVeri = ((0.000186f * gCut2WattPol * gCut2WattPol * gCut2WattPol) \
                                        - (0.0928f * gCut2WattPol * gCut2WattPol) + (23.777f * gCut2WattPol) + (548.8f));

            if ((gCut2WattPol >= 30.0f) && (guc > monoCut2KorumaVeri)) {
                gMono2Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono2 BLEND2 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono2Cut_blend2(void) {

    if (MONO2_CUT_DURUM_BOSTA == gMono2CutDurum) {
        if (0U == gPlate100Overresistance) {
            
            float x = gCut2WattPol;

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

            gMono2CutDurum = MONO2_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {
            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);
            if ((1U == gYuzStart) && (guc > 130U)) {
                gYuzStart = 0U;
                gCut2WattPol = (0.0000173f * gCut2WattPol * gCut2WattPol * gCut2WattPol) - (0.00755f * gCut2WattPol * gCut2WattPol) \
                            + (1.9052f * gCut2WattPol) + 33.0f;

                genx_dacSet(gCut2WattPol);
            }

            uint16_t monoCut2KorumaVeri = ((0.000115f * gCut2WattPol * gCut2WattPol * gCut2WattPol) \
									- (0.058f * gCut2WattPol * gCut2WattPol) + (18.493f * gCut2WattPol) + (704U));

            if ((gCut2WattPol >= 30.0f) && (guc > (uint32_t)monoCut2KorumaVeri)) {
                gMono2Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono2 BLEND3 başlatma ve çıkış gücü izleme
  * @retval None
  */
static void mono2Cut_blend3(void) {

    if (MONO2_CUT_DURUM_BOSTA == gMono2CutDurum) {
        if (0U == gPlate100Overresistance) {
            
            float x = gCut2WattPol;

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

            gMono2CutDurum = MONO2_CUT_DURUM_AKTIF;
            HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
        }
    } else {
        gGucGecikme++;
        if ((gGucGecikme >= 1000U) && (1U == gMonopolarCutSes)) {
            
            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            gGucGecikme = 0U;
            HAL_Delay(20U);
            if ((1U == gYuzStart) && (guc > 130U)) {
                gYuzStart = 0U;

                gCut2WattPol = (0.000039f * gCut2WattPol * gCut2WattPol * gCut2WattPol) - (0.0135f * gCut2WattPol * gCut2WattPol) + (2.5977f * gCut2WattPol) + 33.355f;
                
                genx_dacSet(gCut2WattPol);
            }

            uint16_t monoCut2KorumaVeri = ((0.000465f * gCut2WattPol * gCut2WattPol * gCut2WattPol) \
										- (0.1818f * gCut2WattPol * gCut2WattPol) + (34.623f * gCut2WattPol) + (319.0f)); 

            if ((gCut2WattPol >= 30.0f) && (guc > (uint32_t)monoCut2KorumaVeri)) {
                gMono2Koruma = 1U;
            }
        }
    }
}

/**
  * @brief Mono2 endocut başlatma. DAC'ı TIM5 kesmesi sürer.
  * @retval None
  */
static void mono2Cut_endo(void) {
    
    if ((MONO2_CUT_DURUM_BOSTA == gMono2CutDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_CUT);
        gMonopolarCutSes = 1U;
        HAL_Delay(200U);
        genx_dacSet(0.0f);
        gEndoCutSay = 0U;
        HAL_TIM_Base_Start_IT(&htim5);

        gMono2CutDurum = MONO2_CUT_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono2 CUT çıkışını kapatır
  * @retval None
  */
static void mono2Cut_stop(void) {

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
    gMono2CutDurum = MONO2_CUT_DURUM_BOSTA;
    gYuzStart = 0U;
    HAL_GPIO_WritePin(MONO_SARI_GPIO_Port, MONO_SARI_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_SET);
    HAL_Delay(300U);
    (void)NEXTION_sendCmdRetry("tsw 255,1");
}

/**
  * @brief Mono2 CUT kanalı: Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @retval None
  */
static void mono2Cut_run(void) {

    if (GUCU_ACIK == gMono2CutStart) {
        
        bool pwrKontrol = false;

        switch (gMono2CutMode) {

            case MONO2_CUT_MODE_CUT:
            case MONO2_CUT_MODE_RESECTION1:
            case MONO2_CUT_MODE_RESECTION2: { 
                mono2Cut_cut();
                pwrKontrol = true;
                break;
            }

            case MONO2_CUT_MODE_BLEND1: { 
                mono2Cut_blend1(); 
                pwrKontrol = true; 
                break; 
            }

            case MONO2_CUT_MODE_BLEND2: { 
                mono2Cut_blend2(); 
                pwrKontrol = true; 
                break; 
            }

            case MONO2_CUT_MODE_BLEND3: { 
                mono2Cut_blend3(); 
                pwrKontrol = true; 
                break; 
            }

            case MONO2_CUT_MODE_POLYPECTOMY:
            case MONO2_CUT_MODE_PAPILLOTOMES: { 
                mono2Cut_endo(); 
                break; 
            }

            default: { 
                break; 
            }
        }

        // Güç kaynağı kontrolü
        if ((true == pwrKontrol) && (true == gPwrYeni)) {

            if ((gMono2CutWatt >= 10U) && (gMono2CutWatt <= 400U) && (gHighVolt <= 2U)) {
                
                gPwrKoruma = 1U;
            }

            if ((((gMono2CutWatt >= 10U) && (gMono2CutWatt <= 45U)) || ((gMono2CutWatt >= 50U) && (gMono2CutWatt <= 400U))) \
            && (gHighVolt < 200U)) {
                
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONO2_CUT_DURUM_AKTIF == gMono2CutDurum) {
        
        mono2Cut_stop();
    }
}

/**
  * @brief Mono2 COAG CONTACT başlatma
  * @retval None
  */
static void mono2Coag_contact(void) {
    if ((MONO2_COAG_DURUM_BOSTA == gMono2CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 108U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);
        gCoag2WattPol = (0.000087f * gCoag2WattPol * gCoag2WattPol * gCoag2WattPol) - (0.021f * gCoag2WattPol * gCoag2WattPol) \
							+ (3.0f * gCoag2WattPol) + 29.0f;
        
        genx_dacSet(gCoag2WattPol);
        
        gMono2CoagDurum = MONO2_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono2 COAG SPRAY1 başlatma
  * @retval None
  */
static void mono2Coag_spray1(void) {

    if ((MONO2_COAG_DURUM_BOSTA == gMono2CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 20U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        //spray Relay
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);

        gCoag2WattPol = (0.000007f * gCoag2WattPol * gCoag2WattPol * gCoag2WattPol) - (0.0153f * gCoag2WattPol * gCoag2WattPol) \
					+ (3.5f * gCoag2WattPol) + 50.7f;

        genx_dacSet(gCoag2WattPol);

        gMono2CoagDurum = MONO2_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono2 COAG SPRAY2 başlatma
  * @retval None
  */
static void mono2Coag_spray2(void) {

    if ((MONO2_COAG_DURUM_BOSTA == gMono2CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 22U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        //spray Relay
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);
        gCoag2WattPol = (0.00007f * gCoag2WattPol * gCoag2WattPol * gCoag2WattPol) - (0.0234f * gCoag2WattPol * gCoag2WattPol) \
					+ (3.7182f * gCoag2WattPol) + 42.463f;
        genx_dacSet(gCoag2WattPol);

        gMono2CoagDurum = MONO2_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono2 COAG SPRAY3 ve FULGURATION başlatma
  * @retval None
  */
static void mono2Coag_spray3(void) {
    
    if ((MONO2_COAG_DURUM_BOSTA == gMono2CoagDurum) && (0U == gPlate100Overresistance)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 24U);
        HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        //spray Relay
        genx_monoEnable(MONO_RELAY_Pin, MUX_MONO_COAG);
        gMonopolarCoagSes = 1U;
        HAL_Delay(200U);
        gCoag2WattPol = (0.00009f * gCoag2WattPol * gCoag2WattPol * gCoag2WattPol) - (0.0244f * gCoag2WattPol * gCoag2WattPol) \
					+ (3.6567f * gCoag2WattPol) + 40.692f;
        genx_dacSet(gCoag2WattPol);

        gMono2CoagDurum = MONO2_COAG_DURUM_AKTIF;
        HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_RESET);
    }
}

/**
  * @brief Mono2 COAG çıkışını kapatır
  * @retval None
  */
static void mono2Coag_stop(void) {

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
    gMono2CoagDurum = MONO2_COAG_DURUM_BOSTA;
    HAL_GPIO_WritePin(MONO_MAVI_GPIO_Port, MONO_MAVI_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin, GPIO_PIN_SET);
    HAL_Delay(300U);
    (void)NEXTION_sendCmdRetry("tsw 255,1");
}

/**
  * @brief Mono2 COAG kanalı: Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @retval None
  */
static void mono2Coag_run(void) {

    if (GUCU_ACIK == gMono2CoagStart) {

        switch (gMono2CoagMode) {
            
            case MONO2_COAG_MODE_CONTACT: { 
                mono2Coag_contact(); 
                break; 
            }

            case MONO2_COAG_MODE_SPRAY1: { 
                mono2Coag_spray1(); 
                break; 
            }
            
            case MONO2_COAG_MODE_SPRAY2: { 
                mono2Coag_spray2(); 
                break; 
            }

            case MONO2_COAG_MODE_SPRAY3:
            case MONO2_COAG_MODE_FULGURATION: { 
                mono2Coag_spray3(); 
                break; 
            }

            default: { 
                break; 
            }
        }

        if ((true == gPwrYeni) && (gMono2CoagWatt >= 10U) && (gMono2CoagWatt <= 120U)) {
            
            if (gHighVolt <= 2U) {
                gPwrKoruma = 1U;
            }

            if (gHighVolt < 200U) {
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONO2_COAG_DURUM_AKTIF == gMono2CoagDurum) {
        
        mono2Coag_stop();
    }
}