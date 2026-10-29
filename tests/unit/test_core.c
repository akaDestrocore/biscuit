/**
 * @file   test_core.c
 * @brief  Core/Src/core.c için host tarafında çalışan birim testleri.
 *
 * @details core.c doğrudan include edilir (beyaz kutu): `gpChannels`, `gChannelCount` ve `gMutex`
 *          testler tarafından okunup sıfırlanabilir. Dış bağımlılıklar şu şekilde izole edilir:
 */

#include "unity.h"

#include <string.h>
#include <stdint.h>
#include <math.h>

#include "core.c"
#include "memory_setup.h"
#include "mock_stm32f4xx_hal.h"
#include "mock_stm32f4xx_hal_gpio.h"
#include "mock_nextion.h"

#if !((1 == MONO1_CUT_EN_CUT) || (1 == MONO2_CUT_EN_CUT) || (1 == MONO1_COAG_EN_CONTACT) || (1 == MONO2_COAG_EN_CONTACT))
#error "genx_monoAc() derlenmedi!"
#endif

#if !((1 == BIPOLAR1_CUT_EN_CUTTING) || (1 == BIPOLAR2_CUT_EN_CUTTING) || (1 == BIPOLAR1_COAG_EN_STANDARD) \
    || (1 == BIPOLAR2_COAG_EN_STANDARD) || (1 == BIPOLAR1_COAG_EN_LIGATION) || (1 == BIPOLAR2_COAG_EN_LIGATION))
#error "genx_bipolarAc/Kapat derlenmedi!"
#endif


/* =========================================================================
 * core.c'nin başka modüllerden beklediği semboller
 * ========================================================================= */

DAC_HandleTypeDef hdac;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim13;
Bipolar_Handle_t gBipolar1Cut;


/* =========================================================================
 * Özel tipler ve sabitler
 * ========================================================================= */

#define TEST_NUM_LOG_SIZE           4U
#define TEST_GET_VALUE              0x12345678U
#define TEST_QUEUE_STEP_MS          5U

/**
 * @brief nextion_sendNum() çağrı loglanma kaydı
 */
typedef struct {
    const char *pBuf;
    uint32_t bufSize;
    char prefix[NEXTION_CMD_MAX_LEN + 1U];
    int16_t num;
} TestCore_NumCall_t;

static TestCore_NumCall_t gNumLog[TEST_NUM_LOG_SIZE];
static uint32_t gSimNumCalls = 0U;

static uint32_t gFakeTickMs = 0U;
static uint32_t gFakeTickStepMs = 0U;

static uint32_t gSimSendCalls = 0U;
static uint32_t gSimProcessCalls = 0U;
static const char *gpSimLastCmd = NULL;
static Nextion_Status_e gSimSendResult = NEXTION_STATUS_OK;
static uint32_t gSimQueueFullCount = 0U;
static bool gSimAlwaysQueueFull = false;

static uint32_t gSimGetCalls = 0U;
static void *gpSimGetDst = NULL;
static const char *gpSimGetCmd = NULL;
static uint16_t gSimGetSize = 0U;
static Nextion_Status_e gSimGetResult = NEXTION_STATUS_OK;
static uint32_t gSimTimeoutCount = 0U;

static Core_Channel_t gTestCh[CORE_MAX_CH + 1U];


#define ASSERT_DAC_OUTPUT(input, expected) \
    do { \
        genx_dacUygula((input)); \
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(expected), gDacVeri); \
    } while (0)

#define ASSERT_NUM_CALL(idx, pPrefixExp, numExp) \
    do { \
        TEST_ASSERT_EQUAL_PTR(gNumBuff, gNumLog[(idx)].pBuf); \
        TEST_ASSERT_EQUAL_UINT32(sizeof(gNumBuff), gNumLog[(idx)].bufSize); \
        TEST_ASSERT_EQUAL_STRING((pPrefixExp), gNumLog[(idx)].prefix); \
        TEST_ASSERT_EQUAL_INT16((int16_t)(numExp), gNumLog[(idx)].num); \
    } while (0)


/* =========================================================================
 * HAL ve Nextion geri çağırımları
 * ========================================================================= */

/**
 * @brief HAL_GetTick() için CMock geri çağırımı
 * @param numCalls CMock'un sağladığı çağrı sayacı (kullanılmıyor)
 * @retval Ayarlanmış sahte zaman değeri
 */
static uint32_t cbHalGetTick(int numCalls) {

    uint32_t now = gFakeTickMs;
    (void)numCalls;

    gFakeTickMs += gFakeTickStepMs;
    return now;
}

/**
 * @brief HAL_GPIO_WritePin() için CMock geri çağırımı
 * @param pPort GPIO portu
 * @param pin Pin maskesi
 * @param state Yazılan seviye
 * @param numCalls CMock çağrı sayacı (kullanılmıyor)
 * @retval None
 */
static void cbHalGpioWritePin(GPIO_TypeDef *pPort, uint16_t pin, GPIO_PinState state, int numCalls) {

    (void)numCalls;
    (void)pPort;
    (void)pin;
    (void)state;
}

/**
 * @brief HAL_TIM_PWM_Start() için basit test stub
 * @param pHtim Zamanlayıcı handle'ı
 * @param channel Kanal
 * @retval HAL_OK
 */
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *pHtim, uint32_t channel) {

    (void)pHtim;
    (void)channel;
    return HAL_OK;
}

/**
 * @brief HAL_TIM_PWM_Stop() için basit test stub
 * @param pHtim Zamanlayıcı handle'ı
 * @param channel Kanal
 * @retval HAL_OK
 */
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *pHtim, uint32_t channel) {

    (void)pHtim;
    (void)channel;
    return HAL_OK;
}

/**
 * @brief HAL_DAC_SetValue() için basit test stub
 * @param pHdac DAC handle'ı
 * @param channel DAC kanalı
 * @param alignment Hizalama
 * @param data DAC verisi
 * @retval HAL_OK
 */
HAL_StatusTypeDef HAL_DAC_SetValue(DAC_HandleTypeDef *pHdac, uint32_t channel, uint32_t alignment, uint32_t data) {

    (void)pHdac;
    (void)channel;
    (void)alignment;
    (void)data;
    return HAL_OK;
}

/**
 * @brief Nextion kuyruk yanıtını üretir: önce QUEUE_FULL sayacı, sonra gSimSendResult
 * @retval Sahte Nextion durumu
 */
static Nextion_Status_e simNextQueueResult(void) {

    Nextion_Status_e status = gSimSendResult;

    if (true == gSimAlwaysQueueFull) {
        status = NEXTION_STATUS_QUEUE_FULL;
    } else if (gSimQueueFullCount > 0U) {
        gSimQueueFullCount--;
        status = NEXTION_STATUS_QUEUE_FULL;
    } else {
        // gSimSendResult döner
    }

    return status;
}

/**
 * @brief nextion_sendCmd() için CMock geri çağırımı
 * @param pCmd Komut
 * @param numCalls CMock çağrı sayacı (kullanılmıyor)
 * @retval Sahte Nextion durumu
 */
static Nextion_Status_e cbNextionSendCmd(const char *pCmd, int numCalls) {

    (void)numCalls;

    gSimSendCalls++;
    gpSimLastCmd = pCmd;

    return simNextQueueResult();
}

/**
 * @brief nextion_sendNum() için CMock geri çağırımı
 * @param pBuf Komut buffer'ı
 * @param bufSize Buffer boyutu
 * @param pPrefix Komut öneki
 * @param num Sayı
 * @param numCalls CMock çağrı sayacı (kullanılmıyor)
 * @retval Sahte Nextion durumu
 */
static Nextion_Status_e cbNextionSendNum(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num, int numCalls) {

    (void)numCalls;

    if (gSimNumCalls < TEST_NUM_LOG_SIZE) {
        gNumLog[gSimNumCalls].pBuf = pBuf;
        gNumLog[gSimNumCalls].bufSize = bufSize;
        (void)strncpy(gNumLog[gSimNumCalls].prefix, pPrefix, NEXTION_CMD_MAX_LEN);
        gNumLog[gSimNumCalls].prefix[NEXTION_CMD_MAX_LEN] = '\0';
        gNumLog[gSimNumCalls].num = num;
    }

    gSimNumCalls++;

    return simNextQueueResult();
}

/**
 * @brief nextion_getVarBlocking() için CMock geri çağırımı
 * @param pContent Değerin yazılacağı buffer
 * @param pCmd Sorgu komutu
 * @param contentSize Buffer boyutu
 * @param numCalls CMock çağrı sayacı (kullanılmıyor)
 * @retval Önce gSimTimeoutCount kadar TIMEOUT, sonra gSimGetResult
 */
static Nextion_Status_e cbNextionGetVarBlocking(void *pContent, const char *pCmd, uint16_t contentSize, int numCalls) {

    Nextion_Status_e status = gSimGetResult;
    (void)numCalls;

    gSimGetCalls++;
    gpSimGetDst = pContent;
    gpSimGetCmd = pCmd;
    gSimGetSize = contentSize;

    if (gSimTimeoutCount > 0U) {
        gSimTimeoutCount--;
        status = NEXTION_STATUS_TIMEOUT;
    } else if (NEXTION_STATUS_OK == status) {
        *(uint32_t *)pContent = TEST_GET_VALUE;
    } else {
        // hata kodu aynen döner
    }

    return status;
}

/**
 * @brief nextion_process() için CMock geri çağırımı
 * @param numCalls CMock çağrı sayacı (kullanılmıyor)
 * @retval None
 */
static void cbNextionProcess(int numCalls) {

    (void)numCalls;
    gSimProcessCalls++;
}


/* =========================================================================
 * setUp / tearDown
 * ========================================================================= */

void setUp(void) {

    virtual_mem_set_all();

    HAL_GetTick_StubWithCallback(cbHalGetTick);
    HAL_GPIO_WritePin_StubWithCallback(cbHalGpioWritePin);
    nextion_sendCmd_StubWithCallback(cbNextionSendCmd);
    nextion_sendNum_StubWithCallback(cbNextionSendNum);
    nextion_getVarBlocking_StubWithCallback(cbNextionGetVarBlocking);
    nextion_process_StubWithCallback(cbNextionProcess);

    // core.c durumu
    (void)memset(gTestCh, 0x00, sizeof(gTestCh));
    (void)memset(gpChannels, 0x00, sizeof(gpChannels));
    gChannelCount = 0U;
    gMutex = NULL;
    gPwrYeni = false;
    gHighVolt = 0U;
    gDacVeri = 0U;
    gAcikSayfa = UI_PAGE_BOOT;
    gMonoPwrKoruma = 0U;
    (void)memset(&gBipolar1Cut, 0x00, sizeof(gBipolar1Cut));

    for (uint32_t i = 0U; i < ADC_CH_COUNT; i++) {
        gAdc1DmaBuff[i] = 0U;
    }

    // genx_PwrDegeriniOku() içindeki `static zatenAktif` değişkenini sahipsiz çağrı ile sıfırlar
    genx_PwrDegeriniOku();

    // Sahte ortam durumu
    (void)memset(gNumLog, 0x00, sizeof(gNumLog));
    gSimNumCalls = 0U;
    gFakeTickMs = 0U;
    gFakeTickStepMs = 0U;
    gSimSendCalls = 0U;
    gSimProcessCalls = 0U;
    gpSimLastCmd = NULL;
    gSimSendResult = NEXTION_STATUS_OK;
    gSimQueueFullCount = 0U;
    gSimAlwaysQueueFull = false;
    gSimGetCalls = 0U;
    gpSimGetDst = NULL;
    gpSimGetCmd = NULL;
    gSimGetSize = 0U;
    gSimGetResult = NEXTION_STATUS_OK;
    gSimTimeoutCount = 0U;
}

void tearDown(void) {

    gFakeTickStepMs = 0U;
    gSimAlwaysQueueFull = false;
    gSimQueueFullCount = 0U;
    gSimTimeoutCount = 0U;
    gSimSendResult = NEXTION_STATUS_OK;
    gSimGetResult = NEXTION_STATUS_OK;
}


/* =========================================================================
 * genx_dacUygula() testleri
 * ========================================================================= */

/**
 * @brief 0.0 girişi DAC'a 0 yazmalı. DAC kanal 1, 8 bit sağa hizalı kullanılmalı
 */
void test_core_dacUygula_zero_writesZero(void) {

    gDacVeri = 99U;
    ASSERT_DAC_OUTPUT(0.0f, 0U);
}

/**
 * @brief Ondalıklı değerler yukarı yuvarlanmalı (ceil)
 */
void test_core_dacUygula_fraction_roundsUp(void) {

    ASSERT_DAC_OUTPUT(0.1f, 1U);
    ASSERT_DAC_OUTPUT(10.2f, 11U);
    ASSERT_DAC_OUTPUT(12.0f, 12U);
    ASSERT_DAC_OUTPUT(254.5f, 255U);
}

/**
 * @brief Negatif girişler 0'a sınırlanmalı. -0.5 ceil sonucu -0 olduğundan yine 0 çıkmalı
 */
void test_core_dacUygula_negative_clampsToZero(void) {

    ASSERT_DAC_OUTPUT(-0.5f, 0U);
    ASSERT_DAC_OUTPUT(-1.0f, 0U);
    ASSERT_DAC_OUTPUT(-100.5f, 0U);
    ASSERT_DAC_OUTPUT(-INFINITY, 0U);
}

/**
 * @brief NaN girişi güvenli şekilde 0'a düşmeli
 */
void test_core_dacUygula_nan_clampsToZero(void) {

    ASSERT_DAC_OUTPUT(NAN, 0U);
}

/**
 * @brief Üst sınır 255 aynen yazılmalı
 */
void test_core_dacUygula_upperBoundary_writes255(void) {

    ASSERT_DAC_OUTPUT(255.0f, 255U);
}

/**
 * @brief 255'i aşan girişler (ceil dahil) ve +INF 255'e sınırlanmalı
 */
void test_core_dacUygula_aboveRange_clampsTo255(void) {

    ASSERT_DAC_OUTPUT(255.1f, 255U);
    ASSERT_DAC_OUTPUT(256.0f, 255U);
    ASSERT_DAC_OUTPUT(1.0e9f, 255U);
    ASSERT_DAC_OUTPUT(INFINITY, 255U);
}

/**
 * @brief Her çağrı gDacVeri'yi güncellemeli, önceki değeri tutmamalı
 */
void test_core_dacUygula_consecutiveCalls_updateEachTime(void) {

    ASSERT_DAC_OUTPUT(100.0f, 100U);
    ASSERT_DAC_OUTPUT(0.0f, 0U);
    ASSERT_DAC_OUTPUT(30.0f, 30U);
}


/* =========================================================================
 * genx_kanalUygula() testleri
 * ========================================================================= */

/**
 * @brief NULL kanal kayda eklenmemeli
 */
void test_core_kanalUygula_nullChannel_isIgnored(void) {

    genx_kanalUygula(NULL);

    TEST_ASSERT_EQUAL_UINT8(0U, gChannelCount);
}

/**
 * @brief Geçerli kanal kayda eklenmeli
 */
void test_core_kanalUygula_singleChannel_isRegistered(void) {

    genx_kanalUygula(&gTestCh[0]);

    TEST_ASSERT_EQUAL_UINT8(1U, gChannelCount);
    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gpChannels[0]);
}

/**
 * @brief Kanallar çağrı sırasıyla kaydedilmeli
 */
void test_core_kanalUygula_multipleChannels_keepRegistrationOrder(void) {

    genx_kanalUygula(&gTestCh[2]);
    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);

    TEST_ASSERT_EQUAL_UINT8(3U, gChannelCount);
    TEST_ASSERT_EQUAL_PTR(&gTestCh[2], gpChannels[0]);
    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gpChannels[1]);
    TEST_ASSERT_EQUAL_PTR(&gTestCh[1], gpChannels[2]);
}

/**
 * @brief Kayıt CORE_MAX_CH kadar dolduğunda fazladan kanal reddedilmeli
 */
void test_core_kanalUygula_registryFull_ignoresExtraChannel(void) {

    for (uint32_t i = 0U; i < CORE_MAX_CH; i++) {
        genx_kanalUygula(&gTestCh[i]);
    }
    TEST_ASSERT_EQUAL_UINT8(CORE_MAX_CH, gChannelCount);

    genx_kanalUygula(&gTestCh[CORE_MAX_CH]);

    TEST_ASSERT_EQUAL_UINT8(CORE_MAX_CH, gChannelCount);
    TEST_ASSERT_EQUAL_PTR(&gTestCh[CORE_MAX_CH - 1U], gpChannels[CORE_MAX_CH - 1U]);
}


/* =========================================================================
 * genx_sahiplendir()/genx_serbestBirak() testleri
 * ========================================================================= */

/**
 * @brief Sahipsiz çıkış ilk isteyene verilmeli
 */
void test_core_sahiplendir_freeOutput_grantsOwnership(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}

/**
 * @brief Mevcut sahip tekrar istediğinde başarılı dönmeli
 */
void test_core_sahiplendir_sameOwner_isReentrant(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}

/**
 * @brief Çıkış başka kanaldaysa GENX_ERROR dönmeli ve sahiplik değişmemeli
 */
void test_core_sahiplendir_otherOwnerHolds_returnsError(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sahiplendir(&gTestCh[1]));
    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}

/**
 * @brief Sahip bırakınca başka kanal alabilmeli
 */
void test_core_sahiplendir_afterRelease_otherChannelCanAcquire(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    genx_serbestBirak(&gTestCh[0]);

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[1]));
    TEST_ASSERT_EQUAL_PTR(&gTestCh[1], gMutex);
}

/**
 * @brief NULL sahip, başka bir kanal çıkışı tutarken reddedilmeli
 */
void test_core_sahiplendir_nullOwnerWhileHeld_returnsError(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sahiplendir(NULL));
    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}

/**
 * @brief Çıkış boşken NULL sahip GENX_SUCCESS döner fakat çıkış kilitlenmez (gMutex NULL kalır)
 */
void test_core_sahiplendir_nullOwnerWhileFree_returnsSuccessButStaysFree(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(NULL));
    TEST_ASSERT_NULL(gMutex);
}

/**
 * @brief Sahip çıkışı bırakınca gMutex NULL olmalı
 */
void test_core_serbestBirak_owner_releasesOutput(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    genx_serbestBirak(&gTestCh[0]);

    TEST_ASSERT_NULL(gMutex);
}

/**
 * @brief Sahip olmayan kanal çıkışı bırakamamalı
 */
void test_core_serbestBirak_nonOwner_doesNotRelease(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    genx_serbestBirak(&gTestCh[1]);

    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}

/**
 * @brief NULL işaretçi ile çıkış bırakılamamalı
 */
void test_core_serbestBirak_nullWhileHeld_doesNotRelease(void) {

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    genx_serbestBirak(NULL);

    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}

/**
 * @brief Çıkış zaten boşken bırakma çağrısı durumu değiştirmemeli
 */
void test_core_serbestBirak_freeOutput_staysFree(void) {

    genx_serbestBirak(&gTestCh[0]);

    TEST_ASSERT_NULL(gMutex);
}


/* =========================================================================
 * genx_muxSecimi() testleri
 * ========================================================================= */

/**
 * @brief Kanal 0-7 için CD4051 adres pinleri doğru olmalı (bit0=PA8, bit1=PA11, bit2=PA12).
 *        Not: BSRR'ye önce reset sonra set yazıldığından sanal bellekte yalnızca son yazma görünür.
 */
void test_core_muxSecimi_allChannels_mapToAddressPins(void) {
// -- Test için hazırlık adımları ------------------------------
    static const uint32_t expected[8] = {
        0U, GPIO_PIN_8, GPIO_PIN_11, GPIO_PIN_8 | GPIO_PIN_11, GPIO_PIN_12, 
        GPIO_PIN_8 | GPIO_PIN_12, GPIO_PIN_11 | GPIO_PIN_12, 
        GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12
    };
// -------------------------------------------------------------

    for (uint8_t ch = 0U; ch <= 7U; ch++) {

        GPIOA->BSRR = 0xFFFFFFFFU;   // yazmanın gerçekten olduğunu kanıtlar
        genx_muxSecimi(ch);

        TEST_ASSERT_EQUAL_HEX32_MESSAGE(expected[ch], GPIOA->BSRR, "incorrect MUX channel mapping");
    }
}

/**
 * @brief 0-7 dışındaki kanal değerleri registera dokunmamalı
 */
void test_core_muxSecimi_outOfRange_leavesRegisterUntouched(void) {

    GPIOA->BSRR = 0xFFFFFFFFU;

    genx_muxSecimi(8U);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, GPIOA->BSRR);

    genx_muxSecimi(255U);
    TEST_ASSERT_EQUAL_HEX32(0xFFFFFFFFU, GPIOA->BSRR);
}


/* =========================================================================
 * genx_monoAc() testleri
 * ========================================================================= */

/**
 * @brief Mono1 CUT: çıkış açma sırası doğru olmalı (PWM, koruma röleleri, kanal rölesi, MUX, fan)
 */
void test_core_monoAc_mono1Cut_executesSequenceInOrder(void) {

    genx_monoAc(MONO1_RELAY_Pin, MUX_MONO_CUT);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PIN_8, GPIOA->BSRR);   // MUX_MONO_CUT = 1
}

/**
 * @brief Mono2 COAG: Mono2 rölesi seçilmeli ve MUX_MONO_COAG adreslenmeli
 */
void test_core_monoAc_mono2Coag_selectsMono2Relay(void) {

    genx_monoAc(MONO2_RELAY_Pin, MUX_MONO_COAG);
    TEST_ASSERT_EQUAL_HEX32(GPIO_PIN_11, GPIOA->BSRR);  // MUX_MONO_COAG = 2
}

/**
 * @brief Tanımsız röle pini verilirse röle yazılmamalı. Diğer adımlar yine yürütülür
 */
void test_core_monoAc_unknownRelay_skipsRelayWrite(void) {

    genx_monoAc(GPIO_PIN_15, MUX_MONO_CUT);
}


/* =========================================================================
 * genx_bipolarAc()/genx_bipolarKapat() testleri
 * ========================================================================= */

/**
 * @brief Bipolar CUT: CUT rölesi SET, MUX_BIPOLAR_CUT adreslenmeli, sıra doğru olmalı
 */
void test_core_bipolarAc_cut_executesSequenceInOrder(void) {

    genx_bipolarAc(BIPOLAR_MODE_CUT);
    TEST_ASSERT_EQUAL_HEX32((GPIO_PIN_8 | GPIO_PIN_11), GPIOA->BSRR);   // MUX_BIPOLAR_CUT = 3
}

/**
 * @brief Bipolar COAG: CUT rölesi RESET, MUX_BIPOLAR_COAG adreslenmeli
 */
void test_core_bipolarAc_coag_clearsCutRelay(void) {

    genx_bipolarAc(BIPOLAR_MODE_COAG);
    TEST_ASSERT_EQUAL_HEX32((GPIO_PIN_8 | GPIO_PIN_12), GPIOA->BSRR);   // MUX_BIPOLAR_COAG = 5
}

/**
 * @brief Bipolar kapatma: röleler ve MUX kapanmalı, PWM en son durdurulmalı
 */
void test_core_bipolarKapat_executesSequenceInOrder(void) {

    GPIOA->BSRR = 0xFFFFFFFFU;

    genx_bipolarKapat();
    TEST_ASSERT_EQUAL_HEX32(0U, GPIOA->BSRR);   // MUX kapalı (kanal 0)
}


/* =========================================================================
 * genx_sayacBittiMi() testleri
 * ========================================================================= */

/**
 * @brief Koşul sağlanmıyorsa sayaç sıfırlanmalı ve GENX_ERROR dönmeli
 */
void test_core_sayacBittiMi_conditionFalse_clearsCounter(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t start = 12345U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(false, &start));
    TEST_ASSERT_EQUAL_UINT32(0U, start);
}

/**
 * @brief Koşul ilk kez sağlandığında başlangıç tick'i kaydedilmeli
 */
void test_core_sayacBittiMi_firstTrueCall_latchesTick(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t start = 0U;
    gFakeTickMs = 1000U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(true, &start));
    TEST_ASSERT_EQUAL_UINT32(1000U, start);
}

/**
 * @brief 19 ms'de GENX_ERROR, tam 20 ms'de GENX_SUCCESS dönmeli ve sayaç yeniden kurulmalı
 */
void test_core_sayacBittiMi_boundary19and20ms(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t start = 0U;
    gFakeTickMs = 1000U;
    (void)genx_sayacBittiMi(true, &start);
// -------------------------------------------------------------

    gFakeTickMs = 1019U;
    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(true, &start));
    TEST_ASSERT_EQUAL_UINT32(1000U, start);

    gFakeTickMs = 1020U;
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sayacBittiMi(true, &start));
    TEST_ASSERT_EQUAL_UINT32(0U, start);
}

/**
 * @brief Süre dolmadan koşul bozulursa pencere yeniden başlamalı
 */
void test_core_sayacBittiMi_conditionDrops_restartsWindow(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t start = 0U;
    gFakeTickMs = 1000U;
    (void)genx_sayacBittiMi(true, &start);
    gFakeTickMs = 1010U;
    (void)genx_sayacBittiMi(true, &start);
// -------------------------------------------------------------

    gFakeTickMs = 1015U;
    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(false, &start));
    TEST_ASSERT_EQUAL_UINT32(0U, start);

    gFakeTickMs = 1016U;
    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(true, &start));
    TEST_ASSERT_EQUAL_UINT32(1016U, start);

    gFakeTickMs = 1035U;    // eski pencereye göre dolmuş olurdu, yenisine göre 19 ms
    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(true, &start));

    gFakeTickMs = 1036U;
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sayacBittiMi(true, &start));
}

/**
 * @brief Başarıdan sonra gelen ilk çağrı yeni bir pencere başlatmalı
 */
void test_core_sayacBittiMi_afterSuccess_startsNewWindow(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t start = 0U;
    gFakeTickMs = 1000U;
    (void)genx_sayacBittiMi(true, &start);
    gFakeTickMs = 1020U;
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sayacBittiMi(true, &start));
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(true, &start));     // aynı tick'te tekrar
    TEST_ASSERT_EQUAL_UINT32(1020U, start);
}

/**
 * @brief 32 bit tick taşmasında süre doğru hesaplanmalı
 */
void test_core_sayacBittiMi_tickWrapAround_completesAfter20ms(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t start = 0xFFFFFFF5U;
// -------------------------------------------------------------

    gFakeTickMs = 0x00000008U;      // 19 ms
    TEST_ASSERT_EQUAL_INT(GENX_ERROR, genx_sayacBittiMi(true, &start));

    gFakeTickMs = 0x00000009U;      // 20 ms
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sayacBittiMi(true, &start));
}


/* =========================================================================
 * genx_baskaAktifVarMi() testleri
 * ========================================================================= */

/**
 * @brief Kayıtlı kanal ve sahip yokken false dönmeli
 */
void test_core_baskaAktifVarMi_noChannelsNoOwner_returnsFalse(void) {

    TEST_ASSERT_FALSE(genx_baskaAktifVarMi(NULL));
    TEST_ASSERT_FALSE(genx_baskaAktifVarMi(&gTestCh[0]));
}

/**
 * @brief Hiçbir kanalın Start bayrağı açık değilse false dönmeli
 */
void test_core_baskaAktifVarMi_allStartFlagsZero_returnsFalse(void) {

    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);
    genx_kanalUygula(&gTestCh[2]);

    TEST_ASSERT_FALSE(genx_baskaAktifVarMi(&gTestCh[0]));
    TEST_ASSERT_FALSE(genx_baskaAktifVarMi(NULL));
}

/**
 * @brief Başka bir kanalın Start bayrağı açıksa true dönmeli
 */
void test_core_baskaAktifVarMi_otherChannelStarted_returnsTrue(void) {

    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);
    gTestCh[1].start = 1U;

    TEST_ASSERT_TRUE(genx_baskaAktifVarMi(&gTestCh[0]));
}

/**
 * @brief Sorgulayan kanalın kendi Start bayrağı sonucu etkilememeli
 */
void test_core_baskaAktifVarMi_onlyOwnStartFlag_returnsFalse(void) {

    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);
    gTestCh[0].start = 1U;

    TEST_ASSERT_FALSE(genx_baskaAktifVarMi(&gTestCh[0]));
    TEST_ASSERT_TRUE(genx_baskaAktifVarMi(&gTestCh[1]));
}

/**
 * @brief Start bayrağı sıfırdan farklı her değer aktif sayılmalı
 */
void test_core_baskaAktifVarMi_nonZeroStartValue_countsAsActive(void) {

    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);
    gTestCh[1].start = 2U;

    TEST_ASSERT_TRUE(genx_baskaAktifVarMi(&gTestCh[0]));
}

/**
 * @brief pSelf == NULL ile sorgulandığında herhangi bir aktif kanal true döndürmeli
 */
void test_core_baskaAktifVarMi_nullSelf_anyStartedChannelReturnsTrue(void) {

    genx_kanalUygula(&gTestCh[0]);
    gTestCh[0].start = 1U;

    TEST_ASSERT_TRUE(genx_baskaAktifVarMi(NULL));
}

/**
 * @brief Çıkış başka kanalın elindeyse (Start bayrakları kapalı olsa da) true dönmeli
 */
void test_core_baskaAktifVarMi_outputOwnedByOther_returnsTrue(void) {

    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[1]));

    TEST_ASSERT_TRUE(genx_baskaAktifVarMi(&gTestCh[0]));
    TEST_ASSERT_TRUE(genx_baskaAktifVarMi(NULL));
}

/**
 * @brief Çıkış sorgulayan kanalın kendisindeyse ve başka kanal yoksa false dönmeli
 */
void test_core_baskaAktifVarMi_outputOwnedBySelf_returnsFalse(void) {

    genx_kanalUygula(&gTestCh[0]);
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));

    TEST_ASSERT_FALSE(genx_baskaAktifVarMi(&gTestCh[0]));
}


/* =========================================================================
 * genx_hepsiniDurdur() testleri
 * ========================================================================= */

/**
 * @brief Kayıtlı tüm kanalların Start bayrağı sıfırlanmalı
 */
void test_core_hepsiniDurdur_registeredChannels_allCleared(void) {

    genx_kanalUygula(&gTestCh[0]);
    genx_kanalUygula(&gTestCh[1]);
    genx_kanalUygula(&gTestCh[2]);
    gTestCh[0].start = 1U;
    gTestCh[1].start = 1U;
    gTestCh[2].start = 2U;

    genx_hepsiniDurdur();

    TEST_ASSERT_EQUAL_UINT8(0U, gTestCh[0].start);
    TEST_ASSERT_EQUAL_UINT8(0U, gTestCh[1].start);
    TEST_ASSERT_EQUAL_UINT8(0U, gTestCh[2].start);
}

/**
 * @brief Kayıtlı olmayan kanala dokunulmamalı
 */
void test_core_hepsiniDurdur_unregisteredChannel_untouched(void) {

    genx_kanalUygula(&gTestCh[0]);
    gTestCh[0].start = 1U;
    gTestCh[1].start = 1U;

    genx_hepsiniDurdur();

    TEST_ASSERT_EQUAL_UINT8(0U, gTestCh[0].start);
    TEST_ASSERT_EQUAL_UINT8(1U, gTestCh[1].start);
}

/**
 * @brief Kayıt boşken çağrı hatasız dönmeli
 */
void test_core_hepsiniDurdur_noChannels_doesNothing(void) {

    genx_hepsiniDurdur();

    TEST_ASSERT_EQUAL_UINT8(0U, gChannelCount);
    TEST_ASSERT_NULL(gMutex);
}

/**
 * @brief Çıkış sahipliği bırakılmaz, bunu kanalın kendi durdurma yolu yapar
 */
void test_core_hepsiniDurdur_doesNotReleaseOutputOwnership(void) {

    genx_kanalUygula(&gTestCh[0]);
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gTestCh[0].start = 1U;

    genx_hepsiniDurdur();

    TEST_ASSERT_EQUAL_PTR(&gTestCh[0], gMutex);
}


/* =========================================================================
 * genx_PwrDegeriniOku() testleri
 * ========================================================================= */

/**
 * @brief Çıkış sahibi yokken ADC okunmamalı, ekrana yazılmamalı
 */
void test_core_pwrDegeriniOku_noOwner_doesNotRead(void) {
// -- Test için hazırlık adımları ------------------------------
    gHighVolt = 77U;
    gAdc1DmaBuff[ADC_CH_PWR] = 500U;
    gFakeTickMs = 5000U;
// -------------------------------------------------------------

    genx_PwrDegeriniOku();

    TEST_ASSERT_FALSE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(77U, gHighVolt);
    TEST_ASSERT_EQUAL_UINT32(0U, gSimNumCalls);
}

/**
 * @brief Sahiplik alındıktan sonra 80 ms dolmadan okuma yapılmamalı (79 ms: yok, 80 ms: var)
 */
void test_core_pwrDegeriniOku_boundary79and80ms(void) {
// -- Test için hazırlık adımları ------------------------------
    gAdc1DmaBuff[ADC_CH_PWR] = 1234U;
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gFakeTickMs = 1000U;
    genx_PwrDegeriniOku();  // tick kaydedilir
// -------------------------------------------------------------

    TEST_ASSERT_FALSE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(0U, gHighVolt);

    gFakeTickMs = 1079U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_FALSE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(0U, gSimNumCalls);

    gFakeTickMs = 1080U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_TRUE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(1234U, gHighVolt);
}

/**
 * @brief gPwrYeni yalnızca okumanın yapıldığı çağrıda true olmalı, sonraki çağrıda false'a dönmeli
 */
void test_core_pwrDegeriniOku_newFlag_isTrueForSingleCall(void) {
// -- Test için hazırlık adımları ------------------------------
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gFakeTickMs = 1000U;
    genx_PwrDegeriniOku();
    gFakeTickMs = 1080U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_TRUE(gPwrYeni);
// -------------------------------------------------------------

    genx_PwrDegeriniOku();  // aynı tick

    TEST_ASSERT_FALSE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(3U, gSimNumCalls);
}

/**
 * @brief Okumalar 80 ms aralıkla tekrarlanmalı ve her seferinde güncel ADC değeri alınmalı
 */
void test_core_pwrDegeriniOku_periodicReads_every80ms(void) {
// -- Test için hazırlık adımları ------------------------------
    gAdc1DmaBuff[ADC_CH_PWR] = 1234U;
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gFakeTickMs = 2000U;
    genx_PwrDegeriniOku();
    gFakeTickMs = 2080U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_EQUAL_UINT32(1234U, gHighVolt);
    gAdc1DmaBuff[ADC_CH_PWR] = 900U;
// -------------------------------------------------------------

    gFakeTickMs = 2159U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_FALSE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(1234U, gHighVolt);

    gFakeTickMs = 2160U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_TRUE(gPwrYeni);
    TEST_ASSERT_EQUAL_UINT32(900U, gHighVolt);
    TEST_ASSERT_EQUAL_UINT32(6U, gSimNumCalls);
}

/**
 * @brief Tick taşmasında 80 ms doğru hesaplanmalı
 */
void test_core_pwrDegeriniOku_tickWrapAround_readsAfter80ms(void) {
// -- Test için hazırlık adımları ------------------------------
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gFakeTickMs = 0xFFFFFFF0U;
    genx_PwrDegeriniOku();
// -------------------------------------------------------------

    gFakeTickMs = 0x0000003FU;  // 79 ms
    genx_PwrDegeriniOku();
    TEST_ASSERT_FALSE(gPwrYeni);

    gFakeTickMs = 0x00000040U;  // 80 ms
    genx_PwrDegeriniOku();
    TEST_ASSERT_TRUE(gPwrYeni);
}

/**
 * @brief Sahiplik bırakılıp yeniden alınınca 80 ms gecikme baştan başlamalı
 */
void test_core_pwrDegeriniOku_ownershipReacquired_restartsDelay(void) {
// -- Test için hazırlık adımları ------------------------------
    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gFakeTickMs = 1000U;
    genx_PwrDegeriniOku();
    gFakeTickMs = 1080U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_TRUE(gPwrYeni);

    genx_serbestBirak(&gTestCh[0]);
    gFakeTickMs = 1100U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_FALSE(gPwrYeni);    // sahip yok

    TEST_ASSERT_EQUAL_INT(GENX_SUCCESS, genx_sahiplendir(&gTestCh[0]));
    gFakeTickMs = 5000U;            // uzun süre geçmiş olsa da
// -------------------------------------------------------------

    genx_PwrDegeriniOku();
    TEST_ASSERT_FALSE(gPwrYeni);    // yeni sahiplikte hemen okunmamalı

    gFakeTickMs = 5079U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_FALSE(gPwrYeni);

    gFakeTickMs = 5080U;
    genx_PwrDegeriniOku();
    TEST_ASSERT_TRUE(gPwrYeni);
}


/* =========================================================================
 * genx_sendCmd() testleri
 * ========================================================================= */

/**
 * @brief Kuyruk müsaitse komut tek denemede aynen iletilmeli, nextion_process() çağrılmamalı
 */
void test_core_sendCmd_queueAvailable_passesCommandOnce(void) {
// -- Test için hazırlık adımları ------------------------------
    const char *pCmd = "j0.val=1";
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_sendCmd(pCmd));

    TEST_ASSERT_EQUAL_UINT32(1U, gSimSendCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, gSimProcessCalls);
    TEST_ASSERT_EQUAL_PTR(pCmd, gpSimLastCmd);
}

/**
 * @brief Kuyruk dolu dönerse her denemeden sonra sürücü çalıştırılıp tekrar denenmeli
 */
void test_core_sendCmd_queueFullTwice_retriesUntilAccepted(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimQueueFullCount = 2U;
    gFakeTickStepMs = 1U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_sendCmd("j0.val=1"));

    TEST_ASSERT_EQUAL_UINT32(3U, gSimSendCalls);
    TEST_ASSERT_EQUAL_UINT32(2U, gSimProcessCalls);
}

/**
 * @brief Kuyruk sürekli doluysa GENX_KUYRUK_BEKLE_MS dolunca QUEUE_FULL ile vazgeçilmeli
 */
void test_core_sendCmd_queueAlwaysFull_givesUpAfterWaitWindow(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimAlwaysQueueFull = true;
    gFakeTickStepMs = TEST_QUEUE_STEP_MS;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, genx_sendCmd("j0.val=1"));

    TEST_ASSERT_EQUAL_UINT32(GENX_KUYRUK_BEKLE_MS/TEST_QUEUE_STEP_MS, gSimSendCalls);
    TEST_ASSERT_EQUAL_UINT32(GENX_KUYRUK_BEKLE_MS/TEST_QUEUE_STEP_MS, gSimProcessCalls);
}

/**
 * @brief Bekleme penceresi tick taşmasında da aynı sürede dolmalı
 */
void test_core_sendCmd_queueAlwaysFull_tickWrapAround_givesUp(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimAlwaysQueueFull = true;
    gFakeTickMs = 0xFFFFFFFBU;
    gFakeTickStepMs = TEST_QUEUE_STEP_MS;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, genx_sendCmd("j0.val=1"));

    TEST_ASSERT_EQUAL_UINT32(GENX_KUYRUK_BEKLE_MS/TEST_QUEUE_STEP_MS, gSimSendCalls);
}

/**
 * @brief QUEUE_FULL dışındaki hatalar tekrar denenmeden aynen döndürülmeli
 */
void test_core_sendCmd_otherError_returnedWithoutRetry(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimSendResult = NEXTION_STATUS_INVALID_PARAM;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, genx_sendCmd(NULL));

    TEST_ASSERT_EQUAL_UINT32(1U, gSimSendCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, gSimProcessCalls);
}


/* =========================================================================
 * genx_sendNum() testleri
 * ========================================================================= */

/**
 * @brief Argümanlar nextion_sendNum()'a aynen iletilmeli, nextion_process() çağrılmamalı
 */
void test_core_sendNum_queueAvailable_forwardsArguments(void) {
// -- Test için hazırlık adımları ------------------------------
    char buf[16];
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_sendNum(buf, sizeof(buf), "n0.val=", -5));

    TEST_ASSERT_EQUAL_UINT32(1U, gSimNumCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, gSimProcessCalls);
    TEST_ASSERT_EQUAL_PTR(buf, gNumLog[0].pBuf);
    TEST_ASSERT_EQUAL_UINT32(sizeof(buf), gNumLog[0].bufSize);
    TEST_ASSERT_EQUAL_STRING("n0.val=", gNumLog[0].prefix);
    TEST_ASSERT_EQUAL_INT16(-5, gNumLog[0].num);
}

/**
 * @brief Kuyruk dolu dönerse sürücü çalıştırılıp tekrar denenmeli
 */
void test_core_sendNum_queueFullTwice_retriesUntilAccepted(void) {
// -- Test için hazırlık adımları ------------------------------
    char buf[16];
    gSimQueueFullCount = 2U;
    gFakeTickStepMs = 1U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_sendNum(buf, sizeof(buf), "n0.val=", 1));

    TEST_ASSERT_EQUAL_UINT32(3U, gSimNumCalls);
    TEST_ASSERT_EQUAL_UINT32(2U, gSimProcessCalls);
}

/**
 * @brief Kuyruk sürekli doluysa GENX_KUYRUK_BEKLE_MS dolunca QUEUE_FULL ile vazgeçilmeli
 */
void test_core_sendNum_queueAlwaysFull_givesUpAfterWaitWindow(void) {
// -- Test için hazırlık adımları ------------------------------
    char buf[16];
    gSimAlwaysQueueFull = true;
    gFakeTickStepMs = TEST_QUEUE_STEP_MS;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, genx_sendNum(buf, sizeof(buf), "n0.val=", 1));

    TEST_ASSERT_EQUAL_UINT32(GENX_KUYRUK_BEKLE_MS/TEST_QUEUE_STEP_MS, gSimNumCalls);
    TEST_ASSERT_EQUAL_UINT32(GENX_KUYRUK_BEKLE_MS/TEST_QUEUE_STEP_MS, gSimProcessCalls);
}

/**
 * @brief QUEUE_FULL dışındaki hatalar tekrar denenmeden aynen döndürülmeli
 */
void test_core_sendNum_otherError_returnedWithoutRetry(void) {
// -- Test için hazırlık adımları ------------------------------
    char buf[16];
    gSimSendResult = NEXTION_STATUS_INVALID_PARAM;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, genx_sendNum(buf, sizeof(buf), "n0.val=", 1));

    TEST_ASSERT_EQUAL_UINT32(1U, gSimNumCalls);
    TEST_ASSERT_EQUAL_UINT32(0U, gSimProcessCalls);
}


/* =========================================================================
 * genx_getVarRetry() testleri
 * ========================================================================= */

/**
 * @brief İlk denemede başarılıysa tekrar denenmemeli. Argümanlar iletilmeli, değer hedefe yazılmalı
 */
void test_core_getVarRetry_firstAttemptOk_noRetry(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t value = 0U;
    const char *pCmd = "get S2_m1_cut_watt.USER_VALUE.val";
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_getVarRetry(&value, pCmd, sizeof(value)));

    TEST_ASSERT_EQUAL_UINT32(1U, gSimGetCalls);
    TEST_ASSERT_EQUAL_PTR(&value, gpSimGetDst);
    TEST_ASSERT_EQUAL_PTR(pCmd, gpSimGetCmd);
    TEST_ASSERT_EQUAL_UINT16(sizeof(value), gSimGetSize);
    TEST_ASSERT_EQUAL_HEX32(TEST_GET_VALUE, value);
}

/**
 * @brief Zaman aşımında tekrar denenmeli ve ilk başarıda durmalı
 */
void test_core_getVarRetry_timeoutsThenOk_retriesUntilSuccess(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t value = 0U;
    gSimTimeoutCount = 3U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_getVarRetry(&value, "get j0.val", sizeof(value)));

    TEST_ASSERT_EQUAL_UINT32(4U, gSimGetCalls);
    TEST_ASSERT_EQUAL_HEX32(TEST_GET_VALUE, value);
}

/**
 * @brief Son izinli deneme (NEXTION_MAX_DENEME) başarılıysa sonuç başarılı olmalı
 */
void test_core_getVarRetry_successOnLastAttempt_returnsOk(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t value = 0U;
    gSimTimeoutCount = NEXTION_MAX_DENEME - 1U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_getVarRetry(&value, "get j0.val", sizeof(value)));

    TEST_ASSERT_EQUAL_UINT32(NEXTION_MAX_DENEME, gSimGetCalls);
    TEST_ASSERT_EQUAL_HEX32(TEST_GET_VALUE, value);
}

/**
 * @brief Tüm denemeler zaman aşımına uğrarsa tam NEXTION_MAX_DENEME deneme yapılıp TIMEOUT dönmeli
 */
void test_core_getVarRetry_alwaysTimeout_stopsAtMaxAttempts(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t value = 0U;
    gSimTimeoutCount = 100U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_TIMEOUT, genx_getVarRetry(&value, "get j0.val", sizeof(value)));

    TEST_ASSERT_EQUAL_UINT32(NEXTION_MAX_DENEME, gSimGetCalls);
    TEST_ASSERT_EQUAL_HEX32(0U, value);
}

/**
 * @brief Zaman aşımı dışındaki hatalar tekrar denenmeden aynen döndürülmeli
 */
void test_core_getVarRetry_nonTimeoutError_returnedWithoutRetry(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t value = 0U;
// -------------------------------------------------------------

    gSimGetResult = NEXTION_STATUS_INVALID_PARAM;
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, genx_getVarRetry(&value, "get j0.val", sizeof(value)));
    TEST_ASSERT_EQUAL_UINT32(1U, gSimGetCalls);

    gSimGetResult = NEXTION_STATUS_EMPTY;
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, genx_getVarRetry(&value, "get j0.val", sizeof(value)));
    TEST_ASSERT_EQUAL_UINT32(2U, gSimGetCalls);
}


/* =========================================================================
 * genx_setPage() testleri
 * ========================================================================= */

/**
 * @brief Başarılı geçişte "page N" komutu gNumBuff ile gönderilmeli ve gAcikSayfa güncellenmeli
 */
void test_core_setPage_success_sendsCommandAndUpdatesPage(void) {

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_setPage(UI_PAGE_MAIN));

    TEST_ASSERT_EQUAL_UINT32(1U, gSimNumCalls);
    ASSERT_NUM_CALL(0U, "page ", UI_PAGE_MAIN);
    TEST_ASSERT_EQUAL_INT(UI_PAGE_MAIN, gAcikSayfa);
}

/**
 * @brief Ardışık geçişlerde her sayfa numarası doğru iletilmeli
 */
void test_core_setPage_consecutiveCalls_sendEachPageNumber(void) {

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_setPage(UI_PAGE_ERROR));
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, genx_setPage(UI_PAGE_MONO1_ENDO));

    TEST_ASSERT_EQUAL_UINT32(2U, gSimNumCalls);
    ASSERT_NUM_CALL(0U, "page ", UI_PAGE_ERROR);
    ASSERT_NUM_CALL(1U, "page ", UI_PAGE_MONO1_ENDO);
    TEST_ASSERT_EQUAL_INT(UI_PAGE_MONO1_ENDO, gAcikSayfa);
}

/**
 * @brief Kuyruk dolu kalırsa hata dönmeli ve gAcikSayfa değişmemeli
 */
void test_core_setPage_queueFull_keepsCurrentPage(void) {
// -- Test için hazırlık adımları ------------------------------
    gAcikSayfa = UI_PAGE_PLATE;
    gSimAlwaysQueueFull = true;
    gFakeTickStepMs = TEST_QUEUE_STEP_MS;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, genx_setPage(UI_PAGE_MAIN));

    TEST_ASSERT_EQUAL_INT(UI_PAGE_PLATE, gAcikSayfa);
}

/**
 * @brief Başka bir hata dönerse gAcikSayfa değişmemeli
 */
void test_core_setPage_sendError_keepsCurrentPage(void) {
// -- Test için hazırlık adımları ------------------------------
    gAcikSayfa = UI_PAGE_PLATE;
    gSimSendResult = NEXTION_STATUS_INVALID_PARAM;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, genx_setPage(UI_PAGE_MAIN));

    TEST_ASSERT_EQUAL_INT(UI_PAGE_PLATE, gAcikSayfa);
}


/* =========================================================================
 * genx_delay() testleri
 * ========================================================================= */

/**
 * @brief 0 ms beklemede sürücü çalıştırılmamalı
 */
void test_core_delay_zeroMs_doesNotServiceNextion(void) {

    gFakeTickStepMs = 1U;

    genx_delay(0U);

    TEST_ASSERT_EQUAL_UINT32(0U, gSimProcessCalls);
}

/**
 * @brief Bekleme boyunca nextion_process() çağrılmalı (1 ms adımla 10 ms: 9 döngü)
 */
void test_core_delay_tenMs_servicesNextionWhileWaiting(void) {

    gFakeTickStepMs = 1U;

    genx_delay(10U);

    TEST_ASSERT_EQUAL_UINT32(9U, gSimProcessCalls);
}

/**
 * @brief Tick taşmasında bekleme doğru sürede sonlanmalı
 */
void test_core_delay_tickWrapAround_endsAfterRequestedTime(void) {

    gFakeTickMs = 0xFFFFFFFEU;
    gFakeTickStepMs = 1U;

    genx_delay(5U);

    TEST_ASSERT_EQUAL_UINT32(4U, gSimProcessCalls);
}


/* =========================================================================
 * Makro ve sabit tablo testleri (core.h)
 * ========================================================================= */

/**
 * @brief MODE_BIT yalnızca AcikMi == 1 iken ilgili biti üretmeli
 */
void test_core_modeBit_enabledAndDisabled(void) {

    TEST_ASSERT_EQUAL_HEX32(0x00000002U, MODE_BIT(1U, 1U));
    TEST_ASSERT_EQUAL_HEX32(0x00001000U, MODE_BIT(12U, 1U));
    TEST_ASSERT_EQUAL_HEX32(0U, MODE_BIT(12U, 0U));
    TEST_ASSERT_EQUAL_HEX32(0U, MODE_BIT(12U, 2U));     // yalnızca 1U açık sayılır
}

/**
 * @brief NEXTION_TOUCH_KEY sayfayı üst bayta, nesne ID'sini alt bayta yerleştirmeli
 */
void test_core_touchKey_encodesPageAndComponent(void) {

    TEST_ASSERT_EQUAL_HEX16(0x010FU, NEXTION_TOUCH_KEY(UI_PAGE_MAIN, 15U));
    TEST_ASSERT_EQUAL_HEX16(0x010FU, S1__MONO1_CUT_MODE_BUTTON);
    TEST_ASSERT_EQUAL_HEX16(0x1D02U, S29__OK_BUTTON);
    TEST_ASSERT_EQUAL_HEX16(0x1F18U, S31__OK_BUTTON);
}

/**
 * @brief Alarm tablosu her ses tonu için tek kayıt ve beklenen PSC değerini içermeli
 */
void test_core_alarmConfigTable_hasEntryPerTone(void) {

    TEST_ASSERT_EQUAL_UINT8(4U, ALARM_COUNT);

    TEST_ASSERT_EQUAL_INT(ALARM_SES_MONO_CUT, gAlarmConfigTable[0].sesTonu);
    TEST_ASSERT_EQUAL_UINT16(210U, gAlarmConfigTable[0].PSC);
    TEST_ASSERT_EQUAL_INT(ALARM_SES_MONO_COAG, gAlarmConfigTable[1].sesTonu);
    TEST_ASSERT_EQUAL_UINT16(280U, gAlarmConfigTable[1].PSC);
    TEST_ASSERT_EQUAL_INT(ALARM_SES_BIPOLAR_CUT, gAlarmConfigTable[2].sesTonu);
    TEST_ASSERT_EQUAL_UINT16(320U, gAlarmConfigTable[2].PSC);
    TEST_ASSERT_EQUAL_INT(ALARM_SES_BIPOLAR_COAG, gAlarmConfigTable[3].sesTonu);
    TEST_ASSERT_EQUAL_UINT16(390U, gAlarmConfigTable[3].PSC);
}