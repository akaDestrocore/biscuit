/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    monopolar.h
 * @brief   Monopolar kanal tanımları.
 *
 * @author  destrocore
 * @date    2026
 */

#ifndef MONOPOLAR_H
#define MONOPOLAR_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>
#include "genx.h"

/* =========================================================================
 * Mod tanımları
 * ========================================================================= */

typedef enum {
    MONOPOLAR_CUTMODE_CUT           = 1U,
    MONOPOLAR_CUTMODE_BLEND1        = 2U,
    MONOPOLAR_CUTMODE_BLEND2        = 3U,
    MONOPOLAR_CUTMODE_BLEND3        = 4U,
    MONOPOLAR_CUTMODE_POLYPECTOMY   = 5U,
    MONOPOLAR_CUTMODE_PAPILLOTOMES  = 6U,
    MONOPOLAR_CUTMODE_RESECTION1    = 11U,
    MONOPOLAR_CUTMODE_RESECTION2    = 12U
} Monopolar_CutMode_e;

typedef enum {
    MONOPOLAR_COAGMODE_CONTACT      = 1U,
    MONOPOLAR_COAGMODE_SPRAY1       = 2U,
    MONOPOLAR_COAGMODE_SPRAY2       = 3U,
    MONOPOLAR_COAGMODE_SPRAY3       = 4U,
    MONOPOLAR_COAGMODE_FULGURATION  = 5U
} Monopolar_CoagMode_e;

typedef enum {
    MONOPOLAR_ISLEV_CUT     = 0U,
    MONOPOLAR_ISLEV_COAG    = 1U
} Monopolar_Function_e;

typedef enum {
    MONOPOLAR_DURUM_BOSTA   = 0U,
    MONOPOLAR_DURUM_AKTIF   = 1U
} Monopolar_State_e;

/* ============================================================
 * Mod açma/kapama maskeleri
 * ============================================================*/

#define MONO1_CUT_MASK ( MODE_BIT(MONOPOLAR_CUTMODE_CUT, MONO1_CUT_EN_CUT) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND1, MONO1_CUT_EN_BLEND1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND2, MONO1_CUT_EN_BLEND2) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND3, MONO1_CUT_EN_BLEND3) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_POLYPECTOMY, MONO1_CUT_EN_POLYPECTOMY) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_PAPILLOTOMES, MONO1_CUT_EN_PAPILLOTOMES) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION1, MONO1_CUT_EN_RESECTION1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION2, MONO1_CUT_EN_RESECTION2) )


#define MONO2_CUT_MASK ( MODE_BIT(MONOPOLAR_CUTMODE_CUT, MONO2_CUT_EN_CUT) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND1, MONO2_CUT_EN_BLEND1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND2, MONO2_CUT_EN_BLEND2) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND3, MONO2_CUT_EN_BLEND3) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_POLYPECTOMY, MONO2_CUT_EN_POLYPECTOMY) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_PAPILLOTOMES, MONO2_CUT_EN_PAPILLOTOMES) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION1, MONO2_CUT_EN_RESECTION1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION2, MONO2_CUT_EN_RESECTION2) )


#define MONO1_COAG_MASK ( MODE_BIT(MONOPOLAR_COAGMODE_CONTACT, MONO1_COAG_EN_CONTACT) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY1, MONO1_COAG_EN_SPRAY1) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY2, MONO1_COAG_EN_SPRAY2) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY3, MONO1_COAG_EN_SPRAY3) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_FULGURATION, MONO1_COAG_EN_FULGURATION) )


#define MONO2_COAG_MASK ( MODE_BIT(MONOPOLAR_COAGMODE_CONTACT, MONO2_COAG_EN_CONTACT) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY1, MONO2_COAG_EN_SPRAY1) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY2, MONO2_COAG_EN_SPRAY2) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY3, MONO2_COAG_EN_SPRAY3) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_FULGURATION, MONO2_COAG_EN_FULGURATION) )

/* =========================================================================
 * Handle yapıları
 * ========================================================================= */
typedef struct {
    GPIO_TypeDef *pPort;
    uint16_t pin;
} Monopolar_Pin_t;

/**
 * @brief Kanalın yapılandırması
 */
typedef struct {
    Monopolar_Function_e function;
    uint16_t relay_pin;
    uint8_t mux_ch;                 // MUX_MONO_CUT / MUX_MONO_COAG
    Monopolar_Pin_t led_beyaz;
    Monopolar_Pin_t led_aktif;      // sarı / mavi
    Monopolar_Pin_t kalem_pin;
    Monopolar_Pin_t pedal_pin;
    const uint8_t *pSeciliPedal;  // kanal bazlı pedal seçimi
    uint8_t *pHataBayrak;            // kanal bazlı koruma hatası bayrağı
    uint32_t etkin_maske;
    uint32_t varsayilan_mod;        // kayıtlı mod kapalıysa dönülecek mod
    uint32_t max_watt;
    const char *pWattGetCmd;
    const char *pModeGetCmd;
    const char *pMainSetCmd;
} Monopolar_Config_t;

/**
 * @brief Kanalın çalışma anı durumu
 */
typedef struct {
    Core_Channel_t genel;
    volatile Monopolar_State_e durum;   // TIM5 kesmesi okur
    uint32_t basma;
    uint32_t birakma;
    uint8_t hata;
    uint32_t guc_tick;
    uint8_t yuz_start;
    uint32_t user_watt;
    uint32_t user_mode;
    float pol;                          // işlenmiş watt değeri
} Monopolar_Runtime_t;

/**
 * @brief Kanalın çalışma anı durumu
 */
typedef struct {
    const Monopolar_Config_t *pCfg;
    Monopolar_Runtime_t runtime;
} Monopolar_Handle_t;

/* ============================================================
 * TIM5 (endocut)
 * ============================================================*/
extern Monopolar_Handle_t * volatile gEndoMutex;
extern volatile uint8_t gEndoCutSay;
extern volatile float gEndoCutWattPol;
extern volatile float gEndoCoagWattPol;
extern volatile uint32_t gEndoCutTicks;
extern volatile uint16_t gEndoCoagTimer;

extern Monopolar_Handle_t gMono1Cut;
extern Monopolar_Handle_t gMono1Coag;
extern Monopolar_Handle_t gMono2Cut;
extern Monopolar_Handle_t gMono2Coag;

/* =========================================================================
 * Genel API
 * ========================================================================= */
void monopolar_init(void);
bool monopolar_modEtkinMi(const Monopolar_Handle_t *pH, uint32_t modeId);
void monopolar_girisTara(Monopolar_Handle_t *pH);
void monopolar_polHesap(Monopolar_Handle_t *pH);
void monopolar_endoHesap(const Monopolar_Handle_t *pH);
bool monopolar_modKontrol(Monopolar_Handle_t *pH);



#ifdef __cplusplus
}
#endif

#endif /* MONOPOLAR_H */