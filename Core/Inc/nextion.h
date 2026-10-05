/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    nextion.h
 * @brief   Doğrudan register tabanlı, kesme tabanlı Nextion HMI ekran sürücüsü.
 *
 * @author  destrocore
 * @date    2026
 *
 * @details Bu sürücüde birbirinden bağımsız üç katman bulunur: kesmeler, ring bufferlar
 *          ve durum makinesi (`nextion_process()`). Bu katmanlar yalnızca `volatile` bayraklar 
 *          ve doğrudan ring bufferlar üzerinden birbirleriyle etkileşir. 
 *          TX ve RX için ayrı birer ring buffer kullanılır. Registerlara doğrudan erişim yalnızca 
 *          `nextion_irqHandler()` içerisinde gerçekleştirilir.
 *          
 *      Veri Gönderim Sırası:
 *          1. Ekrana gönderilecek komut `nextion_sendCmd()` fonksiyonu aracılığıyla gönderim kuyruğuna eklenir. 
 *          Bu fonksiyon komutu doğrudan ekrana göndermez. Bunun yerine komutu `gCmdQueue.slots[]` kuyruğuna ekler.
 * 
 *          2. Durum makinesi `IDLE` durumundaysa ve komut kuyruğu boş değilse, `nextion_process()` çağrısı 
 *          `nextion_startTx()` fonksiyonunu çalıştırır. Bu fonksiyon komutun sonuna üç adet `0xFF` sonlandırıcı 
 *          bayt ekler ve elde edilen veriyi `gTxBuff.data[]` içerisine yerleştirir. Ardından `CR1` registerındaki 
 *          `USART_CR1_TXEIE` biti etkinleştirilir.
 * 
 *          3. `TXE` kesmesi, `gTxBuff` içerisindeki baytları tek tek `DR` registerına aktarır. `gTxBuff.data[]` 
 *          tamamen boşaldığında `TXEIE` devre dışı bırakılır ve hemen ardından `TCIE` etkinleştirilir.
 * 
 *          4. Komutun son baytı fiziksel olarak hatta gönderildiğinde `TC` kesmesi oluşur. Bu noktada kesme işleyicisi 
 *          `gTxBusy` değişkenini `false` yapar. ACK henüz gelmemişse, durum makinesi bir sonraki `nextion_process()` 
 *          çağrısında `WAIT_ACK` durumuna geçer.
 * 
 * 
 *      Gelen Veri Akışının İşlenmesi:
 *          1. `RXNE` kesmesi meydana geldiğinde, kesme işleyicisi alınan baytı `gRxBuff.data[]` ring bufferına ekler. 
 *          Kesme işleyicisinin alınan veri baytları üzerinde yaptığı tek işlem budur.
 * 
 *          2. Her `nextion_process()` çağrısında `gRxBuff.data[]` içerisinde bulunan tüm baytlar `nextion_parseRx()` 
 *          aracılığıyla parser'a aktarılır.
 * 
 *          3. Parser önce verinin sonunda `0xFF 0xFF 0xFF` sonlandırıcının bulunup bulunmadığını kontrol eder. 
 *          Sonlandırma bulunduğunda frame, `nextion_determineFrame()` ile sınıflandırılır. İlk bayta ve frame uzunluğuna 
 *          bağlı olarak mesajın tipi belirlenir: `ACK`, `TOUCH`, `PAGE`, `NUMERIC`, `STRING`, `EVENT` veya `UNKNOWN`.
 * 
 *          4. `ACK`, `NUMERIC` ve `STRING` tipindeki frame'ler komutlara verilen yanıtlar olarak kabul edilir. 
 *          Bu sürücünün mimarisine göre bu frame'ler yalnızca MCU tarafından gönderilen bir sorgu/komut sonucunda 
 *          geldiğinden, tek slotlu `gRespFrame.data[]` alanına kaydedilir ve durum makinesinin `WAIT_ACK` durumundan 
 *          çıkmasını sağlamak üzere `gAckReceived` bayrağı set edilir.
 * 
 *          Diğer frame tipleri (`TOUCH`, `PAGE`, `EVENT` ve `UNKNOWN`) ekran tarafından asenkron olarak 
 *          üretildiğinden `gFrameQueue.slots[]` kuyruğuna eklenir. Kullanıcı bu frame'leri ayrıca `nextion_readFrame()` 
 *          fonksiyonu üzerinden işlemek zorundadır.
 * 
 *      Durum Makinesi:
 * 
 *          Durum makinesinin çalışma sırası her zaman lineerdir:
 * 
 *              IDLE -> SENDING -> WAIT_ACK -> IDLE/FAULTED
 * 
 *          \note `WAIT_ACK` durumunda `ack_timeout_ms` süresi dolarsa durum `TIMEOUT` durumuna geçer. Ardından bir sonraki 
 *          `nextion_process()` çağrısında tekrar `IDLE` durumuna döner.
 * 
 *          Bloklayıcı Fonksiyonlar:
 * 
 *          `nextion_sendNumBlocking()` ve `nextion_getVarBlocking()` fonksiyonları, `nextion_waitBusy()` üzerinden 
 *          bir döngü içerisinde `nextion_process()` fonksiyonunu sürekli çağırarak çalışır. Durum makinesi tekrar 
 *          `IDLE` durumuna dönene veya belirtilen bekleme süresi dolana kadar çağıran kod bloklanır.
 * 
 */

#ifndef _NEXTION_H
#define _NEXTION_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f407xx.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>
#include <string.h>
#include <stdint.h>

#define NEXTION_TOUCH_KEY(page, componentId) ((uint16_t)(((uint16_t)(page) << 8U) | (uint16_t)(componentId)))

/* ============================================================
 * Ring buffer ayarları
 * ============================================================*/
#define NEXTION_RING_BUFF_SIZE      256U
#define NEXTION_CMD_QUEUE_DEPTH     8U
#define NEXTION_CMD_MAX_LEN         64U
#define NEXTION_FRAME_QUEUE_DEPTH   8U
#define NEXTION_RX_FRAME_MAX_LEN    256U
#define NEXTION_TERM_BYTE           0xFFU
#define NEXTION_TERM_LEN            3U
#define NEXTION_TIMEOUT_MS          1500U

/* ============================================================
 * Sayısal komut buffer sınırları
 * ============================================================*/
#define NEXTION_NUM_MAX_DIGITS      6U

/* ============================================================
 * Durum kodları
 * ============================================================*/
typedef enum {
    NEXTION_STATUS_OK               = 0,
    NEXTION_STATUS_BUSY             = -1,
    NEXTION_STATUS_INVALID_PARAM    = -2,
    NEXTION_STATUS_QUEUE_FULL       = -3,
    NEXTION_STATUS_TIMEOUT          = -4,
    NEXTION_STATUS_ERROR            = -5,
    NEXTION_STATUS_EMPTY            = -6
} Nextion_Status_e;

/* ============================================================
 * Nextion yanıt kodları (bkcmd=3 modu)
 * ============================================================*/
typedef enum {
    NEXTION_RESP_INVALID_INSTR      = 0x00U,    // Invalid instruction
    NEXTION_RESP_SUCCESS            = 0x01U,    // Instruction Successful
    NEXTION_RESP_INVALID_CMP_ID     = 0x02U,    // Invalid Component ID
    NEXTION_RESP_INVALID_PAGE_ID    = 0x03U,    // Invalid Page ID
    NEXTION_RESP_INVALID_PIC_ID     = 0x04U,    // Invalid Picture ID
    NEXTION_RESP_INVALID_FONT_ID    = 0x05U,    // Invalid Font ID
    NEXTION_RESP_INVALID_FILE_OP    = 0x06U,    // Invalid File Operation
    NEXTION_RESP_INVALID_CRC        = 0x09U,    // Invalid CRC
    NEXTION_RESP_INVALID_BAUD       = 0x11U,    // Invalid Baud rate Setting
    NEXTION_RESP_INVALID_WAVEFORM   = 0x12U,    // Invalid Waveform ID or Channel #
    NEXTION_RESP_INVALID_VAR        = 0x1AU,    // Invalid Variable name or attribute
    NEXTION_RESP_INVALID_VAR_OP     = 0x1BU,    // Invalid Variable Operation
    NEXTION_RESP_ASSIGN_FAIL        = 0x1CU,    // Assignment failed to assign
    NEXTION_RESP_EEPROM_FAIL        = 0x1DU,    // EEPROM Operation failed
    NEXTION_RESP_PARAM_CNT_INVALID  = 0x1EU,    // Invalid Quantity of Parameters
    NEXTION_RESP_PARAM_IO_FAIL      = 0x1FU,    // IO Operation failed
    NEXTION_RESP_ESCAPE_INVALID     = 0x20U,    // Escape Character Invalid
    NEXTION_RESP_VAR_TOO_LONG       = 0x23U,    // Variable name too long
    NEXTION_RESP_SERIAL_BUF_OVF     = 0x24U     // Serial Buffer Overflow
} Nextion_Resp_e;

/* ==============================================================
 * Sürücü durum makinesi
 * =============================================================*/
typedef enum {
    NEXTION_STATE_IDLE      = 0U,
    NEXTION_STATE_SENDING   = 1U,
    NEXTION_STATE_WAIT_ACK  = 2U,
    NEXTION_STATE_TIMEOUT   = 3U,
    NEXTION_STATE_FAULTED   = 4U
} Nextion_State_e;

/* ==============================================================
 * Frame tipleri
 * =============================================================*/

 typedef enum {
    NEXTION_FRAME_ACK       = 0U,
    NEXTION_FRAME_TOUCH     = 1U,
    NEXTION_FRAME_PAGE      = 3U,
    NEXTION_FRAME_NUMERIC   = 4U,
    NEXTION_FRAME_STRING    = 5U,
    NEXTION_FRAME_EVENT     = 6U,
    NEXTION_FRAME_UNKNOWN   = 7U
} Nextion_FrameType_e;

typedef struct {
    Nextion_FrameType_e type;
    uint8_t code;
    uint16_t length;
    uint8_t data[NEXTION_RX_FRAME_MAX_LEN];
} Nextion_Frame_t;

/**
 * @brief Nextion UART için yapılandırma yapısı
 */
typedef struct {
    USART_TypeDef *pUsart;
    GPIO_TypeDef *pGpio_port;
    uint32_t tx_pin_num;
    uint32_t rx_pin_num;
    uint32_t gpio_af;
    uint32_t gpio_ahb1_bits;
    uint32_t apbx_bit;
    uint8_t is_apb2;
    uint32_t apb_freq_hz;
    uint32_t baud_rate;
    IRQn_Type irqn;
    uint32_t nvic_priority;
    uint32_t ack_timeout_ms;
} Nextion_Config_t;

/* =========================================================================
 * Genel API
 * ========================================================================= */

/**
 * @brief GPIO, USART çevre birimini, NVIC başlatır ve bkcmd=3 gönderip cevabı bekler
 * @param pCfg Tamamen doldurulmuş bir Nextion_Config_t yapısına işaretçi
 * @retval Başarılı olursa `NEXTION_STATUS_OK` veya negatif bir hata kodu
 */
Nextion_Status_e nextion_init(const Nextion_Config_t *pCfg);

/**
 * @brief İletim için bir Nextion komut bufferını sıraya ekler
 * @param pCmd NULL karakter ile sonlandırılmış komut stringi
 * @retval Başarıyla sıraya eklendiyse `NEXTION_STATUS_OK`
 */
Nextion_Status_e nextion_sendCmd(const char *pCmd);

/**
 * @brief pCmd'yi sıraya alır ve `nextion_process()` işlevini yoklayarak, 
 *     tamamlanana veya zaman aşımı süresi dolana kadar bloklar.
 * @param  pCmd NULL ile sonlandırılmış komut bufferı (0xFF sonlandırıcı bayt içermez)
 * @param timeoutMs Bekleme süresi
 * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT` 
 *         veya aksi takdirde kuyruk/sıraya alma hata kodu.
 */
Nextion_Status_e nextion_sendCmdBlocking(const char *pCmd, uint32_t timeoutMs);

/**
 * @brief İletim için bir Nextion numeric komut bufferını sıraya ekler
 * @param pBuf Yazılabilir lokal buffer (asla string
 *        literali geçirilmemeli)
 * @param bufSize pBuf'ın toplam boyutu (bayt cinsinden)
 * @param pPrefix NULL ile sonlandırılmış komut öneki
 * @param num Eklenecek işaretli sayı
 * @retval Başarıyla sıraya eklendiyse NEXTION_STATUS_OK, aksi halde
 *      `NEXTION_STATUS_INVALID_PARAM` veya `NEXTION_STATUS_QUEUE_FULL`
 */
Nextion_Status_e nextion_sendNum(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num);

/**
 * @brief pBuf'ı doldurur, sıraya alır ve `nextion_process()` işlevini yoklayarak,
 *     tamamlanana veya zaman aşımı süresi dolana kadar bloklar.
 * @param pBuf Yazılabilir lokal buffer
 * @param bufSize pBuf'ın toplam boyutu (bayt cinsinden)
 * @param pPrefix NULL ile sonlandırılmış, salt okunur komut öneki
 * @param num Eklenecek işaretli sayı
 * @param timeoutMs Bekleme süresi
 * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT`
 *         veya aksi takdirde kuyruk/sıraya alma hata kodu
 */
Nextion_Status_e nextion_sendNumBlocking(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num, uint32_t timeoutMs);

/**
 * @brief Sürücünün bekleyen herhangi bir işi olup olmadığını döndürür
 * @retval Sürücü `IDLE` durumunda değilse veya komut kuyruğu boş değilse `true`
 */
bool nextion_isBusy(void);

/**
 * @brief Geçerli durum makinesi durumunu döndürür
 * @retval Geçerli `Nextion_State_e` değeri
 */
Nextion_State_e nextion_getState(void);

/**
 * @brief Son ACK veya hata yanıt kodunu döndürür
 * @retval Son Nextion yanıt kodu
 */
uint8_t nextion_getLastResponse(void);

/**
 * @brief Son tamamlanan komutun sonucunu döndürür
 * @retval Komut durumu
 */
Nextion_Status_e nextion_getLastCommandStatus(void);

/**
 * @brief Sıradaki alınmış Nextion frame'ini döndürür
 * @param pFrame Çıkış frame yapısı
 * @retval Frame varsa `NEXTION_STATUS_OK`, yoksa `NEXTION_STATUS_EMPTY`
 */
Nextion_Status_e nextion_readFrame(Nextion_Frame_t *pFrame);

/**
 * @brief Bloklama durumunda Nextion `get` komutunu gönderir ve değişkenin değerini döndürür
 * @param pCmd NULL karakter ile sonlandırılmış `get` komutu
 * @param pContent Değişkenin değerinin yazılacağı buffer
 * @param contentSize İçerik buffer boyutu
 * @retval Başarıyla sıraya eklendiyse `NEXTION_STATUS_OK`, aksi halde
 *      `NEXTION_STATUS_INVALID_PARAM` veya `NEXTION_STATUS_QUEUE_FULL`
 */
Nextion_Status_e nextion_getVarBlocking(void *pContent, const char *pCmd, uint16_t contentSize);

/**
 * @brief Nextion durum makinesini yönetir
 * @retval None
 */
void nextion_process(void);

/**
 * @brief USART kesme işleyicisi
 * @retval None
 */
void nextion_irqHandler(void);

#ifdef __cplusplus
}
#endif

#endif /* _NEXTION_H */