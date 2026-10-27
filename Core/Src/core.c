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

#include "genx.h"

// -- Numaralı komutlar için kullanılacak olan global bir buffer 
char gNumBuff[NEXTION_CMD_MAX_LEN + 1U];

// -- ADC / DAC ------------------------------------------------

// DMA buraya CPU'yu meşgul etmeye gerek kalmadan arka planda yazacak
volatile uint16_t gAdc1DmaBuff[5];

volatile uint8_t gDacVeri = 0U;
bool gPwrYeni = false;
uint32_t gHighVolt = 0U;

// -- Monopolar yardımcı bayrakları ----------------------------
uint8_t gMono1Pedal = 0U;
uint8_t gMono2Pedal = 0U;

// -- Bipolar yardımcı bayrakları ------------------------------
uint32_t gLigasureWattYaz = 0U;
uint8_t gBipolarAutoStop = 0U;
uint8_t gLigasureStart = 0U;
uint8_t gLigasurePedal = 0U;
uint8_t gAutoStop2 = 0U;
uint8_t gBipolar1PedalCift = 0U;
uint8_t gBipolar2PedalCift = 0U;
uint8_t gBipolarHand = 0U;
uint8_t gAutoStopTekPedal = 0U;
uint8_t gAutoStopCiftPedal = 0U;

// -- Hata bayrakları ------------------------------------------
uint8_t gMono1Koruma = 0U;
uint8_t gMono2Koruma = 0U;
uint8_t gPwrKoruma = 0U;
uint8_t gMonoPwrKoruma = 0U;
uint8_t gPedalError3 = 0U;
uint8_t gBipolarKoruma = 0U;
uint8_t gBiPwrKoruma = 0U;

// -- Kanal kaydı ve çıkış sahipliği ---------------------------
static Core_Channel_t *gpChannels[CORE_MAX_CH];
static uint8_t gChannelCount = 0U;
static const Core_Channel_t *gMutex = NULL;

// -- Sayfa/plate/alarm ----------------------------------------
uint8_t gPlate100Overresistance = 0U;
uint8_t gMonopolarCutSes = 0U;
uint8_t gMonopolarCoagSes = 0U;
UI_Page_e gAcikSayfa = UI_PAGE_BOOT;
uint8_t gAlarmPlate = 0U;
uint32_t gPlate = 0U;
uint8_t gPlateError = 0U;
uint8_t gDusukOncelikAlarm = 0U;
uint32_t gDusukAlarmBasla = 0U;
uint8_t gBipolarCutSes = 0U;
uint8_t gBipolarCoagSes = 0U;
volatile uint16_t gBeepTick = 0U;
volatile bool gBeepAktif = false;

// -- Ekran ayarları -------------------------------------------
uint32_t gEndoCutKademe = 1U;
uint32_t gEndoCoagKademe = 1U;
uint32_t gEndoCutSure = 1U;
uint32_t gEndoCoagSure = 1U;
uint32_t gSesVeri = 0U;

/* ============================================================
 * Genel API
 * ============================================================*/

/**
  * @brief DAC çıkışını 0-255 aralığında sınırlayıp yazar
  * @param val Ham DAC değeri
  * @retval None
  */
void genx_dacUygula(float val) {
    
    float edge = ceilf(val);
    
    if (false == (edge >= 0.0f)) {
        
        edge = 0.0f; 
    }

    if (edge > 255.0f) {
        edge = 255.0f;
    }

    gDacVeri = (uint8_t)edge;
    (void)HAL_DAC_SetValue(&hdac, DAC_CHANNEL_1, DAC_ALIGN_8B_R, gDacVeri);
}

/**
  * @brief Bir kanalı kanal kaydına ekler
  * @param pCh Core_Channel_t tipinde kanalın ortak başlığı
  * @retval None
  */
void genx_kanalUygula(Core_Channel_t *pCh) {

    if (NULL == pCh) {
        return;
    }

    if (gChannelCount < CORE_MAX_CH) {
        gpChannels[gChannelCount] = pCh;
        gChannelCount++;
    }
}

/**
  * @brief Güç çıkışı sahipliğini almaya çalışır. Aynı sahip tekrar çağırırsa başarılıdır.
  * @param pOwner Sahipliği isteyen kanalın ortak yapısı
  * @retval 0 sahiplik alındı, 1 başka bir kanal sahip
  */
Genx_Status_t genx_sahiplendir(const Core_Channel_t *pSahip) {

    Genx_Status_t status = GENX_ERROR;

    if ((NULL == gMutex) || (pSahip == gMutex)) {
        gMutex = pSahip;
        status = GENX_SUCCESS;
    }

    return status;
}

/**
  * @brief CD4051 kanalını seçer
  * @param ch 0-7 (bit2=PA12, bit1=PA11, bit0=PA8), 0 hepsini kapatır
  * @retval None
  */
void genx_muxSecimi(uint8_t ch) {

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

        // Sıfırlanmaması gereken tüm pinler 0 olsun
        GPIOA->BSRR = (allBits & ~setBits) << 16U;

        // Setlenmesi gereken pinleri 1'e ayarlansın
        GPIOA->BSRR = setBits;
    }
}


#if (1 == MONO1_CUT_EN_CUT) || (1 == MONO2_CUT_EN_CUT) || (1 == MONO1_COAG_EN_CONTACT) || (1 == MONO2_COAG_EN_CONTACT)
/**
  * @brief Monopolar çıkış kanalını açar
  * @param relayPin Kanal rölesi
  * @param muxCh MUX_MONO_CUT yada MUX_MONO_COAG
  * @retval None
  */
void genx_monoAc(uint16_t relayPin, uint8_t muxCh) {

    (void)HAL_TIM_PWM_Start(&htim13, TIM_CHANNEL_1); // PIN_A6 // 380KHz
    HAL_GPIO_WritePin(BIPOLAR_ON_R_GPIO_Port, BIPOLAR_ON_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(MONO_KORUMA_GPIO_Port, MONO_KORUMA_Pin, GPIO_PIN_SET);
    
    if (MONO1_RELAY_Pin == relayPin) {
        HAL_GPIO_WritePin(MONO1_RELAY_GPIO_Port, relayPin | PWR_ON_R_Pin, GPIO_PIN_SET);
    }

    else if (MONO2_RELAY_Pin == relayPin) {
        HAL_GPIO_WritePin(MONO2_RELAY_GPIO_Port, relayPin | PWR_ON_R_Pin, GPIO_PIN_SET);
    }

    genx_muxSecimi(muxCh);

    HAL_GPIO_WritePin(FAN_R_GPIO_Port, FAN_R_Pin, GPIO_PIN_SET);
}
#endif /* (1 == MONO1_CUT_EN_CUT) || (1 == MONO2_CUT_EN_CUT) || (1 == MONO1_COAG_EN_CONTACT) || (1 == MONO2_COAG_EN_CONTACT) */


#if (1 == BIPOLAR1_CUT_EN_CUTTING) || (1 == BIPOLAR2_CUT_EN_CUTTING) || (1 == BIPOLAR1_COAG_EN_STANDARD) || (1 == BIPOLAR2_COAG_EN_STANDARD) \
    || (1 == BIPOLAR1_COAG_EN_LIGATION) || (1 == BIPOLAR2_COAG_EN_LIGATION)
/**
  * @brief Bipolar çıkış kanalını açar
  * @param mode `BIPOLAR_MODE_CUT` veya `BIPOLAR_MODE_COAG`
  * @retval None
  */
void genx_bipolarAc(Bipolar_Mode_e mode) {

    (void)HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(BIPOLAR_CUT_R_GPIO_Port, BIPOLAR_CUT_R_Pin, (BIPOLAR_MODE_CUT == mode) ? GPIO_PIN_SET : GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIPOLAR_ON_R_GPIO_Port, BIPOLAR_ON_R_Pin | PWR_ON_R_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_74LS_R1_GPIO_Port, _74LS_R1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(_74LS_R2_GPIO_Port, _74LS_R2_Pin, GPIO_PIN_SET);
    genx_muxSecimi((BIPOLAR_MODE_CUT == mode) ? MUX_BIPOLAR_CUT : MUX_BIPOLAR_COAG);
    HAL_GPIO_WritePin(FAN_R_GPIO_Port, FAN_R_Pin, GPIO_PIN_SET);
}

/**
  * @brief Bipolar çıkış kanalını kapatır
  * @retval None
  */
void genx_bipolarKapat(void) {

    HAL_GPIO_WritePin(BIPOLAR_CUT_R_GPIO_Port, BIPOLAR_CUT_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(BIPOLAR_ON_R_GPIO_Port, BIPOLAR_ON_R_Pin | PWR_ON_R_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(_74LS_R1_GPIO_Port, _74LS_R1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(_74LS_R2_GPIO_Port, _74LS_R2_Pin, GPIO_PIN_RESET);
    genx_muxSecimi(0U);
    HAL_GPIO_WritePin(FAN_R_GPIO_Port, FAN_R_Pin, GPIO_PIN_RESET);
    (void)HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
}

#endif /* (1 == BIPOLAR1_CUT_EN_CUTTING) || (1 == BIPOLAR2_CUT_EN_CUTTING) || (1 == BIPOLAR1_COAG_EN_STANDARD) || (1 == BIPOLAR2_COAG_EN_STANDARD) \
    || (1 == BIPOLAR1_COAG_EN_LIGATION) || (1 == BIPOLAR2_COAG_EN_LIGATION) */

/**
  * @brief Güç çıkışı sahipliğini bırakır
  * @param pSahip Sahipliği bırakan kanalın ortak yapısı
  * @retval None
  */
void genx_serbestBirak(const Core_Channel_t *pSahip) {

    if (pSahip == gMutex) {
        gMutex = NULL;
    }
}

/**
  * @brief Koşul 20 ms boyunca kesintisiz sağlanırsa başarılı döner
  * @param kosul Sayılacak koşul
  * @param pBasla Koşulun başladığı tick'i tutan değişkene işaretçi (0 = başlamadı)
  * @retval GENX_SUCCESS süre doldu, GENX_ERROR henüz dolmadı
  */
Genx_Status_t genx_sayacBittiMi(bool kosul, uint32_t  *pBasla) {

    Genx_Status_t status = GENX_ERROR;

    if (true == kosul) {

        uint32_t simdi = HAL_GetTick();
        
        if (0U == *pBasla) {
            
            *pBasla = simdi;
        }
        
        if ((simdi - *pBasla) >= 20U) {
            
            *pBasla = 0U;
            status = GENX_SUCCESS;
        }
        
    } else {
        
        *pBasla = 0U;
    }

    return status;
}

/**
  * @brief Sorgulanan kanal dışında Start bayrağı açık başka kanal var mı?
  * @param pSelf Sorgulayan kanalın ortak başlığı, hiçbir kanal sorgulamıyorsa NULL
  * @retval true başka kanal aktif, false hiçbiri aktif değil
  */
bool genx_baskaAktifVarMi(const Core_Channel_t *pSelf) {

    bool aktif = false;

    for (uint8_t i = 0U; i < gChannelCount; i++) {
        
        if ((pSelf != gpChannels[i]) && (0U != gpChannels[i]->start)) {
            aktif = true;
        }
    }

    if ((NULL != gMutex) && (pSelf != gMutex)) {
        aktif = true;
    }

    return aktif;
}

/**
  * @brief Kayıtlı tüm kanalların Start bayrağını kapatır
  * @retval None
  */
void genx_hepsiniDurdur(void) {

    for (uint8_t i = 0U; i < gChannelCount; i++) {
        
        gpChannels[i]->start = 0U;
    }
}

/**
  * @brief Yüksek güç kaynağı gerilimini, çıkış sahipliği alındıktan en az 80 ms sonra okur
  * @retval None
  * @note Yeni değer varsa bu çağrı için gPwrYeni = true olur
  */
void genx_PwrDegeriniOku(void) {

    static uint32_t sonTik = 0U;
    static bool zatenAktif = false;
    bool simdiAktif = (NULL != gMutex);

    gPwrYeni = false;

    if ((true == simdiAktif) && (false == zatenAktif)) {
        sonTik = HAL_GetTick();
    }

    zatenAktif = simdiAktif;

    if ((true == simdiAktif) && ((HAL_GetTick() - sonTik) >= 80U)) {     // 80 ms deneme yanılma ile bulundu
        sonTik = HAL_GetTick();
        gHighVolt = gAdc1DmaBuff[ADC_CH_PWR];
        gPwrYeni = true;
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "n9.val=", (int16_t)gHighVolt);
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "n10.val=", (int16_t)gMonoPwrKoruma);
        (void)genx_sendNum(gNumBuff, sizeof(gNumBuff), "n8.val=", (int16_t)gBipolar1Cut.runtime.koruma_veri);
    }
}

/**
  *  @brief NEXTION metin komutunu kuyruğa alır, kuyruk doluysa GENX_KUYRUK_BEKLE_MS boyunca 
  *         sürücüyü çalıştırıp tekrar dener
  * @param pCmd Gönderilecek olan metin komutu
  * @retval NEXTION_STATUS_OK, kuyruk dolu kalırsa NEXTION_STATUS_QUEUE_FULL, aksi halde kuyruk hata kodu
  */
Genx_Status_t genx_sendCmd(const char *pCmd) {

    Genx_Status_t status;
    uint32_t startTick = HAL_GetTick();

    do {
        status = (Genx_Status_t)nextion_sendCmd(pCmd);

        if ((Genx_Status_t)NEXTION_STATUS_QUEUE_FULL == status) {
            nextion_process();
        }

    } while (((Genx_Status_t)NEXTION_STATUS_QUEUE_FULL == status) && ((HAL_GetTick() - startTick) < GENX_KUYRUK_BEKLE_MS));
 
    return status;
}

/**
  * @brief NEXTION sayı içeren komutu kuyruğa alır, kuyruk doluysa GENX_KUYRUK_BEKLE_MS boyunca 
  *         sürücüyü çalıştırıp tekrar dener
  * @param pBuf Metin önek ve sayıyı içerecek buffer
  * @param bufSize Bufferın tam boyutu
  * @param pPrefix Önek satır metni
  * @param num Komuttaki sayı
  * @retval NEXTION_STATUS_OK, kuyruk dolu kalırsa NEXTION_STATUS_QUEUE_FULL, aksi halde kuyruk hata kodu
  */
Genx_Status_t genx_sendNum(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num) {

    Genx_Status_t status;
    uint32_t startTick = HAL_GetTick();

    do {
        status = (Genx_Status_t)nextion_sendNum(pBuf, bufSize, pPrefix, num);
        
        if ((Genx_Status_t)NEXTION_STATUS_QUEUE_FULL == status) {
            nextion_process();
        }

    } while (((Genx_Status_t)NEXTION_STATUS_QUEUE_FULL == status) && ((HAL_GetTick() - startTick) < GENX_KUYRUK_BEKLE_MS));
 
     return status;
}


/**
  * @brief NEXTION değişken değerini en fazla NEXTION_MAX_DENEME kez sorgular
  * @param pDst Değerin yazılacağı değişkenin göstergeci
  * @param pCmd `get` ile başlayan sorgu komutu
  * @param size Değişkenin boyutu
  * @retval NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT, aksi halde kuyruk hata kodu
  */
Genx_Status_t genx_getVarRetry(void *pDst, const char *pCmd, uint16_t size) {

    Genx_Status_t status = (Genx_Status_t)NEXTION_STATUS_TIMEOUT;
    uint8_t attempt = 0U;

    do {
        status = (Genx_Status_t)nextion_getVarBlocking(pDst, pCmd, size);
        attempt++;
    } while (((Genx_Status_t)NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

    return status;
}

/**
  * @brief NEXTION sayfaya geçiş komutu, başarılıysa gAcikSayfa güncellenir
  * @param page `UI_Page_e` tipindeki sayfa IDsi
  * @retval NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT, aksi halde kuyruk hata kodu
  */
Genx_Status_t genx_setPage(uint16_t page) {

    Genx_Status_t status = genx_sendNum(gNumBuff, sizeof(gNumBuff), "page ", (int16_t)page);

    if ((Genx_Status_t)NEXTION_STATUS_OK == status) {
        gAcikSayfa = (UI_Page_e)page;
    }

    return status;
}

/**
  * @brief Belirtilen süre boyunca bekler, bu sırada Nextion durum makinesini çalıştırır
  * @param ms Bekleme süresi (ms)
  * @retval None
  */
void genx_delay(uint32_t ms) {

    uint32_t startTick = HAL_GetTick();

    while ((HAL_GetTick() - startTick) < ms) {
        nextion_process();
    }
}
