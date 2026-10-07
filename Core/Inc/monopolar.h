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
#include "core.h"
#include "modules.h"
#include "channels_cfg.h"

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
    MONOPOLAR_FUNCTION_CUT  = 0U,
    MONOPOLAR_FUNCTION_COAG = 1U
} Monopolar_Function_e;

typedef enum {
    MONOPOLAR_STATE_IDLE    = 0U,
    MONOPOLAR_STATE_ACTIVE  = 1U
} Monopolar_State_e;

/* =========================================================================
 * Handle yapıları
 * ========================================================================= */
typedef struct {
    GPIO_TypeDef *pPort;
    uint16_t pin;
} Monopolar_Pin_t;

/**
 * @brief Yolun sabit (flash) yapılandırması
 */
typedef struct {
    Monopolar_Function_e function;
    uint16_t relay_pin;                  // PD1 (Mono1) / PD3 (Mono2)
    uint8_t mux_channel;                 // MUX_MONO_CUT / MUX_MONO_COAG
    Monopolar_Pin_t led_white;
    Monopolar_Pin_t led_active;          // sarı (CUT) / mavi (COAG)
    Monopolar_Pin_t stick_pin;
    Monopolar_Pin_t foot_pin;
    const uint8_t *pPedalSelected;      // kanal bazlı pedal seçimi (CUT ve COAG paylaşır)
    uint8_t *pFaultFlag;                // kanal bazlı koruma hatası bayrağı
    uint32_t enabled_mask;               // bit n = modeId n açık (channels_cfg.h anahtarlarından)
    uint32_t fallback_mode;              // kayıtlı mod kapalıysa dönülecek mod
    uint32_t max_watt;                   // ekran OK kontrolü
    const char *pWattGetCmd;
    const char *pModeGetCmd;
    const char *pMainSetCmd;
} Monopolar_Config_t;

/**
 * @brief Yolun çalışma anı durumu (RAM)
 */
typedef struct {
    Core_Channel_t common;              // İLK ALAN: start bayrağı (kayıt/sahiplik bunu kullanır)
    volatile Monopolar_State_e state;   // TIM5 kesmesi okur
    uint8_t basma;
    uint8_t birakma;
    uint8_t hata;
    uint16_t guc_gecikme;
    uint8_t yuz_start;
    uint32_t user_watt;                  // uint32_t kalmalı: nextion_getVarBlocking 32 bit yazar
    uint32_t user_mode;
    float pol;                          // işlenmiş watt
} Monopolar_Runtime_t;

typedef struct {
    const Monopolar_Config_t *pCfg;
    Monopolar_Runtime_t canli;
} Monopolar_Handle_t;

extern Monopolar_Handle_t gMono1Cut;
extern Monopolar_Handle_t gMono1Coag;
extern Monopolar_Handle_t gMono2Cut;
extern Monopolar_Handle_t gMono2Coag;

/* =========================================================================
 * Genel API (monopolar_run() modules.h içinde)
 * ========================================================================= */
void monopolar_init(void);
void monopolar_pollInput(Monopolar_Handle_t *pH);
bool monopolar_isModeEnabled(const Monopolar_Handle_t *pH, uint32_t modeId);
bool monopolar_isModeCorrected(Monopolar_Handle_t *pH);
bool monopolar_isEndoMode(const Monopolar_Handle_t *pH);
void monopolar_updatePol(Monopolar_Handle_t *pH);
void monopolar_endoApply(const Monopolar_Handle_t *pH);

#ifdef __cplusplus
}
#endif

#endif /* MONOPOLAR_H */