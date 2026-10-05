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

/* =========================================================================
 * Tanımlar ve Makrolar
 * ========================================================================= */
// --  MONO1 CUT mod tanımları ---------------------------------
typedef enum {
	MONO1_CUT_MODE_CUT 			= 1U,
	MONO1_CUT_MODE_BLEND1 		= 2U,
	MONO1_CUT_MODE_BLEND2		= 3U,
	MONO1_CUT_MODE_BLEND3		= 4U,
	MONO1_CUT_MODE_POLYPECTOMY 	= 5U,
	MONO1_CUT_MODE_PAPILLOTOMES	= 6U,
	MONO1_CUT_MODE_RESECTION1	= 11U,
	MONO1_CUT_MODE_RESECTION2	= 12U
} Mono1_Cut_Mode_e;


// -- MONO1 COAG mod tanımları ---------------------------------
typedef enum {
	MONO1_COAG_MODE_CONTACT 	= 1U,
	MONO1_COAG_MODE_SPRAY1 		= 2U,
	MONO1_COAG_MODE_SPRAY2		= 3U,
	MONO1_COAG_MODE_SPRAY3		= 4U,
	MONO1_COAG_MODE_FULGURATION = 5U
} Mono1_Coag_Mode_e;

// --  MONO2 CUT mod tanımları ---------------------------------
typedef enum {
	MONO2_CUT_MODE_CUT 			= 1U,
	MONO2_CUT_MODE_BLEND1 		= 2U,
	MONO2_CUT_MODE_BLEND2		= 3U,
	MONO2_CUT_MODE_BLEND3		= 4U,
	MONO2_CUT_MODE_POLYPECTOMY 	= 5U,
	MONO2_CUT_MODE_PAPILLOTOMES	= 6U,
	MONO2_CUT_MODE_RESECTION1	= 11U,
	MONO2_CUT_MODE_RESECTION2	= 12U
} Mono2_Cut_Mode_e;

// -- MONO2 COAG mod tanımları ---------------------------------
typedef enum {
	MONO2_COAG_MODE_CONTACT 	= 1U,
	MONO2_COAG_MODE_SPRAY1		= 2U,
	MONO2_COAG_MODE_SPRAY2		= 3U,
	MONO2_COAG_MODE_SPRAY3		= 4U,
	MONO2_COAG_MODE_FULGURATION = 5U
} Mono2_Coag_Mode_e;

// -- BIPOLAR1 CUT mod tanımları -------------------------------
typedef enum {
	BIPOLAR_CUT_MODE_CUTTING	= 1U,
	BIPOLAR_CUT_MODE_SCISSORS	= 2U,
	BIPOLAR_CUT_MODE_BIVAPO		= 3U,
	BIPOLAR_CUT_MODE_BIREZO		= 4U
} Bipolar_Cut_Mode_e;

// -- BIPOLAR1 COAG mod tanımları ------------------------------
typedef enum {
	BIPOLAR1_COAG_MODE_STANDARD	= 1U,
	BIPOLAR1_COAG_MODE_FORCED	= 2U,
	BIPOLAR1_COAG_MODE_STOP		= 3U,
	BIPOLAR1_COAG_MODE_START	= 4U
} Bipolar_Coag_Mode_e;

// -- BIPOLAR2 CUT mod tanımları -------------------------------
typedef enum {
	SEAL_CUT_MODE_CUTTING	= 1U,
	SEAL_CUT_MODE_SCISSORS	= 2U,
	SEAL_CUT_MODE_BIVAPO	= 3U,
	SEAL_CUT_MODE_BIREZO	= 4U
} Seal_Cut_Mode_e;

// -- BIPOLAR2 COAG mod tanımları ------------------------------
typedef enum {
	SEAL_COAG_MODE_LIGATION		= 1U,
	SEAL_COAG_MODE_SEALSURE		= 2U,
	SEAL_COAG_MODE_TISSUELOCK	= 3U,
} Seal_Coag_Mode_e;

// -- Plate tanımları ------------------------------------------
typedef enum {
	PLATE_SINGLE 	= 1U,
	PLATE_DUAL		= 2U
} Plate_e;

// -- Built-in Self-test tanımları -----------------------------
typedef enum{
	BIST_PWR_SUPPLY 	= 0U,
	BIST_MONO_BOARD		= 1U,
	BIST_BIPOLAR_BOARD	= 2U,
	BIST_OK          	= 3U
} Bist_State_e;

// -- Alarm ses tanımları --------------------------------------
typedef enum {
    ALARM_SES_NONE 			= 0U,
    ALARM_SES_MONO_CUT 		= 1U,
    ALARM_SES_MONO_COAG 	= 2U,
    ALARM_SES_BIPOLAR_CUT 	= 3U,
    ALARM_SES_BIPOLAR_COAG 	= 4U
} Alarm_Ses_e;

// -- Alarm yapılandırma ---------------------------------------
typedef struct {
    Alarm_Ses_e sesTonu;
    uint16_t PSC;
    const char *pErrorMsg;
} Alarm_Config_t;

static const Alarm_Config_t gAlarmConfigTable[] = {
    { ALARM_SES_MONO_CUT,		210U,	NULL },
    { ALARM_SES_MONO_COAG,		280U, 	NULL },
    { ALARM_SES_BIPOLAR_CUT,	320U, 	NULL },
    { ALARM_SES_BIPOLAR_COAG, 	390U, 	NULL }
};

#define ALARM_COUNT ((uint8_t)(sizeof(gAlarmConfigTable) / sizeof(gAlarmConfigTable[0])))


/* ================================================================
 * GPIO okuma makroları
 * ================================================================*/

#define MONO1_CUT_KALEM_BASILI()       			(GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_8)))
#define MONO1_CUT_KALEM_SERBEST()       		(GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_8)))

#define MONO1_COAG_KALEM_BASILI()      			(GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_4)))
#define MONO1_COAG_KALEM_SERBEST()      		(GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_4)))

#define MONO1_CUT_AYAK_BASILI()         		((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gMono1Pedal == 1U))
#define MONO1_CUT_AYAK_SERBEST()         		((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gMono1Pedal == 1U))

#define MONO1_COAG_AYAK_BASILI()        		((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gMono1Pedal == 1U))
#define MONO1_COAG_AYAK_SERBEST()        		((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gMono1Pedal == 1U))

#define MONO2_CUT_KALEM_BASILI()       			(GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_7)))
#define MONO2_CUT_KALEM_SERBEST()       		(GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_7)))

#define MONO2_COAG_KALEM_BASILI()      			(GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_5)))
#define MONO2_COAG_KALEM_SERBEST()      		(GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOB), (GPIO_PIN_5)))

#define MONO2_CUT_AYAK_BASILI()					((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gMono2Pedal == 1U))
#define MONO2_CUT_AYAK_SERBEST()				((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gMono2Pedal == 1U))

#define MONO2_COAG_AYAK_BASILI()				((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gMono2Pedal == 1U))
#define MONO2_COAG_AYAK_SERBEST() 				((GPIO_PIN_SET == HAL_GPIO_ReadPin(GPIOE, GPIO_PIN_10)) && (gMono2Pedal == 1U))

#define BIPOLAR1_CUT_AYAK_BASILI()				((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar1PedalCift == 1U))
#define BIPOLAR1_CUT_AYAK_SERBEST()				((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar1PedalCift == 1U))

#define BIPOLAR1_COAG_AYAK_BASILI()				(GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10)) && (gBipolar1PedalCift == 1U))
#define BIPOLAR1_COAG_AYAK_SERBEST()			(GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10)) && (gBipolar1PedalCift == 1U))

#define BIPOLAR2_CUT_LIGASURE_AYAK_BASILI()		((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar2PedalCift == 1U))
#define BIPOLAR2_CUT_LIGASURE_AYAK_SERBEST()	((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_9))) && (gBipolar2PedalCift == 1U))

#define BIPOLAR2_COAG_LIGASURE_AYAK_BASILI()	((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gBipolar2PedalCift == 1U))
#define BIPOLAR2_COAG_LIGASURE_AYAK_SERBEST()	((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOE), (GPIO_PIN_10))) && (gBipolar2PedalCift == 1U))

#define BIPOLAR_TEK_PEDAL_BASILI()				(GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1)) && (0U == gBipolarHand))
#define BIPOLAR_TEK_PEDAL_SERBEST()				(GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1)) && (0U == gBipolarHand))

#define BIPOLAR_HAND_BASILI()					((GPIO_PIN_RESET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1))) \
												&& (GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOD), (GPIO_PIN_0))) && (1U == gBipolarHand))
#define BIPOLAR_HAND_SERBEST()					((GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOC),  (GPIO_PIN_1))) \
												&& (GPIO_PIN_SET == HAL_GPIO_ReadPin((GPIOD), (GPIO_PIN_0))) && (1U == gBipolarHand))

/* ================================================================
 * Ana durum makinesi durum tanımları
 * ================================================================*/

typedef enum {
	MONO1_CUT_DURUM_BOSTA 		= 0U,
	MONO1_CUT_DURUM_AKTIF 		= 1U,
} Mono1Cut_State_e;

typedef enum {
	MONO1_COAG_DURUM_BOSTA 		= 0U,
	MONO1_COAG_DURUM_AKTIF 		= 1U,
} Mono1Coag_State_e;

typedef enum {
	MONO2_CUT_DURUM_BOSTA 		= 0U,
	MONO2_CUT_DURUM_AKTIF		= 1U
} Mono2Cut_State_e;

typedef enum {
	MONO2_COAG_DURUM_BOSTA 		= 0U,
	MONO2_COAG_DURUM_AKTIF 	= 1U
} Mono2Coag_State_e;

typedef enum {
	BIPOLAR1_CUT_DURUM_BOSTA 	= 0U,
	BIPOLAR1_CUT_DURUM_AKTIF 	= 1U
} Bipolar1Cut_State_e;

typedef enum {
	BIPOLAR1_COAG_DURUM_BOSTA 	= 0U,
	BIPOLAR1_COAG_DURUM_AKTIF	= 1U
} Bipolar1Coag_State_e;

typedef enum {
	BIPOLAR2_CUT_DURUM_BOSTA 	= 0U,
	BIPOLAR2_CUT_DURUM_AKTIF	= 1U
} Bipolar2Cut_State_e;

typedef enum {
	BIPOLAR2_COAG_DURUM_BOSTA 	= 0U,
	BIPOLAR2_COAG_DURUM_AKTIF 	= 1U
} Bipolar2Coag_State_e;

typedef enum {
	GUCU_KAPALI = 0U,
	GUCU_ACIK 	= 1U
} Mode_Start_e;

/* =========================================================================
 * Genel API
 * ========================================================================= */

void mono1_run(void);
void mono2_run(void);
void bipolar1_run(void);
void bipolar2_run(void);
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