/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    bipolar.c
 * @brief   Bipolar tanımları. Moda özgü kısım touch.c içinde uygulanır.
 *
 * @author  destrocore
 * @date    2026
 */

#include "genx.h"

// Varsayılan modların kapatılması halinde
#if (0U == BIPOLAR1_CUT_EN_CUTTING) || (0U == BIPOLAR2_CUT_EN_CUTTING)
#error "BIPOLAR CUT: CUTTING modu kapatildi"
#endif

#if (0U == BIPOLAR1_COAG_EN_STANDARD)
#error "BIPOLAR1 COAG: STANDARD modu kapatildi"
#endif

#if (0U == BIPOLAR2_COAG_EN_LIGATION)
#error "BIPOLAR2 COAG: LIGATION modu kapatildi"
#endif

Bipolar_Handle_t *gBipolarHandSahibi = NULL;

/* ============================================================
 * Mod tablosu
 * ============================================================*/

static const Bipolar_ModeInfo_t gBipolarModeTable[] = {
    { BIPOLAR_ISLEV_CUT,    BIPOLAR_CUTMODE_CUTTING,            BIPOLAR_TRIGGER_PEDAL,  200U,   185U,   10U,    200U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_CUT,    BIPOLAR_CUTMODE_SCISSORS,           BIPOLAR_TRIGGER_PEDAL,  200U,   185U,   10U,    200U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_CUT,    BIPOLAR_CUTMODE_BIVAPO,             BIPOLAR_TRIGGER_PEDAL,  200U,   185U,   10U,    200U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_CUT,    BIPOLAR_CUTMODE_BIREZO,             BIPOLAR_TRIGGER_PEDAL,  200U,   185U,   10U,    200U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_STD_COAGMODE_STANDARD,      BIPOLAR_TRIGGER_PEDAL,  150U,   150U,   10U,    150U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_STD_COAGMODE_FORCED,        BIPOLAR_TRIGGER_PEDAL,  150U,   150U,   10U,    150U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_STD_COAGMODE_STOP,          BIPOLAR_TRIGGER_PEDAL,  150U,   120U,   10U,    150U,   500U,   400U, false },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_STD_COAGMODE_START,         BIPOLAR_TRIGGER_HAND,   150U,   120U,   10U,    150U,   0U,     0U,   false },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_SEAL_COAGMODE_LIGATION,     BIPOLAR_TRIGGER_HAND,   350U,   310U,   0U,     0U,     550U,   450U, true  },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_SEAL_COAGMODE_SEALSURE,     BIPOLAR_TRIGGER_HAND,   350U,   310U,   0U,     0U,     550U,   450U, true  },
    { BIPOLAR_ISLEV_COAG,   BIPOLAR_SEAL_COAGMODE_TISSUELOCK,   BIPOLAR_TRIGGER_PEDAL,  350U,   310U,   0U,     0U,     550U,   450U, true  }
};

#define BIPOLAR_MODE_COUNT ((uint32_t)(sizeof(gBipolarModeTable) / sizeof(gBipolarModeTable[0])))

/* ============================================================
 * Kanal yapılandırmaları
 * ============================================================*/
#if (0 != BIPOLAR1_CUT_MASK)

static const Bipolar_Config_t gBipolar1CutCfg = {
    .function           = BIPOLAR_ISLEV_CUT,
    .category           = BIPOLAR_CATEGORY_STANDARD,
    .etkin_maske        = BIPOLAR1_CUT_MASK,
    .varsayilan_mod     = BIPOLAR_CUTMODE_CUTTING,
    .pWattGetCmd        = "S10_b1_cut_wat.USER_VALUE.val",
    .pModeGetCmd        = "S12_b1_cut_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n6.val=S10_b1_cut_wat.USER_VALUE.val"
};

#endif /* (0 != BIPOLAR1_CUT_MASK) */


#if (0 != BIPOLAR1_COAG_MASK)

static const Bipolar_Config_t gBipolar1CoagCfg = {
    .function           = BIPOLAR_ISLEV_COAG,
    .category           = BIPOLAR_CATEGORY_STANDARD,
    .etkin_maske        = BIPOLAR1_COAG_MASK,
    .varsayilan_mod     = BIPOLAR_STD_COAGMODE_STANDARD,
    .pWattGetCmd        = "S11_b1_cog_wat.USER_VALUE.val",
    .pModeGetCmd        = "S13_b1_cog_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n7.val=S11_b1_cog_wat.USER_VALUE.val"
};

#endif /* (0 != BIPOLAR1_COAG_MASK) */


#if (0 != BIPOLAR2_CUT_MASK)

static const Bipolar_Config_t gBipolar2CutCfg = {
    .function           = BIPOLAR_ISLEV_CUT,
    .category           = BIPOLAR_CATEGORY_SEAL,
    .etkin_maske        = BIPOLAR2_CUT_MASK,
    .varsayilan_mod     = BIPOLAR_CUTMODE_CUTTING,
    .pWattGetCmd        = "S14_b2_cut_wat.USER_VALUE.val",
    .pModeGetCmd        = "S16_b2_cut_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n4.val=S14_b2_cut_wat.USER_VALUE.val"
};

#endif /* (0 != BIPOLAR2_CUT_MASK) */


#if (0 != BIPOLAR2_COAG_MASK)

static const Bipolar_Config_t gBipolar2CoagCfg = {
    .function           = BIPOLAR_ISLEV_COAG,
    .category           = BIPOLAR_CATEGORY_SEAL,
    .etkin_maske        = BIPOLAR2_COAG_MASK,
    .varsayilan_mod     = BIPOLAR_SEAL_COAGMODE_LIGATION,
    .pWattGetCmd        = "S15_b2_cog_wat.USER_VALUE.val",
    .pModeGetCmd        = "S17_b2_cog_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n5.val=S15_b2_cog_wat.USER_VALUE.val"
};

#endif /* (0 != BIPOLAR2_COAG_MASK) */

/* ============================================================
 * Handle'lar
 * ============================================================*/
#if (0 != BIPOLAR1_CUT_MASK)

Bipolar_Handle_t gBipolar1Cut = {
    .pCfg = &gBipolar1CutCfg,
    .runtime = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_CUTMODE_CUTTING, 
        .pol = 10.0f 
    }
};

#endif /* (0 != BIPOLAR1_CUT_MASK) */


#if (0 != BIPOLAR1_COAG_MASK)

Bipolar_Handle_t gBipolar1Coag = {
    .pCfg = &gBipolar1CoagCfg,
    .runtime = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_STD_COAGMODE_STANDARD, 
        .pol = 10.0f 
    }
};

#endif /* (0 != BIPOLAR1_COAG_MASK) */


#if (0 != BIPOLAR2_CUT_MASK)

Bipolar_Handle_t gBipolar2Cut = {
    .pCfg = &gBipolar2CutCfg,
    .runtime = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_CUTMODE_CUTTING, 
        .pol = 10.0f 
    }
};

#endif /* (0 != BIPOLAR2_CUT_MASK) */


#if (0 != BIPOLAR2_COAG_MASK)

Bipolar_Handle_t gBipolar2Coag = {
    .pCfg = &gBipolar2CoagCfg,
    .runtime = { 
        .watt = 10.0f, 
        .user_watt = 10U, 
        .user_mode = BIPOLAR_SEAL_COAGMODE_TISSUELOCK, 
        .pol = 10.0f 
    }
};

#endif /* (0 != BIPOLAR2_COAG_MASK) */


/* ================================================================
 * Özel fonksiyon prototipleri
 * ================================================================*/
static void bipolar_ledAc(Bipolar_Function_e func, bool aktifMi);
static void bipolar_yonetCut(Bipolar_Handle_t *pH);
static void bipolar_yonetCoag(Bipolar_Handle_t *pH);
static void bipolar_pwrKontrol(const Bipolar_Handle_t *pH);

/* ================================================================
 * Genel API
 * ================================================================*/

/**
  * @brief Bütün bipolar kanallarını kanal kaydına ekler
  * @retval None
  */
void bipolar_init(void) {

#if (0 != BIPOLAR1_CUT_MASK)
    genx_kanalUygula(&gBipolar1Cut.runtime.genel);
#endif

#if (0 != BIPOLAR1_COAG_MASK)
    genx_kanalUygula(&gBipolar1Coag.runtime.genel);
#endif

#if (0 != BIPOLAR2_CUT_MASK)
    genx_kanalUygula(&gBipolar2Cut.runtime.genel);
#endif

#if (0 != BIPOLAR2_COAG_MASK)
    genx_kanalUygula(&gBipolar2Coag.runtime.genel);
#endif

}

/**
  * @brief Bipolar yollarının tek giriş noktası
  * @retval None
  */
void bipolar_yonet(void) {

#if (0 != BIPOLAR1_CUT_MASK)
    bipolar_yonetCut(&gBipolar1Cut);
#endif

#if (0 != BIPOLAR1_COAG_MASK)
    bipolar_yonetCoag(&gBipolar1Coag);
#endif

#if (0 != BIPOLAR2_CUT_MASK)
    bipolar_yonetCut(&gBipolar2Cut);
#endif

#if (0 != BIPOLAR2_COAG_MASK)
    bipolar_yonetCoag(&gBipolar2Coag);
#endif

}

/**
  * @brief Modun bu kanalda açık olup olmadığını döndürür
  * @param pH Kanal handle'ı
  * @param modeId Nextion'da saklanan mod değeri
  * @retval true mod açık, false kapalı yada tanımsız
  */
bool bipolar_modEtkinMi(const Bipolar_Handle_t *pH, uint32_t modeId) {

    return (0U != (pH->pCfg->etkin_maske & (1U << modeId)));
}

/**
  * @brief Kullanıcı watt ve mod değerinden kanalın işlenmiş watt değerini hesaplar
  * @param pH Kanal handle'ı
  * @retval None
  */
void bipolar_polHesap(Bipolar_Handle_t *pH) {

    Bipolar_Runtime_t *pBip = &pH->runtime;
    const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(pH->pCfg->function, pH->runtime.user_mode);
    float pol = (float)pBip->user_watt;
    float limit = (BIPOLAR_ISLEV_CUT == pH->pCfg->function) ? 185.0f : 130.0f;

    if (NULL != pModBilgi) {
        limit = (float)pModBilgi->pol_limit;
    }

    if (pol > limit) {
        pol = limit;
    }

    pBip->pol = pol;
}

/**
  * @brief Kayıtlı mod bu kanalda kapalıysa geri dönüş moduna çevirir
  * @param pH Kanal handle'ı
  * @retval true mod düzeltildi, false değişiklik yok
  */
bool bipolar_modKontrol(Bipolar_Handle_t *pH) {

    bool varsayilan = false;

    if (false == bipolar_modEtkinMi(pH, pH->runtime.user_mode)) {
        pH->runtime.user_mode = pH->pCfg->varsayilan_mod;
        varsayilan = true;
    }

    return varsayilan;
}

/* ================================================================
 * Yardımcı fonksiyonlar
 * ================================================================*/

/**
  * @brief Bipolar LED'lerini ayarlar
  * @param func CUT yada COAG
  * @param aktifMi true aktif, false boşta
  * @retval None
  */
static void bipolar_ledAc(Bipolar_Function_e func, bool aktifMi) {

    uint16_t renkPin = (BIPOLAR_ISLEV_CUT == func) ? BIPOLAR_LED_SARI_Pin : BIPOLAR_LED_MAVI_Pin;

    HAL_GPIO_WritePin(GPIOE, renkPin, (true == aktifMi) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIPOLAR_LED_BEYAZ_GPIO_Port, BIPOLAR_LED_BEYAZ_Pin, (true == aktifMi) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

#if (0 != BIPOLAR1_CUT_MASK) || (0 != BIPOLAR2_CUT_MASK)
/**
  * @brief Bipolar CUT çıkış gücü ölçümü, koruma, başlatma ve durdurma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void bipolar_yonetCut(Bipolar_Handle_t *pH) {

    Bipolar_Runtime_t *pBip = &pH->runtime;
    bool trigger = (1U == pBip->genel.start) && (false == genx_baskaAktifVarMi(&pBip->genel));

    // Çıkış gücü ölçümü ve koruma
    if ((1U == pBip->genel.start) && (UI_PAGE_MAIN == gAcikSayfa)) {
        
        pBip->okuma_gecikme++;
        
        if (500U <= pBip->okuma_gecikme) {
            
            float guc = (float)gAdc1DmaBuff[ADC_CH_BIPOLAR_PWR];

            pBip->okuma_gecikme = 0U;
            HAL_Delay(20U);

            if ((19.0f < pBip->pol) && (1U == pBip->koruma_start)) {
                
                pBip->olcum_sonuc++;
                
                if (5U == pBip->olcum_sonuc) {
                    
                    float f = (-(0.000000003f * guc * guc * guc) + (0.00003f * guc * guc) - (0.1022f * guc) + (136.17f));
                    uint32_t veri = (0.0f < f) ? f : 0U;

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

    // Besleme gerilimi koruması
    if ((1U == pBip->genel.start) && (BIPOLAR_STATE_ACTIVE == pBip->durum)) {
        bipolar_pwrKontrol(pH);
    }

    // Başlatma yada yeniden hesap
    if ((true == trigger) && ((BIPOLAR_STATE_IDLE == pBip->durum) || (true == pBip->yeniden_hesap))) {
        
        if (0U == genx_sahiplendir(&pBip->genel)) {
            float x;

            (void)genx_sendCmdRetry("tsw 255,0");
            genx_bipolarAc(BIPOLAR_MODE_CUT);

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

            genx_dacUygula(pBip->watt);
            bipolar_ledAc(BIPOLAR_ISLEV_CUT, true);
            pBip->durum = BIPOLAR_STATE_ACTIVE;
            pBip->yeniden_hesap = false;
        } else {
            pBip->genel.start = 0U;
        }
    }

    // Durdurma
    if ((BIPOLAR_STATE_ACTIVE == pBip->durum) && (0U == pBip->genel.start)) {
        genx_dacUygula(0.0f);
        gBipolarCutSes = 2U;
        HAL_Delay(100U);
        genx_bipolarKapat();
        bipolar_ledAc(BIPOLAR_ISLEV_CUT, false);
        pBip->olcum_sonuc = 0U;
        pBip->koruma_start = 0U;
        pBip->formul_ac = 0U;
        pBip->yeniden_hesap = false;
        pBip->durum = BIPOLAR_STATE_IDLE;
        genx_serbestBirak(&pBip->genel);
        HAL_Delay(300U);
        (void)genx_sendCmdRetry("tsw 255,1");
    }
}
#endif /* (0 != BIPOLAR1_CUT_MASK) || (0 != BIPOLAR2_CUT_MASK) */


#if (0 != BIPOLAR1_COAG_MASK) || (0 != BIPOLAR2_COAG_MASK)
/**
  * @brief Bipolar COAG çıkış gücü ölçümü, koruma, başlatma, durdurma, autostop
  * @param pH Kanal handle'ı
  * @retval None
  */
static void bipolar_yonetCoag(Bipolar_Handle_t *pH) {

    Bipolar_Runtime_t *pBip = &pH->runtime;
    bool trigger = (1U == pBip->genel.start) && (false == genx_baskaAktifVarMi(&pBip->genel));

    // Çıkış gücü ölçümü ve koruma
    if ((1U == pBip->genel.start) && (UI_PAGE_MAIN == gAcikSayfa)) {
        
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

    // Besleme gerilimi koruması
    if ((1U == pBip->genel.start) && (BIPOLAR_STATE_ACTIVE == pBip->durum)) {
        bipolar_pwrKontrol(pH);
    }

    // Başlatma yada yeniden hesap
    if ((true == trigger) && ((BIPOLAR_STATE_IDLE == pBip->durum) || (true == pBip->yeniden_hesap))) {
        
        if (0U == genx_sahiplendir(&pBip->genel)) {
            
            float x;

            (void)genx_sendCmdRetry("tsw 255,0");
            genx_bipolarAc(BIPOLAR_MODE_COAG);

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

            genx_dacUygula(pBip->watt);
            bipolar_ledAc(BIPOLAR_ISLEV_COAG, true);
            pBip->durum = BIPOLAR_STATE_ACTIVE;
            pBip->yeniden_hesap = false;
        } else {

            pBip->genel.start = 0U;
        }
    }

    // Durdurma
    if ((BIPOLAR_STATE_ACTIVE == pBip->durum) && (0U == pBip->genel.start)) {
        
        genx_dacUygula(0.0f);
        gBipolarCoagSes = 2U;
        HAL_Delay(100U);
        genx_bipolarKapat();
        bipolar_ledAc(BIPOLAR_ISLEV_COAG, false);
        pBip->olcum_sonuc = 0U;
        pBip->koruma_start = 0U;
        pBip->formul_ac = 0U;
        pBip->yeniden_hesap = false;
        pBip->durum = BIPOLAR_STATE_IDLE;
        genx_serbestBirak(&pBip->genel);
        HAL_Delay(100U);
        (void)genx_sendCmdRetry("tsw 255,1");
    }

    // Autostop
    {
        const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(pH->pCfg->function, pH->runtime.user_mode);
        uint32_t highLimit = 0U;
        uint32_t lowLimit = 0U;

        if (NULL != pModBilgi) {
            highLimit = pModBilgi->stop_high;
            lowLimit = pModBilgi->stop_low;
        }

        if ((0U != highLimit) && (1U == pBip->genel.start) && (1U == gLigasurePedal) && (gLigasureWattYaz > highLimit)) {

            pBip->autostop_etkin = 1U;
        }

        if ((1U == pBip->autostop_etkin) && (gLigasureWattYaz < lowLimit)) {

            pBip->autostop_etkin = 0U;
            gAutoStop2 = 1U;
            pBip->basma = 0U;
            pBip->birakma = 0U;
            pBip->genel.start = 0U;
            gLigasurePedal = 0U;
        }
    }
}
#endif /* (0 != BIPOLAR1_COAG_MASK) || (0 != BIPOLAR2_COAG_MASK) */

/**
  * @brief Mod tablosunda (function, modeId) kombinasyonunu arar
  * @param func CUT veya COAG
  * @param modeId Mod değeri
  * @retval Satır işaretçisi, bulunamazsa NULL
  */
const Bipolar_ModeInfo_t *bipolar_modBul(Bipolar_Function_e func, uint32_t modeId) {

    const Bipolar_ModeInfo_t *pMod = NULL;

    for (uint32_t i = 0U; (i < BIPOLAR_MODE_COUNT) && (NULL == pMod); i++) {
        if ((func == gBipolarModeTable[i].function) && (modeId == gBipolarModeTable[i].modeId)) {
            pMod = &gBipolarModeTable[i];
        }
    }

    return pMod;
}

/**
  * @brief Mod tablosundaki aralıkta besleme gerilimi korumasını uygular
  * @param pH Kanal handle'ı
  * @retval None
  */
static void bipolar_pwrKontrol(const Bipolar_Handle_t *pH) {

    const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(pH->pCfg->function, pH->runtime.user_mode);
    uint32_t watt = pH->runtime.user_watt;

    if ((true == gPwrYeni) && (NULL != pModBilgi) && (0U != pModBilgi->hv_max_watt)) {

        if ((watt >= pModBilgi->hv_min_watt) && (watt <= pModBilgi->hv_max_watt)) {

            if (gHighVolt <= 2U) {
                gPwrKoruma = 1U;
            }

            if (gHighVolt < 200U) {
                gBiPwrKoruma = 1U;
            }
        }
    }
}