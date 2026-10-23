/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    monopolar.c
 * @brief   Monopolar kanallar (Mono1 CUT/COAG, Mono2 CUT/COAG).
 *          Her mod kendi başlatma ve izleme fonksiyonunu içerir.
 *          Hangi modun hangi kanalda açık olduğu `channels_cfg.h`'de belirlenir.
 *
 * @author  destrocore
 * @date    2026
 */

#include "genx.h"

// Varsayılan modların kapatılması halinde
#if (0U == MONO1_CUT_EN_CUT) || (0U == MONO2_CUT_EN_CUT)
#error "MONO CUT: CUT kapatildi"
#endif

#if (0U == MONO1_COAG_EN_CONTACT) || (0U == MONO2_COAG_EN_CONTACT)
#error "MONO COAG: CONTACT kapatildi"
#endif

/* ============================================================
 * Kanal yapılandırmaları
 * ============================================================*/

#if (0 != MONO1_CUT_MASK)

static const Monopolar_Config_t gMono1CutCfg = {
    .function = MONOPOLAR_ISLEV_CUT,
    .relay_pin = MONO1_RELAY_Pin,
    .mux_ch = MUX_MONO_CUT,
    .led_beyaz = { MONO1_LED_BEYAZ_GPIO_Port, MONO1_LED_BEYAZ_Pin   },
    .led_aktif = { MONO1_LED_SARI_GPIO_Port, MONO1_LED_SARI_Pin     },
    .kalem_pin = { MONO1_KALEM_CUT_GPIO_Port, MONO1_KALEM_CUT_Pin   },
    .pedal_pin = { CIFT_PEDAL_CUT_GPIO_Port, CIFT_PEDAL_CUT_Pin     },
    .pSeciliPedal = &gMono1Pedal,
    .pHataBayrak = &gMono1Koruma,
    .etkin_maske = MONO1_CUT_MASK,
    .varsayilan_mod = MONOPOLAR_CUTMODE_CUT,
    .max_watt = 400U,
    .pWattGetCmd = "S2_m1_cut_watt.USER_VALUE.val",
    .pModeGetCmd = "S3_m1_cut_mod.USER_MODE.val",
    .pMainSetCmd = "S1_anasayfa.n0.val=S2_m1_cut_watt.USER_VALUE.val"
};

#endif /* (0 != MONO1_CUT_MASK) */


#if (0 != MONO1_COAG_MASK)

static const Monopolar_Config_t gMono1CoagCfg = {
    .function = MONOPOLAR_ISLEV_COAG,
    .relay_pin = MONO1_RELAY_Pin,
    .mux_ch = MUX_MONO_COAG,
    .led_beyaz = { MONO1_LED_BEYAZ_GPIO_Port, MONO1_LED_BEYAZ_Pin   },
    .led_aktif = { MONO1_LED_MAVI_GPIO_Port, MONO1_LED_MAVI_Pin     },
    .kalem_pin = { MONO1_KALEM_COAG_GPIO_Port, MONO1_KALEM_COAG_Pin },
    .pedal_pin = { CIFT_PEDAL_COAG_GPIO_Port, CIFT_PEDAL_COAG_Pin   },
    .pSeciliPedal = &gMono1Pedal,
    .pHataBayrak = &gMono1Koruma,
    .etkin_maske = MONO1_COAG_MASK,
    .varsayilan_mod = MONOPOLAR_COAGMODE_CONTACT,
    .max_watt = 120U,
    .pWattGetCmd = "S4_m1_cog_watt.USER_VALUE.val",
    .pModeGetCmd = "S5_m1_cog_mod.USER_MODE.val",
    .pMainSetCmd = "S1_anasayfa.n1.val=S4_m1_cog_watt.USER_VALUE.val"
};

#endif /* (0 != MONO1_CUT_MASK) */


#if (0 != MONO2_CUT_MASK)

static const Monopolar_Config_t gMono2CutCfg = {
    .function = MONOPOLAR_ISLEV_CUT,
    .relay_pin = MONO2_RELAY_Pin,
    .mux_ch = MUX_MONO_CUT,
    .led_beyaz = { MONO2_LED_BEYAZ_GPIO_Port, MONO2_LED_BEYAZ_Pin   },
    .led_aktif = { MONO2_LED_SARI_GPIO_Port, MONO2_LED_SARI_Pin     },
    .kalem_pin = { MONO2_KALEM_CUT_GPIO_Port, MONO2_KALEM_CUT_Pin   },
    .pedal_pin = { CIFT_PEDAL_CUT_GPIO_Port, CIFT_PEDAL_CUT_Pin     },
    .pSeciliPedal = &gMono2Pedal,
    .pHataBayrak = &gMono2Koruma,
    .etkin_maske = MONO2_CUT_MASK,
    .varsayilan_mod = MONOPOLAR_CUTMODE_CUT,
    .max_watt = 400U,
    .pWattGetCmd = "S6_m2_cut_watt.USER_VALUE.val",
    .pModeGetCmd = "S7_m2_cut_mod.USER_MODE.val",
    .pMainSetCmd = "S1_anasayfa.n2.val=S6_m2_cut_watt.USER_VALUE.val"
};

#endif /* (0 != MONO2_CUT_MASK) */


#if (0 != MONO2_COAG_MASK)

static const Monopolar_Config_t gMono2CoagCfg = {
    .function = MONOPOLAR_ISLEV_COAG,
    .relay_pin = MONO2_RELAY_Pin,
    .mux_ch = MUX_MONO_COAG,
    .led_beyaz = { MONO2_LED_BEYAZ_GPIO_Port, MONO2_LED_BEYAZ_Pin   },
    .led_aktif = { MONO2_LED_MAVI_GPIO_Port, MONO2_LED_MAVI_Pin     },
    .kalem_pin = { MONO2_KALEM_COAG_GPIO_Port, MONO2_KALEM_COAG_Pin },
    .pedal_pin = { CIFT_PEDAL_COAG_GPIO_Port, CIFT_PEDAL_COAG_Pin   },
    .pSeciliPedal = &gMono2Pedal,
    .pHataBayrak = &gMono2Koruma,
    .etkin_maske = MONO2_COAG_MASK,
    .varsayilan_mod = MONOPOLAR_COAGMODE_CONTACT,
    .max_watt = 120U,
    .pWattGetCmd = "S8_m2_cog_watt.USER_VALUE.val",
    .pModeGetCmd = "S9_m2_cog_mod.USER_MODE.val",
    .pMainSetCmd = "S1_anasayfa.n3.val=S8_m2_cog_watt.USER_VALUE.val"
};

#endif /* (0 != MONO2_COAG_MASK) */

/* ============================================================
 * TIM5 (endocut)
 * ============================================================*/
volatile uint8_t gEndoCutSay = 0U;
Monopolar_Handle_t * volatile gEndoMutex = NULL;
volatile float gEndoCutWattPol = 50.0f;
volatile float gEndoCoagWattPol = 20.0f;
volatile uint32_t gEndoCutTicks = 1U; // 1 tick = 10 ms
volatile uint16_t gEndoCoagTimer = 1U;

/* ============================================================
 * Handle'lar
 * ============================================================*/
#if (0 != MONO1_CUT_MASK)

Monopolar_Handle_t gMono1Cut = {
    .pCfg = &gMono1CutCfg,
    .runtime = {
        .user_watt = 10U,
        .user_mode = MONOPOLAR_CUTMODE_CUT,
        .pol = 10.0f
    }
};

#endif /* (0 != MONO1_CUT_MASK) */


#if (0 != MONO1_COAG_MASK)

Monopolar_Handle_t gMono1Coag = {
    .pCfg = &gMono1CoagCfg,
    .runtime = {
        .user_watt = 10U,
        .user_mode = MONOPOLAR_COAGMODE_CONTACT,
        .pol = 10.0f
    }
};

#endif /* (0 != MONO1_COAG_MASK) */


#if (0 != MONO2_CUT_MASK)

Monopolar_Handle_t gMono2Cut = {
    .pCfg = &gMono2CutCfg,
    .runtime = {
        .user_watt = 10U,
        .user_mode = MONOPOLAR_CUTMODE_CUT,
        .pol = 10.0f
    }
};

#endif /* (0 != MONO2_CUT_MASK) */


#if (0 != MONO2_COAG_MASK)

Monopolar_Handle_t gMono2Coag = {
    .pCfg = &gMono2CoagCfg,
    .runtime = {
        .user_watt = 10U,
        .user_mode = MONOPOLAR_COAGMODE_CONTACT,
        .pol = 10.0f
    }
};

#endif /* (0 != MONO1_COAG_MASK) */

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void monopolar_yonetCut(Monopolar_Handle_t *pH);
static void monopolar_cutCut(Monopolar_Handle_t *pH);
static bool monopolar_cikisBaslat(Monopolar_Handle_t *pH, uint16_t pwmCCR);
static void monopolar_aktiflestir(Monopolar_Handle_t *pH);
static void monopolar_ledAc(const Monopolar_Config_t *pCfg, bool aktifMi);
static void monopolar_stopCut(Monopolar_Handle_t *pH);

static void monopolar_yonetCoag(Monopolar_Handle_t *pH);
static void monopolar_coagContact(Monopolar_Handle_t *pH);
static void monopolar_stopCoag(Monopolar_Handle_t *pH);

#if (0 != MONO1_CUT_EN_BLEND1) || (0 != MONO2_CUT_EN_BLEND1)
    static void monopolar_cutBlend1(Monopolar_Handle_t *pH);
#endif

#if (0 != MONO1_CUT_EN_BLEND2) || (0 != MONO2_CUT_EN_BLEND2)
    static void monopolar_cutBlend2(Monopolar_Handle_t *pH);
#endif

#if (0 != MONO1_CUT_EN_BLEND3) || (0 != MONO2_CUT_EN_BLEND3)
    static void monopolar_cutBlend3(Monopolar_Handle_t *pH);
#endif

#if (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO1_CUT_EN_PAPILLOTOMES) || (0 != MONO2_CUT_EN_POLYPECTOMY) || (0 != MONO2_CUT_EN_PAPILLOTOMES)
    static void monopolar_cutEndo(Monopolar_Handle_t *pH);
#endif

#if (0 != MONO1_COAG_EN_SPRAY1) || (0 != MONO2_COAG_EN_SPRAY1)
    static void monopolar_coagSpray1(Monopolar_Handle_t *pH);
#endif

#if (0 != MONO1_COAG_EN_SPRAY2) || (0 != MONO2_COAG_EN_SPRAY2)
    static void monopolar_coagSpray2(Monopolar_Handle_t *pH);
#endif

#if (0 != MONO1_COAG_EN_SPRAY3) || (0 != MONO2_COAG_EN_SPRAY3) || (0 != MONO1_COAG_EN_FULGURATION) || (0 != MONO2_COAG_EN_FULGURATION)
    static void monopolar_coagSpray3(Monopolar_Handle_t *pH);
#endif

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief Monopolar kanalları kanal kaydına ekler
  * @retval None
  */
void monopolar_init(void) {

#if (0 != MONO1_CUT_MASK)
    genx_kanalUygula(&gMono1Cut.runtime.genel);
#endif

#if (0 != MONO1_COAG_MASK)
    genx_kanalUygula(&gMono1Coag.runtime.genel);
#endif

#if (0 != MONO2_CUT_MASK)
    genx_kanalUygula(&gMono2Cut.runtime.genel);
#endif

#if (0 != MONO2_COAG_MASK)
    genx_kanalUygula(&gMono2Coag.runtime.genel);
#endif

}

/**
  * @brief Dört monopolar kanalın tek giriş noktası
  * @retval None
  */
void monopolar_yonet(void) {

#if (0 != MONO1_CUT_MASK)
    monopolar_yonetCut(&gMono1Cut);
#endif

#if (0 != MONO1_COAG_MASK)
    monopolar_yonetCoag(&gMono1Coag);
#endif

#if (0 != MONO2_CUT_MASK)
    monopolar_yonetCut(&gMono2Cut);
#endif

#if (0 != MONO2_COAG_MASK)
    monopolar_yonetCoag(&gMono2Coag);
#endif

}

/**
  * @brief Modun bu kanalda açık olup olmadığını döndürür
  * @param pH Kanal handle'ı
  * @param modeId Nextion'da saklanan mod değeri
  * @retval true mod açık, false kapalı yada tanımsız
  */
bool monopolar_modEtkinMi(const Monopolar_Handle_t *pH, uint32_t modeId) {

    return (0U != (pH->pCfg->etkin_maske & (1U << modeId)));
}

/**
  * @brief Kalem/pedal sinyalini kanalın Start bayrağına dönüştürür
  * @param pH Kanal handle'ı
  * @retval None
  */
void monopolar_girisTara(Monopolar_Handle_t *pH) {

    const Monopolar_Config_t *pCfg = pH->pCfg;
    Monopolar_Runtime_t *pRt = &pH->runtime;
    bool kalem = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(pCfg->kalem_pin.pPort, pCfg->kalem_pin.pin));
    bool pedal = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(pCfg->pedal_pin.pPort, pCfg->pedal_pin.pin)) && (1U == *pCfg->pSeciliPedal);
    bool basildi = kalem || pedal;
    bool kilitli = (1U == pRt->hata) && ((1U == pRt->genel.start) || (1U == gAlarmPlate));

    if (GENX_SUCCESS == genx_sayacBittiMi((true != basildi) && (true == kilitli), &pRt->birakma)) {
        
        pRt->basma = 0U;
        pRt->hata = 0U;
        pRt->genel.start = 0U;

        if (1U == gAlarmPlate) {
            gAlarmPlate = 2U;
        }
    }

    if (GENX_SUCCESS == genx_sayacBittiMi((true == basildi) && (false == genx_baskaAktifVarMi(&pRt->genel)), &pRt->basma)) {
        
        pRt->birakma = 0U;
        pRt->hata = 1U;
        if (0U == gPlate100Overresistance) {
            
            pRt->genel.start = 1U;
        } else {
            
            pRt->genel.start = 0U;
            gAlarmPlate = 1U;
        }
    }
}

/**
  * @brief Kullanıcı watt ve mod değerinden kanalın işlenmiş watt değerini hesaplar
  * @param pH Kanal handle'ı
  * @retval None
  */
void monopolar_polHesap(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->runtime;
    float pol = (float)pRt->user_watt;
    float limit;
    float offset = 0.0f;

    if (MONOPOLAR_ISLEV_CUT == pH->pCfg->function) {
        
        limit = 385.0f;

        switch (pRt->user_mode) {
            
            case MONOPOLAR_CUTMODE_CUT: {
                
                offset = 5.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_BLEND1: {
                
                offset = 5.0f;
                limit = 240.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_BLEND2: {
                
                limit = 200.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_BLEND3: {
                
                limit = 150.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_POLYPECTOMY: {
                
                limit = 350.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_PAPILLOTOMES: {
                
                limit = 150.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_RESECTION1:
            case MONOPOLAR_CUTMODE_RESECTION2: {
                
                offset = 4.0f;
                limit = 350.0f;
                break;
            }

            default: {
                break;
            }
        }

        if (pol < 16.0f) {
            
            pol -= offset;
        }

    } else {

        limit = 120.0f;

        switch (pRt->user_mode) {

            case MONOPOLAR_COAGMODE_SPRAY1:
            case MONOPOLAR_COAGMODE_SPRAY2:
            case MONOPOLAR_COAGMODE_SPRAY3:
            case MONOPOLAR_COAGMODE_FULGURATION: {
                
                limit = 100.0f;
                break;
            }

            default: {

                break;
            }
        }
    }

    if (pol > limit) {
        
        pol = limit;
    }

    pRt->pol = pol;
}


#if (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO1_CUT_EN_PAPILLOTOMES)
/**
  * @brief Endo ayarlarını TIM5 kesmesinin kullandığı değerlere çevirir
  * @param pH Kanal handle'ı
  * @retval None
  */
void monopolar_endoHesap(const Monopolar_Handle_t *pH) {

    uint32_t mode = pH->runtime.user_mode;

    if ((gEndoCutKademe >= 1U) && (gEndoCutKademe <= 7U)) {
        
        if (MONOPOLAR_CUTMODE_POLYPECTOMY == mode) {
            
            gEndoCutWattPol = (float)(gEndoCutKademe * 50U);
        }

        if (MONOPOLAR_CUTMODE_PAPILLOTOMES == mode) {
            
            gEndoCutWattPol = (float)(gEndoCutKademe * 22U);
        }
    }

    if ((gEndoCoagKademe >= 1U) && (gEndoCoagKademe <= 5U)) {
        
        if (MONOPOLAR_CUTMODE_POLYPECTOMY == mode) {
            
            gEndoCoagWattPol = (float)(gEndoCoagKademe * 20U);
        }

        if (MONOPOLAR_CUTMODE_PAPILLOTOMES == mode) {
            
            gEndoCoagWattPol = (float)(gEndoCoagKademe * 10U);
        }
    }

    if (gEndoCutSure < 3U) {
        
        gEndoCutSure = 3U;
    }

    if (gEndoCutSure > 5U) {
        
        gEndoCutSure = 5U;
    }

    gEndoCutTicks = gEndoCutSure;

    if ((gEndoCoagSure >= 10U) && (gEndoCoagSure <= 150U)) {
        
        gEndoCoagTimer = (uint16_t)(gEndoCoagSure);
    }
}
#endif /* (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO1_CUT_EN_PAPILLOTOMES) */


/**
  * @brief Kayıtlı mod bu kanalda kapalıysa geri dönüş moduna çevirir
  * @param pH Kanal handle'ı
  * @retval true mod düzeltildi, false değişiklik yok
  */
bool monopolar_modKontrol(Monopolar_Handle_t *pH) {

    bool varsayilan = false;

    if (false == monopolar_modEtkinMi(pH, pH->runtime.user_mode)) {
        
        pH->runtime.user_mode = pH->pCfg->varsayilan_mod;
        varsayilan = true;
    }

    return varsayilan;
}


/* ================================================================
 * Yardımcı fonksiyonlar
 * ================================================================*/

#if (0 != MONO1_CUT_MASK) || (0 != MONO2_CUT_MASK)
/**
  * @brief Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_yonetCut(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRuntime = &pH->runtime;

    if (1U == pRuntime->genel.start) {

        bool pwrOkundu = false;

        if (true == monopolar_modEtkinMi(pH, pRuntime->user_mode)) {
            
            switch (pRuntime->user_mode) {

                case MONOPOLAR_CUTMODE_CUT:
                case MONOPOLAR_CUTMODE_RESECTION1:
                case MONOPOLAR_CUTMODE_RESECTION2: {
                    
                    monopolar_cutCut(pH);
                    pwrOkundu = true;
                    break;
                }

                case MONOPOLAR_CUTMODE_BLEND1: {
                    
                    #if (0 != MONO1_CUT_EN_BLEND1) || (0 != MONO2_CUT_EN_BLEND1)

                    monopolar_cutBlend1(pH);
                    pwrOkundu = true;

                    #endif

                    break;
                }

                case MONOPOLAR_CUTMODE_BLEND2: {
                    
                    #if (0 != MONO1_CUT_EN_BLEND2) || (0 != MONO2_CUT_EN_BLEND2)
                    
                    monopolar_cutBlend2(pH);
                    pwrOkundu = true;
                    
                    #endif

                    break;
                }

                case MONOPOLAR_CUTMODE_BLEND3: {

                    #if (0 != MONO1_CUT_EN_BLEND3) || (0 != MONO2_CUT_EN_BLEND3)
                    
                    monopolar_cutBlend3(pH);
                    pwrOkundu = true;
                    
                    #endif
                    
                    break;
                }

                case MONOPOLAR_CUTMODE_POLYPECTOMY:
                case MONOPOLAR_CUTMODE_PAPILLOTOMES: {

                    #if (0 != MONO1_CUT_EN_PAPILLOTOMES) || (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO2_CUT_EN_PAPILLOTOMES) || (0 != MONO2_CUT_EN_POLYPECTOMY)
                    
                    monopolar_cutEndo(pH);
                    
                    #endif
                    
                    break;
                }

                default: {
                    
                    break;
                }
            }
        }

        // Güç kaynağı kontrolü
        if ((true == pwrOkundu) && (true == gPwrYeni)) {

            if ((pRuntime->user_watt >= 10U) && (pRuntime->user_watt <= 400U) && (gHighVolt <= 2U)) {
                
                gPwrKoruma = 1U;
            }

            if ((((pRuntime->user_watt >= 10U) && (pRuntime->user_watt <= 45U)) || ((pRuntime->user_watt >= 50U) \
            && (pRuntime->user_watt <= 400U))) && (gHighVolt < 200U)) {
                
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONOPOLAR_DURUM_AKTIF == pRuntime->durum) {

        monopolar_stopCut(pH);
    }
}
#endif /* (0 != MONO1_CUT_MASK) || (0 != MONO2_CUT_MASK) */


#if (0 != MONO1_CUT_EN_CUT) || (0 != MONO2_CUT_EN_CUT)
/**
  * @brief CUT/RESECTION1/RESECTION2 başlatma ve çıkış gücü izleme
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_cutCut(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->runtime;

    if (MONOPOLAR_DURUM_BOSTA == pRt->durum) {
        
        if ((0U == gPlate100Overresistance) && (true == monopolar_cikisBaslat(pH, 0U))) {
            float x = pRt->pol;

            if (x >= 100.0f) {
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.000022f * x * x * x) - (0.0205f * x * x) + (10.25f * x) + 287.0f;
            x /= 10.0f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }

    } else {

        if (((HAL_GetTick() - pRt->guc_tick) >= 100U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_tick = HAL_GetTick();

            if ((1U == pRt->yuz_start) && (guc > 70U)) {
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = ((0.000021f * x * x * x) - (0.0205f * x * x) + (10.1f * x) + 287.0f);
                x /= 10.0f;

                genx_dacUygula(x);
            }

            float p = pRt->pol;
            uint16_t korumaVeri = (uint16_t)((0.0000278f * p * p * p) - (0.0383f * p * p) + (19.136f * p) + (683.43f));

            if ((p >= 50.0f) && (guc > (uint32_t)korumaVeri)) {
                
                *pH->pCfg->pHataBayrak = 1U;
            }
        }
    }
}
#endif /* (0 != MONO1_CUT_EN_CUT) || (0 != MONO2_CUT_EN_CUT) */


#if (0 != MONO1_CUT_MASK) || (0 != MONO1_COAG_MASK) || (0 != MONO2_CUT_MASK) || (0 != MONO2_COAG_MASK)
/**
  * @brief Çıkış sahipliğini alır ve kanalı donanımsal olarak açar
  * @param pH Kanal handle'ı
  * @param pwmCCR TIM2 CCR değeri. 0 ise TIM2 durdurulur
  * @retval true çıkış açıldı, false başka kanal sahip
  */
static bool monopolar_cikisBaslat(Monopolar_Handle_t *pH, uint16_t pwmCCR) {

    const Monopolar_Config_t *pCfg = pH->pCfg;
    bool basladiMi = false;

    if (GENX_ERROR != genx_sahiplendir(&pH->runtime.genel)) {
        
        (void)genx_sendCmd("tsw 255,0");

        if (0U == pwmCCR) {
            
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        }

        if ((MONOPOLAR_COAGMODE_SPRAY1 == pH->runtime.user_mode) || (MONOPOLAR_COAGMODE_SPRAY2 == pH->runtime.user_mode) 
        || (MONOPOLAR_COAGMODE_SPRAY3 == pH->runtime.user_mode) || (MONOPOLAR_COAGMODE_FULGURATION == pH->runtime.user_mode)) {
            
            HAL_GPIO_WritePin(SPRAY_RELAY_GPIO_Port, SPRAY_RELAY_Pin, GPIO_PIN_SET);
        }

        if (0U != pwmCCR) {
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwmCCR);
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        }

        genx_monoAc(pCfg->relay_pin, pCfg->mux_ch);

        if (MONOPOLAR_ISLEV_CUT == pCfg->function) {
            
            gMonopolarCutSes = 1U;
        } else {

            gMonopolarCoagSes = 1U;
        }

        genx_delay(200U);
        pH->runtime.guc_tick = HAL_GetTick();

        basladiMi = true;

    } else {
        
        pH->runtime.genel.start = 0U;
    }

    return basladiMi;
}
#endif /* (0 != MONO1_CUT_MASK) || (0 != MONO1_COAG_MASK) || (0 != MONO2_CUT_MASK) || (0 != MONO2_COAG_MASK) */

/**
  * @brief Kanalı aktif duruma geçirir ve LED'lerini yakar
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_aktiflestir(Monopolar_Handle_t *pH) {

    pH->runtime.durum = MONOPOLAR_DURUM_AKTIF;
    monopolar_ledAc(pH->pCfg, true);
}

/**
  * @brief Kanal LED'lerini ayarlar
  * @param pCfg Kanal yapısı
  * @param aktifMi true aktif, false boşta
  * @retval None
  */
static void monopolar_ledAc(const Monopolar_Config_t *pCfg, bool aktifMi) {

    HAL_GPIO_WritePin(pCfg->led_aktif.pPort, pCfg->led_aktif.pin, (true == aktifMi) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(pCfg->led_beyaz.pPort, pCfg->led_beyaz.pin, (true == aktifMi) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}


#if (0 != MONO1_CUT_MASK) || (0 != MONO2_CUT_MASK)
/**
  * @brief CUT çıkışını kapatır ve çıkış sahipliğini bırakır
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_stopCut(Monopolar_Handle_t *pH) {

    const Monopolar_Config_t *pCfg = pH->pCfg;

    genx_dacUygula(0.0f);
    gMonopolarCutSes = 2U;
    genx_delay(100U);
    gEndoCutSay = 11U;
    genx_muxSecimi(0U);
    HAL_TIM_Base_Stop_IT(&htim5);
    gEndoMutex = NULL;
    HAL_GPIO_WritePin(MONO_KORUMA_GPIO_Port, MONO_KORUMA_Pin, GPIO_PIN_RESET);

    if (MONO1_RELAY_Pin == pCfg->relay_pin) {
        HAL_GPIO_WritePin(MONO1_RELAY_GPIO_Port, pCfg->relay_pin | PWR_ON_R_Pin, GPIO_PIN_RESET);
    }

    else if (MONO2_RELAY_Pin == pCfg->relay_pin) {
        HAL_GPIO_WritePin(MONO2_RELAY_GPIO_Port, pCfg->relay_pin | PWR_ON_R_Pin, GPIO_PIN_RESET);
    }
   
    HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(FAN_R_GPIO_Port, FAN_R_Pin, GPIO_PIN_RESET);
    pH->runtime.durum = MONOPOLAR_DURUM_BOSTA;
    pH->runtime.yuz_start = 0U;

    monopolar_ledAc(pCfg, false);
    
    genx_serbestBirak(&pH->runtime.genel);
    genx_delay(300U);

    (void)genx_sendCmd("tsw 255,1");
}
#endif /* (0 != MONO1_CUT_MASK) || (0 != MONO2_CUT_MASK) */


#if (0 != MONO1_CUT_EN_BLEND1) || (0 != MONO2_CUT_EN_BLEND1)
/**
  * @brief BLEND1 başlatma ve çıkış gücü izleme
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_cutBlend1(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->runtime;

    if (MONOPOLAR_DURUM_BOSTA == pRt->durum) {

        if ((0U == gPlate100Overresistance) && (true == monopolar_cikisBaslat(pH, 212U))) {  // %80
            
            float x = pRt->pol;

            if (x >= 100.0f) {

                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.0000126f * x * x * x) - (0.0063f * x * x) + (1.6669f * x) + 31.0f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }
    } else {

        if (((HAL_GetTick() - pRt->guc_tick) >= 100U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_tick = HAL_GetTick();

            if ((1U == pRt->yuz_start) && (guc > 120U)) {
                
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = (0.0000126f * x * x * x) - (0.0063f * x * x) + (1.6669f * x) + 31.0f;

                genx_dacUygula(x);
            }

            float p = pRt->pol;
            uint16_t korumaVeri = (uint16_t)((0.000186f * p * p * p) - (0.0928f * p * p) + (23.777f * p) + (548.8f));

            if ((p >= 30.0f) && (guc > (uint32_t)korumaVeri)) {
                
                *pH->pCfg->pHataBayrak = 1U;
            }
        }
    }
}
#endif /* (0 != MONO1_CUT_EN_BLEND1) || (0 != MONO2_CUT_EN_BLEND1) */


#if (0 != MONO1_CUT_EN_BLEND2) || (0 != MONO2_CUT_EN_BLEND2)
/**
  * @brief BLEND2 başlatma ve çıkış gücü izleme
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_cutBlend2(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->runtime;

    if (MONOPOLAR_DURUM_BOSTA == pRt->durum) {

        if ((0U == gPlate100Overresistance) && (true == monopolar_cikisBaslat(pH, 160U))) {     // %60
            
            float x = pRt->pol;

            if (x >= 100.0f) {
                
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.0000173f * x * x * x) - (0.00755f * x * x) + (1.9052f * x) + 33.0f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }

    } else {

        if (((HAL_GetTick() - pRt->guc_tick) >= 100U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_tick = HAL_GetTick();

            if ((1U == pRt->yuz_start) && (guc > 130U)) {
                
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = (0.0000173f * x * x * x) - (0.00755f * x * x) + (1.9052f * x) + 33.0f;

                genx_dacUygula(x);
            }

            float p = pRt->pol;
            uint16_t korumaVeri = (uint16_t)((0.000115f * p * p * p) - (0.058f * p * p) + (18.493f * p) + (704.0f));

            if ((p >= 30.0f) && (guc > (uint32_t)korumaVeri)) {
                
                *pH->pCfg->pHataBayrak = 1U;
            }
        }
    }
}
#endif /* (0 != MONO1_CUT_EN_BLEND2) || (0 != MONO2_CUT_EN_BLEND2) */


#if (0 != MONO1_CUT_EN_BLEND3) || (0 != MONO2_CUT_EN_BLEND3)
/**
  * @brief BLEND3 başlatma ve çıkış gücü izleme
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_cutBlend3(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->runtime;

    if (MONOPOLAR_DURUM_BOSTA == pRt->durum) {
        
        if ((0U == gPlate100Overresistance) && (true == monopolar_cikisBaslat(pH, 127U))) {     // %50
            
            float x = pRt->pol;

            if (x >= 100.0f) {
                
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.000039f * x * x * x) - (0.0135f * x * x) + (2.5977f * x) + 33.355f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }
    } else {

        if (((HAL_GetTick() - pRt->guc_tick) >= 100U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_tick = HAL_GetTick();

            if ((1U == pRt->yuz_start) && (guc > 130U)) {
                
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = (0.000039f * x * x * x) - (0.0135f * x * x) + (2.5977f * x) + 33.355f;

                genx_dacUygula(x);
            }

            float p = pRt->pol;
            uint16_t korumaVeri = (uint16_t)((0.000465f * p * p * p) - (0.1818f * p * p) + (34.623f * p) + (319.0f));

            if ((p >= 30.0f) && (guc > (uint32_t)korumaVeri)) {
                *pH->pCfg->pHataBayrak = 1U;
            }
        }
    }
}
#endif /* (0 != MONO1_CUT_EN_BLEND3) || (0 != MONO2_CUT_EN_BLEND3) */


#if (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO1_CUT_EN_PAPILLOTOMES) || (0 != MONO2_CUT_EN_POLYPECTOMY) || (0 != MONO2_CUT_EN_PAPILLOTOMES)
/**
  * @brief Endocut başlatma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_cutEndo(Monopolar_Handle_t *pH) {
    
    if ((MONOPOLAR_DURUM_BOSTA == pH->runtime.durum) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_cikisBaslat(pH, 0U)) {
            genx_dacUygula(0.0f);
            gEndoCutSay = 0U;
            gEndoMutex = pH;
            HAL_TIM_Base_Start_IT(&htim5);
            monopolar_aktiflestir(pH);
        }
    }
}
#endif /* (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO1_CUT_EN_PAPILLOTOMES) || (0 != MONO2_CUT_EN_POLYPECTOMY) || (0 != MONO2_CUT_EN_PAPILLOTOMES) */


#if (0 != MONO1_COAG_MASK) || (0 != MONO2_COAG_MASK)
/**
  * @brief Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_yonetCoag(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->runtime;

    if (1U == pRt->genel.start) {

        if (true == monopolar_modEtkinMi(pH, pRt->user_mode)) {
            
            switch (pRt->user_mode) {

                case MONOPOLAR_COAGMODE_CONTACT: {
                    monopolar_coagContact(pH);
                    break;
                }

                case MONOPOLAR_COAGMODE_SPRAY1: {

                    #if (0 != MONO1_COAG_EN_SPRAY1) || (0 != MONO2_COAG_EN_SPRAY1)
                        monopolar_coagSpray1(pH);
                    #endif

                    break;
                }

                case MONOPOLAR_COAGMODE_SPRAY2: {

                    #if (0 != MONO1_COAG_EN_SPRAY2) || (0 != MONO2_COAG_EN_SPRAY2)
                    
                    monopolar_coagSpray2(pH);
                    
                    #endif
                    
                    break;
                }

                case MONOPOLAR_COAGMODE_SPRAY3:
                case MONOPOLAR_COAGMODE_FULGURATION: {
                    
                    #if (0 != MONO1_COAG_EN_SPRAY3) || (0 != MONO2_COAG_EN_SPRAY3) || (0 != MONO1_COAG_EN_FULGURATION) || (0 != MONO2_COAG_EN_FULGURATION)
                    
                    monopolar_coagSpray3(pH);
                    
                    #endif
                    
                    break;
                }

                default: {
                    break;
                }
            }
        }

        if ((true == gPwrYeni) && (pRt->user_watt >= 10U) && (pRt->user_watt <= 120U)) {

            if (gHighVolt <= 2U) {
                gPwrKoruma = 1U;
            }

            if (gHighVolt < 200U) {
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONOPOLAR_DURUM_AKTIF == pRt->durum) {

        monopolar_stopCoag(pH);
    }
}
#endif /* (0 != MONO1_COAG_MASK) || (0 != MONO2_COAG_MASK) */


#if (0 != MONO1_COAG_EN_CONTACT) || (0 != MONO2_COAG_EN_CONTACT)
/**
  * @brief COAG CONTACT başlatma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_coagContact(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_DURUM_BOSTA == pH->runtime.durum) && (0U == gPlate100Overresistance)) {

        if (true == monopolar_cikisBaslat(pH, 108U)) {
            
            float x = pH->runtime.pol;      // kullanıcı değeri bozulmaz

            x = (0.000087f * x * x * x) - (0.021f * x * x) + (3.0f * x) + 29.0f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }
    }
}
#endif /* (0 != MONO1_COAG_EN_CONTACT) || (0 != MONO2_COAG_EN_CONTACT) */


#if (0 != MONO1_COAG_EN_SPRAY1) || (0 != MONO2_COAG_EN_SPRAY1)
/**
  * @brief COAG SPRAY1 başlatma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_coagSpray1(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_DURUM_BOSTA == pH->runtime.durum) && (0U == gPlate100Overresistance)) {
        
        if (true == monopolar_cikisBaslat(pH, 20U)) {
            
            float x = pH->runtime.pol;      // kullanıcı değeri bozulmaz

            x = (0.000007f * x * x * x) - (0.0153f * x * x) + (3.5f * x) + 50.7f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }
    }
}
#endif /* (0 != MONO1_COAG_EN_SPRAY1) || (0 != MONO2_COAG_EN_SPRAY1) */


#if (0 != MONO1_COAG_EN_SPRAY2) || (0 != MONO2_COAG_EN_SPRAY2)
/**
  * @brief COAG SPRAY2 başlatma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_coagSpray2(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_DURUM_BOSTA == pH->runtime.durum) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_cikisBaslat(pH, 22U)) {
            
            float x = pH->runtime.pol;      // kullanıcı değeri bozulmaz

            x = (0.00007f * x * x * x) - (0.0234f * x * x) + (3.7182f * x) + 42.463f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }
    }
}
#endif /* (0 != MONO1_COAG_EN_SPRAY2) || (0 != MONO2_COAG_EN_SPRAY2) */


#if (0 != MONO1_COAG_EN_SPRAY3) || (0 != MONO2_COAG_EN_SPRAY3) || (0 != MONO1_COAG_EN_FULGURATION) || (0 != MONO2_COAG_EN_FULGURATION)
/**
  * @brief COAG SPRAY3 ve FULGURATION başlatma
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_coagSpray3(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_DURUM_BOSTA == pH->runtime.durum) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_cikisBaslat(pH, 24U)) {
            
            float x = pH->runtime.pol;      // kullanıcı değeri bozulmaz

            x = (0.00009f * x * x * x) - (0.0244f * x * x) + (3.6567f * x) + 40.692f;

            genx_dacUygula(x);
            monopolar_aktiflestir(pH);
        }
    }
}
#endif /* (0 != MONO1_COAG_EN_SPRAY3) || (0 != MONO2_COAG_EN_SPRAY3) || (0 != MONO1_COAG_EN_FULGURATION) || (0 != MONO2_COAG_EN_FULGURATION) */


#if (0 != MONO1_COAG_MASK) || (0 != MONO2_COAG_MASK)
/**
  * @brief COAG çıkışını kapatır ve çıkış sahipliğini bırakır
  * @param pH Kanal handle'ı
  * @retval None
  */
static void monopolar_stopCoag(Monopolar_Handle_t *pH) {

    const Monopolar_Config_t *pCfg = pH->pCfg;

    genx_dacUygula(0.0f);
    gMonopolarCoagSes = 2U;
    genx_delay(100U);
    genx_muxSecimi(0U);
    HAL_GPIO_WritePin(MONO_KORUMA_GPIO_Port, MONO_KORUMA_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(SPRAY_RELAY_GPIO_Port, SPRAY_RELAY_Pin, GPIO_PIN_RESET);

    if (MONO1_RELAY_Pin == pCfg->relay_pin) {
        HAL_GPIO_WritePin(MONO1_RELAY_GPIO_Port, pCfg->relay_pin | PWR_ON_R_Pin, GPIO_PIN_RESET);
    }

    else if (MONO2_RELAY_Pin == pCfg->relay_pin) {
        HAL_GPIO_WritePin(MONO2_RELAY_GPIO_Port, pCfg->relay_pin | PWR_ON_R_Pin, GPIO_PIN_RESET);
    }

    HAL_GPIO_WritePin(FAN_R_GPIO_Port, FAN_R_Pin, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    pH->runtime.durum = MONOPOLAR_DURUM_BOSTA;
    monopolar_ledAc(pCfg, false);
    genx_serbestBirak(&pH->runtime.genel);
    genx_delay(300U);
    (void)genx_sendCmd("tsw 255,1");
}
#endif /* (0 != MONO1_COAG_MASK) || (0 != MONO2_COAG_MASK) */