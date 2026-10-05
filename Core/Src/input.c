/**
  ******************************************************************************
  * @file    input.c
  * @brief   Kalem/pedal/el girişlerini ark sayacıyla (debounce) Start bayraklarına çevirir
  ******************************************************************************
  */
#include "core.h"
#include "modules.h"

// Bırakma ve bırakma ark sayaçları ve kanal hata bayrakları (yalnız bu modülde kullanılır)
static uint8_t gMono1CutBirakma = 0U;
static uint8_t gMono1CutBasma = 0U;
static uint8_t gMono1CutError = 0U;
static uint8_t gMono1CoagBirakma = 0U;
static uint8_t gMono1CoagBasma = 0U;
static uint8_t gMono1CoagError = 0U;
static uint8_t gMono2CutBirakma = 0U;
static uint8_t gMono2CutBasma = 0U;
static uint8_t gMono2CutError = 0U;
static uint8_t gMono2CoagBirakma = 0U;
static uint8_t gMono2CoagBasma = 0U;
static uint8_t gMono2CoagError = 0U;
static uint8_t gBipolar1CutBirakma = 0U;
static uint8_t gBipolar1CutBasma = 0U;
static uint8_t gBipolar1CoagBirakma = 0U;
static uint8_t gBipolar2CutBirakma = 0U;
static uint8_t gBipolar2CutBasma = 0U;
static uint8_t gBipolar2CoagBirakma = 0U;
static uint8_t gTekPedalBirakma = 0U;
static uint8_t gTekPedalBasma = 0U;
static uint8_t gHandBirakma = 0U;
static uint8_t gHandBasma = 0U;
static uint8_t gPedalError2Burst = 0U;

/**
  * @brief Koşul sağlandığı sürece sayar, eşiğe (200) ulaşınca sayacı sıfırlar
  * @param kosul Sayılacak koşul
  * @param pSayac Sayaç değişkenine işaretçi
  * @retval true eşik doldu, false henüz dolmadı
  */
static bool esuInput_isArkBitti(bool kosul, uint8_t *pSayac) {
    bool bitti = false;

    if (true == kosul) {
        (*pSayac)++;
        if (200U <= *pSayac) {
            *pSayac = 0U;
            bitti = true;
        }
    }

    return bitti;
}

/**
  * @brief Ham pedal/kalem/el sinyallerini Start bayraklarına dönüştürür
  * @retval None
  */
void input_run(void) {
    if (UI_PAGE_MAIN == gAcikSayfa) {
        bool basildi;
        bool modAktif;
        bool izinVerildi;

        // -- Mono1 CUT ------------------------------------------
        basildi = MONO1_CUT_KALEM_BASILI() || MONO1_CUT_AYAK_BASILI();
        modAktif = (1U == gMono1CutError) && ((1U == gMono1CutStart) || (1U == gAlarmPlate));

        if (true == esuInput_isArkBitti((false == basildi) && modAktif, &gMono1CutBirakma)) {
            gMono1CutBasma = 0U;
            gMono1CutError = 0U;
            gMono1CutStart = GUCU_KAPALI;
            if (1U == gAlarmPlate) { gAlarmPlate = 2U; }
        }

        if (true == esuInput_isArkBitti(basildi && (false == genx_isOtherChannelActive(gMono1CutStart)), &gMono1CutBasma)) {
            gMono1CutBirakma = 0U;
            gMono1CutError = 1U;
            if (0U == gPlate100Overresistance) {
                gMono1CutStart = GUCU_ACIK;
            } else {
                gMono1CutStart = GUCU_KAPALI;
                gAlarmPlate = 1U;
            }
        }

        // -- Mono1 COAG -----------------------------------------
        basildi = MONO1_COAG_KALEM_BASILI() || MONO1_COAG_AYAK_BASILI();
        modAktif = (1U == gMono1CoagError) && ((1U == gMono1CoagStart) || (1U == gAlarmPlate));

        if (true == esuInput_isArkBitti((false == basildi) && modAktif, &gMono1CoagBirakma)) {
            gMono1CoagBasma = 0U;
            gMono1CoagError = 0U;
            gMono1CoagStart = GUCU_KAPALI;
            if (1U == gAlarmPlate) { gAlarmPlate = 2U; }
        }

        if (true == esuInput_isArkBitti(basildi && (false == genx_isOtherChannelActive(gMono1CoagStart)), &gMono1CoagBasma)) {
            gMono1CoagBirakma = 0U;
            gMono1CoagError = 1U;
            if (0U == gPlate100Overresistance) {
                gMono1CoagStart = GUCU_ACIK;
            } else {
                gMono1CoagStart = GUCU_KAPALI;
                gAlarmPlate = 1U;
            }
        }

        // -- Mono2 CUT ------------------------------------------
        basildi = MONO2_CUT_KALEM_BASILI() || MONO2_CUT_AYAK_BASILI();
        modAktif = (1U == gMono2CutError) && ((1U == gMono2CutStart) || (1U == gAlarmPlate));

        if (true == esuInput_isArkBitti(basildi && (false == genx_isOtherChannelActive(gMono2CutStart)), &gMono2CutBasma)) {
            gMono2CutBirakma = 0U;
            gMono2CutError = 1U;
            if (0U == gPlate100Overresistance) {
                gMono2CutStart = GUCU_ACIK;
            } else {
                gMono2CutStart = GUCU_KAPALI;
                gAlarmPlate = 1U;
            }
        }

        if (true == esuInput_isArkBitti((false == basildi) && modAktif, &gMono2CutBirakma)) {
            gMono2CutBasma = 0U;
            gMono2CutError = 0U;
            gMono2CutStart = GUCU_KAPALI;
            if (1U == gAlarmPlate) { gAlarmPlate = 2U; }
        }

        // -- Mono2 COAG -----------------------------------------
        basildi = MONO2_COAG_KALEM_BASILI() || MONO2_COAG_AYAK_BASILI();
        modAktif = (1U == gMono2CoagError) && ((1U == gMono2CoagStart) || (1U == gAlarmPlate));

        if (true == esuInput_isArkBitti(basildi && (false == genx_isOtherChannelActive(gMono2CoagStart)), &gMono2CoagBasma)) {
            gMono2CoagBirakma = 0U;
            gMono2CoagError = 1U;
            if (0U == gPlate100Overresistance) {
                gMono2CoagStart = GUCU_ACIK;
            } else {
                gMono2CoagStart = GUCU_KAPALI;
                gAlarmPlate = 1U;
            }
        }

        if (true == esuInput_isArkBitti((false == basildi) && modAktif, &gMono2CoagBirakma)) {
            gMono2CoagBasma = 0U;
            gMono2CoagError = 0U;
            gMono2CoagStart = GUCU_KAPALI;
            if (1U == gAlarmPlate) { gAlarmPlate = 2U; }
        }

        // -- Bipolar1 CUT (çift pedal) --------------------------
        izinVerildi = (false == genx_isOtherChannelActive(0U));

        if (true == esuInput_isArkBitti(BIPOLAR1_CUT_AYAK_BASILI() && izinVerildi, &gBipolar1CutBasma)) {
            gBipolar1CutBirakma = 0U;
            gBipolar1CutStart = GUCU_ACIK;
        }

        if (true == esuInput_isArkBitti(BIPOLAR1_CUT_AYAK_SERBEST() && (1U == gBipolar1CutStart), &gBipolar1CutBirakma)) {
            gBipolar1CutBasma = 0U;
            gBipolar1CutStart = GUCU_KAPALI;
        }

        // -- Bipolar1 COAG (çift pedal) -------------------------
        izinVerildi = (false == genx_isOtherChannelActive(0U)) && (0U == gAutoStopCiftPedal) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
        modAktif = (0U == gAutoStopTekPedal) && (0U == gBipolarHand) && ((1U == gBipolar1CoagStart) || (1U == gAutoStopCiftPedal)) \
                && ((1U == gBipolar1PedalCift) || (0U == gBipolar2PedalCift));

        if (true == esuInput_isArkBitti(BIPOLAR1_COAG_AYAK_BASILI() && izinVerildi, &gBipolar1CoagBurst2)) {
            gBipolar1CoagBurst1 = 0U;
            gBipolar1CoagStart = GUCU_ACIK;
            gAutoStopCiftPedal = 1U;
            if (1U == gBipolarAutoStop) { gLigasurePedal = 1U; }
        }

        if (true == esuInput_isArkBitti(BIPOLAR1_COAG_AYAK_SERBEST() && modAktif, &gBipolar1CoagBurst1)) {
            gBipolar1CoagBurst2 = 0U;
            gBipolar1CoagStart = GUCU_KAPALI;
            gAutoStopCiftPedal = 0U;
            gAutoStopStart = 0U;
            gAutoStopStart0 = 0U;
            gLigasureWattYaz = 0U;
            if (1U == gBipolarAutoStop) { gLigasurePedal = 0U; }
        }

        // -- Bipolar2 CUT (seal/ligasure) -----------------------
        izinVerildi = (false == genx_isOtherChannelActive(0U));

        if (true == esuInput_isArkBitti(BIPOLAR2_CUT_LIGASURE_AYAK_BASILI() && izinVerildi, &gBipolar2CutBirakma)) {
            gBipolar2CutBasma = 0U;
            gBipolar2CutStart = GUCU_ACIK;
        }

        if (true == esuInput_isArkBitti(BIPOLAR2_CUT_LIGASURE_AYAK_SERBEST() && (1U == gBipolar2CutStart), &gBipolar2CutBasma)) {
            gBipolar2CutBirakma = 0U;
            gBipolar2CutStart = GUCU_KAPALI;
        }

        // -- Bipolar2 COAG (seal/ligasure) ----------------------
        izinVerildi = (false == genx_isOtherChannelActive(0U)) && (0U == gAutoStopCiftPedal) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
        modAktif = (0U == gAutoStopTekPedal) && (0U == gBipolarHand) && ((1U == gBipolar2CoagStart) || (1U == gAutoStopCiftPedal)) \
                && (1U == gBipolar2PedalCift);      // orijinalde vardı, son sürümde düşmüştü

        if (true == esuInput_isArkBitti(BIPOLAR2_COAG_LIGASURE_AYAK_BASILI() && izinVerildi, &gBipolar2CoagBurst2)) {
            gBipolar2CoagBurst1 = 0U;
            gBipolar2CoagStart = GUCU_ACIK;
            gAutoStopCiftPedal = 1U;
            if (1U == gLigasureStart) { gLigasurePedal = 1U; }
        }

        if (true == esuInput_isArkBitti(BIPOLAR2_COAG_LIGASURE_AYAK_SERBEST() && modAktif, &gBipolar2CoagBurst1)) {
            gBipolar2CoagBurst2 = 0U;
            gBipolar2CoagStart = GUCU_KAPALI;
            gAutoStopCiftPedal = 0U;
            gAutoStopStart = 0U;
            gAutoStopStart0 = 0U;
            gLigasureWattYaz = 0U;
            if (1U == gLigasureStart) { gLigasurePedal = 0U; }
        }

        // -- Bipolar tek pedal (Bipolar1 COAG) ------------------
        izinVerildi = (false == genx_isOtherChannelActive(0U)) && (0U == gAutoStopTekPedal) && (0U == gBipolarHand);
        modAktif = (1U == gBipolar1CoagStart) && (1U == gAutoStopTekPedal) \
                && (0U == gBipolarHand);            // orijinalde vardı, son sürümde düşmüştü

        if (true == esuInput_isArkBitti(BIPOLAR_TEK_PEDAL_BASILI() && izinVerildi, &gTekPedalBasma)) {
            gTekPedalBirakma = 0U;
            gBipolar1CoagStart = GUCU_ACIK;
            gAutoStopTekPedal = 1U;
        }

        if (true == esuInput_isArkBitti(BIPOLAR_TEK_PEDAL_SERBEST() && modAktif, &gTekPedalBirakma)) {
            gTekPedalBasma = 0U;
            gBipolar1CoagStart = GUCU_KAPALI;
            gAutoStopTekPedal = 0U;
            gAutoStopStart = 0U;
            gAutoStopStart0 = 0U;
            gLigasureWattYaz = 0U;
        }

        // -- Bipolar el kumandası (hand) ------------------------
        izinVerildi = (0U == gBipolar1CoagStart) && (0U == gAutoStop2) && (1U == gBipolarHand) \
                    && (0U == gBipolar1PedalCift) && (0U == gBipolar2PedalCift) && (0U == gBipolar2CutStart) \
                    && (0U == gBipolar2CoagStart) && (false == genx_isOtherChannelActive(0U));
        modAktif = ((1U == gBipolar2CoagStart) || (1U == gBipolar1CoagStart) || (1U == gAutoStop2)) \
                && (1U == gBipolarHand);            // orijinalde vardı, son sürümde düşmüştü

        if (true == esuInput_isArkBitti(BIPOLAR_HAND_BASILI() && izinVerildi, &gHandBasma)) {
            gHandBirakma = 0U;
            if (0U == gLigasureStart) {
                gBipolar1CoagStart = GUCU_ACIK;
            } else {
                gBipolar2CoagStart = GUCU_ACIK;
                gLigasurePedal = 1U;
            }
        }

        if (true == esuInput_isArkBitti(BIPOLAR_HAND_SERBEST() && modAktif, &gHandBirakma)) {
            gHandBasma = 0U;
            gAutoStopStart = 0U;
            gAutoStopStart0 = 0U;
            gLigasureWattYaz = 0U;
            if (0U == gLigasureStart) {
                gBipolar1CoagStart = GUCU_KAPALI;
            } else {
                gAutoStop2 = 0U;
                gBipolar2CoagStart = GUCU_KAPALI;
                gLigasurePedal = 0U;
            }
        }

        // -- Pedal seçilmedi (Error-3) --------------------------
        basildi = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) || (GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10));
        izinVerildi = (0U == gMono1CutStart) && (0U == gMono1CoagStart) && (0U == gMono2CutStart) && (0U == gMono2CoagStart) \
                    && (0U == gBipolar1CutStart) && (0U == gBipolar1CoagStart) && (0U == gMono1Pedal) && (0U == gMono2Pedal) \
                    && (0U == gBipolar1PedalCift) && (0U == gBipolar2PedalCift) && (0U == gAlarmPlate) && (0U == gPedalError3);

        if (true == esuInput_isArkBitti(basildi && izinVerildi, &gPedalError2Burst)) {
            gPedalError3 = 1U;
        }
    }
}