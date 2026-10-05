/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    nextion.c
 * @brief   Doğrudan register tabanlı, kesme tabanlı Nextion HMI ekran sürücüsü.
 *
 * @author  destrocore
 * @date    2026
 */

#include "nextion.h"

/* ============================================================
 * Ring buffer için gerekn özel tipler
 * ============================================================*/
typedef struct {
    uint8_t data[NEXTION_RING_BUFF_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} Nextion_RingBuff_t;

typedef struct {
    char slots[NEXTION_CMD_QUEUE_DEPTH][NEXTION_CMD_MAX_LEN + 1U];
    volatile uint32_t head;
    volatile uint32_t tail;
} Nextion_CmdQueue_t;

typedef struct {
    Nextion_Frame_t slots[NEXTION_FRAME_QUEUE_DEPTH];
    volatile uint32_t head;
    volatile uint32_t tail;
} Nextion_FrameQueue_t;

/* ============================================================
 * Özel: modül sabitleri
 * ============================================================*/
/**
 * @brief Modül durum yapıları
 */
static Nextion_Config_t gConfig;
static Nextion_RingBuff_t gTxBuff;
static Nextion_RingBuff_t gRxBuff;
static Nextion_State_e gState = NEXTION_STATE_IDLE;
static Nextion_CmdQueue_t gCmdQueue;
static Nextion_FrameQueue_t gFrameQueue;
static Nextion_Frame_t gRespFrame;
static volatile bool gRespFramePending = false;


/**
 * @brief Modül durum bayrakları
 */
static volatile bool gTxBusy = false;
static volatile bool gAckReceived = false;
static uint8_t gLastRespCode = NEXTION_RESP_SUCCESS;
static Nextion_Status_e gLastCommandStatus = NEXTION_STATUS_OK;
static uint32_t gAckStartTick = 0U;

static uint8_t gRxFrame[NEXTION_RX_FRAME_MAX_LEN];
static uint32_t gRxFrameLen = 0U;
static uint32_t gRxTermCount = 0U;
static bool gRxOverflow = false;

/* ============================================================
 * Özel fonksiyon prototipleri
 * ============================================================*/
static void nextion_hwInit(void);
static bool cmdQueue_push(const char *pCmd);
static const char *cmdQueue_peek(void);
static void cmdQueue_pop(void);
static bool cmdQueue_isEmpty(void);
static bool cmdQueue_isFull(void);
static void rxBuff_push(uint8_t byte);
static uint8_t rxBuff_pop(void);
static bool rxBuff_isEmpty(void);
static bool rxBuff_isFull(void);
static bool txBuff_push(uint8_t byte);
static bool txBuff_isEmpty(void);
static bool txBuff_isFull(void);
static uint8_t txBuff_pop(void);
static uint32_t txBuff_freeSpace(void);
static bool nextion_isAckCode(uint8_t code);
static Nextion_FrameType_e nextion_determineFrame(uint8_t code, uint32_t frameLen);
static void nextion_resetRxParser(void);
static void nextion_frameQueuePush(const Nextion_Frame_t *pFrame);
static bool nextion_frameQueuePop(Nextion_Frame_t *pFrame);
static bool nextion_frameQueueIsEmpty(void);
static void nextion_storeFrameFromRxBuffer(void);
static void nextion_rxParserPush(uint8_t byte);
static void nextion_parseRx(void);
static bool nextion_startTx(void);
static bool nextion_takeRespFrame(Nextion_Frame_t *pFrame);
static Nextion_Status_e nextion_waitBusy(uint32_t timeoutMs);


/* ============================================================
 * Genel API
 * ============================================================*/

/**
 * @brief GPIO, USART çevre birimini, NVIC başlatır ve bkcmd=3 gönderip cevabı bekler
 * @param pCfg Tamamen doldurulmuş bir Nextion_Config_t yapısına işaretçi
 * @retval Başarılı olursa `NEXTION_STATUS_OK` veya negatif bir hata kodu
 */
Nextion_Status_e nextion_init(const Nextion_Config_t *pCfg) {
    
    if (NULL == pCfg) {
        return NEXTION_STATUS_INVALID_PARAM;
    }

    gConfig = *pCfg;

    (void)memset(&gTxBuff, 0x00, sizeof(gTxBuff));
    (void)memset(&gRxBuff, 0x00, sizeof(gRxBuff));
    (void)memset(&gCmdQueue, 0x00, sizeof(gCmdQueue));
    (void)memset(&gFrameQueue, 0x00, sizeof(gFrameQueue));
    (void)memset(gRxFrame, 0x00, sizeof(gRxFrame));

    gState = NEXTION_STATE_IDLE;
    gTxBusy = false;
    gLastRespCode = 0U;
    gAckStartTick = 0U;
    nextion_resetRxParser();

    nextion_hwInit();

    Nextion_Status_e status = NEXTION_STATUS_ERROR;
    do { 
        status = nextion_sendCmdBlocking("bkcmd=3", NEXTION_TIMEOUT_MS);
    } while(NEXTION_STATUS_OK != status); 

    return status;
}

/**
 * @brief İletim için bir Nextion komut bufferını sıraya ekler
 * @param pCmd Frame sonlandırıcıları olmayan ve NULL karakter ile sonlandırılmış komut bufferına işaretçi
 * @retval Başarıyla sıraya eklendiyse `NEXTION_STATUS_OK`. Komut tail'inde boş yer yoksa 
 *      `NEXTION_STATUS_QUEUE_FULL`. pCmd NULL gelirse `NEXTION_STATUS_INVALID_PARAM`
 */
Nextion_Status_e nextion_sendCmd(const char *pCmd) {
    
    if (NULL == pCmd) {
        return NEXTION_STATUS_INVALID_PARAM;
    }

    if (true != cmdQueue_push(pCmd)) {
        return NEXTION_STATUS_QUEUE_FULL;
    }

    return NEXTION_STATUS_OK;
}

/**
 * @brief pCmd'yi kuyruğa alır ve `nextion_process()` işlevini yoklayarak, 
 *      tamamlanana veya zaman aşımı süresi dolana kadar bloklar. Komutun 
 *      ürettiği cevap frame'ini (varsa) tüketir.
 * @param  pCmd NULL ile sonlandırılmış komut bufferı (0xFF sonlandırıcı bayt içermez)
 * @param timeoutMs Bekleme süresi
 * @retval ACK başarısında NEXTION_STATUS_OK, zaman aşımında `NEXTION_STATUS_TIMEOUT`
 *         veya aksi takdirde sıraya alma hata kodu.
 */
Nextion_Status_e nextion_sendCmdBlocking(const char *pCmd, uint32_t timeoutMs) {

    Nextion_Status_e status;
    Nextion_Frame_t ownFrame;

    status = nextion_sendCmd(pCmd);
    if (NEXTION_STATUS_OK != status) {
        return status;
    }

    status = nextion_waitBusy(timeoutMs);

    (void)nextion_takeRespFrame(&ownFrame);

    return status;
}


/**
 * @brief pPrefix'i pBuf'a kopyalar, num'un ASCII rakamlarını ekler
 *        ve NULL ile sonlandırır
 * @param pBuf Yazılabilir lokal buffer
 * @param bufSize pBuf'ın toplam boyutu (bayt cinsinden)
 * @param pPrefix NULL ile sonlandırılmış komut öneki
 * @param num Eklenecek işaretli sayı
 * @retval Başarı durumunda true, aksi halde (sınır aşımı veya geçersiz argüman) false
 */
static bool nextion_buildNumCmd(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num) {

    uint32_t prefixLen = 0U;
    uint32_t digitLen = 0U;
    uint32_t i;
    char digits[NEXTION_NUM_MAX_DIGITS];
    uint16_t uVal;
    bool isNeg = false;
    char tmp;

    if ((NULL == pBuf) || (NULL == pPrefix) || (0U == bufSize)) {
        return false;
    }

    // pPrefix'i ham okunur şekilde tarıyoruz, pBuf'a henüz yazmıyoruz
    while ((prefixLen < (bufSize - 1U)) && ('\0' != pPrefix[prefixLen])) {
        prefixLen++;
    }

    if ('\0' != pPrefix[prefixLen]) {
        // pPrefix, bufSize sınırları içinde sonlanmadı
        return false;
    }

    if (0 > num) {
        isNeg = true;
        uVal = (uint16_t)(-((int32_t)num));
    } else {
        uVal = (uint16_t)num;
    }

    if (0U == uVal) {
        digits[digitLen] = '0';
        digitLen++;
    } else {
        while ((0U != uVal) && (digitLen < NEXTION_NUM_MAX_DIGITS)) {
            digits[digitLen] = (char)('0' + (uVal % 10U));
            uVal /= 10U;
            digitLen++;
        }
    }

    if ((true == isNeg) && (digitLen < NEXTION_NUM_MAX_DIGITS)) {
        digits[digitLen] = '-';
        digitLen++;
    }

    // digits'i sondan başa okuduk, bundan dolayı ters çevirmeliyiz
    for (i = 0U; i < (digitLen / 2U); i++) {
        tmp = digits[i];
        digits[i] = digits[digitLen - 1U - i];
        digits[digitLen - 1U - i] = tmp;
    }

    if ((prefixLen + digitLen + 1U) > bufSize) {
        // pBuf'a sığmıyor
        return false;
    }

    if ((prefixLen + digitLen) > NEXTION_CMD_MAX_LEN) {
        // cmdQueue_push'ın kabul edeceği maksimum komut uzunluğunu aşıyor
        return false;
    }

    (void)memcpy(pBuf, pPrefix, prefixLen);
    (void)memcpy(&pBuf[prefixLen], digits, digitLen);
    pBuf[prefixLen + digitLen] = '\0';

    return true;
}

/**
 * @brief İletim için bir Nextion sayılı komut bufferını sıraya ekler
 * @param pBuf Yazılabilir lokal buffer (asla string
 *        literal geçirilmemeli)
 * @param bufSize pBuf'ın toplam boyutu (bayt cinsinden)
 * @param pPrefix NULL ile sonlandırılmış komut öneki
 * @param num Eklenecek işaretli sayı
 * @retval Başarıyla sıraya eklendiyse `NEXTION_STATUS_OK`, aksi halde
 *      `NEXTION_STATUS_INVALID_PARAM` veya `NEXTION_STATUS_QUEUE_FULL`
 */
Nextion_Status_e nextion_sendNum(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num) {

    if (false == nextion_buildNumCmd(pBuf, bufSize, pPrefix, num)) {
        return NEXTION_STATUS_INVALID_PARAM;
    }

    return nextion_sendCmd(pBuf);
}

/**
 * @brief pBuf'ı doldurur, sıraya alır ve `nextion_process()` işlevini yoklayarak, 
 *      tamamlanana veya zaman aşımı süresi dolana kadar bloklar.
 * @param pBuf Yazılabilir lokal buffer
 * @param bufSize pBuf'ın toplam boyutu (bayt cinsinden)
 * @param pPrefix NULL ile sonlandırılmış komut öneki
 * @param num Eklenecek işaretli sayı
 * @param timeoutMs Bekleme süresi
 * @retval ACK başarısında `NEXTION_STATUS_OK`, zaman aşımında `NEXTION_STATUS_TIMEOUT`
 *         veya aksi takdirde sıraya alma hata kodu
 */
Nextion_Status_e nextion_sendNumBlocking(char *pBuf, uint32_t bufSize, const char *pPrefix, int16_t num, uint32_t timeoutMs) {

    Nextion_Status_e status;
    Nextion_Frame_t ownFrame;

    status = nextion_sendNum(pBuf, bufSize, pPrefix, num);
    if (NEXTION_STATUS_OK != status) {
        return status;
    }

    status = nextion_waitBusy(timeoutMs);

    (void)nextion_takeRespFrame(&ownFrame);

    return status;
}

/**
 * @brief Sürücünün bekleyen herhangi bir işi olup olmadığını döndürür
 * @retval Sürücü IDLE durumunda değilse veya komut tail'i boş değilse `true` döndürür
 */
bool nextion_isBusy(void) {

    if (!cmdQueue_isEmpty()) {
        return true;
    }

    if ((NEXTION_STATE_IDLE != gState) || (true == gTxBusy)) {
        return true;
    }

    return false;
}

/**
 * @brief Geçerli durum makinesi durumunu döndürür
 * @retval Geçerli `Nextion_State_e` değeri
 */
Nextion_State_e nextion_getState(void) {
    return gState;
}

/**
  * @brief Son tamamlanan komut için Nextion yanıt kodunu döndürür
  * @retval Son yanıt baytı
  */
uint8_t nextion_getLastResponse(void) {
    return gLastRespCode;
}

/**
 * @brief Son tamamlanan komutun sonucunu döndürür
 * @retval Komut durumu
 */
Nextion_Status_e nextion_getLastCommandStatus(void) {
    return gLastCommandStatus;
}

/**
 * @brief Sıradaki alınmış Nextion frame'ini döndürür
 * @param pFrame Çıkış frame yapısı
 * @retval Frame varsa `NEXTION_STATUS_OK`, yoksa `NEXTION_STATUS_EMPTY`
 */
Nextion_Status_e nextion_readFrame(Nextion_Frame_t *pFrame) {
    
    if (NULL == pFrame) {
        return NEXTION_STATUS_INVALID_PARAM;
    }

    if (false == nextion_frameQueuePop(pFrame)) {
        return NEXTION_STATUS_EMPTY;
    }

    return NEXTION_STATUS_OK;
}

/**
  * @brief Bloklama durumunda Nextion `get` komutunu gönderir ve değişkenin değerini döndürür.
  * @param pContent Değişkenin değerinin yazılacağı buffer
  * @param pCmd NULL karakter ile sonlandırılmış `get` komutu
  * @param contentSize İçerik buffer boyutu
  * @retval Başarıyla okunduysa `NEXTION_STATUS_OK`, aksi halde
  *      `NEXTION_STATUS_INVALID_PARAM`, `NEXTION_STATUS_TIMEOUT` veya diğer hata kodları
  */
Nextion_Status_e nextion_getVarBlocking(void *pContent, const char *pCmd, uint16_t contentSize)
{
    Nextion_Status_e status;
    Nextion_Frame_t packet;
    char cmdBuff[NEXTION_CMD_MAX_LEN];
    const char *pFinalCmd;
    size_t cmdLen;

    if ((NULL == pCmd) || (NULL == pContent) || (0U == contentSize)) {
        return NEXTION_STATUS_INVALID_PARAM;
    }

    cmdLen = strlen(pCmd);

    if (0 == strncmp(pCmd, "get ", 4U)) {
        // Komut olduğu gibi kullanılsın
        pFinalCmd = pCmd;
    } else {
        if ((cmdLen + 4U + 1U) > NEXTION_CMD_MAX_LEN) {
            return NEXTION_STATUS_INVALID_PARAM;
        }

        (void)memcpy(&cmdBuff[0], "get ", 4U);
        // '\0' dahil
        (void)memcpy(&cmdBuff[4U], pCmd, cmdLen + 1U);
        pFinalCmd = &cmdBuff[0];
    }

    status = nextion_sendCmd(pFinalCmd);
    if (NEXTION_STATUS_OK != status) {
        return status;
    }

    status = nextion_waitBusy(NEXTION_TIMEOUT_MS);
    if (NEXTION_STATUS_OK != status) {
        return status;
    }

    if (false == nextion_takeRespFrame(&packet)) {
        // ACK alındığı halde depoda cevap yoksa
        return NEXTION_STATUS_EMPTY;
    }

    if (NEXTION_FRAME_NUMERIC == packet.type) {
        
        if (sizeof(uint32_t) > contentSize) {
            return NEXTION_STATUS_INVALID_PARAM;
        }

        *(uint32_t *)pContent = ((uint32_t)packet.data[0U]) |
                                ((uint32_t)packet.data[1U] << 8U) |
                                ((uint32_t)packet.data[2U] << 16U) |
                                ((uint32_t)packet.data[3U] << 24U);
    } else if (NEXTION_FRAME_STRING == packet.type) {
        if ((packet.length + 1U) > contentSize) {
            return NEXTION_STATUS_INVALID_PARAM;
        }

        (void)memcpy(pContent, packet.data, packet.length);
        ((char *)pContent)[packet.length] = '\0';
    } else {
        return NEXTION_STATUS_INVALID_PARAM;
    }

    return NEXTION_STATUS_OK;
}


/**
  * @brief Nextion durum makinesini yönetir
  * @retval None
  * @details `ack_timeout_ms` süresi dolduğunda başarısız olan komut atılır
  */
void nextion_process(void) {
    
    nextion_parseRx();

    switch (gState) {
        case NEXTION_STATE_IDLE: {
            if ((false == cmdQueue_isEmpty()) && (false == gTxBusy)) {
                if (true == nextion_startTx()) {
                    gState = NEXTION_STATE_SENDING;
                }
            }
            break;
        }

        case NEXTION_STATE_SENDING: {
            if (false == gTxBusy) {
                if (true == gAckReceived) {
                    gAckReceived = false;
                    if (NEXTION_STATUS_OK == gLastCommandStatus) {
                        gState = NEXTION_STATE_IDLE;
                    } else {
                        gState = NEXTION_STATE_FAULTED;
                    }
                } else {
                    gAckStartTick = HAL_GetTick();
                    gLastCommandStatus = NEXTION_STATUS_BUSY;
                    gState = NEXTION_STATE_WAIT_ACK;
                }
            }
            break;
        }

        case NEXTION_STATE_WAIT_ACK: {
            if (true == gAckReceived) {
                gAckReceived = false;
                if (NEXTION_STATUS_OK == gLastCommandStatus) {
                    gState = NEXTION_STATE_IDLE;
                } else {
                    gState = NEXTION_STATE_FAULTED;
                }
            } else if ((HAL_GetTick() - gAckStartTick) >= gConfig.ack_timeout_ms) {
                gLastCommandStatus = NEXTION_STATUS_TIMEOUT;
                gState = NEXTION_STATE_TIMEOUT;
            } else {
                // 
            }
            break;
        }

        case NEXTION_STATE_TIMEOUT: {
            gState = NEXTION_STATE_IDLE;
            break;
        }

        case NEXTION_STATE_FAULTED: {
            gState = NEXTION_STATE_IDLE;
            break;
        }

        default: {
            gState = NEXTION_STATE_IDLE;
            break;
        }
    }
}

/**
 * @brief `USARTx_IRQHandler()` içinde çağrılan USART kesme işleyicisi.
 * @retval None
 */
void nextion_irqHandler(void) {
    
    uint32_t sr = gConfig.pUsart->SR;
    uint32_t cr1 = gConfig.pUsart->CR1;

    if (0U != (sr & (USART_SR_ORE | USART_SR_NE | USART_SR_FE))) {
        // Hata bit alanları önce USART_SR register'ından okuma ardından USART_DR register'ından okuma ile temizlenir
        (void)gConfig.pUsart->DR;
    }

    // RX
    // USART üzerinden gelen bir bayt veriyi ring bufferın içine atar
    if (0U != (sr & USART_SR_RXNE)) {
        uint8_t byte = (uint8_t)(gConfig.pUsart->DR & 0xFFU);
        rxBuff_push(byte);
    }

    // TX
    if ((0U != (cr1 & USART_CR1_TXEIE)) && (0U != (sr & USART_SR_TXE))) {
        if (!txBuff_isEmpty()) {
            // Sonraki baytı USART'a iletir ve bu donanımsal olarak TXE'yi temizler
            gConfig.pUsart->DR = (uint32_t)txBuff_pop();
        } else {
            gConfig.pUsart->CR1 &= ~USART_CR1_TXEIE;
            gConfig.pUsart->CR1 |= USART_CR1_TCIE;
        }
    }

    // TC
    if ((0U != (cr1 & USART_CR1_TCIE)) && (0U != (sr & USART_SR_TC))) {
        // Bayt iletimi sonlandırdığında USART TC biti set eder
        gConfig.pUsart->SR &= ~USART_SR_TC;
        gConfig.pUsart->CR1 &= ~USART_CR1_TCIE;
        gTxBusy = false;
    }
}

/* ================================================================
 * Yardımcı fonskiyonlar
 * ================================================================*/

 /**
  * @brief GPIO alternatif işlev pinlerini ve USART çevre birimini yapılandırır.
  * @param pCfg Doldurulmuş Nextion_Config_t yapısına işaretçi
  * @retval None
  * @details TXEIE burada devre dışı bırakılmıştır. Yalnızca gönderilecek baytlar 
  *         olduğunda etkinleştirilir.
  */
static void nextion_hwInit(void) {
    
    RCC->AHB1ENR |= gConfig.gpio_ahb1_bits;
    (void)RCC->AHB1ENR;

    // TX pin ayarları
    {
        gConfig.pGpio_port->MODER &= ~(0x03UL << (gConfig.tx_pin_num * 2U));
        gConfig.pGpio_port->MODER |= 0x02UL << (gConfig.tx_pin_num * 2U);       // alt function
        gConfig.pGpio_port->OSPEEDR &= ~(0x03UL << (gConfig.tx_pin_num * 2U));
        gConfig.pGpio_port->OSPEEDR |= (0x02UL << (gConfig.tx_pin_num * 2U));   // high speed
        gConfig.pGpio_port->PUPDR &= ~(0x03UL << (gConfig.tx_pin_num * 2U));    // no pull
        gConfig.pGpio_port->OTYPER &= ~(1UL <<  gConfig.tx_pin_num);           // push-pull

        uint32_t afrIdx = (gConfig.tx_pin_num >= 8U) ? 1U : 0U;
        uint32_t afrShift = (gConfig.tx_pin_num % 8U) * 4U;
        gConfig.pGpio_port->AFR[afrIdx] &= ~(0xFUL << afrShift);
        gConfig.pGpio_port->AFR[afrIdx] |=  (gConfig.gpio_af  << afrShift);
    }

    // RX pin ayarları
    {
        gConfig.pGpio_port->MODER &= ~(0x03UL << (gConfig.rx_pin_num * 2U));
        gConfig.pGpio_port->MODER |= 0x02UL << (gConfig.rx_pin_num * 2U);       // alt function
        gConfig.pGpio_port->OSPEEDR &= ~(0x03UL << (gConfig.rx_pin_num * 2U));
        gConfig.pGpio_port->OSPEEDR |= (0x02UL << (gConfig.rx_pin_num * 2U));   // high speed
        gConfig.pGpio_port->PUPDR &= ~(0x03UL << (gConfig.rx_pin_num * 2U));    // no pull
        gConfig.pGpio_port->OTYPER &= ~(1UL <<  gConfig.rx_pin_num);           // push-pull

        uint32_t afrIdx = (gConfig.rx_pin_num >= 8U) ? 1U : 0U;
        uint32_t afrShift = (gConfig.rx_pin_num % 8U) * 4U;
        gConfig.pGpio_port->AFR[afrIdx] &= ~(0xFUL << afrShift);
        gConfig.pGpio_port->AFR[afrIdx] |=  (gConfig.gpio_af  << afrShift);
    }

    // USART çevre birimi saati (APB1 veya APB2)
    if (1U == gConfig.is_apb2) {
        RCC->APB2ENR |= gConfig.apbx_bit;
        (void)RCC->APB2ENR;
    } else {
        RCC->APB1ENR |= gConfig.apbx_bit;
        (void)RCC->APB1ENR;
    }

    gConfig.pUsart->CR1 = 0U;
    gConfig.pUsart->CR2 = 0U;
    gConfig.pUsart->CR3 = 0U;

    // BRR: tamsayı bölücü, 16 kat oversampling
    gConfig.pUsart->BRR = gConfig.apb_freq_hz / gConfig.baud_rate;

    // USART TX, RX ve RXNE etkinleştirme
    gConfig.pUsart->CR1 = USART_CR1_UE | USART_CR1_TE \
                        | USART_CR1_RE | USART_CR1_RXNEIE;

    // RE ilk setlendiğinde eski baytları sıfırlama
    (void)gConfig.pUsart->SR;  
    (void)gConfig.pUsart->DR;

    // NVIC'de USART kesmesinin etkinleştirmesi
    NVIC_SetPriority(gConfig.irqn, gConfig.nvic_priority);
    NVIC_EnableIRQ(gConfig.irqn);
}

/**
  * @brief Bir komut stringini bir sonraki boş tail slotuna kopyalar
  * @param pCmd NULL karatkter ile sonlandırılmış komut stringi
  * @retval Başarılı olursa `true`, tail doluysa veya string 
  *         NEXTION_CMD_MAX_LEN'i aşarsa `false`
  */
static bool cmdQueue_push(const char *pCmd) {
    
    uint32_t i;

    if (true == cmdQueue_isFull()) {
        return false;
    }

    i = 0U;
    while ((i < NEXTION_CMD_MAX_LEN) && ('\0' != pCmd[i])) {
        gCmdQueue.slots[gCmdQueue.head][i] = pCmd[i];
        i++;
    }

    if ('\0' != pCmd[i]) {
        return false;
    }

    gCmdQueue.slots[gCmdQueue.head][i] = '\0';
    gCmdQueue.head = (gCmdQueue.head + 1U) % NEXTION_CMD_QUEUE_DEPTH;
    return true;
}

/**
 * @brief Global kuyruğun boş olup olmadığını döndürür
 * @retval Boş ise `true`, aksi halde `false`
 */
static bool cmdQueue_isEmpty(void) {
    return (gCmdQueue.head == gCmdQueue.tail);
}

/**
 * @brief Global kuyruğun dolu olup olmadığını döndürür
 * @retval Dolu ise `true`, aksi halde `false`
 */
static bool cmdQueue_isFull(void) {

    return (((gCmdQueue.head + 1U) % NEXTION_CMD_QUEUE_DEPTH) == gCmdQueue.tail);
}

/**
  * @brief Ön kuyruk girdisine read-only bir işaretçi döndürür, ancak girdinin kendisini kaldırmaz
  * @retval NULL karakterle sonlandırılmış komut stringine işaretçi veya kuyruk boşsa NULL
  */
static const char *cmdQueue_peek(void) {
    
    if (true == cmdQueue_isEmpty()) {
        return NULL;
    }

    return gCmdQueue.slots[gCmdQueue.tail];
}

/**
  * @brief Kuyruğun önünden bir eleman çıkartır
  * @retval None
  */
static void cmdQueue_pop(void) {
    
    if (!cmdQueue_isEmpty()) {
        gCmdQueue.tail = (gCmdQueue.tail + 1U) % NEXTION_CMD_QUEUE_DEPTH;
    }
}

/**
  * @brief Alınan bir baytı RX ring bufferına ekler
  * @param byte Alınan veri baytı
  * @note Buffer dolduğunda baytı atar
  */
static void rxBuff_push(uint8_t byte) {
    
    if (!rxBuff_isFull()) {
        gRxBuff.data[gRxBuff.head] = byte;
        gRxBuff.head = (gRxBuff.head + 1U) % NEXTION_RING_BUFF_SIZE;
    }
}

/**
  * @brief RX ring bufferından bir baytlık veriyi çıkarır
  * @retval Çıkarılan bayt
  * @note Çağrı yapmadan önce bufferın boş olmadığını doğrulamalıdır
  */
static uint8_t rxBuff_pop(void) {
    
    uint8_t byte = gRxBuff.data[gRxBuff.tail];
    gRxBuff.tail  = (gRxBuff.tail + 1U) % NEXTION_RING_BUFF_SIZE;
    return byte;
}

/**
  * @brief RX ring bufferın boş olup olmadığını döndürür
  * @retval Boş ise `true`, dolu ise `false`
  */
static bool rxBuff_isEmpty(void) {
    
    return (gRxBuff.head == gRxBuff.tail);
}

/**
 * @brief RX ring bufferın dolu olup olmadığını döndürür
 * @retval Dolu ise `true`, boş ise `false`
 */
static bool rxBuff_isFull(void) {
    return (((gRxBuff.head + 1U) % NEXTION_RING_BUFF_SIZE) == gRxBuff.tail);
}

/**
  * @brief RX ring bufferındaki tüm baytları atar ve RX frame sayacını sıfırlar
  * @retval None
  * @details Önceki bir işlemden kalan eski RX verilerini atmak için yeni bir TX'i devreye almadan önce çağırılır
  */
static bool txBuff_push(uint8_t byte) {
    
    if (txBuff_isFull()) {
        return false;
    }

    gTxBuff.data[gTxBuff.head] = byte;
    gTxBuff.head = (gTxBuff.head + 1U) % NEXTION_RING_BUFF_SIZE;
    return true;
}

/**
  * @brief TX ring bufferın boş olup olmadığını döndürür
  * @retval Boş ise `true`, dolu ise `false`
  */
static bool txBuff_isEmpty(void) {
    return (gTxBuff.head == gTxBuff.tail);
}

/**
 * @brief TX ring bufferın dolu olup olmadığını döndürür
 * @retval Dolu ise `true`, boş ise `false`
 */
static bool txBuff_isFull(void) {

    return (gTxBuff.head + 1U) % NEXTION_RING_BUFF_SIZE == gTxBuff.tail;
}

/**
 * @brief TX ring bufferından bir bayt çıkarır (yalnızca kesme işlemi yapılırken)
 * @retval TX ring bufferından çıkarılan bayt
 * @details Çağrı yapmadan önce çağıranın bufferın boş olmadığını doğrulaması gerekir
 */
static uint8_t txBuff_pop(void) {

    uint8_t byte = gTxBuff.data[gTxBuff.tail];
    gTxBuff.tail = (gTxBuff.tail + 1U) % NEXTION_RING_BUFF_SIZE;
    return byte;
}

/**
 * @brief TX ring buffer'da kalan boş bayt sayısını döndürür
 * @retval Kullanılabilir boş alan
 */
static uint32_t txBuff_freeSpace(void) {
    
    uint32_t head = gTxBuff.head;
    uint32_t tail = gTxBuff.tail;

    if (head >= tail) {
        return (NEXTION_RING_BUFF_SIZE - (head - tail) - 1U); 
    }

    return ((tail - head) - 1U);
}

/**
  * @brief Alınan verilerin Nextion dönüş formatında olup olmadığını döndürür
  * @retval Tanımlı dönüş kodları arasında yer alıyorsa true aksi halde false
  */
static bool nextion_isAckCode(uint8_t code) {
    
    switch (code) {
        case NEXTION_RESP_INVALID_INSTR:
        case NEXTION_RESP_SUCCESS:
        case NEXTION_RESP_INVALID_CMP_ID:
        case NEXTION_RESP_INVALID_PAGE_ID:
        case NEXTION_RESP_INVALID_PIC_ID:
        case NEXTION_RESP_INVALID_FONT_ID:
        case NEXTION_RESP_INVALID_FILE_OP:
        case NEXTION_RESP_INVALID_CRC:
        case NEXTION_RESP_INVALID_BAUD:
        case NEXTION_RESP_INVALID_WAVEFORM:
        case NEXTION_RESP_INVALID_VAR:
        case NEXTION_RESP_INVALID_VAR_OP:
        case NEXTION_RESP_ASSIGN_FAIL:
        case NEXTION_RESP_EEPROM_FAIL:
        case NEXTION_RESP_PARAM_CNT_INVALID:
        case NEXTION_RESP_PARAM_IO_FAIL:
        case NEXTION_RESP_ESCAPE_INVALID:
        case NEXTION_RESP_VAR_TOO_LONG:
        case NEXTION_RESP_SERIAL_BUF_OVF: {
            return true;
        }

        default: {
            return false;
        }
    }
}

/**
  * @brief Alınan paketin kodu tanımlı bir kod ise ve paketin boyutu API'dekine
  *      denk geliyorsa paketin tipini döndürür.
  * @retval Nextion_FrameType_e paket tipi.
  */
static Nextion_FrameType_e nextion_determineFrame(uint8_t code, uint32_t frameLen) {
    
    if (true == nextion_isAckCode(code)) {
        return NEXTION_FRAME_ACK;
    }

    if ((0x65U == code) && (frameLen >= 4U)) {
        return NEXTION_FRAME_TOUCH;
    }

    if ((0x66U == code) && (frameLen >= 2U)) {
        return NEXTION_FRAME_PAGE;
    }

    if ((0x71U == code) && (frameLen == 5U)) {
        return NEXTION_FRAME_NUMERIC;
    }

    if (0x70U == code) {
        return NEXTION_FRAME_STRING;
    }

    if (frameLen == 1U) {
        return NEXTION_FRAME_EVENT;
    }

    return NEXTION_FRAME_UNKNOWN;
}

/**
 * @brief RX parser durumunu sıfırlar
 * @retval None
 */
static void nextion_resetRxParser(void) {

    gRxFrameLen = 0U;
    gRxTermCount = 0U;
    gRxOverflow = 0U;
}

/**
 * @brief Alınmış bir frame'i frame kuyruğuna yazar
 * @param pFrame Frame içeriği
 * @retval None
 */
static void nextion_frameQueuePush(const Nextion_Frame_t *pFrame) {

    uint32_t nextHead = (gFrameQueue.head + 1U) % NEXTION_FRAME_QUEUE_DEPTH;

    if (nextHead == gFrameQueue.tail) {
        gFrameQueue.tail = (gFrameQueue.tail + 1U) % NEXTION_FRAME_QUEUE_DEPTH;
    }

    gFrameQueue.slots[gFrameQueue.head] = *pFrame;
    gFrameQueue.head = nextHead;
}

/**
 * @brief Frame kuyruğundan bir frame çıkarır
 * @param pFrame Çıkış frame'i
 * @retval Frame varsa `true`
 */
static bool nextion_frameQueuePop(Nextion_Frame_t *pFrame) {

    if (true == nextion_frameQueueIsEmpty()) {
        return false;
    }

    *pFrame = gFrameQueue.slots[gFrameQueue.tail];
    gFrameQueue.tail = (gFrameQueue.tail + 1U) % NEXTION_FRAME_QUEUE_DEPTH;

    return true;
}

/**
 * @brief Frame kuyruğunun boş olup olmadığını döndürür.
 * @retval Boş ise `true`.
 */
static bool nextion_frameQueueIsEmpty(void) {

    return (gFrameQueue.head == gFrameQueue.tail);
}

/**
 * @brief RX buffer'dan tamamlanmış frame'i çıkarır ve sınıflandırır.
 * @retval None
 */
/**
 * @brief RX buffer'dan tamamlanmış frame'i çıkarır ve sınıflandırır.
 * @retval None
 */
static void nextion_storeFrameFromRxBuffer(void) {

    Nextion_Frame_t frame;
    uint32_t payloadLen;
    uint32_t i;

    if (gRxFrameLen < NEXTION_TERM_LEN) {
        return;
    }

    payloadLen = gRxFrameLen - NEXTION_TERM_LEN;
    if (payloadLen < 1U) {
        return;
    }

    (void)memset(&frame, 0x00, sizeof(frame));
    frame.code = gRxFrame[0];
    frame.type = nextion_determineFrame(frame.code, payloadLen);

    if (payloadLen > 1U) {
        frame.length = (uint16_t)(payloadLen - 1U);
        if (frame.length > NEXTION_RX_FRAME_MAX_LEN) {
            frame.length = NEXTION_RX_FRAME_MAX_LEN;
        }

        for (i = 0U; i < frame.length; i++) {
            frame.data[i] = gRxFrame[i + 1U];
        }
    } else {
        frame.length = 0U;
    }

    if ((NEXTION_FRAME_ACK == frame.type) || (NEXTION_FRAME_NUMERIC == frame.type) || (NEXTION_FRAME_STRING == frame.type)) {

        gRespFrame = frame;
        gRespFramePending = true;

        if (NEXTION_FRAME_ACK == frame.type) {
            gLastRespCode = frame.code;
            if (NEXTION_RESP_SUCCESS == frame.code) {
                gLastCommandStatus = NEXTION_STATUS_OK;
            } else {
                gLastCommandStatus = NEXTION_STATUS_ERROR;
            }
        } else {
            gLastCommandStatus = NEXTION_STATUS_OK;
        }

        if (NEXTION_STATE_IDLE != gState) {
            gAckReceived = true;
        }
    } else {
        nextion_frameQueuePush(&frame);
    }
}

/**
 * @brief Tek bir RX baytını parser'a verir
 * @param byte Alınan veri baytı
 * @retval None
 */
static void nextion_rxParserPush(uint8_t byte) {

    if (false == gRxOverflow) {
        if (gRxFrameLen < NEXTION_RX_FRAME_MAX_LEN) {
            gRxFrame[gRxFrameLen] = byte;
            gRxFrameLen++;
        } else {
            gRxOverflow = true;
        }
    }

    if (NEXTION_TERM_BYTE == byte) {
        gRxTermCount++;
    } else {
        gRxTermCount = 0U;
    }

    if (NEXTION_TERM_LEN == gRxTermCount) {
        if (false == gRxOverflow) {
            nextion_storeFrameFromRxBuffer();
        }

        nextion_resetRxParser();
    }
}

/**
 * @brief RX ring buffer'daki tüm baytları parser'a aktarır
 * @retval None
 */
static void nextion_parseRx(void) {
    
    while (false == rxBuff_isEmpty()) {
        uint8_t byte = rxBuff_pop();
        nextion_rxParserPush(byte);
    }
}

/**
 * @brief Ön sıradaki komutu biçimlendirir ve TX ring bufferına yükler
 * @retval Bir komut yüklendiyse ve iletim etkinleştirildiyse `true`
 */
static bool nextion_startTx(void)
{
    const char *pCmd;
    uint32_t i;
    uint32_t cmdLen;
    uint32_t requiredSpace;

    pCmd = cmdQueue_peek();
    if (NULL == pCmd) {
        return false;
    }

    cmdLen = 0U;
    while ((cmdLen < NEXTION_CMD_MAX_LEN) && ('\0' != pCmd[cmdLen])) {
        cmdLen++;
    }

    // ASCII baytlarını TX ring buffer içine yazma
    if ('\0' != pCmd[cmdLen]) {
        return false;
    }

    requiredSpace = cmdLen + NEXTION_TERM_LEN;
    if (txBuff_freeSpace() < requiredSpace) {
        return false;
    }

    for (i = 0U; i < cmdLen; i++) {
        if (false == txBuff_push((uint8_t)pCmd[i])) {
            return false;
        }
    }

    // 0xFF 0xFF 0xFF
    (void)txBuff_push(NEXTION_TERM_BYTE);
    (void)txBuff_push(NEXTION_TERM_BYTE);
    (void)txBuff_push(NEXTION_TERM_BYTE);

    cmdQueue_pop();

    // gTxBusy bayrağı TXEIE biti setlenmeden önce setlenmelidir aksi halde IDLE geçişinin kaçırmasına neden olur
    gTxBusy = true;
    gAckReceived = false;
    gLastCommandStatus = NEXTION_STATUS_BUSY;
    gConfig.pUsart->CR1 |= USART_CR1_TXEIE;

    return true;
}

/**
 * @brief Tek slotlu senkron cevap deposundan frame alır ve slotu boşaltır.
 * @param pFrame Çıkış frame yapısı
 * @retval Bekleyen bir cevap varsa `true`, aksi halde `false`
 */
static bool nextion_takeRespFrame(Nextion_Frame_t *pFrame) {

    if (false == gRespFramePending) {
        return false;
    }

    *pFrame = gRespFrame;
    gRespFramePending = false;

    return true;
}

/**
 * @brief Sürücü IDLE durumuna dönene veya zaman aşımına uğrayana kadar
 * `nextion_process()` çağırarak bekler.
 * @param timeoutMs Bekleme süresi
 * @retval Ack başarısında NEXTION_STATUS_OK, zaman aşımında NEXTION_STATUS_TIMEOUT
 */
static Nextion_Status_e nextion_waitBusy(uint32_t timeoutMs) {

    uint32_t startTick = HAL_GetTick();

    while (true == nextion_isBusy()) {
        nextion_process();

        if ((HAL_GetTick() - startTick) >= timeoutMs) {
            return NEXTION_STATUS_TIMEOUT;
        }
    }

    return gLastCommandStatus;
}
