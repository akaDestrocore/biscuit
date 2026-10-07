/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    bipolar.c
 * @brief   Bipolar tanımları. Bipolar'da çalışma anı kodu moda bağlı değildir, 
 *          modlar yalnızca seçim doğrulaması için maskelenir. 
 *          Moda özgü kısım touch.c içinde uygulanır!
 *
 * @author  destrocore
 * @date    2026
 */

#include "bipolar.h"

// -- Fallback -------------------------------------------------
#if (0U == BIPOLAR1_CUT_EN_CUTTING) || (0U == BIPOLAR2_CUT_EN_CUTTING)
#error "BIPOLAR CUT: CUTTING modu kapatildi"
#endif

#if (0U == BIPOLAR1_COAG_EN_STANDARD)
#error "BIPOLAR1 COAG: STANDARD modu kapatildi"
#endif

#if (0U == BIPOLAR2_COAG_EN_LIGATION)
#error "BIPOLAR2 COAG: LIGATION modu kapatildi"
#endif

/* ============================================================
 * Kanal yapılandırmaları
 * ============================================================*/
static const Bipolar_Config_t gBipolar1CutCfg = {
    .function           = BIPOLAR_FUNCTION_CUT,
    .profile            = BIPOLAR_PROFILE_STANDARD,
    .enabled_mask       = BIPOLAR1_CUT_MASK,
    .fallback_mode      = BIPOLAR_CUTMODE_CUTTING,
    .max_watt           = 200U,
    .pWattGetCmd        = "get S10_b1_cut_wat.USER_VALUE.val",
    .pModeGetCmd        = "get S12_b1_cut_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n6.val=S10_b1_cut_wat.USER_VALUE.val"
};

static const Bipolar_Config_t gBipolar1CoagCfg = {
    .function           = BIPOLAR_FUNCTION_COAG,
    .profile            = BIPOLAR_PROFILE_STANDARD,
    .enabled_mask       = BIPOLAR1_COAG_MASK,
    .fallback_mode      = BIPOLAR_STD_COAGMODE_STANDARD,
    .max_watt           = 150U,
    .pWattGetCmd        = "get S11_b1_cog_wat.USER_VALUE.val",
    .pModeGetCmd        = "get S13_b1_cog_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n7.val=S11_b1_cog_wat.USER_VALUE.val"
};

static const Bipolar_Config_t gBipolar2CutCfg = {
    .function           = BIPOLAR_FUNCTION_CUT,
    .profile            = BIPOLAR_PROFILE_SEAL,
    .enabled_mask       = BIPOLAR2_CUT_MASK,
    .fallback_mode      = BIPOLAR_CUTMODE_CUTTING,
    .max_watt           = 200U,
    .pWattGetCmd        = "get S14_b2_cut_wat.USER_VALUE.val",
    .pModeGetCmd        = "get S16_b2_cut_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n4.val=S14_b2_cut_wat.USER_VALUE.val"
};

static const Bipolar_Config_t gBipolar2CoagCfg = {
    .function           = BIPOLAR_FUNCTION_COAG,
    .profile            = BIPOLAR_PROFILE_SEAL,
    .enabled_mask       = BIPOLAR2_COAG_MASK,
    .fallback_mode      = BIPOLAR_SEAL_COAGMODE_LIGATION,
    .max_watt           = 350U,
    .pWattGetCmd        = "get S15_b2_cog_wat.USER_VALUE.val",
    .pModeGetCmd        = "get S17_b2_cog_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n5.val=S15_b2_cog_wat.USER_VALUE.val"
};

Bipolar_Handle_t gBipolar1Cut = {
    .pCfg = &gBipolar1CutCfg,
    .canli = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_CUTMODE_CUTTING, 
        .pol = 10.0f 
    }
};

Bipolar_Handle_t gBipolar1Coag = {
    .pCfg = &gBipolar1CoagCfg,
    .canli = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_STD_COAGMODE_STANDARD, 
        .pol = 10.0f 
    }
};

Bipolar_Handle_t gBipolar2Cut = {
    .pCfg = &gBipolar2CutCfg,
    .canli = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_CUTMODE_CUTTING, 
        .pol = 10.0f 
    }
};

Bipolar_Handle_t gBipolar2Coag = {
    .pCfg = &gBipolar2CoagCfg,
    .canli = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_SEAL_COAGMODE_TISSUELOCK, 
        .pol = 10.0f 
    }
};

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void bipolar_setLeds(Bipolar_Function_e function, bool isActive);
static void bipolar_runCut(Bipolar_Handle_t *pH);
static void bipolar_runCoag(Bipolar_Handle_t *pH);

/* ============================================================
 * Genel API
 * ============================================================*/

/**
 * @brief Bütün bipolar kanallarını kaydına ekler
 * @retval None
 */
void bipolar_init(void) {

    genx_registerChannel(&gBipolar1Cut.canli.common);
    genx_registerChannel(&gBipolar1Coag.canli.common);
    genx_registerChannel(&gBipolar2Cut.canli.common);
    genx_registerChannel(&gBipolar2Coag.canli.common);
}

/**
  * @brief Bipolar yollarının tek giriş noktası
  * @retval None
  */
void bipolar_run(void) {

    bipolar_runCut(&gBipolar1Cut);
    bipolar_runCoag(&gBipolar1Coag);
    bipolar_runCut(&gBipolar2Cut);
    bipolar_runCoag(&gBipolar2Coag);
}

/**
  * @brief Modun bu kanalda açık olup olmadığını döndürür
  * @param pH Kanal handle yapısı
  * @param modeId Nextion'da saklanan mod değeri
  * @retval true mod açık, false kapalı yada tanımsız
  */
bool bipolar_isModeEnabled(const Bipolar_Handle_t *pH, uint32_t modeId) {

    return (0U != (pH->pCfg->enabled_mask & (1U << modeId)));
}

/**
  * @brief Kayıtlı mod bu kanalda kapalıysa fallback moduna çevirir
  * @param pH Kanal handle'ı
  * @retval true mod düzeltildi, false değişiklik yok
  */
bool bipolar_isModeCorrected(Bipolar_Handle_t *pH) {

    bool isCorrected = false;

    if (false == bipolar_isModeEnabled(pH, pH->canli.user_mode)) {
        pH->canli.user_mode = pH->pCfg->fallback_mode;
        isCorrected = true;
    }

    return isCorrected;
}

/**
  * @brief Kullanıcı watt ve mod değerinden kanalın işlenmiş watt değerini hesaplar
  * @param pH Kanal handle'ı
  * @retval None
  */
void bipolar_updatePol(Bipolar_Handle_t *pH) {

    Bipolar_Runtime_t *pBip = &pH->canli;

    float pol = (float)pBip->user_watt;
    float limit = 185.0f;

    if (BIPOLAR_FUNCTION_COAG == pH->pCfg->function) {
        if (BIPOLAR_PROFILE_SEAL == pH->pCfg->profile) {
            
            limit = 310.0f;
        } else {
            
            limit = 130.0f;

            switch (pBip->user_mode) {
                
                case BIPOLAR_STD_COAGMODE_STANDARD:
                case BIPOLAR_STD_COAGMODE_FORCED: {
                    limit = 150.0f;
                    break;
                }

                case BIPOLAR_STD_COAGMODE_STOP:
                case BIPOLAR_STD_COAGMODE_START:
                case 5: {
                    limit = 120.0f;
                    break;
                }

                default: {
                    break;
                }
            }
        }
    }

    if (pol > limit) {
        
        pol = limit;
    }

    pBip->pol = pol;
}

/* ================================================================
 * Yardımcı fonksiyonlar
 * ================================================================*/

/**
  * @brief Bipolar LED'lerini ayarlar (Bipolar1 ve Seal aynı LED'leri paylaşır)
  * @param func CUT yada COAG
  * @param aktifMi true aktif, false boşta
  * @retval None
  */
static void bipolar_setLeds(Bipolar_Function_e func, bool aktifMi) {

    uint16_t colorPin = (BIPOLAR_FUNCTION_CUT == func) ? GPIO_PIN_15 : GPIO_PIN_13;     // Sarı / Mavi

    HAL_GPIO_WritePin(GPIOE, colorPin, (true == aktifMi) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_14, (true == aktifMi) ? GPIO_PIN_RESET : GPIO_PIN_SET);  // Beyaz
}

/**
  * @brief Bipolar CUT çıkış gücü ölçümü, koruma, başlatma ve durdurma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void bipolar_runCut(Bipolar_Handle_t *pH) {

    Bipolar_Runtime_t *pBip = &pH->canli;
    bool isTrigger = (1U == pBip->common.start) && (false == genx_isOtherChannelActive(&pBip->common));

    // Çıkış gücü ölçümü ve koruma
    if ((1U == pBip->common.start) && (UI_PAGE_MAIN == gAcikSayfa)) {
        pBip->okuma_gecikme++;
        if (500U <= pBip->okuma_gecikme) {
            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            pBip->okuma_gecikme = 0U;
            HAL_Delay(20U);
            if ((19.0f < pBip->pol) && (1U == pBip->koruma_start)) {
                pBip->olcum_sonuc++;
                if (5U == pBip->olcum_sonuc) {
                    float f = (-(0.000000003f * guc * guc * guc) + (0.00003f * guc * guc) - (0.1022f * guc) + (136.17f));
                    uint32_t veri = (0.0f < f) ? (uint32_t)f : 0U;

                    pBip->olcum_sonuc = 0U;
                    if (20U == veri) {
                        veri = 22U;
                    }

                    pBip->koruma_veri = veri;
                    pBip->yeniden_hesap = true;
                    HAL_Delay(20U);
                    if (((float)veri > pBip->pol) || (veri >= 90U)) {
                        pBip->formul_ac = 1U;
                    }

                    if (((float)veri < pBip->pol) && (veri < 90U)) {
                        pBip->formul_ac = 2U;
                    }
                }
            }
        }
    }

    // Başlatma yada yeniden hesap
    if ((true == isTrigger) && ((BIPOLAR_STATE_IDLE == pBip->state) || (true == pBip->yeniden_hesap))) {
        
        if (0U == genx_claim(&pBip->common)) {
            float x;

            (void)NEXTION_sendCmdRetry("tsw 255,0");
            genx_bipolarEnable(BIPOLAR_MODE_CUT);

            if ((0U == pBip->koruma_start) && (pBip->pol > 19.0f)) {
                x = 20.0f;
                pBip->watt = (0.000007f * x * x * x) - (0.0039f * x * x) + (1.2532f * x) + 40.714f;
                pBip->koruma_start = 1U;
            }

            gBipolarCutSes = 1U;
            HAL_Delay(200U);

            if ((pBip->pol < 20.0f) || (1U == pBip->formul_ac)) {
                
                pBip->formul_ac = 0U;
                if (10.0f == pBip->pol) {
                    pBip->pol = 2.0f;
                }

                if (15.0f == pBip->pol) {
                    pBip->pol = 9.0f;
                }

                x = pBip->pol;
                pBip->watt = (0.000007f * x * x * x) - (0.0039f * x * x) + (1.2532f * x) + 40.714f;
                pBip->koruma_start = 0U;
            }

            if (2U == pBip->formul_ac) {
                pBip->formul_ac = 0U;
                x = (float)pBip->koruma_veri;
                pBip->watt = (0.000007f * x * x * x) - (0.0039f * x * x) + (1.2532f * x) + 40.714f;
                pBip->koruma_start = 0U;
            }

            genx_dacSet(pBip->watt);
            bipolar_setLeds(BIPOLAR_FUNCTION_CUT, true);
            pBip->state = BIPOLAR_STATE_ACTIVE;
            pBip->yeniden_hesap = false;
        } else {
            pBip->common.start = 0U;
        }
    }

    // Durdurma
    if ((BIPOLAR_STATE_ACTIVE == pBip->state) && (0U == pBip->common.start)) {
        genx_dacSet(0.0f);
        gBipolarCutSes = 2U;
        HAL_Delay(100U);
        genx_bipolarDisable();
        bipolar_setLeds(BIPOLAR_FUNCTION_CUT, false);
        pBip->olcum_sonuc = 0U;
        pBip->koruma_start = 0U;
        pBip->formul_ac = 0U;
        pBip->yeniden_hesap = false;
        pBip->state = BIPOLAR_STATE_IDLE;
        genx_release(&pBip->common);
        HAL_Delay(300U);
        (void)NEXTION_sendCmdRetry("tsw 255,1");
    }
}

/**
  * @brief Bipolar COAG çıkış gücü ölçümü, koruma, başlatma, durdurma, autostop
  * @param pH Kanal handle'ı
  * @retval None
  */
static void bipolar_runCoag(Bipolar_Handle_t *pH) {

    Bipolar_Runtime_t *pBip = &pH->canli;
    bool isTrigger = (1U == pBip->common.start) && (false == genx_isOtherChannelActive(&pBip->common));

    // Çıkış gücü ölçümü ve koruma
    if ((1U == pBip->common.start) && (UI_PAGE_MAIN == gAcikSayfa)) {
        pBip->okuma_gecikme++;
        if (500U <= pBip->okuma_gecikme) {

            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            pBip->okuma_gecikme = 0U;
            gLigasureWattYaz = gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];
            HAL_Delay(20U);
            if ((20.0f <= pBip->pol) && (1U == pBip->koruma_start)) {
                pBip->olcum_sonuc++;
                if (5U == pBip->olcum_sonuc) {
                    float f = (0.000003f * guc * guc) - (0.0273f * guc) + 75.577f;
                    uint32_t veri = (f > 0.0f) ? (uint32_t)f : 0U;

                    pBip->olcum_sonuc = 0U;
                    veri = veri * 2U;
                    if (20U == veri) {
                        veri = 22U;
                    }

                    pBip->koruma_veri = veri;
                    pBip->yeniden_hesap = true;
                    HAL_Delay(20U);
                    if (((float)veri > pBip->pol) || (veri >= 120U)) {
                        pBip->formul_ac = 1U;
                    }

                    if (((float)veri < pBip->pol) && (veri < 120U)) {
                        pBip->formul_ac = 2U;
                    }
                }
            }
        }
    }

    // Başlatma yada yeniden hesap
    if ((true == isTrigger) && ((BIPOLAR_STATE_IDLE == pBip->state) || (true == pBip->yeniden_hesap))) {
        if (0U == genx_claim(&pBip->common)) {
            float x;

            (void)NEXTION_sendCmdRetry("tsw 255,0");
            genx_bipolarEnable(BIPOLAR_MODE_COAG);

            if ((0U == pBip->koruma_start) && (20.0f <= pBip->pol)) {
                x = 20.0f;
                pBip->watt = (0.000004f * x * x * x) - (0.0032f * x * x) + (1.224f * x) + 34.541f;
                pBip->koruma_start = 1U;
            }

            gBipolarCoagSes = 1U;
            HAL_Delay(200U);

            if ((pBip->pol < 20.0f) || (1U == pBip->formul_ac)) {
                pBip->formul_ac = 0U;
                if (10.0f == pBip->pol) {
                    pBip->pol = 7.0f;
                }

                x = pBip->pol;
                pBip->watt = (0.000004f * x * x * x) - (0.0032f * x * x) + (1.224f * x) + 34.541f;
                pBip->koruma_start = 0U;
            }

            if (2U == pBip->formul_ac) {
                pBip->formul_ac = 0U;
                x = (float)pBip->koruma_veri;
                pBip->watt = (0.000004f * x * x * x) - (0.0032f * x * x) + (1.224f * x) + 34.541f;
                pBip->koruma_start = 0U;
            }

            genx_dacSet(pBip->watt);
            bipolar_setLeds(BIPOLAR_FUNCTION_COAG, true);
            pBip->state = BIPOLAR_STATE_ACTIVE;
            pBip->yeniden_hesap = false;
        } else {
            pBip->common.start = 0U;
        }
    }

    // Durdurma
    if ((BIPOLAR_STATE_ACTIVE == pBip->state) && (0U == pBip->common.start)) {
        genx_dacSet(0.0f);
        gBipolarCoagSes = 2U;
        HAL_Delay(100U);
        genx_bipolarDisable();
        bipolar_setLeds(BIPOLAR_FUNCTION_COAG, false);
        pBip->olcum_sonuc = 0U;
        pBip->koruma_start = 0U;
        pBip->formul_ac = 0U;
        pBip->yeniden_hesap = false;
        pBip->state = BIPOLAR_STATE_IDLE;
        genx_release(&pBip->common);
        HAL_Delay(300U);
        (void)NEXTION_sendCmdRetry("tsw 255,1");
    }

    // Autostop (Bipolar1: AUTOSTOP modu 500/400, seal: ligasure 550/450)
    {
        bool isAutoStopOn;
        uint32_t highLimit;
        uint32_t lowLimit;

        if (BIPOLAR_PROFILE_STANDARD == pH->pCfg->profile) {
            isAutoStopOn = (1U == gBipolarAutoStop);
            highLimit = 500U;
            lowLimit = 400U;
        } else {
            isAutoStopOn = (1U == gLigasureStart);
            highLimit = 550U;
            lowLimit = 450U;
        }

        if ((1U == pBip->common.start) && (true == isAutoStopOn) && (1U == gLigasurePedal) && (gLigasureWattYaz > highLimit)) {
            pBip->autostop_armed = 1U;
        }

        if ((1U == pBip->autostop_armed) && (gLigasureWattYaz < lowLimit)) {
            pBip->autostop_armed = 0U;
            gAutoStop2 = 1U;
            pBip->basma = 0U;
            pBip->birakma = 0U;
            pBip->common.start = 0U;
            gLigasurePedal = 0U;
        }
    }
}