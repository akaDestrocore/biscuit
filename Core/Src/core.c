/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    core.c
 * @brief   Modüller arası paylaşılan durum ve donanım yardımcıları
 *
 * @author  destrocore
 * @date    2026
 */

#include "core.h"


// -- Numaralı komutlar için kullanılacak olan global bir buffer 
char gNumBuff[NEXTION_CMD_MAX_LEN + 1U];

// -- ADC / DAC ------------------------------------------------

// DMA buraya CPU'yu meşgul etmeye gerek kalmadan arka planda yazacak
volatile uint16_t gAdc1DmaBuff[5];
volatile uint8_t gDacVeri = 0U;
uint32_t gHighVolt = 0U;
bool gPwrYeni = false;

// -- Sayfa / plate / alarm ------------------------------------
UI_Page_e gAcikSayfa = UI_PAGE_BOOT;
uint32_t gPlate = 0U;
uint8_t gPlate100Overresistance = 0U;
uint8_t gPlateError = 0U;
uint8_t gAlarmPlate = 0U;
uint8_t gDusukOncelikAlarm = 0U;
uint16_t gDusukAlarmSay = 0U;
uint8_t gMonopolarCutSes = 0U;
uint8_t gMonopolarCoagSes = 0U;
uint8_t gBipolarCutSes = 0U;
uint8_t gBipolarCoagSes = 0U;

// -- Bipolar yardımcı bayrakları ------------------------------
uint8_t gMono1Pedal = 0U;
uint8_t gMono2Pedal = 0U;
uint8_t gBipolar1PedalCift = 0U;
uint8_t gBipolar2PedalCift = 0U;
uint8_t gBipolarHand = 0U;
uint8_t gBipolarAutoStop = 0U;
uint8_t gLigasureStart = 0U;
uint8_t gLigasurePedal = 0U;
uint32_t gLigasureWattYaz = 0U;
uint8_t gAutoStop2 = 0U;
uint8_t gAutoStopCiftPedal = 0U;
uint8_t gAutoStopTekPedal = 0U;

// -- Hata bayrakları ------------------------------------------
uint8_t gPedalError3 = 0U;
uint8_t gPedal2KezError = 0U;
uint8_t gPwrKoruma = 0U;
uint8_t gBiPwrKoruma = 0U;
uint8_t gMonoPwrKoruma = 0U;
uint8_t gBipolarKoruma = 0U;
uint8_t gMono1Koruma = 0U;
uint8_t gMono2Koruma = 0U;

// -- Ekran ayarları -------------------------------------------
uint32_t gEndoCutKademe = 1U;
uint32_t gEndoCoagKademe = 1U;
uint32_t gEndoCutSure = 1U;
uint32_t gEndoCoagSure = 1U;
uint32_t gSesVeri = 0U;

// -- Kanal kaydı ve çıkış sahipliği ---------------------------
static Core_Channel_t *gpChannels[CORE_MAX_CHANNELS];
static uint8_t gChannelCount = 0U;
static const Core_Channel_t *gpOwner = NULL;

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief DAC çıkışını 0-255 aralığında sınırlayıp yazar
  * @param val Ham DAC değeri
  * @retval None
  */
void genx_dacSet(float val) {

    float edge = ceilf(val);

    if (false == (edge >= 0.0f)) {
        edge = 0.0f;
    }

    if (edge > 255.0f) {
        edge = 255.0f;
    }

    gDacVeri = (uint8_t)edge;
    HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_8B_R, gDacVeri);
}

/**
  * @brief CD4051 kanalını seçer
  * @param ch 0-7 (bit2=PA12, bit1=PA11, bit0=PA8), 0 hepsini kapatır
  * @retval None
  */
void genx_muxSelect(uint8_t ch) {

    if (ch <= 7U) {
        uint32_t allBits = (uint32_t)GPIO_PIN_8 | (uint32_t)GPIO_PIN_11 | (uint32_t)GPIO_PIN_12;
        uint32_t setBits = 0U;

        if (0U != (ch & 0x01U)) {

            setBits |= GPIO_PIN_8;
        }

        if (0U != (ch & 0x02U)) {

            setBits |= GPIO_PIN_11;
        }

        if (0U != (ch & 0x04U)) {

            setBits |= GPIO_PIN_12;
        }

        GPIOA->BSRR = setBits | ((allBits & ~setBits) << 16U);
    }
}

/**
  * @brief Monopolar çıkış yolunu açar
  * @param relayPin Kanal rölesi (PD1 yada PD3)
  * @param muxCh CD4051 kanalı (MUX_MONO_CUT / MUX_MONO_COAG)
  * @retval None
  */
void genx_monoEnable(uint16_t relayPin, uint8_t muxCh) {

    HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1);                          // PIN_A6 // 380KHz
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11, GPIO_PIN_RESET);              // Monopolar Relay
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_4, GPIO_PIN_SET);                 // CUT koruma
    HAL_GPIO_WritePin(GPIOD, relayPin | GPIO_PIN_8, GPIO_PIN_SET);      // Kanal ve güç röleleri
    genx_muxSelect(muxCh);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);                 // FAN
}

/**
  * @brief Bipolar çıkış yolunu açar
  * @param mode `BIPOLAR_MODE_CUT` veya `BIPOLAR_MODE_COAG`
  * @retval None
  */
void genx_bipolarEnable(Bipolar_Mode_e mode) {

    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, (BIPOLAR_MODE_CUT == mode) ? GPIO_PIN_SET : GPIO_PIN_RESET); // Bipolar Cut Relay
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11 | GPIO_PIN_8, GPIO_PIN_SET);   // Bipolar Relay, Power Relay On
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);                // 74LS
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET);                // 74LS
    genx_muxSelect((BIPOLAR_MODE_CUT == mode) ? MUX_BIPOLAR_CUT : MUX_BIPOLAR_COAG);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);                 // FAN
}

/**
  * @brief Bipolar çıkış yolunu kapatır
  * @retval None
  */
void genx_bipolarDisable(void) {

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);              // Bipolar Cut Relay
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11 | GPIO_PIN_8, GPIO_PIN_RESET); // Bipolar Relay, Power Relay Off
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);              // 74LS
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET);              // 74LS
    genx_muxSelect(0U);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);               // FAN
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
}

/**
  * @brief Yüksek güç kaynağı voltajını, çıkış sahipliği alındıktan en az 80 ms sonra okur
  * @retval None
  * @note Yeni değer varsa bu çağrı için gPwrYeni = true olur
  */
void genx_getPwrVal(void) {

    static uint32_t lastTick = 0U;
    static bool isWasActive = false;
    bool isNowActive = (NULL != gpOwner);

    gPwrYeni = false;

    if ((true == isNowActive) && (false == isWasActive)) {
        lastTick = HAL_GetTick();
    }

    isWasActive = isNowActive;

    if ((true == isNowActive) && ((HAL_GetTick() - lastTick) >= 80U)) {     // 80 ms deneme yanılma ile bulundu
        lastTick = HAL_GetTick();
        gHighVolt = gAdc1DmaBuff[ADC_CH_PWR];
        gPwrYeni = true;
    }
}

/**
  * @brief Koşul sağlandığı sürece sayar, eşiğe (200) ulaşınca sayacı sıfırlar
  * @param isCondition Sayılacak koşul
  * @param pCnt Sayaç değişkenine işaretçi
  * @retval true sayaç bitti, false henüz bitmedi
  */
bool genx_isCounterDone(bool isCondition, uint8_t *pCnt) {

    bool isDone = false;

    if (true == isCondition) {
        (*pCnt)++;

        if (200U <= *pCnt) {
            *pCnt = 0U;
            isDone = true;
        }
    }

    return isDone;
}

/**
  * @brief Bir yolu kanal kaydına ekler (başlangıçta bir kez çağrılır)
  * @param pCh Yolun ortak başlığı (Core_Channel_t)
  * @retval None
  */
void genx_registerChannel(Core_Channel_t *pCh) {

    if (NULL == pCh) {
        return;
    }
    if (gChannelCount < CORE_MAX_CHANNELS) {

        gpChannels[gChannelCount] = pCh;
        gChannelCount++;
    }
}

/**
  * @brief Güç çıkışı sahipliğini almaya çalışır. Aynı sahip tekrar çağırırsa başarılıdır.
  * @param pOwner Sahipliği isteyen yolun ortak başlığı
  * @retval 0 sahiplik alındı, 1 başka bir yol sahip
  */
uint8_t genx_claim(const Core_Channel_t *pOwner) {

    uint8_t result = 1U;

    if ((NULL == gpOwner) || (pOwner == gpOwner)) {
        gpOwner = pOwner;
        result = 0U;
    }

    return result;
}

/**
  * @brief Güç çıkışı sahipliğini bırakır (yalnızca mevcut sahip bırakabilir)
  * @param pOwner Sahipliği bırakan yolun ortak başlığı
  * @retval None
  */
void genx_release(const Core_Channel_t *pOwner) {

    if (pOwner == gpOwner) {
        gpOwner = NULL;
    }
}

/**
  * @brief Sorgulanan yol dışında aktif (Start bayrağı açık yada çıkış sahibi) başka yol var mı?
  * @param pSelf Sorgulayan yolun ortak başlığı, hiçbir yol sorgulamıyorsa NULL
  * @retval true başka yol aktif, false hiçbiri aktif değil
  */
bool genx_isOtherChannelActive(const Core_Channel_t *pSelf) {

    bool isActive = false;

    for (uint8_t i = 0U; i < gChannelCount; i++) {
        if ((pSelf != gpChannels[i]) && (0U != gpChannels[i]->start)) {
            isActive = true;
        }
    }

    if ((NULL != gpOwner) && (pSelf != gpOwner)) {
        isActive = true;
    }

    return isActive;
}

/**
  * @brief Kayıtlı tüm yolların Start bayrağını kapatır
  * @retval None
  */
void genx_requestStopAll(void) {

    for (uint8_t i = 0U; i < gChannelCount; i++) {
        gpChannels[i]->start = 0U;
    }
}

/**
  * @brief NEXTION metin komutunu ACK gelene kadar en fazla NEXTION_MAX_DENEME kez gönderir
  * @param pCmd Gönderilecek olan metin komutu
  * @retval NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT, aksi halde kuyruk hata kodu
  */
Nextion_Status_e NEXTION_sendCmdRetry(const char *pCmd) {

    Nextion_Status_e status = NEXTION_STATUS_TIMEOUT;
    uint8_t attempt = 0U;

    do {
        status = nextion_sendCmdBlocking(pCmd, NEXTION_TIMEOUT_MS);
        attempt++;
    } while ((NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

    return status;
}

/**
  * @brief NEXTION sayı içeren komutu ACK gelene kadar en fazla NEXTION_MAX_DENEME kez gönderir
  * @param pBuf Metin önek ve sayıyı içerecek buffer
  * @param bufSize Bufferın tam boyutu
  * @param pPrefix Önek satır metni
  * @param num Komuttaki sayı
  * @retval NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT, aksi halde kuyruk hata kodu
  */
Nextion_Status_e NEXTION_sendNumRetry(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num) {

    Nextion_Status_e status = NEXTION_STATUS_TIMEOUT;
    uint8_t attempt = 0U;

    do {
        status = nextion_sendNumBlocking(pBuf, bufSize, pPrefix, num, NEXTION_TIMEOUT_MS);
        attempt++;
    } while ((NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

    return status;
}

/**
  * @brief NEXTION değişken değerini en fazla NEXTION_MAX_DENEME kez sorgular
  * @param pDst Değerin yazılacağı değişkenin göstergeci
  * @param pCmd `get` ile başlayan sorgu komutu
  * @param size Değişkenin boyutu
  * @retval NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT, aksi halde kuyruk hata kodu
  */
Nextion_Status_e NEXTION_getVarRetry(void *pDst, const char *pCmd, uint16_t size) {

    Nextion_Status_e status = NEXTION_STATUS_TIMEOUT;
    uint8_t attempt = 0U;

    do {
        status = nextion_getVarBlocking(pDst, pCmd, size);
        attempt++;
    } while ((NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

    return status;
}

/**
  * @brief NEXTION sayfaya geçiş komutu, başarılıysa gAcikSayfa güncellenir
  * @param page `UI_Page_e` tipindeki sayfa IDsi
  * @retval NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT, aksi halde kuyruk hata kodu
  */
Nextion_Status_e NEXTION_setPage(uint16_t page) {

    Nextion_Status_e status = NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), "page ", (int16_t)page);

    if (NEXTION_STATUS_OK == status) {
        gAcikSayfa = (UI_Page_e)page;
    }

    return status;
}