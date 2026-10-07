/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    bipolar.h
 * @brief   Bipolar tanımları.
 *
 * @author  destrocore
 * @date    2026
 */

#ifndef BIPOLAR_H
#define BIPOLAR_H

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
    BIPOLAR_CUTMODE_CUTTING     = 1U,
    BIPOLAR_CUTMODE_SCISSORS    = 2U,
    BIPOLAR_CUTMODE_BIVAPO      = 3U,
    BIPOLAR_CUTMODE_BIREZO      = 4U
} Bipolar_CutMode_e;

typedef enum {
    BIPOLAR_STD_COAGMODE_STANDARD    = 1U,
    BIPOLAR_STD_COAGMODE_FORCED      = 2U,
    BIPOLAR_STD_COAGMODE_STOP        = 3U,
    BIPOLAR_STD_COAGMODE_START       = 4U,
} Bipolar_StdCoagMode_e;

typedef enum {
    BIPOLAR_SEAL_COAGMODE_LIGATION   = 6U,
    BIPOLAR_SEAL_COAGMODE_SEALSURE   = 7U,
    BIPOLAR_SEAL_COAGMODE_TISSUELOCK = 8U
} Bipolar_SealCoagMode_e;

typedef enum {
    BIPOLAR_FUNCTION_CUT    = 0U,
    BIPOLAR_FUNCTION_COAG   = 1U
} Bipolar_Function_e;

typedef enum {
    BIPOLAR_PROFILE_STANDARD    = 0U,
    BIPOLAR_PROFILE_SEAL        = 1U
} Bipolar_Profile_e;

typedef enum {
    BIPOLAR_STATE_IDLE      = 0U,
    BIPOLAR_STATE_ACTIVE    = 1U
} Bipolar_State_e;

/* =========================================================================
 * Yapılar
 * ========================================================================= */

 /**
 * @brief Yolun sabit yapılandırması
 */
typedef struct {
    Bipolar_Function_e function;
    Bipolar_Profile_e profile;
    uint32_t enabled_mask;
    uint32_t fallback_mode;
    uint32_t max_watt;
    const char *pWattGetCmd;
    const char *pModeGetCmd;
    const char *pMainSetCmd;
} Bipolar_Config_t;

/**
 * @brief Yolun çalışma anı durumu
 */
typedef struct {
    Core_Channel_t common;
    Bipolar_State_e state;
    uint8_t basma;
    uint8_t birakma;
    uint16_t okuma_gecikme;
    uint8_t olcum_sonuc;
    uint8_t formul_ac;
    uint8_t koruma_start;
    bool yeniden_hesap;
    uint32_t koruma_veri;
    float watt;
    uint8_t autostop_armed;
    uint32_t user_watt;
    uint32_t user_mode;
    float pol;
} Bipolar_Runtime_t;

typedef struct {
    const Bipolar_Config_t *pCfg;
    Bipolar_Runtime_t canli;
} Bipolar_Handle_t;


/* ============================================================
 * Mod açma/kapama maskeleri
 * ============================================================*/

#define BIPOLAR1_CUT_MASK ( MODE_BIT(BIPOLAR_CUTMODE_CUTTING,   BIPOLAR1_CUT_EN_CUTTING) \
                            | MODE_BIT(BIPOLAR_CUTMODE_SCISSORS,  BIPOLAR1_CUT_EN_SCISSORS) \
                            | MODE_BIT(BIPOLAR_CUTMODE_BIVAPO,    BIPOLAR1_CUT_EN_BIVAPO) \
                            | MODE_BIT(BIPOLAR_CUTMODE_BIREZO,    BIPOLAR1_CUT_EN_BIREZO) )


#define BIPOLAR2_CUT_MASK ( MODE_BIT(BIPOLAR_CUTMODE_CUTTING,   BIPOLAR2_CUT_EN_CUTTING) \
                            | MODE_BIT(BIPOLAR_CUTMODE_SCISSORS,  BIPOLAR2_CUT_EN_SCISSORS) \
                            | MODE_BIT(BIPOLAR_CUTMODE_BIVAPO,    BIPOLAR2_CUT_EN_BIVAPO) \
                            | MODE_BIT(BIPOLAR_CUTMODE_BIREZO,    BIPOLAR2_CUT_EN_BIREZO) )


#define BIPOLAR1_COAG_MASK ( MODE_BIT(BIPOLAR_STD_COAGMODE_STANDARD,  BIPOLAR1_COAG_EN_STANDARD) \
                            | MODE_BIT(BIPOLAR_STD_COAGMODE_FORCED,    BIPOLAR1_COAG_EN_FORCED) \
                            | MODE_BIT(BIPOLAR_STD_COAGMODE_STOP,      BIPOLAR1_COAG_EN_STOP) \
                            | MODE_BIT(BIPOLAR_STD_COAGMODE_START,     BIPOLAR1_COAG_EN_START) \
                            | MODE_BIT(BIPOLAR_SEAL_COAGMODE_LIGATION,  BIPOLAR1_COAG_EN_LIGATION) \
                            | MODE_BIT(BIPOLAR_SEAL_COAGMODE_SEALSURE,  BIPOLAR1_COAG_EN_SEALSURE) \
                            | MODE_BIT(BIPOLAR_SEAL_COAGMODE_TISSUELOCK, BIPOLAR1_COAG_EN_TISSUELOCK))

#define BIPOLAR2_COAG_MASK ( MODE_BIT(BIPOLAR_STD_COAGMODE_STANDARD,  BIPOLAR2_COAG_EN_STANDARD) \
                            | MODE_BIT(BIPOLAR_STD_COAGMODE_FORCED,    BIPOLAR2_COAG_EN_FORCED) \
                            | MODE_BIT(BIPOLAR_STD_COAGMODE_STOP,      BIPOLAR2_COAG_EN_STOP) \
                            | MODE_BIT(BIPOLAR_STD_COAGMODE_START,     BIPOLAR2_COAG_EN_START) \
                            | MODE_BIT(BIPOLAR_SEAL_COAGMODE_LIGATION,     BIPOLAR2_COAG_EN_LIGATION) \
                            | MODE_BIT(BIPOLAR_SEAL_COAGMODE_SEALSURE,     BIPOLAR2_COAG_EN_SEALSURE) \
                            | MODE_BIT(BIPOLAR_SEAL_COAGMODE_TISSUELOCK,   BIPOLAR2_COAG_EN_TISSUELOCK) )


extern Bipolar_Handle_t gBipolar1Cut;
extern Bipolar_Handle_t gBipolar1Coag;
extern Bipolar_Handle_t gBipolar2Cut;
extern Bipolar_Handle_t gBipolar2Coag;

/* =========================================================================
 * Genel API
 * ========================================================================= */
void bipolar_init(void);
bool bipolar_isModeEnabled(const Bipolar_Handle_t *hbip, uint32_t modeId);
bool bipolar_isModeCorrected(Bipolar_Handle_t *hbip);
void bipolar_updatePol(Bipolar_Handle_t *hbip);


#ifdef __cplusplus
}
#endif

#endif /* BIPOLAR_H */