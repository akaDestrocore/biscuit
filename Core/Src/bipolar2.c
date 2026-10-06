/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    bipolar2.c
 * @brief   Bipolar2 (seal/ligasure) kanalı (CUT + COAG + AUTOSTOP). 
 *          Bipolar'da çalışma anı kodu moda bağlı değildir.
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
static Bipolar2Cut_State_e gCutDurum = BIPOLAR2_CUT_DURUM_BOSTA;
static Bipolar2Coag_State_e gCoagDurum = BIPOLAR2_COAG_DURUM_BOSTA;

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
static void bipolar2Cut_run(void);
static void bipolar2Coag_run(void);

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief Bipolar2 kanalının (CUT + COAG) tek giriş noktası
  * @retval None
  */
void bipolar2_run(void) {

    bipolar2Cut_run();
    bipolar2Coag_run();
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Bipolar2 CUT çıkış gücü ölçümü, koruma, başlatma ve durdurma
  * @retval None
  */
static void bipolar2Cut_run(void) {

    bool tetik = (GUCU_ACIK == gBipolar2CutStart) && (false == genx_isOtherChannelActive(gBipolar2CutStart));

    // Çıkış gücü ölçümü ve koruma
    if ((GUCU_ACIK == gBipolar2CutStart) && (UI_PAGE_MAIN == gAcikSayfa)) {
        gCutOkumaGecikme++;
        if (500U <= gCutOkumaGecikme) {
            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            gCutOkumaGecikme = 0U;
            HAL_Delay(20U);
            if ((19.0f < gBipolar2CutPol) && (1U == gCutKorumaStart)) {
                gCutOlcumSonuc++;
                if (5U == gCutOlcumSonuc) {
                     float f = (-(0.000000003f * guc * guc * guc) + (0.00003f * guc * guc) - (0.1022f * guc) + (136.17f));
                    uint32_t veri = (0.0f < f) ? (uint32_t)f : 0U;

                    gCutOlcumSonuc = 0U;
                    if (20U == veri) {

                        veri = 22U;
                    }

                    gCutKorumaVeri = veri;
                    gCutYenidenHesap = true;
                    HAL_Delay(20U);
                    if (((float)veri > gBipolar2CutPol) || (veri >= 90U)) {
                        gCutFormulAc = 1U;
                    }

                    if (((float)veri < gBipolar2CutPol) && (veri < 90U)) {
                        gCutFormulAc = 2U;
                    }
                }
            }
        }
    }

    // Başlatma yada yeniden hesap
    if ((true == tetik) && ((BIPOLAR2_CUT_DURUM_BOSTA == gCutDurum) || (true == gCutYenidenHesap))) {
        
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        genx_bipolarEnable(BIPOLAR_MODE_CUT);

        if ((0U == gCutKorumaStart) && (gBipolar2CutPol > 19.0f)) {
            gCutWatt = (0.000007f * 20U * 20U * 20U) - (0.0039f * 20U * 20U) + (1.2532f * 20U) + 40.714f;
            gCutKorumaStart = 1U;
        }

        gBipolarCutSes = 1U;
        HAL_Delay(200U);

        if ((gBipolar2CutPol < 20.0f) || (1U == gCutFormulAc)) {
            gCutFormulAc = 0U;
            if (10.0f == gBipolar2CutPol) gBipolar2CutPol = 2.0f;
            if (15.0f == gBipolar2CutPol) gBipolar2CutPol = 9.0f;
            gCutWatt = (0.000007f * gBipolar2CutPol * gBipolar2CutPol * gBipolar2CutPol) \
					- (0.0039f * gBipolar2CutPol * gBipolar2CutPol) + (1.2532f * gBipolar2CutPol) + 40.714f;
            gCutKorumaStart = 0U;
        }

        if (2U == gCutFormulAc) {
            gCutFormulAc = 0U;
            gCutWatt = (0.000007f * gCutKorumaVeri * gCutKorumaVeri * gCutKorumaVeri) \
                    - (0.0039f * gCutKorumaVeri * gCutKorumaVeri) + (1.2532f * gCutKorumaVeri) + 40.714f;
            gCutKorumaStart = 0U;
        }

        genx_dacSet(gCutWatt);
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_15, GPIO_PIN_SET);        // Sarı
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, GPIO_PIN_RESET);      // Beyaz
        gCutDurum = BIPOLAR2_CUT_DURUM_AKTIF;
        gCutYenidenHesap = false;
    }

    // Durdurma
    if ((BIPOLAR2_CUT_DURUM_AKTIF == gCutDurum) && (gBipolar2CutStart == GUCU_KAPALI)) {
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
        gCutDurum = BIPOLAR2_CUT_DURUM_BOSTA;
        HAL_Delay(300U);
        (void)NEXTION_sendCmdRetry("tsw 255,1");
    }
}

/**
  * @brief Bipolar2 COAG çıkış gücü ölçümü/koruma, başlatma (yeniden hesap dahil), durdurma, autostop
  * @retval None
  */
static void bipolar2Coag_run(void) {
    
    bool tetik = (GUCU_ACIK == gBipolar2CoagStart) && (false == genx_isOtherChannelActive(gBipolar2CoagStart));

    // Çıkış gücü ölçümü ve koruma
    if ((GUCU_ACIK == gBipolar2CoagStart) && (UI_PAGE_MAIN == gAcikSayfa)) {
        gCoagOkumaGecikme++;
        if (500U <= gCoagOkumaGecikme) {
            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            gCoagOkumaGecikme = 0U;
            gLigasureWattYaz = gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];
            HAL_Delay(20U);
            if ((20.0f <= gBipolar2CoagPol) && (1U == gCoagKorumaStart)) {
                gCoagOlcumSonuc++;
                if (5U == gCoagOlcumSonuc) {
                    float f = (0.000003f * guc * guc) - (0.0273f * guc) + 75.577f;
                    uint32_t veri = (f > 0.0f) ? (uint32_t)f : 0U;

                    gCoagOlcumSonuc = 0U;
                    veri = veri * 2U;
                    if (20U == veri) {
                        veri = 22U;
                    }
                    gCoagKorumaVeri = veri;
                    gCoagYenidenHesap = true;
                    HAL_Delay(20U);
                    if (((float)veri > gBipolar2CoagPol) || (veri >= 120U)) {
                        gCoagFormulAc = 1U;
                    }

                    if (((float)veri < gBipolar2CoagPol) && (veri < 120U)) {
                        gCoagFormulAc = 2U;
                    }
                }
            }
        }
    }

    // Başlatma yada yeniden hesap
    if ((true == tetik) && ((BIPOLAR2_COAG_DURUM_BOSTA == gCoagDurum) || (true == gCoagYenidenHesap))) {
        
        (void)NEXTION_sendCmdRetry("tsw 255,0");
        genx_bipolarEnable(BIPOLAR_MODE_COAG);

        if ((0U == gCoagKorumaStart) && (20.0f <= gBipolar2CoagPol)) {
            gCoagWatt = (0.000004f * 20.0f * 20.0f * 20.0f) - (0.0032f * 20.0f * 20.0f) + (1.224f * 20.0f) + 34.541f;
            gCoagKorumaStart = 1U;
        }

        gBipolarCoagSes = 1U;
        HAL_Delay(200U);

        if ((gBipolar2CoagPol < 20.0f) || (1U == gCoagFormulAc)) {
            gCoagFormulAc = 0U;
            if (10.0f == gBipolar2CoagPol) gBipolar2CoagPol = 7.0f;
            gCoagWatt = (0.000004f * gBipolar2CoagPol * gBipolar2CoagPol * gBipolar2CoagPol) \
								- (0.0032f * gBipolar2CoagPol * gBipolar2CoagPol) + (1.224f * gBipolar2CoagPol) + 34.541f;

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
        gCoagDurum = BIPOLAR2_COAG_DURUM_AKTIF;
        gCoagYenidenHesap = false;
    }

    // Durdurma
    if ((BIPOLAR2_COAG_DURUM_AKTIF == gCoagDurum) && (GUCU_KAPALI == gBipolar2CoagStart)) {
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
        gCoagDurum = BIPOLAR2_COAG_DURUM_BOSTA;
        HAL_Delay(300U);
        (void)NEXTION_sendCmdRetry("tsw 255,1");
    }

    // Autosotop
    if ((GUCU_ACIK == gBipolar2CoagStart) && (GUCU_ACIK == gLigasureStart) && (1U == gLigasurePedal) && (gLigasureWattYaz > 550U)) {
        gAutoStopStart = GUCU_ACIK;
    }

    if ((GUCU_ACIK == gAutoStopStart) && (gLigasureWattYaz < 450U)) {
        gAutoStopStart = GUCU_KAPALI;
        gAutoStop2 = 1U;
        gBipolar2CoagBurst1 = 0U;
        gBipolar2CoagBurst2 = 0U;
        gBipolar2CoagStart = GUCU_KAPALI;
        gLigasurePedal = 0U;
    }
}