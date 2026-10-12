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
static uint8_t gPedalError2Burst = 0U;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/

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

        // -- Pedal seçilmedi (Error-3) --------------------------
        basildi = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(CIFT_PEDAL_CUT_GPIO_Port, CIFT_PEDAL_CUT_Pin)) \
                || (GPIO_PIN_RESET == HAL_GPIO_ReadPin(CIFT_PEDAL_COAG_GPIO_Port, CIFT_PEDAL_COAG_Pin));
        
        izinVerildi = (0U == gMono1Cut.runtime.genel.start) && (0U == gMono1Coag.runtime.genel.start) \
                    && (0U == gMono2Cut.runtime.genel.start) && (0U == gMono2Coag.runtime.genel.start) \
                    && (0U == gMono1Pedal) && (0U == gMono2Pedal) && (0U == gAlarmPlate) && (0U == gPedalError3);

        if (true == genx_sayacBittiMi(basildi && izinVerildi, &gPedalError2Burst)) {
            gPedalError3 = 1U;
        }
    }
}