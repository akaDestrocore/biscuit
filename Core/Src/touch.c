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

#include <stdio.h>
#include <string.h>
#include "bipolar.h"
#include "core.h"
#include "genx.h"
#include "monopolar.h"

/* ============================================================
 * Özel: modül sabitleri
 * ============================================================*/
static uint8_t gVeriKayit   = 1U;
static uint8_t gVolumeData  = 4U;

typedef enum {
    PEDAL_MONO1 = 1U,
    PEDAL_MONO2 = 2U,
    PEDAL_BIP1  = 3U,
    PEDAL_BIP2  = 4U
} Pedal_e;

// -- Açılışta ekranın hafızasından okunması gereken değişkenler
typedef struct {
    const char *pGetCmd;
    void *pDst;
    uint16_t size;
} Get_Eeprom_t;

static const Get_Eeprom_t gSaveSteps[] = {
    { "get S2_m1_cut_watt.USER_VALUE.val",	&gMono1Cut.runtime.user_watt,       sizeof(gMono1Cut.runtime.user_watt)     },
    { "get S3_m1_cut_mod.USER_MODE.val",	&gMono1Cut.runtime.user_mode,       sizeof(gMono1Cut.runtime.user_mode)     },
    { "get S4_m1_cog_watt.USER_VALUE.val", 	&gMono1Coag.runtime.user_watt,      sizeof(gMono1Coag.runtime.user_watt)    },
    { "get S5_m1_cog_mod.USER_MODE.val",   	&gMono1Coag.runtime.user_mode, 	    sizeof(gMono1Coag.runtime.user_mode)    },
	{ "get S6_m2_cut_watt.USER_VALUE.val", 	&gMono2Cut.runtime.user_watt, 	    sizeof(gMono2Cut.runtime.user_watt)     },
	{ "get S7_m2_cut_mod.USER_MODE.val", 	&gMono2Cut.runtime.user_mode,       sizeof(gMono2Cut.runtime.user_mode)     },
	{ "get S8_m2_cog_watt.USER_VALUE.val",	&gMono2Coag.runtime.user_watt,	    sizeof(gMono2Coag.runtime.user_watt)    },
	{ "get S9_m2_cog_mod.USER_MODE.val",	&gMono2Coag.runtime.user_mode,	    sizeof(gMono2Coag.runtime.user_mode)    },
	{ "get S10_b1_cut_wat.USER_VALUE.val",	&gBipolar1Cut.runtime.user_watt,    sizeof(gBipolar1Cut.runtime.user_watt)  },
	{ "get S12_b1_cut_mod.USER_MODE.val",	&gBipolar1Cut.runtime.user_mode,	sizeof(gBipolar1Cut.runtime.user_mode)  },
	{ "get S11_b1_cog_wat.USER_VALUE.val",	&gBipolar1Coag.runtime.user_watt,   sizeof(gBipolar1Coag.runtime.user_watt) },
	{ "get S13_b1_cog_mod.USER_MODE.val",	&gBipolar1Coag.runtime.user_mode,	sizeof(gBipolar1Coag.runtime.user_mode)	},
	{ "get S14_b2_cut_wat.USER_VALUE.val",	&gBipolar2Cut.runtime.user_watt,	sizeof(gBipolar2Cut.runtime.user_watt)	},
	{ "get S16_b2_cut_mod.USER_MODE.val",	&gBipolar2Cut.runtime.user_mode,	sizeof(gBipolar2Cut.runtime.user_mode)	},
	{ "get S15_b2_cog_wat.USER_VALUE.val",	&gBipolar2Coag.runtime.user_watt,	sizeof(gBipolar2Coag.runtime.user_watt)	},
	{ "get S17_b2_cog_mod.USER_MODE.val",	&gBipolar2Coag.runtime.user_mode,	sizeof(gBipolar2Coag.runtime.user_mode)	},
    { "get S18_plate_sec.USER_PLATE.val",  	&gPlate,         	                sizeof(gPlate)				            },
    { "get S25_ses_ayar.SES_VERI.val",     	&gSesVeri,       	                sizeof(gSesVeri)			            },
    { "get S31_endo_ayar.CUT_KADEME.val",	&gEndoCutKademe,	                sizeof(gEndoCutKademe)		            },
	{ "get S31_endo_ayar.COAG_KADEME.val",	&gEndoCoagKademe,	                sizeof(gEndoCoagKademe)		            },
	{ "get S31_endo_ayar.CUT_SURE.val",		&gEndoCutSure,		                sizeof(gEndoCutSure)		            },
	{ "get S31_endo_ayar.COAG_SURE.val",	&gEndoCoagSure,		                sizeof(gEndoCoagSure)		            }
};

#define VAR_COUNT ((uint8_t)(sizeof(gSaveSteps) / sizeof(gSaveSteps[0])))

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void touch_basmaOlayi(uint16_t objName);
static void touch_tumPedalKapat(uint8_t haric);
static void touch_monoWattOk(Monopolar_Handle_t *pH);
static void touch_polGuncelle(void);
static void touch_monoModOk(Monopolar_Handle_t *pH);
static void touch_sesUygula(uint8_t seviye);
static void touch_veriKayitOku(void);
static void touch_modDogrula(void);
static void touch_bipolarWattOk(Bipolar_Handle_t *pH);
static void touch_bipolarSayfaDonus(const Bipolar_Handle_t *pH);
static bool touch_bipolarWattKontrol(const Bipolar_Handle_t *pH);
static void touch_bipolarRoleUygula(void);
static void touch_bipolarCoagModUygula(Bipolar_Handle_t *pH);


/* ============================================================
 * Genel API
 * ============================================================*/

 /**
  * @brief Gelen dokunmatik frame'leri işler, gerekirse program yüklemeyi çalıştırır
  * @retval None
  */
void touch_yonet(void) {

    Nextion_Frame_t frame;

    (void)memset(&frame, 0x00, sizeof(Nextion_Frame_t));

    while (NEXTION_STATUS_OK == nextion_readFrame(&frame)) {

        if (NEXTION_FRAME_TOUCH == frame.type) {

            uint16_t objName = NEXTION_TOUCH_KEY(frame.data[0], frame.data[1]);

            gAcikSayfa = (UI_Page_e)frame.data[0];

            if (1U == frame.data[2]) {
                if (false == genx_baskaAktifVarMi(NULL)) {

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
  * @brief Buton basma olayını işler
  * @param objName Sayfa ve nesne ID'den oluşan kombinasyon
  * @retval None
  */
static void touch_basmaOlayi(uint16_t objName) {

    if (UI_PAGE_PLATE == gAcikSayfa) {
        
        if (PLATE_DUAL == gPlate) {
            (void)genx_sendCmd("S18_plate_sec.p3.pic=111");
            (void)genx_sendCmd("S18_plate_sec.p2.pic=103");
            (void)genx_sendCmd("S18_plate_sec.p5.pic=110");
        }

        if (PLATE_SINGLE == gPlate) {
            (void)genx_sendCmd("S18_plate_sec.p5.pic=111");
            (void)genx_sendCmd("S18_plate_sec.p0.pic=102");
            (void)genx_sendCmd("S18_plate_sec.p3.pic=110");
        }
    }

    if (0 == objName) {
        return;
    }

    switch (objName) {

        // -- Ana sayfadan ayar sayfalarına geçiş -------------------
        case S1__MONO1_CUT_WATT_BUTTON: {
            
            if (( (MONOPOLAR_CUTMODE_POLYPECTOMY == gMono1Cut.runtime.user_mode) && (true == monopolar_modEtkinMi(&gMono1Cut, MONOPOLAR_CUTMODE_POLYPECTOMY)) ) \
                || ((MONOPOLAR_CUTMODE_PAPILLOTOMES == gMono1Cut.runtime.user_mode) && (true == monopolar_modEtkinMi(&gMono1Cut, MONOPOLAR_CUTMODE_PAPILLOTOMES))) ) {
                
                (void)genx_setPage(UI_PAGE_MONO1_ENDO);

            } else {
                
                (void)genx_setPage(UI_PAGE_MONO1_CUT_WATT);
            }
            break;
        }

        case S1__MONO1_CUT_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_MONO1_CUT_MODE);
            break;
        }

        case S1__MONO1_COAG_WATT_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_MONO1_COAG_WATT);
            break;
        }

        case S1__MONO1_COAG_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_MONO1_COAG_MODE);
            break;
        }

        case S1__MONO2_CUT_WATT_BUTTON: {

            if (( (MONOPOLAR_CUTMODE_POLYPECTOMY == gMono2Cut.runtime.user_mode) && (true == monopolar_modEtkinMi(&gMono2Cut, MONOPOLAR_CUTMODE_POLYPECTOMY)) ) \
                || ((MONOPOLAR_CUTMODE_PAPILLOTOMES == gMono2Cut.runtime.user_mode) && (true == monopolar_modEtkinMi(&gMono2Cut, MONOPOLAR_CUTMODE_PAPILLOTOMES))) ) {
                 
                // (void)genx_setPage(UI_PAGE_MONO2_ENDO);
            } else {
                
                (void)genx_setPage(UI_PAGE_MONO2_CUT_WATT);
            }
            break;
        }

        case S1__MONO2_CUT_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_MONO2_CUT_MODE);
            break;
        }

        case S1__MONO2_COAG_WATT_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_MONO2_COAG_WATT);
            break;
        }

        case S1__MONO2_COAG_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_MONO2_COAG_MODE);
            break;
        }

        case S1__BIPOLAR1_CUT_WATT_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR1_CUT_WATT);
            break;
        }

        case S1__BIPOLAR1_CUT_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR1_CUT_MODE);
            break;
        }

        case S1__BIPOLAR1_COAG_WATT_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR1_COAG_WATT);
            break;
        }

        case S1__BIPOLAR1_COAG_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR1_COAG_MODE);
            break;
        }

        case S1__BIPOLAR2_CUT_WATT_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR2_CUT_WATT);
            break;
        }

        case S1__BIPOLAR2_CUT_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR2_CUT_MODE);
            break;
        }

        case S1__BIPOLAR2_COAG_WATT_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR2_COAG_WATT);
            break;
        }

        case S1__BIPOLAR2_COAG_MODE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_BIPOLAR2_COAG_MODE);
            break;
        }

        case S1__PLATE_BUTTON: {
            
            (void)genx_setPage(UI_PAGE_PLATE);
            break;
        }

        case S1__PROGRAM_FAV_BUTTON:
        case S1__SELECTED_PROG_FIELD: {
            
            (void)genx_setPage(UI_PAGE_PROGRAM);
            break;
        }

        case S1__LOG_BUTTON: {
            (void)genx_setPage(UI_PAGE_LOG);
            break;
        }

        case S1__MENU_BUTTON_BUTTON: {
            (void)genx_setPage(UI_PAGE_MENU_SETTINGS);
            break;
        }

        // -- Pedal seçimi -----------------------------------------
        case S1__MONO1_PEDAL_BUTTON: {
            if (1U == gMono1Pedal) {
                gMono1Pedal = 0U;
                (void)genx_sendCmd("S1_anasayfa.p4.pic=6");
            } else {
                gMono1Pedal = 1U;
                (void)genx_sendCmd("S1_anasayfa.p4.pic=0");
                touch_tumPedalKapat(PEDAL_MONO1);
            }
            break;
        }

        case S1__MONO2_PEDAL_BUTTON: {
            if (1U == gMono2Pedal) {
                gMono2Pedal = 0U;
                (void)genx_sendCmd("S1_anasayfa.p5.pic=6");
            } else {
                gMono2Pedal = 1U;
                (void)genx_sendCmd("S1_anasayfa.p5.pic=0");
                touch_tumPedalKapat(PEDAL_MONO2);
            }
            break;
        }

        case S1__BIPOLAR_PEDAL_BUTTON: {

            if (1U == gBipolar1PedalCift) {
                
                gBipolar1PedalCift = 0U;
                (void)genx_sendCmd("S1_anasayfa.p6.pic=5");
                touch_bipolarRoleUygula();
            } else {
                
                const Bipolar_ModeInfo_t *pModBilgi;

                gBipolar1PedalCift = 1U;
                (void)genx_sendCmd("S1_anasayfa.p6.pic=4");
                touch_tumPedalKapat(PEDAL_BIP1);

                // START gibi hand modu pedalla çalışmaz, STANDARD'a döner
                pModBilgi = bipolar_modBul(gBipolar1Coag.pCfg->function, gBipolar1Coag.runtime.user_mode);

                if ((NULL != pModBilgi) && (BIPOLAR_TRIGGER_HAND == pModBilgi->trigger) && (false == pModBilgi->ligature_mu)) {
                    
                    gBipolar1Coag.runtime.user_mode = BIPOLAR_STD_COAGMODE_STANDARD;
                    touch_polGuncelle();
                    (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S13_b1_cog_mod.USER_MODE.val=", (int16_t)BIPOLAR_STD_COAGMODE_STANDARD);
                    (void)genx_sendCmd("S1_anasayfa.p20.pic=52");
                }
            }
            break;
        }

        case S1__SEAL_PEDAL_BUTTON: {

            if (1U == gBipolar2PedalCift) {

                gBipolar2PedalCift = 0U;
                (void)genx_sendCmd("S1_anasayfa.p7.pic=5");
                touch_bipolarRoleUygula();

            } else {
                
                gBipolar2PedalCift = 1U;
                (void)genx_sendCmd("S1_anasayfa.p7.pic=4");
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
            (void)genx_setPage(UI_PAGE_MAIN);
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
            
            if (true == monopolar_modEtkinMi(&gMono1Cut, MONOPOLAR_CUTMODE_POLYPECTOMY)) {
                gMono1Cut.runtime.user_mode = MONOPOLAR_CUTMODE_POLYPECTOMY;
                (void)genx_setPage(UI_PAGE_MONO1_ENDO);
            }
            break;
        }

        case S3__PAPILLOTOMES_BUTTON: {
            
            if (true == monopolar_modEtkinMi(&gMono1Cut, MONOPOLAR_CUTMODE_PAPILLOTOMES)) {
                gMono1Cut.runtime.user_mode = MONOPOLAR_CUTMODE_PAPILLOTOMES;
                (void)genx_setPage(UI_PAGE_MONO1_ENDO);
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
            
            (void)genx_getVarRetry(&gBipolar1Cut.runtime.user_mode, gBipolar1Cut.pCfg->pModeGetCmd, sizeof(gBipolar1Cut.runtime.user_mode));
    
            if (true == bipolar_modKontrol(&gBipolar1Cut)) {
                
                char varsayilanModStr[64];
                int w = snprintf(varsayilanModStr, sizeof(varsayilanModStr), "%s=", gBipolar1Cut.pCfg->pModeGetCmd);

                if ((w > 0) && ((size_t)w < sizeof(varsayilanModStr))) {
                    (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), varsayilanModStr, (int16_t)gBipolar1Cut.pCfg->varsayilan_mod);
                }
                return;
            }
            touch_bipolarSayfaDonus(&gBipolar1Cut);
            break;
        }

        case S16__OK_BUTTON: {
            
            (void)genx_getVarRetry(&gBipolar2Cut.runtime.user_mode, gBipolar2Cut.pCfg->pModeGetCmd, sizeof(gBipolar2Cut.runtime.user_mode));
    
            if (true == bipolar_modKontrol(&gBipolar2Cut)) {
                
                char varsayilanModStr[64];
                int w = snprintf(varsayilanModStr, sizeof(varsayilanModStr), "%s=", gBipolar2Cut.pCfg->pModeGetCmd);

                if ((w > 0) && ((size_t)w < sizeof(varsayilanModStr))) {
                    (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), varsayilanModStr, (int16_t)gBipolar2Cut.pCfg->varsayilan_mod);
                }
                return;
            }
            touch_bipolarSayfaDonus(&gBipolar2Cut);
            break;
        }

        case S13__OK_BUTTON: {
            
            (void)genx_getVarRetry(&gBipolar1Coag.runtime.user_mode, gBipolar1Coag.pCfg->pModeGetCmd, sizeof(gBipolar1Coag.runtime.user_mode));
    
            if (true == bipolar_modKontrol(&gBipolar1Coag)) {
                
                char varsayilanModStr[64];
                int w = snprintf(varsayilanModStr, sizeof(varsayilanModStr), "%s=", gBipolar1Coag.pCfg->pModeGetCmd);

                if ((w > 0) && ((size_t)w < sizeof(varsayilanModStr))) {
                    (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), varsayilanModStr, (int16_t)gBipolar1Coag.pCfg->varsayilan_mod);
                }
                return;
            }
            touch_bipolarCoagModUygula(&gBipolar1Coag);
            touch_bipolarSayfaDonus(&gBipolar1Coag);
            break;
        }

        case S17__OK_BUTTON: {
            
            (void)genx_getVarRetry(&gBipolar2Coag.runtime.user_mode, gBipolar2Coag.pCfg->pModeGetCmd, sizeof(gBipolar2Coag.runtime.user_mode));
    
            if (true == bipolar_modKontrol(&gBipolar2Coag)) {
                
                char varsayilanModStr[64];
                int w = snprintf(varsayilanModStr, sizeof(varsayilanModStr), "%s=", gBipolar2Coag.pCfg->pModeGetCmd);

                if ((w > 0) && ((size_t)w < sizeof(varsayilanModStr))) {
                    (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), varsayilanModStr, (int16_t)gBipolar2Coag.pCfg->varsayilan_mod);
                }
                return;
            }
            touch_bipolarCoagModUygula(&gBipolar2Coag);
            touch_bipolarSayfaDonus(&gBipolar2Coag);
            break;
        }

        case S31__OK_BUTTON: {
            // Mono2'nin endocut sayfası bu değil
            if ( ((MONOPOLAR_CUTMODE_POLYPECTOMY == gMono1Cut.runtime.user_mode) & (true == monopolar_modEtkinMi(&gMono1Cut, MONOPOLAR_CUTMODE_POLYPECTOMY))) \
                || ((MONOPOLAR_CUTMODE_PAPILLOTOMES == gMono1Cut.runtime.user_mode) & (true == monopolar_modEtkinMi(&gMono1Cut, MONOPOLAR_CUTMODE_PAPILLOTOMES))) ) {
                (void)genx_getVarRetry(&gEndoCutKademe, "get S31_endo_ayar.CUT_KADEME.val", sizeof(gEndoCutKademe));
                (void)genx_getVarRetry(&gEndoCoagKademe, "get S31_endo_ayar.COAG_KADEME.val", sizeof(gEndoCoagKademe));
                (void)genx_getVarRetry(&gEndoCutSure, "get S31_endo_ayar.CUT_SURE.val", sizeof(gEndoCutSure));
                (void)genx_getVarRetry(&gEndoCoagSure, "get S31_endo_ayar.COAG_SURE.val", sizeof(gEndoCoagSure));
                #if (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO1_CUT_EN_PAPILLOTOMES)
                monopolar_endoHesap(&gMono1Cut);
                #endif
                (void)genx_setPage(UI_PAGE_MAIN);
            }

            break;
        }

        // -- Plate seçimi -----------------------------------------
        case S18__DUALPLATE_BUTTON: {
            
            gPlate = PLATE_DUAL;
            (void)genx_sendCmd("S18_plate_sec.p3.pic=111");
            (void)genx_sendCmd("S18_plate_sec.p5.pic=110");
            (void)genx_sendCmd("S18_plate_sec.p2.pic=103");
            (void)genx_sendCmd("S18_plate_sec.j2.val=0");
            break;
        }

        case S18__SINGLEPLATE_BUTTON: {
            
            gPlate = PLATE_SINGLE;
            (void)genx_sendCmd("S18_plate_sec.p3.pic=110");
            (void)genx_sendCmd("S18_plate_sec.p5.pic=111");
            (void)genx_sendCmd("S18_plate_sec.p0.pic=102");
            (void)genx_sendCmd("S18_plate_sec.j0.val=0");
            break;
        }

        // -- Program yükleme ve ses -------------------------------
        case S21__OK_BUTTON: {
            
            (void)genx_sendCmd("tsw 255,0");
            gVeriKayit = 1U;
            break;
        }

        case S25__INCREASE_BUTTON: {

            if (gVolumeData < 4U) {
                gMonopolarCutSes = 1U;
                gVolumeData++;
                touch_sesUygula(gVolumeData);
                (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S25_ses_ayar.j0.val=", (int16_t)(gVolumeData * 25U));
            }
            break;
        }

        case S25__DECREASE_BUTTON: {

            if (gVolumeData > 0U) {
                gMonopolarCutSes = 1U;
                gVolumeData--;
                touch_sesUygula(gVolumeData);
                (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S25_ses_ayar.j0.val=", (int16_t)(gVolumeData * 25U));
            }
            break;
        }

        // -- Hata sayfasından dönüş -------------------------------
        case S29__OK_BUTTON: {
            gAlarmPlate = 2U;
            alarm_yonet();
            genx_delay(50U);
            gPedalError3 = 0U;
            gPlateError = 0U;
            gMono1Koruma = 0U;
            gMono2Koruma = 0U;
            gBipolarKoruma = 0U;
            gBiPwrKoruma = 0U;
            gMonoPwrKoruma = 0U;
            gPwrKoruma = 0U;
            (void)genx_setPage(UI_PAGE_MAIN);
            genx_delay(100U);
            (void)genx_sendCmd("tsw 255,1");
            break;
        }

        default: {
            break;
        }
    }
}

/**
  * @brief Seçilen pedal dışındaki tüm pedalları kapatır
  * @param haric Kapatılmayacak olan pedal, hepsi için 0
  * @retval None
  */
static void touch_tumPedalKapat(uint8_t haric) {

    if (0U != haric) {
        gBipolarHandSahibi = NULL;
    }

    if ((PEDAL_MONO1 != haric) && (1U == gMono1Pedal)) {
        gMono1Pedal = 0U;
        (void)genx_sendCmd("S1_anasayfa.p4.pic=6");
    }

    if ((PEDAL_MONO2 != haric) && (1U == gMono2Pedal)) {
        gMono2Pedal = 0U;
        (void)genx_sendCmd("S1_anasayfa.p5.pic=6");
    }

    if ((PEDAL_BIP1 != haric) && (1U == gBipolar1PedalCift)) {
        gBipolar1PedalCift = 0U;
        (void)genx_sendCmd("S1_anasayfa.p6.pic=5");
    }

    if ((PEDAL_BIP2 != haric) && (1U == gBipolar2PedalCift)) {
        gBipolar2PedalCift = 0U;
        (void)genx_sendCmd("S1_anasayfa.p7.pic=5");
    }

    touch_bipolarRoleUygula();
}

/**
  * @brief Monopolar watt sayfası OK: değeri okur, işler, ana sayfaya yazar ve sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_monoWattOk(Monopolar_Handle_t *pH) {

    (void)genx_getVarRetry(&pH->runtime.user_watt, pH->pCfg->pWattGetCmd, sizeof(pH->runtime.user_watt));
    touch_polGuncelle();
    (void)genx_sendCmd(pH->pCfg->pMainSetCmd);
    (void)genx_setPage(UI_PAGE_MAIN);
}

/**
  * @brief Kullanıcı watt ve mod değerlerinden işlenmiş değerlerini hesaplar
  * @retval None
  */
static void touch_polGuncelle(void) {

    monopolar_polHesap(&gMono1Cut);
    monopolar_polHesap(&gMono1Coag);
    monopolar_polHesap(&gMono2Cut);
    monopolar_polHesap(&gMono2Coag);
    bipolar_polHesap(&gBipolar1Cut);
    bipolar_polHesap(&gBipolar1Coag);
    bipolar_polHesap(&gBipolar2Cut);
    bipolar_polHesap(&gBipolar2Coag);
}

/**
  * @brief Monopolar mod sayfası OK: modu okur, doğrular, işler. Watt geçerliyse sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_monoModOk(Monopolar_Handle_t *pH) {

    (void)genx_getVarRetry(&pH->runtime.user_mode, pH->pCfg->pModeGetCmd, sizeof(pH->runtime.user_mode));

    if (true == monopolar_modKontrol(pH)) {
        
        char varsayilanModStr[64];
        int w = snprintf(varsayilanModStr, sizeof(varsayilanModStr), "%s=", pH->pCfg->pModeGetCmd);

        if ((w > 0) && ((size_t)w < sizeof(varsayilanModStr))) {
            (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), varsayilanModStr, (int16_t)pH->pCfg->varsayilan_mod);
        }
        return;
    }
    
    touch_polGuncelle();

    if ((pH->runtime.user_watt > 0U) && (pH->runtime.user_watt <= pH->pCfg->max_watt)) {
        (void)genx_setPage(UI_PAGE_MAIN);
    }
}

/**
  * @brief Ses seviyesine göre dört ses rölesini ayarlar
  * @param seviye 0-4
  * @retval None
  */
static void touch_sesUygula(uint8_t seviye) {

    HAL_GPIO_WritePin(VOLUME_1_GPIO_Port, VOLUME_1_Pin, (seviye >= 1U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(VOLUME_2_GPIO_Port, VOLUME_2_Pin, (seviye >= 2U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(VOLUME_3_GPIO_Port, VOLUME_3_Pin, (seviye >= 3U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
    HAL_GPIO_WritePin(VOLUME_4_GPIO_Port, VOLUME_4_Pin, (seviye >= 4U) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
  * @brief Nextion'daki kayıtlı programı okur ve tüm ayarları uygular
  * @retval None
  */
static void touch_veriKayitOku(void) {

    Nextion_Status_e status;

    for (uint8_t i = 0U; i < VAR_COUNT; i++) {
        const Get_Eeprom_t *pAdim = &gSaveSteps[i];
        uint8_t ilerleme;

        do {
            status = nextion_getVarBlocking(pAdim->pDst, pAdim->pGetCmd, pAdim->size);
        } while (NEXTION_STATUS_OK != status);

        ilerleme = (uint8_t)(((uint16_t)(i + 1U) * 100U) / VAR_COUNT);

        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "j0.val=", (int16_t)ilerleme);
    }

    (void)genx_sendCmd("S0_acilma.t1.txt=\"OK\"");

    // Kayıtlı mod bu kanalda kapalıysa varsayılana dönülür
    touch_modDogrula();
    touch_polGuncelle();

    // Son uygulanan hand sahibi olur
    if (true == touch_bipolarWattKontrol(&gBipolar1Coag)) {
        touch_bipolarCoagModUygula(&gBipolar1Coag);
    }

    if (true == touch_bipolarWattKontrol(&gBipolar2Coag)) {
        touch_bipolarCoagModUygula(&gBipolar2Coag);
    }

    if ((PLATE_SINGLE != gPlate) && (PLATE_DUAL != gPlate)) {
        
        gPlate = PLATE_SINGLE;
    }

    if (gSesVeri <= 4U) {
        
        gVolumeData = (uint8_t)gSesVeri;
        touch_sesUygula(gVolumeData);
    }

#if (0 != MONO1_CUT_EN_PAPILLOTOMES) || (0 != MONO1_CUT_EN_POLYPECTOMY) || (0 != MONO2_CUT_EN_PAPILLOTOMES) || (0 != MONO2_CUT_EN_POLYPECTOMY)
    monopolar_endoHesap(&gMono1Cut);
    monopolar_endoHesap(&gMono2Cut);
#endif

    genx_delay(50U);
    gVeriKayit = 0U;
    (void)genx_sendCmd("tsw 255,1");
    (void)genx_setPage(UI_PAGE_MAIN);
}

/**
  * @brief Her kanalın kayıtlı modunu doğrular, kapalı modsa geri dönüş moduna çevirir
  * @retval None
  */
static void touch_modDogrula(void) {

    if (true == monopolar_modKontrol(&gMono1Cut)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S3_m1_cut_mod.USER_MODE.val=", (int16_t)gMono1Cut.pCfg->varsayilan_mod);
    }

    if (true == monopolar_modKontrol(&gMono1Coag)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S5_m1_cog_mod.USER_MODE.val=", (int16_t)gMono1Coag.pCfg->varsayilan_mod);
    }

    if (true == monopolar_modKontrol(&gMono2Cut)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S7_m2_cut_mod.USER_MODE.val=", (int16_t)gMono2Cut.pCfg->varsayilan_mod);
    }

    if (true == monopolar_modKontrol(&gMono2Coag)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S9_m2_cog_mod.USER_MODE.val=", (int16_t)gMono2Coag.pCfg->varsayilan_mod);
    }

    if (true == bipolar_modKontrol(&gBipolar1Cut)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S12_b1_cut_mod.USER_MODE.val=", (int16_t)gBipolar1Cut.pCfg->varsayilan_mod);
    }

    if (true == bipolar_modKontrol(&gBipolar1Coag)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S13_b1_cog_mod.USER_MODE.val=", (int16_t)gBipolar1Coag.pCfg->varsayilan_mod);
    }
    
    if (true == bipolar_modKontrol(&gBipolar2Cut)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S16_b2_cut_mod.USER_MODE.val=", (int16_t)gBipolar2Cut.pCfg->varsayilan_mod);
    }

    if (true == bipolar_modKontrol(&gBipolar2Coag)) {
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "S17_b2_cog_mod.USER_MODE.val=", (int16_t)gBipolar2Coag.pCfg->varsayilan_mod);
    }
}

/**
  * @brief Bipolar watt sayfası OK: değeri okur, işler, ana sayfaya yazar ve sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_bipolarWattOk(Bipolar_Handle_t *pH) {

    (void)genx_getVarRetry(&pH->runtime.user_watt, pH->pCfg->pWattGetCmd, sizeof(pH->runtime.user_watt));
    touch_polGuncelle();
    (void)genx_sendCmd(pH->pCfg->pMainSetCmd);
    (void)genx_setPage(UI_PAGE_MAIN);
}

/**
  * @brief Bipolar mod sayfası OK, son kısım: watt geçerliyse ana sayfaya döner
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_bipolarSayfaDonus(const Bipolar_Handle_t *pH) {

    const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(pH->pCfg->function, pH->runtime.user_mode);

    if ((pH->runtime.user_watt > 0U) && (pH->runtime.user_watt <= pModBilgi->max_watt)) {
        (void)genx_setPage(UI_PAGE_MAIN);
    }
}

/**
  * @brief Bipolar COAG kanalının watt değeri modun sınırı içinde mi
  * @param pH Kanal handle'ı
  * @retval true geçerli, false geçersiz
  */
static bool touch_bipolarWattKontrol(const Bipolar_Handle_t *pH) {

    if (NULL == pH) {
        return false;
    }

    const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(pH->pCfg->function, pH->runtime.user_mode);
    
    if (NULL == pModBilgi) {
        return false;
    }

    return ((pH->runtime.user_watt > 0U) && (pH->runtime.user_watt <= pModBilgi->max_watt));
}

/**
  * @brief Aktif bipolar kanalı (hand sahibi, yoksa seçili pedal) bulur ve bayrakları ona göre ayarlar
  * @retval None
  */
static void touch_bipolarRoleUygula(void) {

    const Bipolar_Handle_t *pAktif = NULL;
    const Bipolar_ModeInfo_t *pModBilgi;
    bool ligature = false;
    bool autostop = false;

    if (NULL != gBipolarHandSahibi) {
        pAktif = gBipolarHandSahibi;
    
    } else if (1U == gBipolar2PedalCift) {
        pAktif = &gBipolar2Coag;
    
    } else if (1U == gBipolar1PedalCift) {
        pAktif = &gBipolar1Coag;
    
    } else {
        // aktif kanal yok
    }

    if (NULL != pAktif) {
        pModBilgi = bipolar_modBul(pAktif->pCfg->function, pAktif->runtime.user_mode);

        if (NULL != pModBilgi) {
            ligature = pModBilgi->ligature_mu;
            autostop = (0U != pModBilgi->stop_high);
        }
    }

    gBipolarHand = (NULL != gBipolarHandSahibi) ? 1U : 0U;
    gLigasureStart = (true == ligature) ? 1U : 0U;
    gBipolarAutoStop = (true == autostop) ? 1U : 0U;

    HAL_GPIO_WritePin(HAND_RELAY_GPIO_Port, HAND_RELAY_Pin, (1U == gBipolarHand) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(LIGASURE_R1_GPIO_Port, LIGASURE_R1_Pin | LIGASURE_R2_Pin | LIGASURE_R3_Pin, (true == ligature) ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

/**
  * @brief Kanalın COAG modunu uygular: hand sahipliği, pedal seçimi, röleler, bayraklar
  * @param pH Kanal handle'ı
  * @retval None
  */
static void touch_bipolarCoagModUygula(Bipolar_Handle_t *pH) {

    const Bipolar_ModeInfo_t *pModBilgi = bipolar_modBul(pH->pCfg->function, pH->runtime.user_mode);

    if (NULL != pModBilgi) {
        gAutoStop2 = 0U;

        if (BIPOLAR_TRIGGER_HAND == pModBilgi->trigger) {
            // Son uygulanan hand modu sahip olur, tüm pedallar kapanır
            gBipolarHandSahibi = pH;
            touch_tumPedalKapat(0U);

        } else {

            if (pH == gBipolarHandSahibi) {
                gBipolarHandSahibi = NULL;
            }

            touch_bipolarRoleUygula();
        }
    }
}
