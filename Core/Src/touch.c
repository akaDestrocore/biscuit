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

#include <string.h>
#include "core.h"
#include "modules.h"

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
    { "get S2_m1_cut_watt.USER_VALUE.val",	&gMono1CutWatt,		sizeof(gMono1CutWatt)		},
    { "get S3_m1_cut_mod.USER_MODE.val",	&gMono1CutMode,		sizeof(gMono1CutMode)		},
    { "get S4_m1_cog_watt.USER_VALUE.val", 	&gMono1CoagWatt,	sizeof(gMono1CoagWatt)		},
    { "get S5_m1_cog_mod.USER_MODE.val",   	&gMono1CoagMode, 	sizeof(gMono1CoagMode)		},
	{ "get S6_m2_cut_watt.USER_VALUE.val", 	&gMono2CutWatt, 	sizeof(gMono2CutWatt)		},
	{ "get S7_m2_cut_mod.USER_MODE.val", 	&gMono2CutMode,		sizeof(gMono2CutMode)		},
	{ "get S8_m2_cog_watt.USER_VALUE.val",	&gMono2CoagWatt,	sizeof(gMono2CoagWatt)		},
	{ "get S9_m2_cog_mod.USER_MODE.val",	&gMono2CoagMode,	sizeof(gMono2CoagMode)		},
	{ "get S10_b1_cut_wat.USER_VALUE.val",	&gBipolar1CutWatt,	sizeof(gBipolar1CutWatt)	},
	{ "get S12_b1_cut_mod.USER_MODE.val",	&gBipolar1CutMode,	sizeof(gBipolar1CutMode)	},
	{ "get S11_b1_cog_wat.USER_VALUE.val",	&gBipolar1CoagWatt, sizeof(gBipolar1CoagWatt)	},
	{ "get S13_b1_cog_mod.USER_MODE.val",	&gBipolar1CoagMode,	sizeof(gBipolar1CoagMode)	},
	{ "get S14_b2_cut_wat.USER_VALUE.val",	&gBipolar2CutWatt,	sizeof(gBipolar2CutWatt)	},
	{ "get S16_b2_cut_mod.USER_MODE.val",	&gBipolar2CutMode,	sizeof(gBipolar2CutMode)	},
	{ "get S15_b2_cog_wat.USER_VALUE.val",	&gBipolar2CoagWatt,	sizeof(gBipolar2CoagWatt)	},
	{ "get S17_b2_cog_mod.USER_MODE.val",	&gBipolar2CoagMode,	sizeof(gBipolar2CoagMode)	},
    { "get S18_plate_sec.USER_PLATE.val",  	&gPlate,         	sizeof(gPlate)				},
    { "get S25_ses_ayar.SES_VERI.val",     	&gSesVeri,       	sizeof(gSesVeri)			},
    { "get S31_endo_ayar.CUT_KADEME.val",	&gEndoCutKademe,	sizeof(gEndoCutKademe)		},
	{ "get S31_endo_ayar.COAG_KADEME.val",	&gEndoCoagKademe,	sizeof(gEndoCoagKademe)		},
	{ "get S31_endo_ayar.CUT_SURE.val",		&gEndoCutSure,		sizeof(gEndoCutSure)		},
	{ "get S31_endo_ayar.COAG_SURE.val",	&gEndoCoagSure,		sizeof(gEndoCoagSure)		}
};

#define VAR_COUNT ((uint8_t)(sizeof(gSaveSteps) / sizeof(gSaveSteps[0])))

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void touch_tumPedalKapat(uint8_t haric);
static void touch_sesUygula(uint8_t seviye);
static void touch_endoUygula(void);
static void touch_polGuncelle(void);
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
                if (false == genx_isOtherChannelActive(0U)) {
                    
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
        if (BIPOLAR1_COAG_MODE_START != gBipolar1CoagMode) {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);   //hand relay off
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
  * @brief Endocut ayarlarını çevirir
  * @retval None
  */
static void touch_endoUygula(void) {

    if ((gEndoCutKademe >= 1U) && (gEndoCutKademe <= 7U)) {

        if (MONO1_CUT_MODE_POLYPECTOMY == gMono1CutMode) gEndoCutWattPol = (float)(gEndoCutKademe * 50U);
        if (MONO1_CUT_MODE_PAPILLOTOMES == gMono1CutMode) gEndoCutWattPol = (float)(gEndoCutKademe * 22U);
    }

    if ((gEndoCoagKademe >= 1U) && (gEndoCoagKademe <= 5U)) {

        if (MONO1_CUT_MODE_POLYPECTOMY == gMono1CutMode) gEndoCoagWattPol = (float)(gEndoCoagKademe * 20U);
        if (MONO1_CUT_MODE_PAPILLOTOMES == gMono1CutMode) gEndoCoagWattPol = (float)(gEndoCoagKademe * 10U);
    }

    if (gEndoCutSure < 6U) gEndoCutSure = gEndoCutSure + 2U;
    if (gEndoCutSure > 5U) gEndoCutSure = 5U;

    if ((gEndoCoagSure >= 1U) && (gEndoCoagSure <= 15U)) {
        gEndoCoagTimer = (uint16_t)(gEndoCoagSure * 10U);
    }
}

/**
  * @brief Kullanıcı watt ve mod değerlerinden sekiz kanalın işlenmiş değerlerini hesaplar
  * @retval None
  */
static void touch_polGuncelle(void) {
    
    float pol;
    float limit;
    float offset;

    // Mono1 CUT
    pol = (float)gMono1CutWatt;
    limit = 385.0f;
    offset = 0.0f;
    switch (gMono1CutMode) {
        case MONO1_CUT_MODE_CUT: { 
            offset = 5.0f; 
            break; 
        }

        case MONO1_CUT_MODE_BLEND1: { 
            offset = 5.0f; 
            limit = 240.0f; 
            break; 
        }
        
        case MONO1_CUT_MODE_BLEND2: { 
            limit = 200.0f; 
            break; 
        }
        
        case MONO1_CUT_MODE_BLEND3: { 
            limit = 150.0f; 
            break; 
        }
        
        case MONO1_CUT_MODE_POLYPECTOMY: { 
            limit = 350.0f; 
            break; 
        }
        
        case MONO1_CUT_MODE_PAPILLOTOMES: { 
            limit = 150.0f; 
            break; 
        }
        
        case MONO1_CUT_MODE_RESECTION1:
        case MONO1_CUT_MODE_RESECTION2: { 
            offset = 4.0f; 
            limit = 350.0f; 
            break; 
        }

        default: { 
            break; 
        }
    }

    if (pol < 16.0f) pol -= offset;
    if (pol > limit) pol = limit;

    gCut1WattPol = pol;

    // Mono1 COAG
    pol = (float)gMono1CoagWatt;
    limit = 120.0f;
    switch (gMono1CoagMode) {
        case MONO1_COAG_MODE_SPRAY1:
        case MONO1_COAG_MODE_SPRAY2:
        case MONO1_COAG_MODE_SPRAY3:
        case MONO1_COAG_MODE_FULGURATION: { 
            limit = 100.0f; 
            break; 
        }

        default: { 
            break; 
        }
    }
    
    if (pol > limit) pol = limit;

    gCoag1WattPol = pol;

    // Mono2 CUT
    pol = (float)gMono2CutWatt;
    limit = 385.0f;
    offset = 0.0f;

    switch (gMono2CutMode) {
        case MONO2_CUT_MODE_CUT: { 
            offset = 5.0f; 
            break; 
        }
        
        case MONO2_CUT_MODE_BLEND1: { 
            offset = 5.0f; 
            limit = 240.0f; 
            break; 
        }
        
        case MONO2_CUT_MODE_BLEND2: { 
            limit = 200.0f; 
            break; 
        }
        
        case MONO2_CUT_MODE_BLEND3: { 
            limit = 150.0f; 
            break; 
        }
        
        case MONO2_CUT_MODE_POLYPECTOMY:
        case MONO2_CUT_MODE_PAPILLOTOMES: { 
            limit = 350.0f; 
            break; 
        }
        
        case MONO2_CUT_MODE_RESECTION1:
        case MONO2_CUT_MODE_RESECTION2: { 
            offset = 4.0f; 
            limit = 350.0f; 
            break; 
        }
        
        default: { 
            break; 
        }
    }

    if (pol < 16.0f) pol -= offset;
    if (pol > limit) pol = limit;

    gCut2WattPol = pol;

    // Mono2 COAG
    pol = (float)gMono2CoagWatt;
    limit = 120.0f;

    switch (gMono2CoagMode) {
        case MONO2_COAG_MODE_SPRAY1:
        case MONO2_COAG_MODE_SPRAY2:
        case MONO2_COAG_MODE_SPRAY3:
        case MONO2_COAG_MODE_FULGURATION: { 
            limit = 100.0f; 
            break; 
        }
        
        default: { 
            break; 
        }
    }

    if (pol > limit) pol = limit;
    
    gCoag2WattPol = pol;

    // Bipolar1 CUT
    pol = (float)gBipolar1CutWatt;
    limit = 200.0f;
    switch (gBipolar1CutMode) {
        
        case BIPOLAR_CUT_MODE_CUTTING:
        case BIPOLAR_CUT_MODE_SCISSORS:
        case BIPOLAR_CUT_MODE_BIVAPO:
        case BIPOLAR_CUT_MODE_BIREZO: { 
            limit = 185.0f; 
            break; 
        }

        default: { 
            break; 
        }
    }

    if (pol > limit) pol = limit;

    gBipolar1CutPol = pol;

    // Bipolar1 COAG
    pol = (float)gBipolar1CoagWatt;
    limit = 130.0f;
    switch (gBipolar1CoagMode) {
        
        case BIPOLAR1_COAG_MODE_STANDARD:
        case BIPOLAR1_COAG_MODE_FORCED: { 
            limit = 150.0f; 
            break; 
        }

        case BIPOLAR1_COAG_MODE_STOP:
        case BIPOLAR1_COAG_MODE_START:
        case 5: { 
            limit = 120.0f; 
            break; 
        }

        default: { 
            break; 
        }
    }

    if (pol > limit) pol = limit;

    gBipolar1CoagPol = pol;

    // Bipolar2 (seal): modlar aynı sınırı kullanır
    pol = (float)gBipolar2CutWatt;
    gBipolar2CutPol = (pol > 185.0f) ? 185.0f : pol;

    pol = (float)gBipolar2CoagWatt;
    gBipolar2CoagPol = (pol > 310.0f) ? 310.0f : pol;
}

/**
  * @brief Bipolar2 COAG modunun ayarını uygular
  * @retval None
  */
static void touch_bipolar2CoagModUygula(void) {

    switch (gBipolar2CoagMode) {

        case SEAL_COAG_MODE_LIGATION:
        case SEAL_COAG_MODE_SEALSURE: {
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

        case SEAL_COAG_MODE_TISSUELOCK: {
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
            if ((MONO1_CUT_MODE_POLYPECTOMY == gMono1CutMode) || (MONO1_CUT_MODE_PAPILLOTOMES == gMono1CutMode)) {
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
            (void)NEXTION_setPage(UI_PAGE_MONO2_CUT_WATT); 
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
                gBipolarAutoStop = (5 == gBipolar1CoagMode) ? 1U : 0U;
                (void)NEXTION_sendCmdRetry("S1_anasayfa.p6.pic=4");
                HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
                gBipolarHand = 0U;
                touch_tumPedalKapat(PEDAL_BIP1);

                if (BIPOLAR1_COAG_MODE_START == gBipolar1CoagMode) {
                    gLigasureStart = GUCU_KAPALI;
                    gBipolarAutoStop = 0U;
                    gBipolar1CoagMode = BIPOLAR1_COAG_MODE_STANDARD;
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
            (void)NEXTION_getVarRetry(&gMono1CutWatt, "get S2_m1_cut_watt.USER_VALUE.val", sizeof(gMono1CutWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n0.val=S2_m1_cut_watt.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        case S4__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gMono1CoagWatt, "get S4_m1_cog_watt.USER_VALUE.val", sizeof(gMono1CoagWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n1.val=S4_m1_cog_watt.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        case S6__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gMono2CutWatt, "get S6_m2_cut_watt.USER_VALUE.val", sizeof(gMono2CutWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n2.val=S6_m2_cut_watt.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }
        
        case S8__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gMono2CoagWatt, "get S8_m2_cog_watt.USER_VALUE.val", sizeof(gMono2CoagWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n3.val=S8_m2_cog_watt.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        case S10__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar1CutWatt, "get S10_b1_cut_wat.USER_VALUE.val", sizeof(gBipolar1CutWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n6.val=S10_b1_cut_wat.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        case S11__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar1CoagWatt, "get S11_b1_cog_wat.USER_VALUE.val", sizeof(gBipolar1CoagWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n7.val=S11_b1_cog_wat.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        case S14__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar2CutWatt, "get S14_b2_cut_wat.USER_VALUE.val", sizeof(gBipolar2CutWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n4.val=S14_b2_cut_wat.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }

        case S15__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar2CoagWatt, "get S15_b2_cog_wat.USER_VALUE.val", sizeof(gBipolar2CoagWatt));
            touch_polGuncelle();
            (void)NEXTION_sendCmdRetry("S1_anasayfa.n5.val=S15_b2_cog_wat.USER_VALUE.val");
            (void)NEXTION_setPage(UI_PAGE_MAIN);
            break;
        }


        // -- Mod sayfaları ----------------------------------------
        case S3__POLYPECTOMY_BUTTON: {
            gMono1CutMode = MONO1_CUT_MODE_POLYPECTOMY;
            (void)NEXTION_setPage(UI_PAGE_MONO1_ENDO);
            break;
        }

        case S3__PAPILLOTOMES_BUTTON: {
            gMono1CutMode = MONO1_CUT_MODE_PAPILLOTOMES;
            (void)NEXTION_setPage(UI_PAGE_MONO1_ENDO);
            break;
        }

        case S3__OK_BUTTON: {
            (void)NEXTION_getVarRetry((void *)&gMono1CutMode, "get S3_m1_cut_mod.USER_MODE.val", sizeof(gMono1CutMode));
            touch_polGuncelle();
            if ((gMono1CutWatt > 0U) && (gMono1CutWatt <= 400U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S5__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gMono1CoagMode, "get S5_m1_cog_mod.USER_MODE.val", sizeof(gMono1CoagMode));
            touch_polGuncelle();
            if ((gMono1CoagWatt > 0U) && (gMono1CoagWatt <= 120U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S7__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gMono2CutMode, "get S7_m2_cut_mod.USER_MODE.val", sizeof(gMono2CutMode));
            touch_polGuncelle();
            if ((gMono2CutWatt > 0U) && (gMono2CutWatt <= 400U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S9__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gMono2CoagMode, "get S9_m2_cog_mod.USER_MODE.val", sizeof(gMono2CoagMode));
            touch_polGuncelle();
            if ((gMono2CoagWatt > 0U) && (gMono2CoagWatt <= 120U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S12__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar1CutMode, "get S12_b1_cut_mod.USER_MODE.val", sizeof(gBipolar1CutMode));
            touch_polGuncelle();
            if ((gBipolar1CutWatt > 0U) && (gBipolar1CutWatt <= 200U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S16__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar2CutMode, "get S16_b2_cut_mod.USER_MODE.val", sizeof(gBipolar2CutMode));
            touch_polGuncelle();
            if ((gBipolar2CutWatt > 0U) && (gBipolar2CutWatt <= 200U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S13__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar1CoagMode, "get S13_b1_cog_mod.USER_MODE.val", sizeof(gBipolar1CoagMode));
            touch_polGuncelle();

            switch (gBipolar1CoagMode) {

                case BIPOLAR1_COAG_MODE_STANDARD:
                case BIPOLAR1_COAG_MODE_FORCED:
                case BIPOLAR1_COAG_MODE_STOP: {
                    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);   // el rölesi kapalı
                    gBipolarHand = 0U;
                    gLigasureStart = GUCU_KAPALI;
                    gBipolarAutoStop = 0U;
                    break;
                }

                case BIPOLAR1_COAG_MODE_START: {
                    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_SET);     // el rölesi açık
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

                default: { 
                    break; 
                }
            }

            if ((gBipolar1CoagWatt > 0U) && (gBipolar1CoagWatt <= 150U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S17__OK_BUTTON: {
            (void)NEXTION_getVarRetry(&gBipolar2CoagMode, "get S17_b2_cog_mod.USER_MODE.val", sizeof(gBipolar2CoagMode));
            touch_polGuncelle();
            touch_bipolar2CoagModUygula();
            if ((gBipolar2CoagWatt > 0U) && (gBipolar2CoagWatt <= 350U)) { (void)NEXTION_setPage(UI_PAGE_MAIN); }
            break;
        }

        case S31__OK_BUTTON: {
            if ((MONO1_CUT_MODE_POLYPECTOMY == gMono1CutMode) || (MONO1_CUT_MODE_PAPILLOTOMES == gMono1CutMode)) {
                (void)NEXTION_getVarRetry(&gEndoCutKademe, "get S31_endo_ayar.CUT_KADEME.val", sizeof(gEndoCutKademe));
                (void)NEXTION_getVarRetry(&gEndoCoagKademe, "get S31_endo_ayar.COAG_KADEME.val", sizeof(gEndoCoagKademe));
                (void)NEXTION_getVarRetry((void *)&gEndoCutSure, "get S31_endo_ayar.CUT_SURE.val", sizeof(gEndoCutSure));
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
    touch_polGuncelle();

    // Bipolar1 COAG modu
    if ((gBipolar1CoagWatt > 0U) && (gBipolar1CoagWatt <= 150U)) {
        HAL_GPIO_WritePin(GPIOD, GPIO_PIN_0, GPIO_PIN_RESET);
        gLigasureStart = GUCU_KAPALI;
        gBipolarAutoStop = 0U;
        
        if ((BIPOLAR1_COAG_MODE_START == gBipolar1CoagMode) || (5 == gBipolar1CoagMode)) {
            HAL_GPIO_WritePin(GPIOD, GPIO_PIN_2 | GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
        }
    }

    // Bipolar2 COAG modu
    if ((gBipolar2CoagWatt > 0U) && (gBipolar2CoagWatt <= 350U)) {
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