/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    monopolar.c
 * @brief   Monopolar kanallar (Mono1 CUT/COAG, Mono2 CUT/COAG).
 *          Her mod kendi başlatma/izleme fonksiyonunu içerir, polinomlar elle girilir.
 *          Hangi modun hangi yolda açık olduğu channels_cfg.h'de belirlenir.
 *
 * @author  destrocore
 * @date    2026
 */

#include "monopolar.h"

/* ============================================================
 * Mod açma/kapama maskeleri
 * ============================================================*/

#define MONO1_CUT_MASK ( MODE_BIT(MONOPOLAR_CUTMODE_CUT,         MONO1_CUT_EN_CUT) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND1,      MONO1_CUT_EN_BLEND1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND2,      MONO1_CUT_EN_BLEND2) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND3,      MONO1_CUT_EN_BLEND3) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_POLYPECTOMY, MONO1_CUT_EN_POLYPECTOMY) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_PAPILLOTOMES, MONO1_CUT_EN_PAPILLOTOMES) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION1,  MONO1_CUT_EN_RESECTION1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION2,  MONO1_CUT_EN_RESECTION2) )


#define MONO2_CUT_MASK ( MODE_BIT(MONOPOLAR_CUTMODE_CUT,         MONO2_CUT_EN_CUT) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND1,      MONO2_CUT_EN_BLEND1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND2,      MONO2_CUT_EN_BLEND2) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_BLEND3,      MONO2_CUT_EN_BLEND3) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_POLYPECTOMY, MONO2_CUT_EN_POLYPECTOMY) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_PAPILLOTOMES, MONO2_CUT_EN_PAPILLOTOMES) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION1,  MONO2_CUT_EN_RESECTION1) \
                        | MODE_BIT(MONOPOLAR_CUTMODE_RESECTION2,  MONO2_CUT_EN_RESECTION2) )


#define MONO1_COAG_MASK ( MODE_BIT(MONOPOLAR_COAGMODE_CONTACT,     MONO1_COAG_EN_CONTACT) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY1,      MONO1_COAG_EN_SPRAY1) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY2,      MONO1_COAG_EN_SPRAY2) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY3,      MONO1_COAG_EN_SPRAY3) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_FULGURATION, MONO1_COAG_EN_FULGURATION) )


#define MONO2_COAG_MASK ( MODE_BIT(MONOPOLAR_COAGMODE_CONTACT,     MONO2_COAG_EN_CONTACT) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY1,      MONO2_COAG_EN_SPRAY1) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY2,      MONO2_COAG_EN_SPRAY2) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_SPRAY3,      MONO2_COAG_EN_SPRAY3) \
                        | MODE_BIT(MONOPOLAR_COAGMODE_FULGURATION, MONO2_COAG_EN_FULGURATION) )


// -- Geri dönüş (fallback) modları kapatılamaz
#if (0U == MONO1_CUT_EN_CUT) || (0U == MONO2_CUT_EN_CUT)
#error "MONO CUT: CUT kapatildi"
#endif
#if (0U == MONO1_COAG_EN_CONTACT) || (0U == MONO2_COAG_EN_CONTACT)
#error "MONO COAG: CONTACT kapatildi"
#endif

/* ============================================================
 * Kanal yapılandırmaları
 * ============================================================*/
static const Monopolar_Config_t gMono1CutCfg = {
    .function           = MONOPOLAR_FUNCTION_CUT,
    .relay_pin          = MONO_RELAY_Pin,                               // PD1
    .mux_channel        = MUX_MONO_CUT,
    .led_white          = { MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin },   // PC7
    .led_active         = { MONO_SARI_GPIO_Port, MONO_SARI_Pin },       // PD14
    .stick_pin          = { GPIOB, GPIO_PIN_8 },
    .foot_pin           = { GPIOE, GPIO_PIN_9 },
    .pPedalSelected     = &gMono1Pedal,
    .pFaultFlag         = &gMono1Koruma,
    .enabled_mask       = MONO1_CUT_MASK,
    .fallback_mode      = MONOPOLAR_CUTMODE_CUT,
    .max_watt           = 400U,
    .pWattGetCmd        = "get S2_m1_cut_watt.USER_VALUE.val",
    .pModeGetCmd        = "get S3_m1_cut_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n0.val=S2_m1_cut_watt.USER_VALUE.val"
};

static const Monopolar_Config_t gMono1CoagCfg = {
    .function           = MONOPOLAR_FUNCTION_COAG,
    .relay_pin          = MONO_RELAY_Pin,                               // PD1
    .mux_channel        = MUX_MONO_COAG,
    .led_white          = { MONO1_BEYAZ_GPIO_Port, MONO1_BEYAZ_Pin },
    .led_active         = { MONO_MAVI_GPIO_Port, MONO_MAVI_Pin },       // PB13
    .stick_pin          = { GPIOB, GPIO_PIN_4 },
    .foot_pin           = { GPIOE, GPIO_PIN_10 },
    .pPedalSelected     = &gMono1Pedal,
    .pFaultFlag         = &gMono1Koruma,
    .enabled_mask       = MONO1_COAG_MASK,
    .fallback_mode      = MONOPOLAR_COAGMODE_CONTACT,
    .max_watt           = 120U,
    .pWattGetCmd        = "get S4_m1_cog_watt.USER_VALUE.val",
    .pModeGetCmd        = "get S5_m1_cog_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n1.val=S4_m1_cog_watt.USER_VALUE.val"
};

static const Monopolar_Config_t gMono2CutCfg = {
    .function           = MONOPOLAR_FUNCTION_CUT,
    .relay_pin          = GPIO_PIN_3,                                   // PD3
    .mux_channel        = MUX_MONO_CUT,
    .led_white          = { MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin },   // PD13
    .led_active         = { GPIOB, GPIO_PIN_15 },                       // sarı
    .stick_pin          = { GPIOB, GPIO_PIN_7 },
    .foot_pin           = { GPIOE, GPIO_PIN_9 },
    .pPedalSelected     = &gMono2Pedal,
    .pFaultFlag         = &gMono2Koruma,
    .enabled_mask       = MONO2_CUT_MASK,
    .fallback_mode      = MONOPOLAR_CUTMODE_CUT,
    .max_watt           = 400U,
    .pWattGetCmd        = "get S6_m2_cut_watt.USER_VALUE.val",
    .pModeGetCmd        = "get S7_m2_cut_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n2.val=S6_m2_cut_watt.USER_VALUE.val"
};

static const Monopolar_Config_t gMono2CoagCfg = {
    .function           = MONOPOLAR_FUNCTION_COAG,
    .relay_pin          = GPIO_PIN_3,                                   // PD3
    .mux_channel        = MUX_MONO_COAG,
    .led_white          = { MONO2_BEYAZ_GPIO_Port, MONO2_BEYAZ_Pin },
    .led_active         = { GPIOB, GPIO_PIN_12 },                       // mavi
    .stick_pin          = { GPIOB, GPIO_PIN_5 },
    .foot_pin           = { GPIOE, GPIO_PIN_10 },
    .pPedalSelected     = &gMono2Pedal,
    .pFaultFlag         = &gMono2Koruma,
    .enabled_mask       = MONO2_COAG_MASK,
    .fallback_mode      = MONOPOLAR_COAGMODE_CONTACT,
    .max_watt           = 120U,
    .pWattGetCmd        = "get S8_m2_cog_watt.USER_VALUE.val",
    .pModeGetCmd        = "get S9_m2_cog_mod.USER_MODE.val",
    .pMainSetCmd        = "S1_anasayfa.n3.val=S8_m2_cog_watt.USER_VALUE.val"
};

/* ============================================================
 * Handle'lar (RAM)
 * ============================================================*/
Monopolar_Handle_t gMono1Cut = {
    .pCfg = &gMono1CutCfg,
    .canli   = { 
        .user_watt = 10U, 
        .user_mode = MONOPOLAR_CUTMODE_CUT, 
        .pol = 10.0f 
    }
};

Monopolar_Handle_t gMono1Coag = {
    .pCfg = &gMono1CoagCfg,
    .canli   = { 
        .user_watt = 10U, 
        .user_mode = MONOPOLAR_COAGMODE_CONTACT, 
        .pol = 10.0f 
    }
};

Monopolar_Handle_t gMono2Cut = {
    .pCfg = &gMono2CutCfg,
    .canli   = { 
        .user_watt = 10U, 
        .user_mode = MONOPOLAR_CUTMODE_CUT, 
        .pol = 10.0f 
    }
};

Monopolar_Handle_t gMono2Coag = {
    .pCfg = &gMono2CoagCfg,
    .canli   = { 
        .user_watt = 10U, 
        .user_mode = MONOPOLAR_COAGMODE_CONTACT, 
        .pol = 10.0f 
    }
};

/* ============================================================
 * TIM5 (endo)
 * ============================================================*/
static Monopolar_Handle_t * volatile gpEndoOwner = NULL;
static volatile uint8_t gEndoCutSay = 0U;
static volatile float gEndoCutWattPol = 50.0f;
static volatile float gEndoCoagWattPol = 20.0f;
static volatile uint32_t gEndoCutTicks = 1U;        // 1 == 0.01 saniye
static volatile uint16_t gEndoCoagTimer = 1U;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void monopolar_setLeds(const Monopolar_Config_t *pCfg, bool isActive);
static bool monopolar_startOutput(Monopolar_Handle_t *pH, uint16_t pwmCompare, bool isSpray);
static void monopolar_setActive(Monopolar_Handle_t *pH);
static void monopolar_cutCut(Monopolar_Handle_t *pH);
static void monopolar_cutBlend1(Monopolar_Handle_t *pH);
static void monopolar_cutBlend2(Monopolar_Handle_t *pH);
static void monopolar_cutBlend3(Monopolar_Handle_t *pH);
static void monopolar_cutEndo(Monopolar_Handle_t *pH);
static void monopolar_stopCut(Monopolar_Handle_t *pH);
static void monopolar_runCut(Monopolar_Handle_t *pH);
static void monopolar_coagContact(Monopolar_Handle_t *pH);
static void monopolar_coagSpray1(Monopolar_Handle_t *pH);
static void monopolar_coagSpray2(Monopolar_Handle_t *pH);
static void monopolar_coagSpray3(Monopolar_Handle_t *pH);
static void monopolar_stopCoag(Monopolar_Handle_t *pH);
static void monopolar_runCoag(Monopolar_Handle_t *pH);

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief Monopolar yolları kanal kaydına ekler (main döngüsünden önce bir kez çağrılır)
  * @retval None
  */
void monopolar_init(void) {

    genx_registerChannel(&gMono1Cut.canli.common);
    genx_registerChannel(&gMono1Coag.canli.common);
    genx_registerChannel(&gMono2Cut.canli.common);
    genx_registerChannel(&gMono2Coag.canli.common);
}

/**
  * @brief Dört monopolar yolun tek giriş noktası
  * @retval None
  */
void monopolar_run(void) {

    monopolar_runCut(&gMono1Cut);
    monopolar_runCoag(&gMono1Coag);
    monopolar_runCut(&gMono2Cut);
    monopolar_runCoag(&gMono2Coag);
}

/**
  * @brief Kalem/pedal sinyalini yolun Start bayrağına dönüştürür (yalnız ana sayfada çağrılır)
  * @param pH Yol handle'ı
  * @retval None
  */
void monopolar_pollInput(Monopolar_Handle_t *pH) {

    const Monopolar_Config_t *pCfg = pH->pCfg;
    Monopolar_Runtime_t *pRt = &pH->canli;
    bool isPen = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(pCfg->stick_pin.pPort, pCfg->stick_pin.pin));
    bool isFoot = (GPIO_PIN_RESET == HAL_GPIO_ReadPin(pCfg->foot_pin.pPort, pCfg->foot_pin.pin)) && (1U == *pCfg->pPedalSelected);
    bool isPressed = isPen || isFoot;
    bool isLatched = (1U == pRt->hata) && ((GUCU_ACIK == pRt->common.start) || (1U == gAlarmPlate));

    if (true == genx_isCounterDone((false == isPressed) && isLatched, &pRt->birakma)) {
        pRt->basma = 0U;
        pRt->hata = 0U;
        pRt->common.start = GUCU_KAPALI;
        if (1U == gAlarmPlate) {
            gAlarmPlate = 2U;
        }
    }

    if (true == genx_isCounterDone(isPressed && (false == genx_isOtherChannelActive(&pRt->common)), &pRt->basma)) {
        pRt->birakma = 0U;
        pRt->hata = 1U;
        if (0U == gPlate100Overresistance) {
            pRt->common.start = GUCU_ACIK;
        } else {
            pRt->common.start = GUCU_KAPALI;
            gAlarmPlate = 1U;
        }
    }
}

/**
  * @brief Modun bu yolda açık olup olmadığını döndürür
  * @param pH Yol handle'ı
  * @param modeId Nextion'da saklanan mod değeri
  * @retval true mod açık, false kapalı yada tanımsız
  */
bool monopolar_isModeEnabled(const Monopolar_Handle_t *pH, uint32_t modeId) {

    return (modeId < 32U) && (0U != (pH->pCfg->enabled_mask & (1U << modeId)));
}

/**
  * @brief Kayıtlı mod bu yolda kapalıysa geri dönüş moduna çevirir
  * @param pH Yol handle'ı
  * @retval true mod düzeltildi, false değişiklik yok
  */
bool monopolar_isModeCorrected(Monopolar_Handle_t *pH) {

    bool isCorrected = false;

    if (false == monopolar_isModeEnabled(pH, pH->canli.user_mode)) {
        pH->canli.user_mode = pH->pCfg->fallback_mode;
        isCorrected = true;
    }

    return isCorrected;
}

/**
  * @brief Yolun modu endo modu mu (POLYPECTOMY / PAPILLOTOMES)?
  * @param pH Yol handle'ı
  * @retval true endo modu, false değil
  */
bool monopolar_isEndoMode(const Monopolar_Handle_t *pH) {

    return (MONOPOLAR_FUNCTION_CUT == pH->pCfg->function) \
        && ((MONOPOLAR_CUTMODE_POLYPECTOMY == pH->canli.user_mode) || (MONOPOLAR_CUTMODE_PAPILLOTOMES == pH->canli.user_mode));
}

/**
  * @brief Kullanıcı watt ve mod değerinden yolun işlenmiş watt değerini hesaplar
  * @param pH Yol handle'ı
  * @retval None
  */
void monopolar_updatePol(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;
    float pol = (float)pRt->user_watt;
    float limit;
    float offset = 0.0f;

    if (MONOPOLAR_FUNCTION_CUT == pH->pCfg->function) {
        limit = 385.0f;

        switch (pRt->user_mode) {
            case MONOPOLAR_CUTMODE_CUT: {
                offset = 5.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_BLEND1: {
                offset = 5.0f;
                limit = 240.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_BLEND2: {
                limit = 200.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_BLEND3: {
                limit = 150.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_POLYPECTOMY: {
                limit = 350.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_PAPILLOTOMES: {
                limit = 150.0f;
                break;
            }

            case MONOPOLAR_CUTMODE_RESECTION1:
            case MONOPOLAR_CUTMODE_RESECTION2: {
                offset = 4.0f;
                limit = 350.0f;
                break;
            }

            default: {
                break;
            }
        }

        if (pol < 16.0f) {
            pol -= offset;
        }
    } else {
        limit = 120.0f;

        switch (pRt->user_mode) {
            case MONOPOLAR_COAGMODE_SPRAY1:
            case MONOPOLAR_COAGMODE_SPRAY2:
            case MONOPOLAR_COAGMODE_SPRAY3:
            case MONOPOLAR_COAGMODE_FULGURATION: {
                limit = 100.0f;
                break;
            }

            default: {
                break;
            }
        }
    }

    if (pol > limit) {
        pol = limit;
    }

    pRt->pol = pol;
}

/**
  * @brief Endo ayarlarını (kademe/süre) TIM5 kesmesinin kullandığı değerlere çevirir
  * @param pH Endo modundaki yol handle'ı (çarpanlar moda göre seçilir)
  * @retval None
  */
void monopolar_endoApply(const Monopolar_Handle_t *pH) {

    uint32_t cutTicks;
    uint32_t mode = pH->canli.user_mode;

    if ((gEndoCutKademe >= 1U) && (gEndoCutKademe <= 7U)) {
        if (MONOPOLAR_CUTMODE_POLYPECTOMY == mode) {
            gEndoCutWattPol = (float)(gEndoCutKademe * 50U);
        }

        if (MONOPOLAR_CUTMODE_PAPILLOTOMES == mode) {
            gEndoCutWattPol = (float)(gEndoCutKademe * 22U);
        }
    }

    if ((gEndoCoagKademe >= 1U) && (gEndoCoagKademe <= 5U)) {
        if (MONOPOLAR_CUTMODE_POLYPECTOMY == mode) {
            gEndoCoagWattPol = (float)(gEndoCoagKademe * 20U);
        }

        if (MONOPOLAR_CUTMODE_PAPILLOTOMES == mode) {
            gEndoCoagWattPol = (float)(gEndoCoagKademe * 10U);
        }
    }

    cutTicks = gEndoCutSure;
    if (cutTicks < 6U) {
        cutTicks = cutTicks + 2U;
    }

    if (cutTicks > 5U) {
        cutTicks = 5U;
    }

    gEndoCutTicks = cutTicks;

    if ((gEndoCoagSure >= 1U) && (gEndoCoagSure <= 15U)) {
        gEndoCoagTimer = (uint16_t)(gEndoCoagSure * 10U);
    }
}

/**
  * @brief TIM5 periyot kesmesi: Endo (POLYPECTOMY/PAPILLOTOMES) cut/coag darbe dizisi
  * @param pHtim Kesmeyi üreten zamanlayıcı
  * @retval None
  * @note HAL'ın zayıf tanımını ezen sabit isimli callback (cb ön ekine uyamaz)
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *pHtim) {

    if (TIM5 == pHtim->Instance) {

        Monopolar_Handle_t *pH = gpEndoOwner;

        if ((NULL != pH) && (GUCU_ACIK == pH->canli.common.start)) {

            float cutWatt = gEndoCutWattPol;
            float coagWatt = gEndoCoagWattPol;
            uint32_t cutTicks = gEndoCutTicks;
            uint8_t say = gEndoCutSay;
            float dacVal;

            say++;
            if (say <= cutTicks) {
                dacVal = ((0.000022f * cutWatt * cutWatt * cutWatt) - (0.0205f * cutWatt * cutWatt) \
                        + (10.25f * cutWatt) + 287.0f);
                dacVal /= 10.0f;

                genx_dacSet(dacVal);
            }

            if (say > cutTicks) {
                __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 108U);
                HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);

                dacVal = ((0.000075f * coagWatt * coagWatt * coagWatt) \
                        - (0.0212f * coagWatt * coagWatt) + (3.2617f * coagWatt) + 34.4f);

                genx_dacSet(dacVal);
            }

            if (say >= (gEndoCoagTimer + cutTicks)) {
                say = 0U;
                HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
            }

            gEndoCutSay = say;
        }
    }
}

/* ================================================================
 * Yardımcı fonksiyonlar
 * ================================================================*/

/**
  * @brief Yolun LED'lerini ayarlar (aktif: renkli açık + beyaz kapalı)
  * @param pCfg Yol yapılandırması
  * @param isActive true aktif, false boşta
  * @retval None
  */
static void monopolar_setLeds(const Monopolar_Config_t *pCfg, bool isActive) {

    HAL_GPIO_WritePin(pCfg->led_active.pPort, pCfg->led_active.pin, (true == isActive) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(pCfg->led_white.pPort, pCfg->led_white.pin, (true == isActive) ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

/**
  * @brief Çıkış sahipliğini alır ve yolu donanımsal olarak açar (röle, PWM, MUX, FAN)
  * @param pH Yol handle'ı
  * @param pwmCompare TIM2 CCR değeri, 0U ise TIM2 durdurulur
  * @param isSpray true ise spray rölesi açılır
  * @retval true çıkış açıldı, false başka yol sahip (Start bayrağı kapatıldı)
  */
static bool monopolar_startOutput(Monopolar_Handle_t *pH, uint16_t pwmCompare, bool isSpray) {

    const Monopolar_Config_t *pCfg = pH->pCfg;
    bool isStarted = false;

    if (0U == genx_claim(&pH->canli.common)) {
        (void)NEXTION_sendCmdRetry("tsw 255,0");

        if (0U == pwmCompare) {
            HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
        }

        if (true == isSpray) {
            HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_SET);        // spray rölesi
        }

        if (0U != pwmCompare) {
            __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pwmCompare);
            HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
        }

        genx_monoEnable(pCfg->relay_pin, pCfg->mux_channel);

        if (MONOPOLAR_FUNCTION_CUT == pCfg->function) {
            gMonopolarCutSes = 1U;
        } else {
            gMonopolarCoagSes = 1U;
        }

        pH->canli.guc_gecikme = 0U;
        HAL_Delay(200U);
        isStarted = true;
    } else {
        pH->canli.common.start = GUCU_KAPALI;
    }

    return isStarted;
}

/**
  * @brief Yolu aktif duruma geçirir ve LED'lerini günceller
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_setActive(Monopolar_Handle_t *pH) {

    pH->canli.state = MONOPOLAR_STATE_ACTIVE;
    monopolar_setLeds(pH->pCfg, true);
}

/**
  * @brief CUT / RESECTION1 / RESECTION2 başlatma ve çıkış gücü izleme
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_cutCut(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;

    if (MONOPOLAR_STATE_IDLE == pRt->state) {
        if ((0U == gPlate100Overresistance) && (true == monopolar_startOutput(pH, 0U, false))) {
            float x = pRt->pol;

            if (x >= 100.0f) {
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.000022f * x * x * x) - (0.0205f * x * x) + (10.25f * x) + 287.0f;
            x /= 10.0f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    } else {
        pRt->guc_gecikme++;
        if ((pRt->guc_gecikme >= 1000U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_gecikme = 0U;
            HAL_Delay(20U);

            if ((1U == pRt->yuz_start) && (guc > 70U)) {
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = ((0.000021f * x * x * x) - (0.0205f * x * x) + (10.1f * x) + 287.0f);
                x /= 10.0f;

                genx_dacSet(x);
            }

            {
                float p = pRt->pol;
                uint16_t korumaVeri = (uint16_t)((0.0000278f * p * p * p) - (0.0383f * p * p) + (19.136f * p) + (683.43f));

                if ((p >= 50.0f) && (guc > (uint32_t)korumaVeri)) {
                    *pH->pCfg->pFaultFlag = 1U;
                }
            }
        }
    }
}

/**
  * @brief BLEND1 başlatma ve çıkış gücü izleme
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_cutBlend1(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;

    if (MONOPOLAR_STATE_IDLE == pRt->state) {
        if ((0U == gPlate100Overresistance) && (true == monopolar_startOutput(pH, 212U, false))) {     // %80
            float x = pRt->pol;

            if (x >= 100.0f) {
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.0000126f * x * x * x) - (0.0063f * x * x) + (1.6669f * x) + 31.0f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    } else {
        pRt->guc_gecikme++;
        if ((pRt->guc_gecikme >= 1000U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_gecikme = 0U;
            HAL_Delay(20U);

            if ((1U == pRt->yuz_start) && (guc > 120U)) {
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = (0.0000126f * x * x * x) - (0.0063f * x * x) + (1.6669f * x) + 31.0f;

                genx_dacSet(x);
            }

            {
                float p = pRt->pol;
                uint16_t korumaVeri = (uint16_t)((0.000186f * p * p * p) - (0.0928f * p * p) + (23.777f * p) + (548.8f));

                if ((p >= 30.0f) && (guc > (uint32_t)korumaVeri)) {
                    *pH->pCfg->pFaultFlag = 1U;
                }
            }
        }
    }
}

/**
  * @brief BLEND2 başlatma ve çıkış gücü izleme
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_cutBlend2(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;

    if (MONOPOLAR_STATE_IDLE == pRt->state) {
        if ((0U == gPlate100Overresistance) && (true == monopolar_startOutput(pH, 160U, false))) {     // %60
            float x = pRt->pol;

            if (x >= 100.0f) {
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.0000173f * x * x * x) - (0.00755f * x * x) + (1.9052f * x) + 33.0f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    } else {
        pRt->guc_gecikme++;
        if ((pRt->guc_gecikme >= 1000U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_gecikme = 0U;
            HAL_Delay(20U);

            if ((1U == pRt->yuz_start) && (guc > 130U)) {
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = (0.0000173f * x * x * x) - (0.00755f * x * x) + (1.9052f * x) + 33.0f;

                genx_dacSet(x);
            }

            {
                float p = pRt->pol;
                uint16_t korumaVeri = (uint16_t)((0.000115f * p * p * p) - (0.058f * p * p) + (18.493f * p) + (704.0f));

                if ((p >= 30.0f) && (guc > (uint32_t)korumaVeri)) {
                    *pH->pCfg->pFaultFlag = 1U;
                }
            }
        }
    }
}

/**
  * @brief BLEND3 başlatma ve çıkış gücü izleme
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_cutBlend3(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;

    if (MONOPOLAR_STATE_IDLE == pRt->state) {
        if ((0U == gPlate100Overresistance) && (true == monopolar_startOutput(pH, 127U, false))) {     // %50
            float x = pRt->pol;

            if (x >= 100.0f) {
                pRt->yuz_start = 1U;
                x = 100.0f;
            }

            x = (0.000039f * x * x * x) - (0.0135f * x * x) + (2.5977f * x) + 33.355f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    } else {
        pRt->guc_gecikme++;
        if ((pRt->guc_gecikme >= 1000U) && (1U == gMonopolarCutSes)) {

            uint32_t guc = gAdc1DmaBuff[ADC_CH_MONO_PWR];

            pRt->guc_gecikme = 0U;
            HAL_Delay(20U);

            if ((1U == pRt->yuz_start) && (guc > 130U)) {
                float x = pRt->pol;

                pRt->yuz_start = 0U;
                x = (0.000039f * x * x * x) - (0.0135f * x * x) + (2.5977f * x) + 33.355f;

                genx_dacSet(x);
            }

            {
                float p = pRt->pol;
                uint16_t korumaVeri = (uint16_t)((0.000465f * p * p * p) - (0.1818f * p * p) + (34.623f * p) + (319.0f));

                if ((p >= 30.0f) && (guc > (uint32_t)korumaVeri)) {
                    *pH->pCfg->pFaultFlag = 1U;
                }
            }
        }
    }
}

/**
  * @brief Endocut başlatma. DAC'ı TIM5 kesmesi sürer.
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_cutEndo(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_STATE_IDLE == pH->canli.state) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_startOutput(pH, 0U, false)) {
            genx_dacSet(0.0f);
            gEndoCutSay = 0U;
            gpEndoOwner = pH;
            HAL_TIM_Base_Start_IT(&htim5);
            monopolar_setActive(pH);
        }
    }
}

/**
  * @brief CUT çıkışını kapatır ve çıkış sahipliğini bırakır
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_stopCut(Monopolar_Handle_t *pH) {

    const Monopolar_Config_t *pCfg = pH->pCfg;

    genx_dacSet(0.0f);
    gMonopolarCutSes = 2U;
    HAL_Delay(100U);
    gEndoCutSay = 11U;
    genx_muxSelect(0U);
    HAL_TIM_Base_Stop_IT(&htim5);
    gpEndoOwner = NULL;
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, pCfg->relay_pin | GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);       // FAN
    pH->canli.state = MONOPOLAR_STATE_IDLE;
    pH->canli.yuz_start = 0U;
    monopolar_setLeds(pCfg, false);
    genx_release(&pH->canli.common);
    HAL_Delay(300U);
    (void)NEXTION_sendCmdRetry("tsw 255,1");
}

/**
  * @brief CUT yolu: Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_runCut(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;

    if (GUCU_ACIK == pRt->common.start) {

        bool isPwrCheck = false;

        if (true == monopolar_isModeEnabled(pH, pRt->user_mode)) {
            switch (pRt->user_mode) {

                case MONOPOLAR_CUTMODE_CUT:
                case MONOPOLAR_CUTMODE_RESECTION1:
                case MONOPOLAR_CUTMODE_RESECTION2: {
                    monopolar_cutCut(pH);
                    isPwrCheck = true;
                    break;
                }

                case MONOPOLAR_CUTMODE_BLEND1: {
                    monopolar_cutBlend1(pH);
                    isPwrCheck = true;
                    break;
                }

                case MONOPOLAR_CUTMODE_BLEND2: {
                    monopolar_cutBlend2(pH);
                    isPwrCheck = true;
                    break;
                }

                case MONOPOLAR_CUTMODE_BLEND3: {
                    monopolar_cutBlend3(pH);
                    isPwrCheck = true;
                    break;
                }

                case MONOPOLAR_CUTMODE_POLYPECTOMY:
                case MONOPOLAR_CUTMODE_PAPILLOTOMES: {
                    monopolar_cutEndo(pH);
                    break;
                }

                default: {
                    break;
                }
            }
        }

        // Güç kaynağı kontrolü
        if ((true == isPwrCheck) && (true == gPwrYeni)) {

            if ((pRt->user_watt >= 10U) && (pRt->user_watt <= 400U) && (gHighVolt <= 2U)) {
                gPwrKoruma = 1U;
            }

            if ((((pRt->user_watt >= 10U) && (pRt->user_watt <= 45U)) || ((pRt->user_watt >= 50U) && (pRt->user_watt <= 400U))) \
            && (gHighVolt < 200U)) {
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONOPOLAR_STATE_ACTIVE == pRt->state) {

        monopolar_stopCut(pH);
    }
}

/**
  * @brief COAG CONTACT başlatma
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_coagContact(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_STATE_IDLE == pH->canli.state) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_startOutput(pH, 108U, false)) {
            float x = pH->canli.pol;       // yerel kopya: kullanıcı değeri bozulmaz

            x = (0.000087f * x * x * x) - (0.021f * x * x) + (3.0f * x) + 29.0f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    }
}

/**
  * @brief COAG SPRAY1 başlatma
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_coagSpray1(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_STATE_IDLE == pH->canli.state) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_startOutput(pH, 20U, true)) {
            float x = pH->canli.pol;

            x = (0.000007f * x * x * x) - (0.0153f * x * x) + (3.5f * x) + 50.7f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    }
}

/**
  * @brief COAG SPRAY2 başlatma
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_coagSpray2(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_STATE_IDLE == pH->canli.state) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_startOutput(pH, 22U, true)) {
            float x = pH->canli.pol;

            x = (0.00007f * x * x * x) - (0.0234f * x * x) + (3.7182f * x) + 42.463f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    }
}

/**
  * @brief COAG SPRAY3 ve FULGURATION başlatma
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_coagSpray3(Monopolar_Handle_t *pH) {

    if ((MONOPOLAR_STATE_IDLE == pH->canli.state) && (0U == gPlate100Overresistance)) {
        if (true == monopolar_startOutput(pH, 24U, true)) {
            float x = pH->canli.pol;

            x = (0.00009f * x * x * x) - (0.0244f * x * x) + (3.6567f * x) + 40.692f;

            genx_dacSet(x);
            monopolar_setActive(pH);
        }
    }
}

/**
  * @brief COAG çıkışını kapatır ve çıkış sahipliğini bırakır
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_stopCoag(Monopolar_Handle_t *pH) {

    const Monopolar_Config_t *pCfg = pH->pCfg;

    genx_dacSet(0.0f);
    gMonopolarCoagSes = 2U;
    HAL_Delay(100U);
    genx_muxSelect(0U);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_11, GPIO_PIN_RESET);      // spray rölesi
    HAL_GPIO_WritePin(GPIOD, pCfg->relay_pin | GPIO_PIN_8, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);       // FAN
    HAL_TIM_PWM_Stop(&htim13, TIM_CHANNEL_1);
    HAL_TIM_PWM_Stop(&htim2, TIM_CHANNEL_1);
    pH->canli.state = MONOPOLAR_STATE_IDLE;
    monopolar_setLeds(pCfg, false);
    genx_release(&pH->canli.common);
    HAL_Delay(300U);
    (void)NEXTION_sendCmdRetry("tsw 255,1");
}

/**
  * @brief COAG yolu: Start bayrağına göre mod fonksiyonunu veya durdurmayı çağırır
  * @param pH Yol handle'ı
  * @retval None
  */
static void monopolar_runCoag(Monopolar_Handle_t *pH) {

    Monopolar_Runtime_t *pRt = &pH->canli;

    if (GUCU_ACIK == pRt->common.start) {

        if (true == monopolar_isModeEnabled(pH, pRt->user_mode)) {
            switch (pRt->user_mode) {

                case MONOPOLAR_COAGMODE_CONTACT: {
                    monopolar_coagContact(pH);
                    break;
                }

                case MONOPOLAR_COAGMODE_SPRAY1: {
                    monopolar_coagSpray1(pH);
                    break;
                }

                case MONOPOLAR_COAGMODE_SPRAY2: {
                    monopolar_coagSpray2(pH);
                    break;
                }

                case MONOPOLAR_COAGMODE_SPRAY3:
                case MONOPOLAR_COAGMODE_FULGURATION: {
                    monopolar_coagSpray3(pH);
                    break;
                }

                default: {
                    break;
                }
            }
        }

        if ((true == gPwrYeni) && (pRt->user_watt >= 10U) && (pRt->user_watt <= 120U)) {

            if (gHighVolt <= 2U) {
                gPwrKoruma = 1U;
            }

            if (gHighVolt < 200U) {
                gMonoPwrKoruma = 1U;
            }
        }
    } else if (MONOPOLAR_STATE_ACTIVE == pRt->state) {

        monopolar_stopCoag(pH);
    }
}