/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    input.c
 * @brief   Start bayraklarının yönetimi
 *
 * @author  destrocore
 * @date    2026
 */

#include "genx.h"

/* ============================================================
 * Özel: modül sabitleri
 * ============================================================*/
static uint32_t gPedalError2Burst = 0U;
static uint32_t gHandBirakma = 0U;
static uint32_t gHandBasma = 0U;
static uint32_t gTekPedalBirakma = 0U;
static uint32_t gTekPedalBasma = 0U;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void input_autoStopTemizle(void);
static void input_bipolarTara(void);

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief Ham pedal/kalem/el sinyallerini Start bayraklarına dönüştürür
  * @retval None
  */
void input_yonet(void) {

    if (UI_PAGE_MAIN == gAcikSayfa) {

        bool basildi;
        bool izinVerildi;

        monopolar_girisTara(&gMono1Cut);
        monopolar_girisTara(&gMono1Coag);
        monopolar_girisTara(&gMono2Cut);
        monopolar_girisTara(&gMono2Coag);

        input_bipolarTara();

        // -- Pedal seçilmedi (Error-3) --------------------------
        basildi = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) || (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10));
        izinVerildi = (0U == gMono1Cut.runtime.genel.start) && (0U == gMono1Coag.runtime.genel.start) \
                    && (0U == gMono2Cut.runtime.genel.start) && (0U == gMono2Coag.runtime.genel.start) \
                    && (0U == gBipolar1Cut.runtime.genel.start) && (0U == gBipolar1Coag.runtime.genel.start) \
                    && (0U == gMono1Pedal) && (0U == gMono2Pedal) \
                    && (0U == gBipolar1PedalCift) && (0U == gBipolar2PedalCift) && (0U == gAlarmPlate) && (0U == gPedalError3);

        if (GENX_SUCCESS == genx_sayacBittiMi(basildi && izinVerildi, &gPedalError2Burst)) {
            gPedalError3 = 1U;
        }
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Her iki bipolar COAG kanalın autostop kurulumunu ve ligasure watt değerini sıfırlar
  * @retval None
  */
static void input_autoStopTemizle(void) {

    gBipolar1Coag.runtime.autostop_etkin = 0U;
#if (0 != BIPOLAR2_COAG_MASK)
    gBipolar2Coag.runtime.autostop_etkin = 0U;
#endif
    gLigasureWattYaz = 0U;
}

/**
  * @brief Bipolar pedal/tek pedal/hand girişlerini Start bayraklarına dönüştürür
  * @retval None
  */
static void input_bipolarTara(void) {
#if (0 != BIPOLAR1_CUT_MASK)
    Bipolar_Runtime_t *pB1Cut = &gBipolar1Cut.runtime;
#endif

#if (0 != BIPOLAR1_COAG_MASK)
    Bipolar_Runtime_t *pB1Coag = &gBipolar1Coag.runtime;
#endif

#if (0 != BIPOLAR2_CUT_MASK)
    Bipolar_Runtime_t *pB2Cut = &gBipolar2Cut.runtime;
#endif

#if (0 != BIPOLAR2_COAG_MASK)
    Bipolar_Runtime_t *pB2Coag = &gBipolar2Coag.runtime;
#endif
    bool izinVerildi;
    bool modAktif;

    // -- Bipolar1 CUT ---------------------------------------
    izinVerildi = (false == genx_baskaAktifVarMi(NULL));

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR1_CUT_AYAK_BASILI() && izinVerildi, &pB1Cut->basma)) {
        pB1Cut->birakma = 0U;
        pB1Cut->genel.start = 1U;
    }

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR1_CUT_AYAK_SERBEST() && (1U == pB1Cut->genel.start), &pB1Cut->birakma)) {
        pB1Cut->basma = 0U;
        pB1Cut->genel.start = 0U;
    }

    // -- Bipolar1 COAG --------------------------------------
    izinVerildi = (false == genx_baskaAktifVarMi(NULL)) && (0U == gAutoStopCiftPedal) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
    modAktif = (0U == gAutoStopTekPedal) && (0U == gBipolarHand) && ((1U == pB1Coag->genel.start) || (1U == gAutoStopCiftPedal)) \
            && ((1U == gBipolar1PedalCift) || (0U == gBipolar2PedalCift));

    const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(gBipolar1Coag.pCfg->function, gBipolar1Coag.runtime.user_mode);

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR1_COAG_AYAK_BASILI() && izinVerildi, &pB1Coag->basma)) {
        pB1Coag->birakma = 0U;
        pB1Coag->genel.start = 1U;
        gAutoStopCiftPedal = 1U;
        
        if ((NULL != pModBilgi) && (0U != pModBilgi->stop_high)) {
            gLigasurePedal = 1U;
        }
    }

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR1_COAG_AYAK_SERBEST() && modAktif, &pB1Coag->birakma)) {
        pB1Coag->basma = 0U;
        pB1Coag->genel.start = 0U;
        gAutoStopCiftPedal = 0U;
        input_autoStopTemizle();
        if ((NULL != pModBilgi) && (0U != pModBilgi->stop_high)) {
            gLigasurePedal = 0U;
        }
    }

    // -- Bipolar2 CUT ---------------------------------------
#if (0 != BIPOLAR2_CUT_MASK)
    izinVerildi = (false == genx_baskaAktifVarMi(NULL));

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR2_CUT_LIGASURE_AYAK_BASILI() && izinVerildi, &pB2Cut->basma)) {
        pB2Cut->birakma = 0U;
        pB2Cut->genel.start = 1U;
    }

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR2_CUT_LIGASURE_AYAK_SERBEST() && (1U == pB2Cut->genel.start), &pB2Cut->birakma)) {
        pB2Cut->basma = 0U;
        pB2Cut->genel.start = 0U;
    }
#endif /* (0 != BIPOLAR2_CUT_MASK) */

    // -- Bipolar2 COAG --------------------------------------
#if (0 != BIPOLAR2_COAG_MASK)
    izinVerildi = (false == genx_baskaAktifVarMi(NULL)) && (0U == gAutoStopCiftPedal) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
    modAktif = (0U == gAutoStopTekPedal) && (0U == gBipolarHand) && ((1U == pB2Coag->genel.start) || (1U == gAutoStopCiftPedal)) \
                && (1U == gBipolar2PedalCift);

    pModBilgi = bipolar_modBul(gBipolar2Coag.pCfg->function, gBipolar2Coag.runtime.user_mode);
                
    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR2_COAG_LIGASURE_AYAK_BASILI() && izinVerildi, &pB2Coag->basma)) {

        pB2Coag->birakma = 0U;
        pB2Coag->genel.start = 1U;
        gAutoStopCiftPedal = 1U;
        if ((NULL != pModBilgi) && (0U != pModBilgi->stop_high)) {

            gLigasurePedal = 1U;
        }
    }

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR2_COAG_LIGASURE_AYAK_SERBEST() && modAktif, &pB2Coag->birakma)) {
        pB2Coag->basma = 0U;
        pB2Coag->genel.start = 0U;
        gAutoStopCiftPedal = 0U;
        input_autoStopTemizle();
        if ((NULL != pModBilgi) && (0U != pModBilgi->stop_high)) {
            gLigasurePedal = 0U;
        }
    }
#endif /* (0 != BIPOLAR2_COAG_MASK) */
    // -- Bipolar tek pedal ----------------------------------
    izinVerildi = (false == genx_baskaAktifVarMi(NULL)) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
    modAktif = (1U == pB1Coag->genel.start) && (1U == gAutoStopTekPedal) && (0U == gBipolarHand);

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR_TEK_PEDAL_BASILI() && izinVerildi, &gTekPedalBasma)) {
        gTekPedalBirakma = 0U;
        pB1Coag->genel.start = 1U;
        gAutoStopTekPedal = 1U;
    }

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR_TEK_PEDAL_SERBEST() && modAktif, &gTekPedalBirakma)) {
        gTekPedalBasma = 0U;
        pB1Coag->genel.start = 0U;
        gAutoStopTekPedal = 0U;
        input_autoStopTemizle();
    }

    // -- Bipolar hand ---------------------------------------
    izinVerildi = (0U == pB1Coag->genel.start) && (0U == gAutoStop2) && (1U == gBipolarHand) && (0U == gBipolar1PedalCift) &&
    (0U == gBipolar2PedalCift)
#if (0 != BIPOLAR2_CUT_MASK)
    && (0U == pB2Cut->genel.start)
#endif
#if (0 != BIPOLAR2_COAG_MASK)
    && (0U == pB2Coag->genel.start)
#endif
    && (false == genx_baskaAktifVarMi(NULL));
    
    modAktif = ((1U == pB1Coag->genel.start) || (1U == gAutoStop2) 
#if (0 != BIPOLAR2_COAG_MASK)
    || (1U == pB2Coag->genel.start)
#endif
) && (1U == gBipolarHand);

    if (NULL != gBipolarHandSahibi) {
        pModBilgi = bipolar_modBul(gBipolarHandSahibi->pCfg->function, gBipolarHandSahibi->runtime.user_mode);
    } else {
        pModBilgi = NULL;
    }


    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR_HAND_BASILI() && izinVerildi, &gHandBasma)) {

        gHandBirakma = 0U;

        if (NULL != gBipolarHandSahibi) {
            
            gBipolarHandSahibi->runtime.genel.start = 1U;
            
            if ((NULL != pModBilgi) && (0U != pModBilgi->stop_high)) {
                gLigasurePedal = 1U;
            }
        }
    }

    if (GENX_SUCCESS == genx_sayacBittiMi(BIPOLAR_HAND_SERBEST() && modAktif, &gHandBirakma)) {

        gHandBasma = 0U;

        input_autoStopTemizle();

        if (NULL != gBipolarHandSahibi) {
            gBipolarHandSahibi->runtime.genel.start = 0U;

            if ((NULL != pModBilgi) && (0U != pModBilgi->stop_high)) {
                gAutoStop2 = 0U;
                gLigasurePedal = 0U;
            }
        }
    }

}