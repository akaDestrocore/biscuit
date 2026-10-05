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

#include <math.h>
#include "core.h"
#include "modules.h"


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

// -- Kanal Start bayrakları -----------------------------------
volatile uint8_t gMono1CutStart = GUCU_KAPALI;
uint8_t gMono1CoagStart = GUCU_KAPALI;
uint8_t gMono2CutStart = GUCU_KAPALI;
uint8_t gMono2CoagStart = GUCU_KAPALI;
uint8_t gBipolar1CutStart = GUCU_KAPALI;
uint8_t gBipolar1CoagStart = GUCU_KAPALI;
uint8_t gBipolar2CutStart = GUCU_KAPALI;
uint8_t gBipolar2CoagStart = GUCU_KAPALI;

// -- Pedal seçimi / bipolar yardımcı bayrakları ---------------
uint8_t gMono1Pedal = 0U;
uint8_t gMono2Pedal = 0U;
uint8_t gBipolar1PedalCift = 0U;
uint8_t gBipolar2PedalCift = 0U;
uint8_t gBipolarHand = 0U;
uint8_t gBipolarAutoStop = 0U;
uint8_t gLigasureStart = GUCU_KAPALI;
uint8_t gLigasurePedal = 0U;
uint32_t gLigasureWattYaz = 0U;
uint8_t gAutoStop2 = 0U;
uint8_t gAutoStopStart = 0U;
uint8_t gAutoStopStart0 = 0U;
uint8_t gAutoStopCiftPedal = 0U;
uint8_t gAutoStopTekPedal = 0U;
uint8_t gBipolar1CoagBurst1 = 0U;
uint8_t gBipolar1CoagBurst2 = 0U;
uint8_t gBipolar2CoagBurst1 = 0U;
uint8_t gBipolar2CoagBurst2 = 0U;

// -- Hata bayrakları ------------------------------------------
uint8_t gPedalError3 = 0U;
uint8_t gPedal2KezError = 0U;
uint8_t gPwrKoruma = 0U;
uint8_t gBiPwrKoruma = 0U;
uint8_t gMonoPwrKoruma = 0U;
uint8_t gBipolarKoruma = 0U;
uint8_t gMono1Koruma = 0U;
uint8_t gMono2Koruma = 0U;

// -- Kullanıcı ayarları ---------------------------------------
uint32_t gMono1CutWatt = 10U;
uint32_t gMono1CutMode = MONO1_CUT_MODE_CUT;
uint32_t gMono1CoagWatt = 10U;
uint32_t gMono1CoagMode = MONO1_COAG_MODE_CONTACT;
uint32_t gEndoCutKademe = 1U;
uint32_t gEndoCoagKademe = 1U;
uint32_t gEndoCutSure = 1U;
uint32_t gEndoCoagSure = 1U;
volatile uint16_t gEndoCoagTimer = 1U;
uint32_t gMono2CutWatt = 10U;
uint32_t gMono2CutMode = MONO2_CUT_MODE_CUT;
uint32_t gMono2CoagWatt = 10U;
uint32_t gMono2CoagMode = MONO2_COAG_MODE_CONTACT;
uint32_t gBipolar1CutWatt = 10U;
uint32_t gBipolar1CutMode = BIPOLAR_CUT_MODE_CUTTING;
uint32_t gBipolar1CoagWatt = 10U;
uint32_t gBipolar1CoagMode = BIPOLAR1_COAG_MODE_STANDARD;
uint32_t gBipolar2CutWatt = 10U;
uint32_t gBipolar2CutMode = SEAL_CUT_MODE_CUTTING;
uint32_t gBipolar2CoagWatt = 10U;
uint32_t gBipolar2CoagMode = SEAL_COAG_MODE_TISSUELOCK;
uint32_t gSesVeri = 0U;

// -- İşlenmiş güç değerleri -----------------------------------
float gCut1WattPol = 10.0f;
float gCoag1WattPol = 10.0f;
float gCut2WattPol = 10.0f;
float gCoag2WattPol = 10.0f;
float gBipolar1CutPol = 10.0f;
float gBipolar1CoagPol = 10.0f;
float gBipolar2CutPol = 10.0f;
float gBipolar2CoagPol = 10.0f;
volatile float gEndoCutWattPol = 50.0f;
volatile float gEndoCoagWattPol = 20.0f;
volatile uint8_t gEndoCutSay = 0U;

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
    
    switch (ch) {

        case 0: {
            // Bütün bitler kapalı
            GPIOA->BSRR = (GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12) << 16U;
            break;
        }
            
        case 1: {
            // Sadece PA8
            GPIOA->BSRR = GPIO_PIN_8 | ((GPIO_PIN_11 | GPIO_PIN_12) << 16U);
            break;
        }
            
        case 2: {
            // Sadece PA11
            GPIOA->BSRR = GPIO_PIN_11 | ((GPIO_PIN_8 | GPIO_PIN_12) << 16U);
            break;
        }
            
        case 3: {
            // PA8 ve PA11 açık, PA12 kapalı
            GPIOA->BSRR = GPIO_PIN_8 | GPIO_PIN_11 | (GPIO_PIN_12 << 16U);
            break;
        }
            
        case 4: {
            // PA12 açık, PA8 ve PA11 kapalı
            GPIOA->BSRR = GPIO_PIN_12 | ((GPIO_PIN_8 | GPIO_PIN_11) << 16U);
            break;
        }
            
        case 5: {
            // PA8 ve PA12 açık, PA11 kapalı
            GPIOA->BSRR = GPIO_PIN_8 | GPIO_PIN_12 | (GPIO_PIN_11 << 16U);
            break;
        }
            
        case 6: {
            // PA11 ve PA12 açık, PA8 kapalı
            GPIOA->BSRR = GPIO_PIN_11 | GPIO_PIN_12 | (GPIO_PIN_8 << 16U);
            break;
        }
            
        case 7:
            // Hepsi açık
            GPIOA->BSRR = GPIO_PIN_8 | GPIO_PIN_11 | GPIO_PIN_12;
            break;
            
        default: {
            break;
        }
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
    HAL_GPIO_WritePin(GPIOD, relayPin | GPIO_PIN_8, GPIO_PIN_SET);       // Kanal ve güç röleleri
    genx_muxSelect(muxCh);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET);
}

/**
  * @brief Bipolar çıkış yolunu açar
  * @param mode `BIPOLAR_MODE_CUT` veya `BIPOLAR_MODE_COAG`
  * @retval None
  */
void genx_bipolarEnable(Bipolar_Mode_e mode) {
    
    HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, (BIPOLAR_MODE_CUT == mode) ? GPIO_PIN_SET : GPIO_PIN_RESET); // Bipolar Cut Relay
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11 | GPIO_PIN_8, GPIO_PIN_SET); // Bipolar Relay, Power Relay On
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // 74LS
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_SET); // 74LS
    genx_muxSelect((BIPOLAR_MODE_CUT == mode) ? MUX_BIPOLAR_CUT : MUX_BIPOLAR_COAG);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_SET); // FAN
}

/**
  * @brief Bipolar çıkış yolunu kapatır
  * @retval None
  */
void genx_bipolarDisable(void) {

    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); // Bipolar Cut Relay
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_11 | GPIO_PIN_8, GPIO_PIN_RESET); // Bipolar Relay, Power Relay Off
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); // 74LS
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_10, GPIO_PIN_RESET); // 74LS
    genx_muxSelect(0U);
    HAL_GPIO_WritePin(GPIOD, GPIO_PIN_9, GPIO_PIN_RESET);
    HAL_TIM_PWM_Stop(&htim4, TIM_CHANNEL_1);
}

/**
  * @brief Sorgulanan kanal dışında aktif başka kanal var mı?
  * @param selfStart Sorgulayan kanalın Start bayrağı yada hiçbir kanal sorgulamıyorsa 0U
  * @retval true başka kanal aktif, false hiçbiri aktif değil
  */
bool genx_isOtherChannelActive(uint8_t selfStart) {
    
    uint8_t all = (uint8_t)(gMono1CutStart + gMono1CoagStart + gMono2CutStart + gMono2CoagStart \
                    + gBipolar1CutStart + gBipolar1CoagStart + gBipolar2CutStart + gBipolar2CoagStart);

    return (all > selfStart);
}

/**
  * @brief Yüksek güç kaynağı voltajını herhangi bir kanal aktifken okur
  * @note Yeni değer varsa bu çağrı için gPwrYeni = true olur
  */
void genx_getPwrVal(void) {

    uint32_t lastTick = 0U;
    bool alreadyWasActive = false;
    bool nowActive = genx_isOtherChannelActive(0U);

    gPwrYeni = false;

    if ((true == nowActive) && (false == alreadyWasActive)) {
        lastTick = HAL_GetTick();
    }
    
    alreadyWasActive = nowActive;

    if ((true == nowActive) && ((HAL_GetTick() - lastTick) >= 80U)) { // 80 ms deneme yanılma ile bulundu
        lastTick = HAL_GetTick();
        gHighVolt = gAdc1DmaBuff[ADC_CH_PWR];
        gPwrYeni = true;
    }
}

/**
  * @brief NEXTION Metin Komut Gönderme Fonksiyonu
  * @param pCmd Gönderilecek olan metin komutu
  * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT` 
  * 		veya aksi takdirde kuyruk/sıraya alma hata kodu.
  * @note Komutu sıraya alır ve `nextion_process()` işlevini yoklayarak, tamamlanana veya 
  * 	zaman aşımı süresi dolana kadar bloklar. Eğer `NEXTION_MAX_DENEME` sayısı kadar gönderme 
  * 	denemesi tamamlanıp ACK gelmezse `NEXTION_STATUS_TIMEOUT` döndürüp çıkar.
  */
Nextion_Status_e NEXTION_sendCmdRetry(const char *pCmd)
{
    Nextion_Status_e status = NEXTION_STATUS_TIMEOUT;
    uint8_t attempt = 0U;

    do {
        status = nextion_sendCmdBlocking(pCmd, NEXTION_TIMEOUT_MS);
        attempt++;
    } while ((NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

    return status;
}

/**
  * @brief NEXTION Sayı içeren Komut Gönderme Fonksiyonu
  * @param pBuf Metin önek ve sayı içerecek olan buffer
  * @param bufSize Gönderilecek olan bufferın tam boyutu
  * @param pPrefix Önek satır metni
  * @param num Komuttaki sayı
  * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT`
  * 		veya aksi takdirde kuyruk/sıraya alma hata kodu.
  * @note Komutu sıraya alır ve `nextion_process()` işlevini yoklayarak, tamamlanana veya 
  * 	zaman aşımı süresi dolana kadar bloklar. Eğer `NEXTION_MAX_DENEME` sayısı kadar gönderme 
  * 	denemesi tamamlanıp ACK gelmezse `NEXTION_STATUS_TIMEOUT` döndürüp çıkar.
  */
Nextion_Status_e NEXTION_sendNumRetry(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num)
{
	Nextion_Status_e status = NEXTION_STATUS_TIMEOUT;
	uint8_t attempt = 0U;

	do {
		status = nextion_sendNumBlocking(pBuf, bufSize, pPrefix, num, NEXTION_TIMEOUT_MS);
		attempt++;
	} while ((NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

	return status;
}

/**
  * @brief NEXTION Değişken Değeri Sorgulama Fonksiyonu
  * @param pDst Sorgulanan değerinin atanacak olan değişken göstergeci
  * @param pCmd `get` ile başlayan değişken sorgulama önek komutu
  * @param size Değeri içerecek olan değikenin boyutu
  * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT`
  * 		veya aksi takdirde kuyruk/sıraya alma hata kodu.
  * @note Komutu sıraya alır ve `nextion_process()` işlevini yoklayarak, tamamlanana veya 
  * 	zaman aşımı süresi dolana kadar bloklar. Eğer `NEXTION_MAX_DENEME` sayısı kadar gönderme 
  * 	denemesi tamamlanıp ACK gelmezse `NEXTION_STATUS_TIMEOUT` döndürüp çıkar.
  */
Nextion_Status_e NEXTION_getVarRetry(void *pDst, const char *pCmd, uint16_t size)
{
	Nextion_Status_e status = NEXTION_STATUS_TIMEOUT;
	uint8_t attempt = 0U;

	do {
		status = nextion_getVarBlocking(pDst, pCmd, size);
		attempt++;
	} while ((NEXTION_STATUS_TIMEOUT == status) && (attempt < NEXTION_MAX_DENEME));

	return status;
}

/**
  * @brief NEXTION sayfaya geçiş komutu
  * @param page `UI_Page_e` tipindeki sayfa IDsi
  * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT`
  * 		veya aksi takdirde kuyruk/sıraya alma hata kodu.
  * @note Bu, `NEXTION_sendNumRetry()` fonksiyonu için bir sarmalayıcı fonksiyondur. Bu fonksiyonun 
  * 	temel amacı, MCU ve Nextion arasında sayfa geçişlerini senkronize etmektir.
  */
Nextion_Status_e NEXTION_setPage(uint16_t page)
{
	Nextion_Status_e status = NEXTION_sendNumRetry(gNumBuff, sizeof(gNumBuff), "page ", (int16_t)page);

    if (NEXTION_STATUS_OK == status) 
	{
        gAcikSayfa = page;
    }

    return status;
}