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
#include "genx.h"

/* =========================================================================
 * Mod tanımları
 * ========================================================================= */
typedef enum {
    BIPOLAR_CUTMODE_CUTTING         = 1U,
    BIPOLAR_CUTMODE_SCISSORS        = 2U,
    BIPOLAR_CUTMODE_BIVAPO          = 3U,
    BIPOLAR_CUTMODE_BIREZO          = 4U
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
    BIPOLAR_ISLEV_CUT               = 0U,
    BIPOLAR_ISLEV_COAG              = 1U
} Bipolar_Function_e;

typedef enum {
    BIPOLAR_CATEGORY_STANDARD        = 0U,
    BIPOLAR_CATEGORY_SEAL            = 1U
} Bipolar_Category_e;

typedef enum {
    BIPOLAR_STATE_IDLE              = 0U,
    BIPOLAR_STATE_ACTIVE            = 1U
} Bipolar_State_e;

typedef enum {
    BIPOLAR_TRIGGER_PEDAL           = 0U,
    BIPOLAR_TRIGGER_HAND            = 1U
} Bipolar_Trigger_e;

/* =========================================================================
 * Yapılar
 * ========================================================================= */
 /**
 * @brief Yolun sabit yapılandırması
 */
typedef struct {
    Bipolar_Function_e function;
    Bipolar_Category_e category;
    uint32_t etkin_maske;
    uint32_t varsayilan_mod;
    const char *pWattGetCmd;
    const char *pModeGetCmd;
    const char *pMainSetCmd;
} Bipolar_Config_t;

/**
 * @brief Yolun çalışma anı durumu
 */
typedef struct {
    Core_Channel_t genel;
    Bipolar_State_e durum;
    uint8_t basma;
    uint8_t birakma;
    uint16_t okuma_gecikme;
    uint8_t olcum_sonuc;
    uint8_t formul_ac;
    uint8_t koruma_start;
    bool yeniden_hesap;
    uint32_t koruma_veri;
    float watt;
    uint8_t autostop_etkin;
    uint32_t user_watt;
    uint32_t user_mode;
    float pol;
} Bipolar_Runtime_t;

typedef struct {
    const Bipolar_Config_t *pCfg;
    Bipolar_Runtime_t runtime;
} Bipolar_Handle_t;

/**
 * @brief Mod başına sabit parametreler
 */
typedef struct {
    Bipolar_Function_e function;
    uint32_t modeId;
    Bipolar_Trigger_e trigger;
    uint32_t max_watt;
    uint32_t pol_limit;
    uint32_t hv_min_watt;
    uint32_t hv_max_watt;
    uint32_t stop_high; // 0 = autostop yok
    uint32_t stop_low;
    bool ligature_mu;
} Bipolar_ModeInfo_t;

/* =========================================================================
 * Mod açma/kapama maskeleri
 * ========================================================================= */
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


/* ================================================================
 * GPIO okuma makroları
 * ================================================================*/
#define BIPOLAR1_CUT_AYAK_BASILI()      ((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar1PedalCift == 1U))
#define BIPOLAR1_CUT_AYAK_SERBEST()     ((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar1PedalCift == 1U))

#define BIPOLAR1_COAG_AYAK_BASILI()     (GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10)) && (gBipolar1PedalCift == 1U))
#define BIPOLAR1_COAG_AYAK_SERBEST()    (GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10)) && (gBipolar1PedalCift == 1U))

#define BIPOLAR2_CUT_LIGASURE_AYAK_BASILI()		((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar2PedalCift == 1U))
#define BIPOLAR2_CUT_LIGASURE_AYAK_SERBEST()	((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar2PedalCift == 1U))

#define BIPOLAR2_COAG_LIGASURE_AYAK_BASILI()	((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gBipolar2PedalCift == 1U))
#define BIPOLAR2_COAG_LIGASURE_AYAK_SERBEST()	((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gBipolar2PedalCift == 1U))

#define BIPOLAR_TEK_PEDAL_BASILI()      (GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1)) && (0U == gBipolarHand))
#define BIPOLAR_TEK_PEDAL_SERBEST()     (GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1)) && (0U == gBipolarHand))

#define BIPOLAR_HAND_BASILI()           ((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1))) \
                                        && (GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOD), (GPIO_PIN_0))) && (1U == gBipolarHand))
#define BIPOLAR_HAND_SERBEST()          ((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1))) \
                                        && (GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOD), (GPIO_PIN_0))) && (1U == gBipolarHand))


extern Bipolar_Handle_t gBipolar1Cut;
extern Bipolar_Handle_t gBipolar1Coag;
extern Bipolar_Handle_t gBipolar2Cut;
extern Bipolar_Handle_t gBipolar2Coag;
extern Bipolar_Handle_t *gBipolarHandSahibi;

/* =========================================================================
 * Genel API
 * ========================================================================= */
void bipolar_init(void);
bool bipolar_modEtkinMi(const Bipolar_Handle_t *hbip, uint32_t modeId);
void bipolar_polHesap(Bipolar_Handle_t *hbip);
bool bipolar_modKontrol(Bipolar_Handle_t *hbip);
const Bipolar_ModeInfo_t *bipolar_modBul(Bipolar_Function_e func, uint32_t modeId);


#ifdef __cplusplus
}
#endif

#endif /* BIPOLAR_H */