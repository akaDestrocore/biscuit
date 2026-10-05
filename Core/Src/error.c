/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    error.c
 * @brief   Hata sayfası gösterimi
 *
 * @author  destrocore
 * @date    2026
 */

#include "core.h"
#include "modules.h"

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void error_show(const char *pKomut);

/* ============================================================
 * Genel API
 * ============================================================*/


/**
  * @brief Koruma/hata bayraklarını izler ve öncelik sırasına göre ilk hatayı bir kez gösterir
  * @retval None
  * @note Eski sekiz "ErrorYaz" mandalı tek bir mandalla değiştirildi. Öncelik sırası eski kodla aynıdır.
  */
void error_run(void) {
   
    static bool gosterildi = false;
    bool hataVar = (1U == gPedalError3) || (1U == gPedal2KezError) || (1U == gPwrKoruma) || (1U == gBiPwrKoruma) \
                || (1U == gMonoPwrKoruma) || (1U == gBipolarKoruma) || (1U == gMono1Koruma) || (1U == gMono2Koruma);

    if (false == hataVar) {
        gosterildi = false;
    } else if ((false == gosterildi) && (0U == gAlarmPlate) && (UI_PAGE_ERROR != gAcikSayfa)) {
        gosterildi = true;

        if (1U == gPedalError3) {
            error_show("t0.txt=\"ERROR-04\"");
        } else if (1U == gPedal2KezError) {
            error_show("t0.txt=\"ERROR-14\"");
        } else if (1U == gPwrKoruma) {
            error_show("t0.txt=\"ERROR-10\"");
        } else if (1U == gBiPwrKoruma) {
            error_show("t0.txt=\"ERROR-08\"");
        } else if (1U == gMonoPwrKoruma) {
            error_show("t0.txt=\"ERROR-09\"");
        } else if (1U == gBipolarKoruma) {
            error_show("t0.txt=\"ERROR-05\"");
        } else if (1U == gMono1Koruma) {
            error_show("t0.txt=\"ERROR-06\"");
        } else {
            error_show("t0.txt=\"ERROR-07\"");
        }
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Tüm Start bayraklarını kapatır, alarmı başlatır ve hata sayfasını gösterir
  * @param pKomut Hata metnini yazan Nextion komutu
  * @retval None
  */
static void error_show(const char *pKomut) {
    gDusukOncelikAlarm = 1U;
    gMono1CutStart = GUCU_KAPALI;
    gMono1CoagStart = GUCU_KAPALI;
    gMono2CutStart = GUCU_KAPALI;
    gMono2CoagStart = GUCU_KAPALI;
    gBipolar1CutStart = GUCU_KAPALI;
    gBipolar1CoagStart = GUCU_KAPALI;
    gBipolar2CutStart = GUCU_KAPALI;
    gBipolar2CoagStart = GUCU_KAPALI;

    (void)NEXTION_setPage(UI_PAGE_ERROR);
    (void)NEXTION_sendCmdRetry(pKomut);
    HAL_Delay(100U);
}