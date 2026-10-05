/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    plate.c
 * @brief   Hasta plakası direnç ölçümü, LED gösterimi ve plate hata sayfası
 *
 * @author  destrocore
 * @date    2026
 */

#include "core.h"
#include "modules.h"

/**
  * @brief Plate direncini her 10000 döngüde bir ölçer, eşik/histerezis uygular, ekranı günceller
  * @retval None
  */
void plate_run(void) {
    static uint16_t gecikme = 0U;
    static uint8_t dualToSingle = 0U;
    static uint32_t sonPlate = 0U;
    static uint8_t sonAsiriDirenc = 0xFFU;

    gecikme++;
    if (10000U <= gecikme) {
        float direnc = (float)gAdc1DmaBuff[ADC_CH_PLATE_RES];
        float ohmF;
        uint32_t ohm;
        uint8_t ohmYaz;
        uint8_t ohmPb;

        gecikme = 0U;
        if (direnc > 4090.0f) { direnc = 4090.0f; }
        if (direnc < 150.0f) { direnc = 150.0f; }
        
        ohmF = ohm = ((0.0000000265f * direnc * direnc * direnc) - (0.000076f * direnc * direnc) + (0.11f * direnc) - (22));//-22
        if (ohmF < 0.0f) { 
            ohmF = 0.0f; 
        }
        ohm = (uint32_t)ohmF;
        if (ohm > 200U) { ohm = 200U; }
        if ((ohm > 10U) && (ohm < 46U)) { ohm = ohm - 4U; }
        ohmYaz = (uint8_t)ohm;
        ohmPb = (uint8_t)(ohm / 2U);

        // Histerezisli aşırı direnç (plate temassızlığı) kararı
        if ((ohmYaz >= 100U) && (PLATE_DUAL == gPlate) && (0U == gPlate100Overresistance)) {
            gPlate100Overresistance = 1U;
        }
        HAL_Delay(10U);
        if ((ohmYaz <= 99U) && (ohmYaz >= 10U) && (1U == gPlate100Overresistance) && (PLATE_DUAL == gPlate)) {
            gPlate100Overresistance = 0U;
            dualToSingle = 0U;
        }
        if ((ohmYaz <= 10U) && (PLATE_DUAL == gPlate) && (0U == gPlate100Overresistance)) {
            gPlate100Overresistance = 1U;
            dualToSingle = 1U;
        }
        if ((ohmYaz >= 51U) && (PLATE_SINGLE == gPlate) && (0U == gPlate100Overresistance)) {
            gPlate100Overresistance = 1U;
            dualToSingle = 0U;
        }
        HAL_Delay(10U);
        if ((ohmYaz <= 50U) && (1U == gPlate100Overresistance) && (PLATE_SINGLE == gPlate)) {
            gPlate100Overresistance = 0U;
            dualToSingle = 0U;
        }

        // Ana sayfada plate resmi ve LED
        if ((PLATE_DUAL == gPlate) || (PLATE_SINGLE == gPlate)) {
            const char *pKomut;

            HAL_Delay(10U);
            if (PLATE_DUAL == gPlate) {
                pKomut = (1U == gPlate100Overresistance) ? "S1_anasayfa.p1.pic=129" : "S1_anasayfa.p1.pic=130";
            } else {
                pKomut = (1U == gPlate100Overresistance) ? "S1_anasayfa.p1.pic=131" : "S1_anasayfa.p1.pic=132";
            }

            if ((UI_PAGE_MAIN == gAcikSayfa) && ((gPlate != sonPlate) || (gPlate100Overresistance != sonAsiriDirenc))) {
                (void)NEXTION_sendCmdRetry(pKomut);
                sonPlate = gPlate;
                sonAsiriDirenc = gPlate100Overresistance;
            }

            HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, (1U == gPlate100Overresistance) ? GPIO_PIN_RESET : GPIO_PIN_SET);    // Yeşil
            HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, (1U == gPlate100Overresistance) ? GPIO_PIN_SET : GPIO_PIN_RESET);    // Kırmızı
            HAL_Delay(10U);
        }

        // Plate hata sayfası
        if ((1U == gAlarmPlate) && (0U == gPlateError)) {
            gPlateError = 1U;
            (void)NEXTION_setPage(UI_PAGE_ERROR);
            HAL_Delay(100U);
            if (PLATE_DUAL == gPlate) {
                (void)NEXTION_sendCmdRetry((0U == dualToSingle) ? "t0.txt=\"ERROR-01\"" : "t0.txt=\"ERROR-03\"");
            }
            HAL_Delay(50U);
            if (PLATE_SINGLE == gPlate) {
                (void)NEXTION_sendCmdRetry("t0.txt=\"ERROR-02\"");
            }
            HAL_Delay(50U);
        }

        // Plate seçim sayfası
        if ((UI_PAGE_PLATE == gAcikSayfa) && (0U == gPlateError)) {
            (void)NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), "S18_plate_sec.n0.val=", (int16_t)ohmYaz);
            HAL_Delay(10U);

            if ((PLATE_DUAL == gPlate) || (PLATE_SINGLE == gPlate)) {
                const char *pPbKomut = (PLATE_DUAL == gPlate) ? "S18_plate_sec.j0.val=" : "S18_plate_sec.j2.val=";
                const char *pResim;

                if (PLATE_DUAL == gPlate) {
                    pResim = (1U == gPlate100Overresistance) ? "S18_plate_sec.p0.pic=106" : "S18_plate_sec.p0.pic=109";
                } else {
                    pResim = (1U == gPlate100Overresistance) ? "S18_plate_sec.p2.pic=104" : "S18_plate_sec.p2.pic=107";
                }

                if (ohmPb <= 100U) {
                    (void)NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), pPbKomut, (int16_t)ohmPb);
                }
                HAL_Delay(1U);
                (void)NEXTION_sendCmdRetry(pResim);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_11, (1U == gPlate100Overresistance) ? GPIO_PIN_RESET : GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_12, (1U == gPlate100Overresistance) ? GPIO_PIN_SET : GPIO_PIN_RESET);
                HAL_Delay(50U);
            }
        }
    }
}