/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    core.h
 * @brief   Modüller arası paylaşılan durum ve donanım yardımcıları
 *
 * @author  destrocore
 * @date    2026
 */

#ifndef CORE_H
#define CORE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <math.h>
#include "genx.h"

#define CORE_MAX_CH 				8U // toplam mono/bipolar yol sayısı
#define NEXTION_MAX_DENEME 			10U
#define GENX_KUYRUK_BEKLE_MS		20U
#define ALARM_BEEP_TOTAL_TICKS     	375U

extern char gNumBuff[NEXTION_CMD_MAX_LEN + 1U];

extern DAC_HandleTypeDef hdac;
extern TIM_HandleTypeDef htim2;
extern TIM_HandleTypeDef htim3;
extern TIM_HandleTypeDef htim4;
extern TIM_HandleTypeDef htim5;
extern TIM_HandleTypeDef htim9;
extern TIM_HandleTypeDef htim13;

/**
 * @brief Her kanalın ilk alanı olan ortak değişken. Start bayrağı ve sahiplik bu değişken üzerinden yönetilir
 */
typedef struct {
    volatile uint8_t start;
} Core_Channel_t;

// -- Ölçüm / DAC ----------------------------------------------
typedef enum {
    ADC_CH_MONO_PWR     = 0U,
    ADC_CH_PLATE_RES    = 1U,
    ADC_CH_PWR          = 2U,
    ADC_CH_BIPOLAR_PWR  = 3U,
    ADC_CH_NTC          = 4U,
    ADC_CH_COUNT              
} ADC_Channel_e;

extern volatile uint16_t gAdc1DmaBuff[ADC_CH_COUNT];
extern volatile uint8_t gDacVeri;
extern bool gPwrYeni;
extern uint32_t gHighVolt;

#define MODE_BIT(modeId, AcikMi)     ((1U == (AcikMi)) ? (1UL << (modeId)) : 0UL)

typedef enum {
    MUX_MONO_CUT      = 1U,
    MUX_MONO_COAG     = 2U,
    MUX_BIPOLAR_CUT   = 3U,
    MUX_BIPOLAR_COAG  = 5U
} Mux_Combo_e;

typedef enum {
    GENX_SUCCESS  	= 0,
    GENX_ERROR    	= -1
} Genx_Status_t;

/**
 * @brief `BIPOLAR_MODE_CUT` veya `BIPOLAR_MODE_COAG`
 */
typedef enum {
  BIPOLAR_MODE_COAG = 0U,
  BIPOLAR_MODE_CUT  = 1U
} Bipolar_Mode_e;

/**
 * @brief Sayfa tanımları
 */
typedef enum {
    UI_PAGE_BOOT                = 0U,
    UI_PAGE_MAIN               	= 1U,
    UI_PAGE_MONO1_CUT_WATT     	= 2U,
    UI_PAGE_MONO1_CUT_MODE     	= 3U,
    UI_PAGE_MONO1_COAG_WATT    	= 4U,
    UI_PAGE_MONO1_COAG_MODE    	= 5U,
    UI_PAGE_MONO2_CUT_WATT     	= 6U,
    UI_PAGE_MONO2_CUT_MODE     	= 7U,
    UI_PAGE_MONO2_COAG_WATT    	= 8U,
    UI_PAGE_MONO2_COAG_MODE    	= 9U,
    UI_PAGE_BIPOLAR1_CUT_WATT  	= 10U,
    UI_PAGE_BIPOLAR1_COAG_WATT 	= 11U,
    UI_PAGE_BIPOLAR1_CUT_MODE  	= 12U,
    UI_PAGE_BIPOLAR1_COAG_MODE 	= 13U,
    UI_PAGE_BIPOLAR2_CUT_WATT  	= 14U,
    UI_PAGE_BIPOLAR2_COAG_WATT 	= 15U,
    UI_PAGE_BIPOLAR2_CUT_MODE  	= 16U,
    UI_PAGE_BIPOLAR2_COAG_MODE 	= 17U,
    UI_PAGE_PLATE              	= 18U,
    UI_PAGE_LOG                	= 19U,
    UI_PAGE_MENU_SETTINGS       = 20U,
    UI_PAGE_PROGRAM            	= 21U,
    UI_PAGE_VOLUME             	= 25U,
    UI_PAGE_ERROR              	= 29U,
    UI_PAGE_MONO1_ENDO         	= 31U
} UI_Page_e;

// -- Sayfa tanımları ------------------------------------------
#define NEXTION_TOUCH_KEY(page, componentId) ((uint16_t)(((uint16_t)(page) << 8U) | (uint16_t)(componentId)))

typedef enum {
	S1__MONO1_CUT_MODE_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 15U),
	S1__MONO1_CUT_WATT_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 17U),
	S1__MONO1_COAG_MODE_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 16U),
	S1__MONO1_COAG_WATT_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 18U),
	S1__BIPOLAR1_CUT_MODE_BUTTON 		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 29U),
	S1__BIPOLAR1_CUT_WATT_BUTTON 		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 23U),
	S1__BIPOLAR1_COAG_MODE_BUTTON		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 30U),
	S1__BIPOLAR1_COAG_WATT_BUTTON		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 24U),
	S1__BIPOLAR2_CUT_MODE_BUTTON 		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 27U),
	S1__BIPOLAR2_CUT_WATT_BUTTON 		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 21U),
	S1__BIPOLAR2_COAG_MODE_BUTTON		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 28U),
	S1__BIPOLAR2_COAG_WATT_BUTTON		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 22U),
	S1__BIPOLAR_RF_CUT_MODE_BUTTON  	= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 27U),
	S1__BIPOLAR_RF_CUT_WATT_BUTTON  	= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 21U),
	S1__BIPOLAR_RF_COAG_MODE_BUTTON		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 28U),
	S1__BIPOLAR_RF_COAG_WATT_BUTTON		= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 22U),
	S1__MONO2_CUT_MODE_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 25U),
	S1__MONO2_CUT_WATT_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 19U),
	S1__MONO2_COAG_MODE_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 26U),
	S1__MONO2_COAG_WATT_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 20U),
	S1__MENU_BUTTON_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 9U),
	S1__LOG_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 10U),
	S1__PROGRAM_FAV_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 11U),
	S1__PLATE_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 12U),
	S1__SELECTED_PROG_FIELD				= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 14U),
	S1__MONO1_PEDAL_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 5U),
	S1__BIPOLAR1_PEDAL_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 8U),
	S1__BIPOLAR2_PEDAL_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 6U),
	S1__BIPOLAR_RF_PEDAL_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 7U),
	S1__SEAL_PEDAL_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 7U),
	S1__BIPOLAR_PEDAL_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 8U),
	S1__MONO2_PEDAL_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 6U),

	S2__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_CUT_WATT, 7U),
	S2__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_CUT_WATT, 8U),

	S3__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_CUT_MODE, 24U),
	S3__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_CUT_MODE, 25U),
	S3__POLYPECTOMY_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_CUT_MODE, 12U),
	S3__PAPILLOTOMES_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_CUT_MODE, 18U),

	S4__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_COAG_WATT, 13U),
	S4__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_COAG_WATT, 12U),

	S5__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_COAG_MODE, 16U),
	S5__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_COAG_MODE, 17U),

	S6__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_CUT_WATT, 7U),
	S6__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_CUT_WATT, 8U),

	S7__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_CUT_MODE, 16U),
	S7__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_CUT_MODE, 17U),

	S8__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_COAG_WATT, 7U),
	S8__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_COAG_WATT, 8U),

	S9__BACK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_COAG_MODE, 16U),
	S9__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO2_COAG_MODE, 17U),

	S10__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_CUT_WATT, 12U),
	S10__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_CUT_WATT, 13U),

	S11__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_COAG_WATT, 12U),
	S11__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_COAG_WATT, 13U),

	S12__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_CUT_MODE, 16U),
	S12__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_CUT_MODE, 17U),

	S13__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_COAG_MODE, 16U),
	S13__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR1_COAG_MODE, 17U),

	S14__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_CUT_WATT, 12U),
	S14__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_CUT_WATT, 13U),

	S15__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_COAG_WATT, 12U),
	S15__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_COAG_WATT, 13U),

	S16__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_CUT_MODE, 16U),
	S16__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_CUT_MODE, 17U),

	S17__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_COAG_MODE, 14U),
	S17__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_BIPOLAR2_COAG_MODE, 15U),

	S18__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_PLATE, 4U),
	S18__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_PLATE, 5U),
	S18__DUALPLATE_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_PLATE, 11U),
	S18__SINGLEPLATE_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_PLATE, 13U),

	S19__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_LOG, 2U),
	S19__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_LOG, 3U),

	S20__PROGRAM_FAV_BUTTON				= (uint16_t)S1__PROGRAM_FAV_BUTTON,
	S20__SYS_INFO_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_MENU_SETTINGS, 3U),
	S20__SYS_SETTINGS_BUTTON			= NEXTION_TOUCH_KEY(UI_PAGE_MENU_SETTINGS, 4U),
	S20__VOLUME_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_MENU_SETTINGS, 5U),
	S20__SERVICE_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_MENU_SETTINGS, 6U),
	S20__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_MENU_SETTINGS, 1U),

	S21__SAVE_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_PROGRAM, 23U),
	S21__BACK_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_PROGRAM, 3U),
	S21__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_PROGRAM, 5U),
	S21__EDIT_BUTTON					= NEXTION_TOUCH_KEY(UI_PAGE_PROGRAM, 22U),

	S22__BACK_BUTTON					= NEXTION_TOUCH_KEY(22U, 3U),
	S22__OK_BUTTON						= NEXTION_TOUCH_KEY(22U, 4U),

	S23__BACK_BUTTON					= NEXTION_TOUCH_KEY(23U, 1U),

	S24__BACK_BUTTON					= NEXTION_TOUCH_KEY(24U, 1U),

	S25__DECREASE_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_VOLUME, 4U),
	S25__INCREASE_BUTTON				= NEXTION_TOUCH_KEY(UI_PAGE_VOLUME, 5U),

	S29__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_ERROR, 2U),

	S31__OK_BUTTON						= NEXTION_TOUCH_KEY(UI_PAGE_MONO1_ENDO, 24U)
} Display_Component_e;


// -- Kanal bazlı pedal/hand/ligasure bayrakları ---------------
extern uint8_t gMono1Pedal;
extern uint8_t gMono2Pedal;
extern uint32_t gLigasureWattYaz;
extern uint8_t gBipolarAutoStop;
extern uint8_t gLigasureStart;
extern uint8_t gLigasurePedal;
extern uint8_t gAutoStop2;
extern uint8_t gBipolar1PedalCift;
extern uint8_t gBipolar2PedalCift;
extern uint8_t gBipolarHand;
extern uint8_t gAutoStopTekPedal;
extern uint8_t gAutoStopCiftPedal;

// -- Hata bayrakları ------------------------------------------
extern uint8_t gMono1Koruma;
extern uint8_t gMono2Koruma;
extern uint8_t gPwrKoruma;
extern uint8_t gMonoPwrKoruma;
extern uint8_t gPedalError3;
extern uint8_t gBipolarKoruma;
extern uint8_t gBiPwrKoruma;

// -- Sayfa/plate/alarm ----------------------------------------
extern uint8_t gPlate100Overresistance;
extern uint8_t gMonopolarCutSes;
extern uint8_t gMonopolarCoagSes;
extern uint8_t gAlarmPlate;
extern UI_Page_e gAcikSayfa;
extern uint32_t gPlate;
extern uint8_t gPlateError;
extern uint8_t gDusukOncelikAlarm;
extern uint32_t gDusukAlarmBasla;
extern uint8_t gBipolarCutSes;
extern uint8_t gBipolarCoagSes;
extern volatile uint16_t gBeepTick;
extern volatile bool gBeepAktif;

// -- Nextion'dan okunan ham değerler --------------------------
extern uint32_t gEndoCutKademe;
extern uint32_t gEndoCoagKademe;
extern uint32_t gEndoCutSure;
extern uint32_t gEndoCoagSure;
extern uint32_t gSesVeri;

/* =========================================================================
 * Genel API
 * ========================================================================= */

void genx_dacUygula(float val);
void genx_kanalUygula(Core_Channel_t *pCh);
Genx_Status_t genx_sahiplendir(const Core_Channel_t *pSahip);
void genx_muxSecimi(uint8_t ch);
void genx_monoAc(uint16_t relayPin, uint8_t muxCh);
void genx_bipolarAc(Bipolar_Mode_e mode);
void genx_bipolarKapat(void);
void genx_serbestBirak(const Core_Channel_t *pSahip);
Genx_Status_t genx_sayacBittiMi(bool kosul, uint32_t *pBasla);
bool genx_baskaAktifVarMi(const Core_Channel_t *pSelf);
void genx_hepsiniDurdur(void);
void genx_PwrDegeriniOku(void);

// -- Nextion yardımcıları
Genx_Status_t genx_sendCmd(const char *pCmd);
Genx_Status_t genx_sendNum(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num);
Genx_Status_t genx_getVarRetry(void *pDst, const char *pCmd, uint16_t size);
Genx_Status_t genx_setPage(uint16_t page);
void genx_delay(uint32_t ms);

#ifdef __cplusplus
}
#endif

#endif /* CORE_H */