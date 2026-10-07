/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    modules.h
 * @brief   Her modülün tek giriş noktası. `main` döngüsü yalnızca bunları çağırır.
 *
 * @author  destrocore
 * @date    2026
 */

#ifndef MODULES_H
#define MODULES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include "main.h"

/* =========================================================================
 * Tanımlar ve Makrolar
 * ========================================================================= */


// -- Plate tanımları ------------------------------------------
typedef enum {
    PLATE_SINGLE    = 1U,
    PLATE_DUAL      = 2U
} Plate_e;

// -- Built-in Self-test tanımları -----------------------------
typedef enum {
    BIST_PWR_SUPPLY     = 0U,
    BIST_MONO_BOARD     = 1U,
    BIST_BIPOLAR_BOARD  = 2U,
    BIST_OK             = 3U
} Bist_State_e;

// -- Alarm ses tanımları --------------------------------------
typedef enum {
    ALARM_SES_NONE          = 0U,
    ALARM_SES_MONO_CUT      = 1U,
    ALARM_SES_MONO_COAG     = 2U,
    ALARM_SES_BIPOLAR_CUT   = 3U,
    ALARM_SES_BIPOLAR_COAG  = 4U
} Alarm_Ses_e;

// -- Alarm yapılandırma ---------------------------------------
typedef struct {
    Alarm_Ses_e sesTonu;
    uint16_t PSC;
    const char *pErrorMsg;
} Alarm_Config_t;

static const Alarm_Config_t gAlarmConfigTable[] = {
    { ALARM_SES_MONO_CUT,       210U, NULL },
    { ALARM_SES_MONO_COAG,      280U, NULL },
    { ALARM_SES_BIPOLAR_CUT,    320U, NULL },
    { ALARM_SES_BIPOLAR_COAG,   390U, NULL }
};

#define ALARM_COUNT ((uint8_t)(sizeof(gAlarmConfigTable) / sizeof(gAlarmConfigTable[0])))

typedef enum {
    GUCU_KAPALI = 0U,
    GUCU_ACIK   = 1U
} Mode_Start_e;

/* ================================================================
 * Bipolar pedal / hand GPIO okuma makroları
 * (Monopolar kalem/pedal pinleri artık monopolar.c yapılandırmasında)
 * ================================================================*/

#define BIPOLAR1_CUT_AYAK_BASILI()              ((GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) && (1U == gBipolar1PedalCift))
#define BIPOLAR1_CUT_AYAK_SERBEST()             ((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) && (1U == gBipolar1PedalCift))

#define BIPOLAR1_COAG_AYAK_BASILI()             ((GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10)) && (1U == gBipolar1PedalCift))
#define BIPOLAR1_COAG_AYAK_SERBEST()            ((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10)) && (1U == gBipolar1PedalCift))

#define BIPOLAR2_CUT_LIGASURE_AYAK_BASILI()     ((GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) && (1U == gBipolar2PedalCift))
#define BIPOLAR2_CUT_LIGASURE_AYAK_SERBEST()    ((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_9)) && (1U == gBipolar2PedalCift))

#define BIPOLAR2_COAG_LIGASURE_AYAK_BASILI()    ((GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10)) && (1U == gBipolar2PedalCift))
#define BIPOLAR2_COAG_LIGASURE_AYAK_SERBEST()   ((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10)) && (1U == gBipolar2PedalCift))

#define BIPOLAR_TEK_PEDAL_BASILI()              ((GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)) && (0U == gBipolarHand))
#define BIPOLAR_TEK_PEDAL_SERBEST()             ((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)) && (0U == gBipolarHand))

#define BIPOLAR_HAND_BASILI()                   ((GPIO_PIN_RESET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)) \
                                                && (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0)) && (1U == gBipolarHand))
#define BIPOLAR_HAND_SERBEST()                  ((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_1)) \
                                                && (GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOD, GPIO_PIN_0)) && (1U == gBipolarHand))

/* =========================================================================
 * Genel API (main döngüsü giriş noktaları)
 * ========================================================================= */

void monopolar_run(void);
void bipolar_run(void);
void input_run(void);
void alarm_run(void);
void plate_run(void);
void error_run(void);
void touch_run(void);
void selftest_run(void);

#ifdef __cplusplus
}
#endif

#endif /* MODULES_H */