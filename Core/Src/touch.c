/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    touch.c
 * @brief   Dokunmatik olayları, ayar sayfaları ve kayıtlı programın yüklenmesi
 *
 * @author  destrocore
 * @date    2026
 */

#include <string.h>
#include "core.h"
#include "modules.h"
#include "monopolar.h"
#include "bipolar.h"

/* ============================================================
 * Özel: modül sabitleri
 * ============================================================*/

typedef enum {
    PEDAL_MONO1 = 1U,
    PEDAL_MONO2 = 2U,
    PEDAL_BIP1  = 3U,
    PEDAL_BIP2  = 4U
} Pedal_e;

static uint8_t gVolumeData  = 4U;
static uint8_t gVeriKayit   = 1U;

// -- Açılışta ekranın hafızasından okunması gereken değişkenler
typedef struct {
    const char *pGetCmd;
    void *pDst;
    uint16_t size;
} Get_Eeprom_t;

static const Get_Eeprom_t gSaveSteps[] = {
    { "get S2_m1_cut_watt.USER_VALUE.val",	&gMono1Cut.canli.user_watt,     sizeof(gMono1Cut.canli.user_watt)       },
    { "get S3_m1_cut_mod.USER_MODE.val",	&gMono1Cut.canli.user_mode,     sizeof(gMono1Cut.canli.user_mode)       },
    { "get S4_m1_cog_watt.USER_VALUE.val", 	&gMono1Coag.canli.user_watt,    sizeof(gMono1Coag.canli.user_watt)      },
    { "get S5_m1_cog_mod.USER_MODE.val",   	&gMono1Coag.canli.user_mode, 	sizeof(gMono1Coag.canli.user_mode)      },
	{ "get S6_m2_cut_watt.USER_VALUE.val", 	&gMono2Cut.canli.user_watt, 	sizeof(gMono2Cut.canli.user_watt)       },
	{ "get S7_m2_cut_mod.USER_MODE.val", 	&gMono2Cut.canli.user_watt,		sizeof(gMono2Cut.canli.user_watt)       },
	{ "get S8_m2_cog_watt.USER_VALUE.val",	&gMono2Coag.canli.user_watt,	sizeof(gMono2Coag.canli.user_watt)      },
	{ "get S9_m2_cog_mod.USER_MODE.val",	&gMono2Coag.canli.user_mode,	sizeof(gMono2Coag.canli.user_mode)      },
	{ "get S10_b1_cut_wat.USER_VALUE.val",	&gBipolar1Cut.canli.user_watt,	sizeof(gBipolar1Cut.canli.user_watt)    },
	{ "get S12_b1_cut_mod.USER_MODE.val",	&gBipolar1Cut.canli.user_mode,	sizeof(gBipolar1Cut.canli.user_mode)    },
	{ "get S11_b1_cog_wat.USER_VALUE.val",	&gBipolar1Coag.canli.user_watt, sizeof(gBipolar1Coag.canli.user_watt)   },
	{ "get S13_b1_cog_mod.USER_MODE.val",	&gBipolar1Coag.canli.user_mode,	sizeof(gBipolar1Coag.canli.user_mode)	},
	{ "get S14_b2_cut_wat.USER_VALUE.val",	&gBipolar2Cut.canli.user_watt,	sizeof(gBipolar2Cut.canli.user_watt)	},
	{ "get S16_b2_cut_mod.USER_MODE.val",	&gBipolar2Cut.canli.user_mode,	sizeof(gBipolar2Cut.canli.user_mode)	},
	{ "get S15_b2_cog_wat.USER_VALUE.val",	&gBipolar2Coag.canli.user_watt,	sizeof(gBipolar2Coag.canli.user_watt)	},
	{ "get S17_b2_cog_mod.USER_MODE.val",	&gBipolar2Coag.canli.user_mode,	sizeof(gBipolar2Coag.canli.user_mode)	},
    { "get S18_plate_sec.USER_PLATE.val",  	&gPlate,         	            sizeof(gPlate)				            },
    { "get S25_ses_ayar.SES_VERI.val",     	&gSesVeri,       	            sizeof(gSesVeri)			            },
    { "get S31_endo_ayar.CUT_KADEME.val",	&gEndoCutKademe,	            sizeof(gEndoCutKademe)		            },
	{ "get S31_endo_ayar.COAG_KADEME.val",	&gEndoCoagKademe,	            sizeof(gEndoCoagKademe)		            },
	{ "get S31_endo_ayar.CUT_SURE.val",		&gEndoCutSure,		            sizeof(gEndoCutSure)		            },
	{ "get S31_endo_ayar.COAG_SURE.val",	&gEndoCoagSure,		            sizeof(gEndoCoagSure)		            }
};

#define VAR_COUNT ((uint8_t)(sizeof(gSaveSteps) / sizeof(gSaveSteps[0])))

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void touch_tumPedalKapat(uint8_t haric);
static void touch_sesUygula(uint8_t seviye);
static void touch_endoUygula(void);
static void touch_polGuncelle(void);
static void touch_modDogrula(void);
static void touch_monoWattOk(Monopolar_Handle_t *pH);
static void touch_monoModOk(Monopolar_Handle_t *pH);
static void touch_bipolarWattOk(Bipolar_Handle_t *pH);
static void touch_bipolarModOku(Bipolar_Handle_t *pH);
static void touch_bipolarSayfaDonus(const Bipolar_Handle_t *pH);
static void touch_bipolar1CoagModUygula(void);
static void touch_bipolar2CoagModUygula(void);
static void touch_basmaOlayi(uint16_t objName);
static void touch_veriKayitOku(void);

/* ============================================================
 * Genel API
 * ============================================================*/

 /**
  * @brief Gelen dokunmatik frame'leri işler, gerekirse program yüklemeyi çalıştırır
  * @retval None
  */
void touch_run(void) {

    Nextion_Frame_t frame;

    (void)memset(&frame, 0x00, sizeof(Nextion_Frame_t));

    while (NEXTION_STATUS_OK == nextion_readFrame(&frame)) {

        if (NEXTION_FRAME_TOUCH == frame.type) {

            uint16_t objName = NEXTION_TOUCH_KEY(frame.data[0], frame.data[1]);

            gAcikSayfa = (UI_Page_e)frame.data[0];

            if (1U == frame.data[2]) {
                if (false == genx_isOtherChannelActive(NULL)) {

                    touch_basmaOlayi(objName);
                }
            } else if (0U == frame.data[2]) {
                // Ses butonu bırakıldı
                if ((UI_PAGE_VOLUME == gAcikSayfa) && ((S25__INCREASE_BUTTON == objName) || (S25__DECREASE_BUTTON == objName))) {
                    gMonopolarCutSes = 2U;
                }
            } else {
                // geçersiz olay
            }
        }
    }

    if (1U == gVeriKayit) {
        touch_veriKayitOku();
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

/**
  * @brief Seçilen pedal dışındaki tüm pedalları kapatır (ekran resimleri ve röleler dahil)
  * @param haric Kapatılmayacak olan pedal, hepsi için 0U
  * @retval None
  */
static void touch_tumPedalKapat(uint8_t haric) {

    if ((PEDAL_MONO1 != haric) && (1U == gMono1Pedal)) {
        gMono1Pedal = 0U;
        (void)NEXTION_sendCmdRetry("S1_anasayfa.p4.pic=6");
    }

    if ((PEDAL_MONO2 != haric) && (1U == gMono2Pedal)) {
        gMono2Pedal = 0U;
        (void)NEXTION_sendCmdRetry("S1_anasayfa.p5.pic=6");
    }

    if ((PEDAL_BIP1 != haric) && (1U == gBipolar1PedalCift)) {
        gBipolarAutoStop = 0U;
        gBipolar1PedalCift = 0U;
        (void)NEXTION_sendCmdRetry("S1_anasayfa.p6.pic=5");
    }

    if ((PEDAL_BIP2 != haric) && ((1U == gBipolar2PedalCift) || (1U == gLigasureStart))) {
        gBipolar2PedalCift = 0U;
        (void)NEXTION_sendCmdRetry("S1_anasayfa.p7.pic=5");
        gLigasureStart = GUCU_KAPALI;
        if (BIPOLAR_STD_COAGMODE_START != gBipolar1Coag.canli.user_mode) {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);   // hand röle kapalı
        }
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
    }
}

/**
  * @brief Ses seviyesine göre dört ses rölesini ayarlar
  * @param seviye 0-4
  * @retval None
  */
static void touch_sesUygula(uint8_t seviye) {

    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, (seviye >= 1U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_1, (seviye >= 2U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_7, (seviye >= 3U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_8, (seviye >= 4U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
  * @brief Endocut ayarlarını TIM5 kesmesinin kullandığı değerlere çevirir
  * @retval None
  */
static void touch_endoUygula(void) {

    monopolar_endoApply(&gMono1Cut);
    monopolar_endoApply(&gMono2Cut);
}

/**
  * @brief Kullanıcı watt ve mod değerlerinden sekiz yolun işlenmiş değerlerini hesaplar
  * @retval None
  */
static void touch_polGuncelle(void) {

    monopolar_updatePol(&gMono1Cut);
    monopolar_updatePol(&gMono1Coag);
    monopolar_updatePol(&gMono2Cut);
    monopolar_updatePol(&gMono2Coag);
    bipolar_updatePol(&gBipolar1Cut);
    bipolar_updatePol(&gBipolar1Coag);
    bipolar_updatePol(&gBipolar2Cut);
    bipolar_updatePol(&gBipolar2Coag);
}

/**
  * @brief Her yolun kayıtlı modunu doğrular, kapalı modsa geri dönüş moduna çevirir
  * @retval None
  */
static void touch_modDogrula(void) {

    (void)monopolar_isModeCorrected(&gMono1Cut);
    (void)monopolar_isModeCorrected(&gMono1Coag);
    (void)monopolar_isModeCorrected(&gMono2Cut);
    (void)monopolar_isModeCorrected(&gMono2Coag);
    (void)bipolar_isModeCorrected(&gBipolar1Cut);
    (void)bipolar_isModeCorrected(&gBipolar1Coag);
    (void)bipolar_isModeCorrected(&gBipolar2Cut);
    (void)bipolar_isModeCorrected(&gBipolar2Coag);
}

/**
  * @brief Monopolar watt sayfası OK: değeri okur, işler, ana sayfaya yazar ve sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_monoWattOk(Monopolar_Handle_t *pH) {

    (void)NEXTION_getVarRetry(&pH->canli.user_watt, pH->pCfg->pWattGetCmd, sizeof(pH->canli.user_watt));
    touch_polGuncelle();
    (void)NEXTION_sendCmdRetry(pH->pCfg->pMainSetCmd);
    (void)NEXTION_setPage(UI_PAGE_MAIN);
}

/**
  * @brief Monopolar mod sayfası OK: modu okur, doğrular, işler; watt geçerliyse sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_monoModOk(Monopolar_Handle_t *pH) {

    (void)NEXTION_getVarRetry(&pH->canli.user_mode, pH->pCfg->pModeGetCmd, sizeof(pH->canli.user_mode));
    (void)monopolar_isModeCorrected(pH);
    touch_polGuncelle();

    if ((pH->canli.user_watt > 0U) && (pH->canli.user_watt <= pH->pCfg->max_watt)) {
        (void)NEXTION_setPage(UI_PAGE_MAIN);
    }
}

/**
  * @brief Bipolar watt sayfası OK: değeri okur, işler, ana sayfaya yazar ve sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_bipolarWattOk(Bipolar_Handle_t *pH) {

    (void)NEXTION_getVarRetry(&pH->canli.user_watt, pH->pCfg->pWattGetCmd, sizeof(pH->canli.user_watt));
    touch_polGuncelle();
    (void)NEXTION_sendCmdRetry(pH->pCfg->pMainSetCmd);
    (void)NEXTION_setPage(UI_PAGE_MAIN);
}

/**
  * @brief Bipolar mod sayfası OK, ilk kısım: modu okur, doğrular ve işlenmiş watt'ı günceller
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_bipolarModOku(Bipolar_Handle_t *pH) {

    (void)NEXTION_getVarRetry(&pH->canli.user_mode, pH->pCfg->pModeGetCmd, sizeof(pH->canli.user_mode));
    (void)bipolar_isModeCorrected(pH);
    touch_polGuncelle();
}

/**
  * @brief Bipolar mod sayfası OK, son kısım: watt geçerliyse ana sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_bipolarSayfaDonus(const Bipolar_Handle_t *pH) {

    if ((pH->canli.user_watt > 0U) && (pH->canli.user_watt <= pH->pCfg->max_watt)) {
        (void)NEXTION_setPage(UI_PAGE_MAIN);
    }
}

/**
  * @brief Bipolar1 COAG modunun röle/bayrak ayarlarını uygular
  * @retval None
  */
static void touch_bipolar1CoagModUygula(void) {

    switch (gBipolar1Coag.canli.user_mode) {

        case BIPOLAR_STD_COAGMODE_STANDARD:
        case BIPOLAR_STD_COAGMODE_FORCED:
        case BIPOLAR_STD_COAGMODE_STOP: {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);   // hand relay off
            gBipolarHand = 0U;
            gLigasureStart = GUCU_KAPALI;
            gBipolarAutoStop = 0U;
            break;
        }

        case BIPOLAR_STD_COAGMODE_START: {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);     // hand relay on
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
            gBipolarHand = 1U;
            gLigasureStart = GUCU_KAPALI;
            gBipolarAutoStop = 0U;
            gAutoStop2 = 0U;
            gBipolar1PedalCift = 0U;
            (void)NEXTION_sendCmdRetry("S1_anasayfa.p6.pic=5");
            break;
        }

        case 5: {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
            gBipolarHand = 0U;
            gLigasureStart = GUCU_KAPALI;
            gBipolarAutoStop = 1U;
            break;
        }

        case BIPOLAR_SEAL_COAGMODE_LIGATION:
        case BIPOLAR_SEAL_COAGMODE_SEALSURE: {
            gBipolarHand = 1U;
            gLigasureStart = GUCU_ACIK;
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);                             // hand relay on
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);   // ligasure relay on
            touch_tumPedalKapat(PEDAL_BIP1);
            if (1U == gBipolar1PedalCift) {
                gBipolar1PedalCift = 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p7.pic=5");
            }
            break;
        }

        case BIPOLAR_SEAL_COAGMODE_TISSUELOCK: {
            gBipolarHand = 0U;
            gLigasureStart = GUCU_ACIK;
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);                           // hand relay off
            break;
        }

        default: {
            break;
        }
    }
}

/**
  * @brief Bipolar2 (seal) COAG modunun ayarını uygular
  * @retval None
  */
static void touch_bipolar2CoagModUygula(void) {

    switch (gBipolar2Coag.canli.user_mode) {

        case BIPOLAR_STD_COAGMODE_STANDARD:
        case BIPOLAR_STD_COAGMODE_FORCED:
        case BIPOLAR_STD_COAGMODE_STOP: {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);   // hand relay off
            gBipolarHand = 0U;
            gLigasureStart = GUCU_KAPALI;
            gBipolarAutoStop = 0U;
            break;
        }

        case BIPOLAR_STD_COAGMODE_START: {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);     // hand relay on
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
            gBipolarHand = 1U;
            gLigasureStart = GUCU_KAPALI;
            gBipolarAutoStop = 0U;
            gAutoStop2 = 0U;
            gBipolar1PedalCift = 0U;
            (void)NEXTION_sendCmdRetry("S1_anasayfa.p6.pic=5");
            break;
        }

        case 5: {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
            gBipolarHand = 0U;
            gLigasureStart = GUCU_KAPALI;
            gBipolarAutoStop = 1U;
            break;
        }

        case BIPOLAR_SEAL_COAGMODE_LIGATION:
        case BIPOLAR_SEAL_COAGMODE_SEALSURE: {
            gBipolarHand = 1U;
            gLigasureStart = GUCU_ACIK;
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);                             // hand relay on
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);   // ligasure relay on
            touch_tumPedalKapat(PEDAL_BIP2);
            if (1U == gBipolar2PedalCift) {
                gBipolar2PedalCift = 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p7.pic=5");
            }
            break;
        }

        case BIPOLAR_SEAL_COAGMODE_TISSUELOCK: {
            gBipolarHand = 0U;
            gLigasureStart = GUCU_ACIK;
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);                           // hand relay off
            break;
        }

        default: {
            break;
        }
    }
}

/**
  * @brief Buton basma olayını işler (yalnız tüm kanallar kapalıyken çağrılır)
  * @param objName Sayfa ve nesne kimliğinden oluşan anahtar (NEXTION_TOUCH_KEY)
  * @retval None
  */
static void touch_basmaOlayi(uint16_t objName) {

    if (UI_PAGE_PLATE == gAcikSayfa) {
        
        if (PLATE_DUAL == gPlate) {
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p3.pic=111");
        }

        if (PLATE_SINGLE == gPlate) {
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p5.pic=111");
        }
    }

    switch (objName) {

        // -- Ana sayfadan ayar sayfalarına geçiş -------------------
        case S1__MONO1_CUT_WATT_BUTTON: {
            
            if (true == monopolar_isEndoMode(&gMono1Cut)) {
                
                (void)NEXTION_setPage(UI_PAGE_MONO1_ENDO);
            } else {
                
                (void)NEXTION_setPage(UI_PAGE_MONO1_CUT_WATT);
            }
            break;
        }

        case S1__MONO1_CUT_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_MONO1_CUT_MODE);
            break;
        }

        case S1__MONO1_COAG_WATT_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_MONO1_COAG_WATT);
            break;
        }

        case S1__MONO1_COAG_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_MONO1_COAG_MODE);
            break;
        }

        case S1__MONO2_CUT_WATT_BUTTON: {

            if (true == monopolar_isEndoMode(&gMono2Cut)) {
                
                // (void)NEXTION_setPage(UI_PAGE_MONO2_ENDO);
            } else {
                
                (void)NEXTION_setPage(UI_PAGE_MONO2_CUT_WATT);
            }
            break;
        }

        case S1__MONO2_CUT_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_MONO2_CUT_MODE);
            break;
        }

        case S1__MONO2_COAG_WATT_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_MONO2_COAG_WATT);
            break;
        }

        case S1__MONO2_COAG_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_MONO2_COAG_MODE);
            break;
        }

        case S1__BIPOLAR_CUT_WATT_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR1_CUT_WATT);
            break;
        }

        case S1__BIPOLAR_CUT_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR1_CUT_MODE);
            break;
        }

        case S1__BIPOLAR_COAG_WATT_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR1_COAG_WATT);
            break;
        }

        case S1__BIPOLAR_COAG_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR1_COAG_MODE);
            break;
        }

        case S1__SEAL_CUT_WATT_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR2_CUT_WATT);
            break;
        }

        case S1__SEAL_CUT_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR2_CUT_MODE);
            break;
        }

        case S1__SEAL_COAG_WATT_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR2_COAG_WATT);
            break;
        }

        case S1__SEAL_COAG_MODE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_BIPOLAR2_COAG_MODE);
            break;
        }

        case S1__PLATE_BUTTON: {
            
            (void)NEXTION_setPage(UI_PAGE_PLATE);
            break;
        }

        case S1__PROGRAM_FAV_BUTTON:
        case S1__SELECTED_PROG_FIELD: {
            
            (void)NEXTION_setPage(UI_PAGE_PROGRAM);
            break;
        }

        case S1__LOG_BUTTON: {
            (void)NEXTION_setPage(UI_PAGE_LOG);
            break;
        }

        case S1__MENU_BUTTON_BUTTON: {
            (void)NEXTION_setPage(UI_PAGE_MENU_SETTINGS);
            break;
        }

        // -- Pedal seçimi -----------------------------------------
        case S1__MONO1_PEDAL_BUTTON: {
            if (1U == gMono1Pedal) {
                gMono1Pedal = 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p4.pic=6");
            } else {
                gMono1Pedal = 1U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p4.pic=0");
                touch_tumPedalKapat(PEDAL_MONO1);
            }
            break;
        }

        case S1__MONO2_PEDAL_BUTTON: {
            if (1U == gMono2Pedal) {
                gMono2Pedal = 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p5.pic=6");
            } else {
                gMono2Pedal = 1U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p5.pic=0");
                touch_tumPedalKapat(PEDAL_MONO2);
            }
            break;
        }

        case S1__BIPOLAR_PEDAL_BUTTON: {
            
            if (1U == gBipolar1PedalCift) {
                gBipolar1PedalCift = 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p6.pic=5");
            } else {
                gBipolar1PedalCift = 1U;
                gBipolarAutoStop = (5 == gBipolar1Coag.canli.user_mode) ? 1U : 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p6.pic=4");
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
                gBipolarHand = 0U;
                touch_tumPedalKapat(PEDAL_BIP1);

                if (BIPOLAR_STD_COAGMODE_START == gBipolar1Coag.canli.user_mode) {
                    gLigasureStart = GUCU_KAPALI;
                    gBipolarAutoStop = 0U;
                    gBipolar1Coag.canli.user_mode = BIPOLAR_STD_COAGMODE_STANDARD;
                    (void)NEXTION_sendCmdRetry("S1_anasayfa.p20.pic=52");
                }
            }
            break;
        }

        case S1__SEAL_PEDAL_BUTTON: {

            if (1U == gBipolar2PedalCift) {
                gBipolar2PedalCift = 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p7.pic=5");
                gLigasureStart = GUCU_KAPALI;
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
            } else {
                gBipolar2PedalCift = 1U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p7.pic=4");
                gLigasureStart = GUCU_ACIK;
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_SET);   // ligasure rölesi açık
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
                gBipolarHand = 0U;
                touch_tumPedalKapat(PEDAL_BIP2);
            }
            break;
        }

        // -- Geri butonları ---------------------------------------
        case S2__BACK_BUTTON:
        case S3__BACK_BUTTON:
        case S4__BACK_BUTTON:
        case S5__BACK_BUTTON:
        case S6__BACK_BUTTON:
        case S7__BACK_BUTTON:
        case S8__BACK_BUTTON:
        case S9__BACK_BUTTON:
        case S10__BACK_BUTTON:
        case S11__BACK_BUTTON:
        case S12__BACK_BUTTON:
        case S13__BACK_BUTTON:
        case S14__BACK_BUTTON:
        case S15__BACK_BUTTON:
        case S16__BACK_BUTTON:
        case S17__BACK_BUTTON:
        case S18__BACK_BUTTON:
        case S18__OK_BUTTON:
        case S20__BACK_BUTTON:
        case S21__BACK_BUTTON: {
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        // -- Watt sayfaları ---------------------------------------
        case S2__OK_BUTTON: {
            
            touch_monoWattOk(&gMono1Cut);
            break;
        }

        case S4__OK_BUTTON: {
            
            touch_monoWattOk(&gMono1Coag);
            break;
        }

        case S6__OK_BUTTON: {
            
            touch_monoWattOk(&gMono2Cut);
            break;
        }

        case S8__OK_BUTTON: {
            
            touch_monoWattOk(&gMono2Coag);
            break;
        }

        case S10__OK_BUTTON: {
            
            touch_bipolarWattOk(&gBipolar1Cut);
            break;
        }

        case S11__OK_BUTTON: {
            
            touch_bipolarWattOk(&gBipolar1Coag);
            break;
        }

        case S14__OK_BUTTON: {
            
            touch_bipolarWattOk(&gBipolar2Cut);
            break;
        }

        case S15__OK_BUTTON: {
            
            touch_bipolarWattOk(&gBipolar2Coag);
            break;
        }

        // -- Mod sayfaları ----------------------------------------
        case S3__POLYPECTOMY_BUTTON: {
            
            if (true == monopolar_isModeEnabled(&gMono1Cut, MONOPOLAR_CUTMODE_POLYPECTOMY)) {
                gMono1Cut.canli.user_mode = MONOPOLAR_CUTMODE_POLYPECTOMY;
                (void)NEXTION_setPage(UI_PAGE_MONO1_ENDO);
            }
            break;
        }

        case S3__PAPILLOTOMES_BUTTON: {
            
            if (true == monopolar_isModeEnabled(&gMono1Cut, MONOPOLAR_CUTMODE_PAPILLOTOMES)) {
                gMono1Cut.canli.user_mode = MONOPOLAR_CUTMODE_PAPILLOTOMES;
                (void)NEXTION_setPage(UI_PAGE_MONO1_ENDO);
            }
            break;
        }

        case S3__OK_BUTTON: {
            
            touch_monoModOk(&gMono1Cut);
            break;
        }

        case S5__OK_BUTTON: {
            
            touch_monoModOk(&gMono1Coag);
            break;
        }

        case S7__OK_BUTTON: {
            
            touch_monoModOk(&gMono2Cut);
            break;
        }

        case S9__OK_BUTTON: {
            
            touch_monoModOk(&gMono2Coag);
            break;
        }

        case S12__OK_BUTTON: {
            
            touch_bipolarModOku(&gBipolar1Cut);
            touch_bipolarSayfaDonus(&gBipolar1Cut);
            break;
        }

        case S16__OK_BUTTON: {
            
            touch_bipolarModOku(&gBipolar2Cut);
            touch_bipolarSayfaDonus(&gBipolar2Cut);
            break;
        }

        case S13__OK_BUTTON: {
            
            touch_bipolarModOku(&gBipolar1Coag);
            touch_bipolar1CoagModUygula();
            touch_bipolarSayfaDonus(&gBipolar1Coag);
            break;
        }

        case S17__OK_BUTTON: {
            
            touch_bipolarModOku(&gBipolar2Coag);
            touch_bipolar2CoagModUygula();
            touch_bipolarSayfaDonus(&gBipolar2Coag);
            break;
        }

        case S31__OK_BUTTON: {
            
            if (true == monopolar_isEndoMode(&gMono1Cut)) {
                (void)NEXTION_getVarRetry(&gEndoCutKademe, "get S31_endo_ayar.CUT_KADEME.val", sizeof(gEndoCutKademe));
                (void)NEXTION_getVarRetry(&gEndoCoagKademe, "get S31_endo_ayar.COAG_KADEME.val", sizeof(gEndoCoagKademe));
                (void)NEXTION_getVarRetry(&gEndoCutSure, "get S31_endo_ayar.CUT_SURE.val", sizeof(gEndoCutSure));
                (void)NEXTION_getVarRetry(&gEndoCoagSure, "get S31_endo_ayar.COAG_SURE.val", sizeof(gEndoCoagSure));
                touch_endoUygula();
                (void)NEXTION_setPage(UI_PAGE_MAIN);
            }

            break;
        }

        // -- Plate seçimi -----------------------------------------
        case S18__DUALPLATE_BUTTON: {
            
            gPlate = PLATE_DUAL;
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p3.pic=111");
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p5.pic=110");
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p2.pic=103");
            (void)NEXTION_sendCmdRetry("S18_plate_sec.j2.val=0");
            break;
        }

        case S18__SINGLEPLATE_BUTTON: {
            
            gPlate = PLATE_SINGLE;
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p3.pic=110");
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p5.pic=111");
            (void)NEXTION_sendCmdRetry("S18_plate_sec.p0.pic=102");
            (void)NEXTION_sendCmdRetry("S18_plate_sec.j0.val=0");
            break;
        }

        // -- Program yükleme ve ses -------------------------------
        case S21__OK_BUTTON: {
            
            (void)NEXTION_sendCmdRetry("tsw 255,0");
            gVeriKayit = 1U;
            break;
        }

        case S25__INCREASE_BUTTON: {

            if (gVolumeData < 4U) {
                gMonopolarCutSes = 1U;
                gVolumeData++;
                touch_sesUygula(gVolumeData);
                (void)NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), "S25_ses_ayar.j0.val=", (int16_t)(gVolumeData * 25U));
            }
            break;
        }

        case S25__DECREASE_BUTTON: {

            if (gVolumeData > 0U) {
                gMonopolarCutSes = 1U;
                gVolumeData--;
                touch_sesUygula(gVolumeData);
                (void)NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), "S25_ses_ayar.j0.val=", (int16_t)(gVolumeData * 25U));
            }
            break;
        }

        // -- Hata sayfasından dönüş -------------------------------
        case S29__OK_BUTTON: {
            
            HAL_Delay(50U);
            gPedalError3 = 0U;
            gPlateError = 0U;
            gMono1Koruma = 0U;
            gMono2Koruma = 0U;
            gBipolarKoruma = 0U;
            gBiPwrKoruma = 0U;
            gMonoPwrKoruma = 0U;
            gPwrKoruma = 0U;
            gPedal2KezError = 0U;
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            HAL_Delay(100U);
            (void)NEXTION_sendCmdRetry("tsw 255,1");
            break;
        }

        default: {
            break;
        }
    }
}

/**
  * @brief Nextion'daki kayıtlı programı okur ve tüm ayarları uygular
  * @retval None
  */
static void touch_veriKayitOku(void) {

    Nextion_Status_e durum;

    for (uint8_t i = 0U; i < VAR_COUNT; i++) {
        const Get_Eeprom_t *pAdim = &gSaveSteps[i];
        uint8_t ilerleme;

        do {
            durum = nextion_getVarBlocking(pAdim->pDst, pAdim->pGetCmd, pAdim->size);
        } while (NEXTION_STATUS_OK != durum);

        ilerleme = (uint8_t)(((uint16_t)(i + 1U) * 100U) / VAR_COUNT);
        (void)NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), "j0.val=", (int16_t)ilerleme);
    }

    (void)NEXTION_sendCmdRetry("S0_acilma.t1.txt=\"OK\"");

    // Kayıtlı mod bu yolda kapalıysa CUT/CONTACT/STANDARD'a dönülür
    touch_modDogrula();
    touch_polGuncelle();

    // Bipolar1 COAG modu
    if ((gBipolar1Coag.canli.user_watt > 0U) && (gBipolar1Coag.canli.user_watt <= 150U)) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
        gLigasureStart = GUCU_KAPALI;
        gBipolarAutoStop = 0U;

        if ((BIPOLAR_STD_COAGMODE_START == gBipolar1Coag.canli.user_mode) || (5 == gBipolar1Coag.canli.user_mode)) {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
        }
    }

    // Bipolar2 COAG modu
    if ((gBipolar2Coag.canli.user_watt > 0U) && (gBipolar2Coag.canli.user_watt <= 350U)) {
        
        touch_bipolar2CoagModUygula();
    }

    if ((PLATE_SINGLE != gPlate) && (PLATE_DUAL != gPlate)) {
        
        gPlate = PLATE_SINGLE;
    }

    if (gSesVeri <= 4U) {
        
        gVolumeData = (uint8_t)gSesVeri;
        touch_sesUygula(gVolumeData);
    }

    touch_endoUygula();

    HAL_Delay(50U);
    gVeriKayit = 0U;
    (void)NEXTION_sendCmdRetry("tsw 255,1");
    (void)NEXTION_setPage(UI_PAGE_MAIN);
}