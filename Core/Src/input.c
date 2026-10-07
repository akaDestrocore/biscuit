/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    input.c
 * @brief   Hasta plakası direnç ölçümü, LED gösterimi ve plate hata sayfası
 *
 * @author  destrocore
 * @date    2026
 */

#include "core.h"
#include "modules.h"
#include "monopolar.h"
#include "bipolar.h"

/* ============================================================
 * Özel: modül sabitleri
 * ============================================================*/
static uint8_t gHandBirakma = 0U;
static uint8_t gHandBasma = 0U;
static uint8_t gTekPedalBirakma = 0U;
static uint8_t gTekPedalBasma = 0U;
static uint8_t gPedalError2Burst = 0U;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void input_clearAutoStop(void);
static void input_pollBipolar(void);

/* ============================================================
 * Genel API
 * ============================================================*/

 
/**
  * @brief Ham pedal/kalem/el sinyallerini Start bayraklarına dönüştürür
  * @retval None
  */
void input_run(void) {

    if (UI_PAGE_MAIN == gAcikSayfa) {

        bool basildi;
        bool izinVerildi;

        monopolar_pollInput(&gMono1Cut);
        monopolar_pollInput(&gMono1Coag);
        monopolar_pollInput(&gMono2Cut);
        monopolar_pollInput(&gMono2Coag);

        input_pollBipolar();

        // -- Pedal seçilmedi (Error-3) --------------------------
        basildi = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) || (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10));
        izinVerildi = (0U == gMono1Cut.canli.common.start) && (0U == gMono1Coag.canli.common.start) \
                    && (0U == gMono2Cut.canli.common.start) && (0U == gMono2Coag.canli.common.start) \
                    && (0U == gBipolar1Cut.canli.common.start) && (0U == gBipolar1Coag.canli.common.start) \
                    && (0U == gMono1Pedal) && (0U == gMono2Pedal) \
                    && (0U == gBipolar1PedalCift) && (0U == gBipolar2PedalCift) && (0U == gAlarmPlate) && (0U == gPedalError3);

        if (true == genx_isCounterDone(basildi && izinVerildi, &gPedalError2Burst)) {
            gPedalError3 = 1U;
        }
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

 /**
  * @brief Her iki bipolar COAG yolunun autostop kurulumunu ve ligasure watt değerini sıfırlar
  * @retval None
  */
static void input_clearAutoStop(void) {

    gBipolar1Coag.canli.autostop_armed = 0U;
    gBipolar2Coag.canli.autostop_armed = 0U;
    gLigasureWattYaz = 0U;
}

/**
  * @brief Bipolar pedal / tek pedal / el kumandası girişlerini Start bayraklarına dönüştürür
  * @retval None
  */
static void input_pollBipolar(void) {

    Bipolar_Runtime_t *pB1Cut = &gBipolar1Cut.canli;
    Bipolar_Runtime_t *pB1Coag = &gBipolar1Coag.canli;
    Bipolar_Runtime_t *pB2Cut = &gBipolar2Cut.canli;
    Bipolar_Runtime_t *pB2Coag = &gBipolar2Coag.canli;
    bool izinVerildi;
    bool modAktif;

    // -- Bipolar1 CUT (çift pedal) --------------------------
    izinVerildi = (false == genx_isOtherChannelActive(NULL));

    if (true == genx_isCounterDone(BIPOLAR1_CUT_AYAK_BASILI() && izinVerildi, &pB1Cut->basma)) {
        pB1Cut->birakma = 0U;
        pB1Cut->common.start = GUCU_ACIK;
    }

    if (true == genx_isCounterDone(BIPOLAR1_CUT_AYAK_SERBEST() && (GUCU_ACIK == pB1Cut->common.start), &pB1Cut->birakma)) {
        pB1Cut->basma = 0U;
        pB1Cut->common.start = GUCU_KAPALI;
    }

    // -- Bipolar1 COAG (çift pedal) -------------------------
    izinVerildi = (false == genx_isOtherChannelActive(NULL)) && (0U == gAutoStopCiftPedal) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
    modAktif = (0U == gAutoStopTekPedal) && (0U == gBipolarHand) && ((GUCU_ACIK == pB1Coag->common.start) || (1U == gAutoStopCiftPedal)) \
            && ((1U == gBipolar1PedalCift) || (0U == gBipolar2PedalCift));

    if (true == genx_isCounterDone(BIPOLAR1_COAG_AYAK_BASILI() && izinVerildi, &pB1Coag->basma)) {
        pB1Coag->birakma = 0U;
        pB1Coag->common.start = GUCU_ACIK;
        gAutoStopCiftPedal = 1U;
        if (1U == gBipolarAutoStop) {
            gLigasurePedal = 1U;
        }
    }

    if (true == genx_isCounterDone(BIPOLAR1_COAG_AYAK_SERBEST() && modAktif, &pB1Coag->birakma)) {
        pB1Coag->basma = 0U;
        pB1Coag->common.start = GUCU_KAPALI;
        gAutoStopCiftPedal = 0U;
        input_clearAutoStop();
        if (1U == gBipolarAutoStop) {
            gLigasurePedal = 0U;
        }
    }

    // -- Bipolar2 CUT (seal/ligasure) -----------------------
    izinVerildi = (false == genx_isOtherChannelActive(NULL));

    if (true == genx_isCounterDone(BIPOLAR2_CUT_LIGASURE_AYAK_BASILI() && izinVerildi, &pB2Cut->basma)) {
        pB2Cut->birakma = 0U;
        pB2Cut->common.start = GUCU_ACIK;
    }

    if (true == genx_isCounterDone(BIPOLAR2_CUT_LIGASURE_AYAK_SERBEST() && (GUCU_ACIK == pB2Cut->common.start), &pB2Cut->birakma)) {
        pB2Cut->basma = 0U;
        pB2Cut->common.start = GUCU_KAPALI;
    }

    // -- Bipolar2 COAG (seal/ligasure) ----------------------
    izinVerildi = (false == genx_isOtherChannelActive(NULL)) && (0U == gAutoStopCiftPedal) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
    modAktif = (0U == gAutoStopTekPedal) && (0U == gBipolarHand) && ((GUCU_ACIK == pB2Coag->common.start) || (1U == gAutoStopCiftPedal)) \
            && (1U == gBipolar2PedalCift);

    if (true == genx_isCounterDone(BIPOLAR2_COAG_LIGASURE_AYAK_BASILI() && izinVerildi, &pB2Coag->basma)) {
        pB2Coag->birakma = 0U;
        pB2Coag->common.start = GUCU_ACIK;
        gAutoStopCiftPedal = 1U;
        if (GUCU_ACIK == gLigasureStart) {
            gLigasurePedal = 1U;
        }
    }

    if (true == genx_isCounterDone(BIPOLAR2_COAG_LIGASURE_AYAK_SERBEST() && modAktif, &pB2Coag->birakma)) {
        pB2Coag->basma = 0U;
        pB2Coag->common.start = GUCU_KAPALI;
        gAutoStopCiftPedal = 0U;
        input_clearAutoStop();
        if (GUCU_ACIK == gLigasureStart) {
            gLigasurePedal = 0U;
        }
    }

    // -- Bipolar tek pedal (Bipolar1 COAG) ------------------
    izinVerildi = (false == genx_isOtherChannelActive(NULL)) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
    modAktif = (GUCU_ACIK == pB1Coag->common.start) && (1U == gAutoStopTekPedal) && (0U == gBipolarHand);

    if (true == genx_isCounterDone(BIPOLAR_TEK_PEDAL_BASILI() && izinVerildi, &gTekPedalBasma)) {
        gTekPedalBirakma = 0U;
        pB1Coag->common.start = GUCU_ACIK;
        gAutoStopTekPedal = 1U;
    }

    if (true == genx_isCounterDone(BIPOLAR_TEK_PEDAL_SERBEST() && modAktif, &gTekPedalBirakma)) {
        gTekPedalBasma = 0U;
        pB1Coag->common.start = GUCU_KAPALI;
        gAutoStopTekPedal = 0U;
        input_clearAutoStop();
    }

    // -- Bipolar el kumandası (hand) ------------------------
    izinVerildi = (0U == pB1Coag->common.start) && (0U == gAutoStop2) && (1U == gBipolarHand) && (0U == gBipolar1PedalCift) \
                && (0U == gBipolar2PedalCift) && (0U == pB2Cut->common.start) && (0U == pB2Coag->common.start) \
                && (false == genx_isOtherChannelActive(NULL));
    modAktif = ((GUCU_ACIK == pB2Coag->common.start) || (GUCU_ACIK == pB1Coag->common.start) || (1U == gAutoStop2)) && (1U == gBipolarHand);

    if (true == genx_isCounterDone(BIPOLAR_HAND_BASILI() && izinVerildi, &gHandBasma)) {
        gHandBirakma = 0U;

        if (GUCU_KAPALI == gLigasureStart) {
            pB1Coag->common.start = GUCU_ACIK;
        } else {
            pB2Coag->common.start = GUCU_ACIK;
            gLigasurePedal = 1U;
        }
    }

    if (true == genx_isCounterDone(BIPOLAR_HAND_SERBEST() && modAktif, &gHandBirakma)) {
        gHandBasma = 0U;
        input_clearAutoStop();

        if (GUCU_KAPALI == gLigasureStart) {
            pB1Coag->common.start = GUCU_KAPALI;
        } else {
            gAutoStop2 = 0U;
            pB2Coag->common.start = GUCU_KAPALI;
            gLigasurePedal = 0U;
        }
    }
}