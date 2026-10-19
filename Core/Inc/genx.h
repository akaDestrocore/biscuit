/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    genx.h
 * @brief   Her modülün tek giriş noktası
 *
 * @author  destrocore
 * @date    2026
 */

#ifndef GENX_H
#define GENX_H

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

#include "nextion.h"
#include "core.h"
#include "channels_cfg.h"
#include "monopolar.h"
#include "bipolar.h"


/* =========================================================================
 * Genel API
 * ========================================================================= */
void monopolar_yonet(void);
void bipolar_yonet();
void touch_yonet(void);
void plate_yonet(void);
void error_yonet(void);
void input_yonet(void);
void alarm_yonet(void);
void alarm_beepBaslat(void);
void alarm_beepDurdur(void);
void selftest(void);

#ifdef __cplusplus
}
#endif

#endif /* GENX_H */