/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    bipolar1.c
 * @brief   Bipolar1 kanalı. Bipolar'da çalışma anı kodu moda bağlı değildir. 
 *          Moda özgü kısım dokunmatik modülünde uygulanır.
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
static Bipolar1Cut_State_e gCutDurum = BIPOLAR1_CUT_DURUM_BOSTA;
static Bipolar1Coag_State_e gCoagDurum = BIPOLAR1_COAG_DURUM_BOSTA;

static float gCutWatt = 10.0f;
static float gCoagWatt = 10.0f;
static uint32_t gCutKorumaVeri = 0U;
static uint32_t gCoagKorumaVeri = 0U;
static uint16_t gCutOkumaGecikme = 0U;
static uint16_t gCoagOkumaGecikme = 0U;
static uint8_t gCutOlcumSonuc = 0U;
static uint8_t gCoagOlcumSonuc = 0U;
static uint8_t gCutFormulAc = 0U;
static uint8_t gCoagFormulAc = 0U;
static uint8_t gCutKorumaStart = 0U;
static uint8_t gCoagKorumaStart = 0U;
static bool gCutYenidenHesap = false;
static bool gCoagYenidenHesap = false;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void bipolar1Cut_run(void);
static void bipolar1Coag_run(void);

/* ============================================================
 * Genel API
 * ============================================================*/

 /**
  * @brief Bipolar1 kanalının (CUT + COAG) tek giriş noktası
  * @retval None
  */
void bipolar1_run(void) {

    bipolar1Cut_run();
    bipolar1Coag_run();
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Bipolar1 CUT çıkış gücü ölçümü, koruma, başlatma ve durdurma
  * @retval None
  */
static void bipolar1Cut_run(void) {

    bool tetik = (gBipolar1CutStart == GUCU_ACIK) && (false == genx_isOtherChannelActive(gBipolar1CutStart));

    // Çıkış gücü ölçümü ve koruma
    if ((gBipolar1CutStart == GUCU_ACIK) && (UI_PAGE_MAIN == gAcikSayfa)) {
        
        gCutOkumaGecikme++;
        if (gCutOkumaGecikme >= 500U) {
            
            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            gCutOkumaGecikme = 0U;
            HAL_Delay(20U);
            if ((gBipolar1CutPol > 19.0f) && (gCutKorumaStart == 1U)) {

                gCutOlcumSonuc++;
                if (5U == gCutOlcumSonuc) {
                    float f = (-(0.000000003f * guc * guc * guc) + (0.00003f * guc * guc) - (0.1022f * guc) + (136.17f));
                    uint32_t veri = (f > 0U) ? (uint32_t)f : 0U;
                    
                    gCutOlcumSonuc = 0U;
                    if (20U == veri) {
                        veri = 22U;
                    }

                    gCutKorumaVeri = veri;
                    gCutYenidenHesap = true;
                    HAL_Delay(20U);
                    if (((float)veri > gBipolar1CutPol) || (veri >= 90U)) {

                        gCutFormulAc = 1U;
                    }

                    if (((float)veri < gBipolar1CutPol) && (veri < 90U)) {
                        gCutFormulAc = 2U;
                    }
                }
            }
        }
    }

    // Başlatma yada yeniden hesap
    if ((tetik == true) && ((BIPOLAR1_CUT_DURUM_BOSTA == gCutDurum) || (true == gCutYenidenHesap))) {

        (void)NEXTION_sendCmdRetry("tsw 255,0");
        genx_bipolarEnable(BIPOLAR_MODE_CUT);

        if ((0U == gCutKorumaStart) && (gBipolar1CutPol > 19.0f)) {

            gCutWatt = (0.000007f * 20U * 20U * 20U) - (0.0039f * 20U * 20U) + (1.2532f * 20U) + 40.714f;
            gCutKorumaStart = 1U;
        }

        gBipolarCutSes = 1U;
        HAL_Delay(200U);

        if ((gBipolar1CutPol < 20.0f) || (1U == gCutFormulAc)) {

            gCutFormulAc = 0U;
            if (10.0f == gBipolar1CutPol) gBipolar1CutPol = 2.0f;
            if (15.0f == gBipolar1CutPol) gBipolar1CutPol = 9.0f;

            gCutWatt = (0.000007f * gBipolar1CutPol * gBipolar1CutPol * gBipolar1CutPol) \
					- (0.0039f * gBipolar1CutPol * gBipolar1CutPol) + (1.2532f * gBipolar1CutPol) + 40.714f;
            gCutKorumaStart = 0U;
        }

        if (2U == gCutFormulAc) {
            gCutFormulAc = 0U;
            gCutWatt = (0.000007f * gCutKorumaVeri * gCutKorumaVeri * gCutKorumaVeri) \
                    - (0.0039f * gCutKorumaVeri * gCutKorumaVeri) + (1.2532f * gCutKorumaVeri) + 40.714f;
        }

        genx_dacSet(gCutWatt);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);        // Sarı
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET);      // Beyaz
        gCutDurum = BIPOLAR1_CUT_DURUM_AKTIF;
        gCutYenidenHesap = false;
    }

    // Durdurma
    if ((BIPOLAR1_CUT_DURUM_AKTIF == gCutDurum) && (GUCU_KAPALI == gBipolar1CutStart)) {

        genx_dacSet(0.0f);
        gBipolarCutSes = 2U;
        HAL_Delay(100U);
        genx_bipolarDisable();
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET);
        gCutOlcumSonuc = 0U;
        gCutKorumaStart = 0U;
        gCutFormulAc = 0U;
        gCutYenidenHesap = false;
        gCutDurum = BIPOLAR1_CUT_DURUM_BOSTA;
        HAL_Delay(300U);
        (void)NEXTION_sendCmdRetry("tsw 255,1");
    }
}

/**
  * @brief Bipolar1 COAG çıkış gücü ölçümü/koruma, başlatma (yeniden hesap dahil), durdurma, autostop
  * @retval None
  */
static void bipolar1Coag_run(void) {

    bool tetik = (gBipolar1CoagStart == GUCU_ACIK) && (false == genx_isOtherChannelActive(gBipolar1CoagStart));

    // Çıkış gücü ölçümü ve koruma
    if ((GUCU_ACIK == gBipolar1CoagStart) && (UI_PAGE_MAIN == gAcikSayfa)) {
        
        gCoagOkumaGecikme++;
        if (gCoagOkumaGecikme >= 500U) {

            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            gCoagOkumaGecikme = 0U;
            gLigasureWattYaz = gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];
            HAL_Delay(20U);
            if ((gBipolar1CoagPol >= 20.0f) && (gCoagKorumaStart == 1U)) {

                gCoagOlcumSonuc++;
                if (5U == gCoagOlcumSonuc) {
                    
                    float f = ((0.000003f * guc * guc) - (0.0273f * guc) + (75.577f));
                    uint32_t veri = (f > 0.0f) ? (uint32_t)f : 0U;

                    gCoagOlcumSonuc = 0U;
                    veri *= 2U;

                    if (20U == veri) {
                        veri = 22U;
                    }

                    gCoagKorumaVeri = veri;
                    gCoagYenidenHesap = true;
                    HAL_Delay(20U);

                    if (((float)veri > gBipolar1CoagPol) || (veri >= 120U)) {
                        gCoagFormulAc = 1U;
                    }

                    if (((float)veri < gBipolar1CoagPol) && (veri < 120U)) {
                        gCoagFormulAc = 2U;
                    }
                }
            }
        }
    }

    // Başlatma yada yeniden hesap
    if ((true == tetik) && ((BIPOLAR1_COAG_DURUM_BOSTA == gCoagDurum) || (true == gCoagYenidenHesap))) {
        
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        genx_bipolarEnable(BIPOLAR_MODE_COAG);
        
        if ((0U == gCoagKorumaStart) && (20.0f <= gBipolar1CoagPol)) {
            gCoagWatt = (0.000004f * 20.0f * 20.0f * 20.0f) - (0.0032f * 20.0f * 20.0f) + (1.224f * 20.0f) + 34.541f;
            gCoagKorumaStart = 1U;
        }

        gBipolarCoagSes = 1U;
        HAL_Delay(200U);

        if ((gBipolar1CoagPol < 20.0f) || (1U == gCoagFormulAc)) {
            gCoagFormulAc = 0U;
            if (10.0f == gBipolar1CoagPol) gBipolar1CoagPol = 7.0f;

            gCoagWatt = (0.000004f * gBipolar1CoagPol * gBipolar1CoagPol * gBipolar1CoagPol) \
								- (0.0032f * gBipolar1CoagPol * gBipolar1CoagPol) + (1.224f * gBipolar1CoagPol) + 34.541f;

            gCoagKorumaStart = 0U;
        }

        if (2U == gCoagFormulAc) {
            gCoagFormulAc = 0U;
            gCoagWatt = (0.000004f * gCoagKorumaVeri * gCoagKorumaVeri * gCoagKorumaVeri) \
					- (0.0032f * gCoagKorumaVeri * gCoagKorumaVeri) + (1.224f * gCoagKorumaVeri) + 34.541f;

            gCoagKorumaStart = 0U;
        }

        genx_dacSet(gCoagWatt);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_SET);        // Mavi
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET);      // Beyaz
        gCoagDurum = BIPOLAR1_COAG_DURUM_AKTIF;
        gCoagYenidenHesap = false;
    }

    // Durdurma
    if ((BIPOLAR1_COAG_DURUM_AKTIF == gCoagDurum) && (gBipolar1CoagStart == GUCU_KAPALI)) {
        genx_dacSet(0.0f);
        gBipolarCoagSes = 2U;
        HAL_Delay(100U);
        genx_bipolarDisable();
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_13, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_SET);
        gCoagOlcumSonuc = 0U;
        gCoagKorumaStart = 0U;
        gCoagFormulAc = 0U;
        gCoagYenidenHesap = false;
        gCoagDurum = BIPOLAR1_COAG_DURUM_BOSTA;
        HAL_Delay(300U);
        (void)NEXTION_sendCmdRetry("tsw 255,1");
    }

    // Autostop
    if ((gBipolar1CoagStart == GUCU_ACIK) && (1U == gBipolarAutoStop) && (1U == gLigasurePedal) && (gLigasureWattYaz > 500U)) {
        gAutoStopStart0 = 1U;
    }

    if ((1U == gAutoStopStart0) && (gLigasureWattYaz < 400U)) {
        gAutoStopStart0 = 0U;
        gAutoStop2 = 1U;
        gBipolar1CoagBurst1 = 0U;
        gBipolar1CoagBurst2 = 0U;
        gBipolar1CoagStart = GUCU_KAPALI;
        gLigasurePedal = 0U;
    }
}