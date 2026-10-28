/**
 * @file   test_nextion.c
 * @brief  common/src/nextion.c için host tarafında çalışan birim testleri.
 *
 * @details RX baytları SR/DR üzerinden yazılıp `nextion_irqHandler()` çağrılarak 
 *          beslenir; TX baytları aynı şekilde TXE bayrağı setlenip 
 *          `nextion_irqHandler()` çağrılarak DR'den okunarak yakalanır.
 */

#include "unity.h"

#include <string.h>
#include <stdint.h>

#include "nextion.c"
#include "memory_setup.h"
#include "mock_stm32f4xx_hal.h"


/* =========================================================================
 * Özel sabitler
 * ========================================================================= */

static Nextion_Config_t gTestCfg;
static uint32_t gFakeTickMs = 0U;
static uint32_t gFakeTickStepMs = 0U;
static volatile bool gSimDisplayDoStuff = false;
static const uint8_t *gSimRespFrame = NULL;
static uint32_t gSimRespFrameLen = 0U;
static uint8_t gSimTxLog[128];
static uint32_t gSimTxLen = 0U;
static uint32_t gSimAckFailCount = 0U;
static bool gSimSuppressAck = false;
static bool gSimStealRespFrame = false;

#define TEST_ACK_TIMEOUT_MS 250U



/* =========================================================================
 * Özel yardımcı fonksiyonlar
 * ========================================================================= */

/**
 * @brief USART üzerinden tek bir RX baytını sürücüye besler
 * @param byte Alınan veri baytı
 * @retval None
 */
static void simRxByte(uint8_t byte) {
    
    gTestCfg.pUsart->DR = (uint32_t)byte;
    gTestCfg.pUsart->SR |= USART_SR_RXNE;

    nextion_irqHandler();

    gTestCfg.pUsart->SR &= ~USART_SR_RXNE;
}

/**
 * @brief Bir RX frame'ini sürücüye besler
 * @param pPayload Frame'ın verisi
 * @param len Payload uzunluğu
 * @retval None
 */
static void simRxFrame(const uint8_t *pPayload, uint32_t len) {
    
    uint32_t i;
    for (i = 0U; i < len; i++) {
        simRxByte(pPayload[i]);
    }

    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
}

/**
  * @brief Ekran davranışını simüle eder: TX'i boşaltır ve TC'den sonra yanıt verir
  * @retval None
  */
static void simPumpDisplay(void) {

    if (0U != (gTestCfg.pUsart->CR1 & USART_CR1_TXEIE)) {
        
        gTestCfg.pUsart->SR |= USART_SR_TXE;
        nextion_irqHandler();

        if ((0U != (gTestCfg.pUsart->CR1 & USART_CR1_TXEIE)) && (gSimTxLen < sizeof(gSimTxLog))) {
            gSimTxLog[gSimTxLen] = (uint8_t)(gTestCfg.pUsart->DR & 0xFFU);
            gSimTxLen++;
        }
    } else if (0U != (gTestCfg.pUsart->CR1 & USART_CR1_TCIE)) {
        
        gTestCfg.pUsart->SR |= USART_SR_TC;
        nextion_irqHandler();

        if (false == gSimSuppressAck) {
            
            if (NULL != gSimRespFrame) {
                
                simRxFrame(gSimRespFrame, gSimRespFrameLen);
            } else {
                
                uint8_t ack[1] = {(uint8_t)NEXTION_RESP_SUCCESS};

                if (gSimAckFailCount > 0U) {
                    ack[0] = (uint8_t)NEXTION_RESP_INVALID_INSTR;
                    gSimAckFailCount--;
                }

                simRxFrame(ack, 1U);
            }
        }
    } else {
        // Hiçbir şey yapma
    }
}

/**
  * @brief HAL_GetTick() için CMock geri çağırımı
  * @param numCalls CMock'un sağladığı çağrı sayacı (kullanılmıyor)
  * @retval Ayarlanmış sahte zaman değeri
  */
static uint32_t cbHalGetTick(int numCalls) {

    uint32_t now;
    (void)numCalls;

    if (true == gSimDisplayDoStuff) {
        simPumpDisplay();
    }

    // Cevap çerçevesini waitBusy dönmeden önce çal
    if ((true == gSimStealRespFrame) && (true == gRespFramePending)) {
        Nextion_Frame_t tmp;
        (void)nextion_takeRespFrame(&tmp);
    }

    now = gFakeTickMs;
    gFakeTickMs += gFakeTickStepMs;
    return now;
}

/**
  * @brief Ön sıradaki komutun iletimini TXEIE temizlenene kadar sürer ve 
  *         iletilen baytları yakalar.
  * @param pOut Yakalanan baytlar için çıkış buffer'ı
  * @param maxLen pOut buffer'ın kapasitesi
  * @retval Yakalanan bayt sayısı
  */
static uint32_t simDrainTx(uint8_t *pOut, uint32_t maxLen) {
    
    uint32_t n = 0U;

    while (0U != (gTestCfg.pUsart->CR1 & USART_CR1_TXEIE)) {
        
        gTestCfg.pUsart->SR |= USART_SR_TXE;
        nextion_irqHandler(); 

        // TXEIE setliyse bu çağrıda bir bayt iletildi anlamına gelir
        if (0U != (gTestCfg.pUsart->CR1 & USART_CR1_TXEIE)) {
            if (n < maxLen) {
                pOut[n] = (uint8_t)(gTestCfg.pUsart->DR & 0xFFU);
            }
            n++;
        }
    }

    // TX kesmesi
    gTestCfg.pUsart->SR |= USART_SR_TC;
    nextion_irqHandler();
    
    return n;
}

/**
 * @brief Ön sıradaki komutu IDLE -> SENDING durumuna geçirir
 * @param pOut Yakalanan baytlar için çıkış buffer'ı
 * @param maxLen pOut buffer'ın boyutu
 * @retval Yakalanan bayt sayısı
 */
static uint32_t simTransmit(uint8_t *pOut, uint32_t maxLen) {
    
    nextion_process();

    return simDrainTx(pOut, maxLen);
}

/**
 * @brief Bir komutu sıraya alır, iletimi sürer, başarılı ACK simüle eder 
 *      ve IDLE durumuna geçirir.
 * @param pCmd Gönderilecek komut
 * @param pOut Yakalanan TX baytlar için çıkış buffer'ı
 * @param maxLen pOut buffer'ın boyutu
 * @retval Yakalanan TX bayt sayısı
 */
static uint32_t simSendCmd(char *pCmd, uint8_t *pOut, uint32_t maxLen) {

    Nextion_Status_e sendStatus = nextion_sendCmd(pCmd);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);

    uint32_t txLen = simTransmit(pOut, maxLen);

    uint8_t ack[1] = {(uint8_t)NEXTION_RESP_SUCCESS};
    simRxFrame(ack, 1);
    nextion_process();

    return txLen;
}

/**
  * @brief İstediğimiz frame'inin enjekte edilmesinin sağlayan fonksiyon
  * @param pPayload, gönderilen frame'in payloadı
  * @param len payload  uzunluğu (başlıkla birlikte)
  * @param type, gönderilen frame'in tipi
  * @param dataLen ayıklanan payload uzunluğu
  * @retval none
  */
static void feedAndExpectFrame(const uint8_t *pPayload, uint32_t len, Nextion_FrameType_e type, uint16_t dataLen) {
    
    Nextion_Frame_t frame;

    simRxFrame(pPayload, len);
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_readFrame(&frame));
    TEST_ASSERT_EQUAL_INT(type, frame.type);
    TEST_ASSERT_EQUAL_UINT8(pPayload[0], frame.code);
    TEST_ASSERT_EQUAL_UINT16(dataLen, frame.length);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, nextion_readFrame(&frame));
}

/**
  * @brief İstediğimiz frame'inin enjekte edilmesinin sağlayan fonksiyon
  * @param pPrefix Sayısallı komutunun karakterli kısmı
  * @param num Sayısal değer
  * @retval none
  */
static void sendNumAndExpect(const char *pPrefix, int16_t num, const char *pExpected) {
    
    char buf[NEXTION_CMD_MAX_LEN + 1U];
    uint8_t tx[NEXTION_CMD_MAX_LEN + 8U];
    uint8_t ack[1] = {(uint8_t)NEXTION_RESP_SUCCESS};
    uint32_t len = (uint32_t)strlen(pExpected);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendNum(buf, sizeof(buf), pPrefix, num));
    TEST_ASSERT_EQUAL_UINT32(len + NEXTION_TERM_LEN, simTransmit(tx, sizeof(tx)));
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)pExpected, tx, len);

    simRxFrame(ack, 1U);
    nextion_process();
}

/* =========================================================================
 * setUp / tearDown
 * ========================================================================= */

void setUp(void) {
    
    virtual_mem_set_all();

    HAL_GetTick_StubWithCallback(cbHalGetTick);

    (void)memset(&gTestCfg, 0x00, sizeof(gTestCfg));
    gTestCfg.pUsart = USART3;
    gTestCfg.pGpio_port = GPIOB;
    gTestCfg.rx_pin_num = 11U;
    gTestCfg.tx_pin_num = 10U;
    gTestCfg.gpio_af = 7U;
    gTestCfg.gpio_ahb1_bits = RCC_AHB1ENR_GPIOBEN;
    gTestCfg.apbx_bit = RCC_APB1ENR_USART3EN;
    gTestCfg.is_apb2 = 0U;
    gTestCfg.apb_freq_hz = 48000000U;
    gTestCfg.baud_rate = 9600U;
    gTestCfg.irqn = USART3_IRQn;
    gTestCfg.nvic_priority = 8U;
    gTestCfg.ack_timeout_ms = TEST_ACK_TIMEOUT_MS;

    gSimDisplayDoStuff = true;
    Nextion_Status_e initStatus = nextion_init(&gTestCfg);
    gSimDisplayDoStuff = false;

    gSimTxLen = 0U;
    gFakeTickMs = 0U;
    gSimRespFrameLen = 0U;

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, initStatus);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
    TEST_ASSERT_FALSE(nextion_isBusy());
}

void tearDown(void) {

    gSimDisplayDoStuff = false;
    gSimSuppressAck = false;
    gSimAckFailCount = 0U;
    gFakeTickStepMs = 0U;
    gSimRespFrame = NULL;
    gSimStealRespFrame = false;
}


/* =========================================================================
 * nextion_init() testleri
 * ========================================================================= */

/**
 * @brief NULL yapılandırma ile init INVALID_PARAM döndürmelidir
 */
void test_nextion_init_nullConfig_returnsInvalidParam(void) {
    Nextion_Status_e status = nextion_init(NULL);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, status);
}

/**
 * @brief Başarılı olana kadar tekrar çağrılmalı
 */
void test_nextion_init_ackFailsTwice_retriesUntilSuccess(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimTxLen = 0U;
    gSimAckFailCount = 2U;
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_init(&gTestCfg);
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(3U * (7U + NEXTION_TERM_LEN), gSimTxLen);  // "bkcmd=3" 3 tane
}

/**
 * @brief Başarılı olana kadar tekrar çağrılmalı
 */
void test_nextion_init_apb2AndLowPins_configuresRegisters(void) {
// -- Test için hazırlık adımları ------------------------------
    Nextion_Config_t cfg = gTestCfg;

    cfg.pUsart = USART6;
    cfg.pGpio_port = GPIOC;
    cfg.tx_pin_num = 6U;
    cfg.rx_pin_num = 7U;
    cfg.gpio_af = 8U;
    cfg.gpio_ahb1_bits = RCC_AHB1ENR_GPIOCEN;
    cfg.apbx_bit = RCC_APB2ENR_USART6EN;
    cfg.is_apb2 = 1U;
    cfg.apb_freq_hz = 84000000U;
    cfg.baud_rate = 115200U;
    cfg.irqn = USART6_IRQn;
    gTestCfg = cfg;

    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_init(&gTestCfg);
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(84000000U / 115200U, USART6->BRR);
    TEST_ASSERT_BITS_HIGH(USART_CR1_UE | USART_CR1_TE | USART_CR1_RE | USART_CR1_RXNEIE, USART6->CR1);
    TEST_ASSERT_BITS_HIGH(RCC_APB2ENR_USART6EN, RCC->APB2ENR);
    TEST_ASSERT_BITS_HIGH(RCC_AHB1ENR_GPIOCEN, RCC->AHB1ENR);
    TEST_ASSERT_EQUAL_UINT32(2U, (GPIOC->MODER >> 12U) & 0x3U);
    TEST_ASSERT_EQUAL_UINT32(2U, (GPIOC->MODER >> 14U) & 0x3U);
    TEST_ASSERT_EQUAL_UINT32(8U, (GPIOC->AFR[0] >> 24U) & 0xFU);
    TEST_ASSERT_EQUAL_UINT32(8U, (GPIOC->AFR[0] >> 28U) & 0xFU);
}



/* =========================================================================
 * nextion_sendCmd() testleri
 * ========================================================================= */

/**
 * @brief NULL işaretcisi girildiğinde NEXTION_STATUS_INVALID döndürmeli
 */
void test_nextion_sendCmd_nullInput_returnsInvalidParam(void) {

    Nextion_Status_e sendStatus = nextion_sendCmd(NULL);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, sendStatus);
}

/**
  * @brief Gönderilen komutun TX baytları ve sıralaması doğru olmalıdır
  */
void test_nextion_sendCmd_txBytesAndOrder_correct(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t txBuff[32];
// -------------------------------------------------------------

    Nextion_Status_e sendStatus = nextion_sendCmd("j0.val=50");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);

    uint32_t txLen = simTransmit(txBuff, sizeof(txBuff));

    TEST_ASSERT_EQUAL_UINT32(12U, txLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"j0.val=50", txBuff, 9);
    TEST_ASSERT_EQUAL_UINT8(NEXTION_TERM_BYTE, txBuff[9]);
    TEST_ASSERT_EQUAL_UINT8(NEXTION_TERM_BYTE, txBuff[10]);
    TEST_ASSERT_EQUAL_UINT8(NEXTION_TERM_BYTE, txBuff[11]);
}

/**
 * @brief NEXTION_CMD_MAX_LEN uzunluğundan fazla bir komut reddedilmelidir.
 */
void test_nextion_sendCmd_overLengthReject(void) {
// -- Test için hazırlık adımları ------------------------------    
    char oversized[NEXTION_CMD_MAX_LEN + 2U];
    uint32_t i;

    for (i = 0U; i < (NEXTION_CMD_MAX_LEN + 1U); i++) {
        oversized[i] = 'n';
    }
    oversized[NEXTION_CMD_MAX_LEN + 1U] = '\0';
// -------------------------------------------------------------

    Nextion_Status_e sendStatus = nextion_sendCmd(oversized);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, sendStatus);
}

/**
 * @brief Komut kuyruğu dolduğunda ek bir gönderim QUEUE_FULL döndürmelidir
 */
void test_nextion_sendCmd_queueFullReject(void) {
// -- Test için hazırlık adımları ------------------------------ 
    uint32_t slots = NEXTION_CMD_QUEUE_DEPTH - 1U;
    uint32_t i;
    Nextion_Status_e sendStatus;
// -------------------------------------------------------------

    for (i = 0U; i < slots; i++) {
        sendStatus = nextion_sendCmd("t0.txt=\"test\"");
        TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);
    }

    sendStatus = nextion_sendCmd("t0.txt=\"test overflow\"");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, sendStatus);
}

/**
 * @brief Sınır değeri uzunlukta olan komut kabul edilmeli
 */
void test_nextion_sendCmd_exactMaxLength_accepted(void) {
// -- Test için hazırlık adımları ------------------------------ 
    char cmd[NEXTION_CMD_MAX_LEN + 1U];
    uint8_t tx[NEXTION_CMD_MAX_LEN + 8U];
    // 'n' karakteri ile dolduruyoruz
    (void)memset(cmd, 'n', NEXTION_CMD_MAX_LEN);
    cmd[NEXTION_CMD_MAX_LEN] = '\0';
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd(cmd));
    TEST_ASSERT_EQUAL_UINT32(NEXTION_CMD_MAX_LEN + NEXTION_TERM_LEN, simTransmit(tx, sizeof(tx)));
}

/* =========================================================================
 * nextion_sendNum() testleri
 * ========================================================================= */

/**
 * @brief Fonksiyon pozitif, negatif, ve int16_t formatındaki sayıları kabul etmeli ve doğru formatlamalı
 */
void test_nextion_sendNum_formatsPositiveNegativeZeroAndInt16Min(void) {

    sendNumAndExpect("n0.val=", 123, "n0.val=123");
    sendNumAndExpect("x=", -45, "x=-45");
    sendNumAndExpect("x=", 0, "x=0");
    sendNumAndExpect("x=", (int16_t)-32768, "x=-32768");
}

/**
 * @brief Tam doldurulmuş bufferı kabul etmeli
 */
void test_nextion_sendNum_exactFitBuffer_accepted(void) {
// -- Test için hazırlık adımları ------------------------------ 
    char buf[6];
// -------------------------------------------------------------
    
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendNum(buf, sizeof(buf), "x=", 123));
}

/**
 * @brief Bufferın içine sığmayan prefix, bufferın içindekilerin NEXTION_CMD_MAX_LEN maksimum 
 *      kabul edilebilir komut uzunluğundan uzun olması veya prefixin sığıp sayısal değerin 
 *      bufferın içine sığmaması durumunda NEXTION_STATUS_INVALID_PARAM döndürmelidir.
 */
void test_nextion_sendNum_invalidArguments_returnInvalidParam(void) {
// -- Test için hazırlık adımları ------------------------------ 
    char buf[8];
    char longPrefix[NEXTION_CMD_MAX_LEN - 1U];
    char bigBuf[NEXTION_CMD_MAX_LEN * 2U];
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(NULL, 8U, "x=", 1));
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(buf, sizeof(buf), NULL, 1));
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(buf, 0U, "x=", 1));

    // prefix bufSize sınırı içinde sonlanmıyor
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(buf, 4U, "abcdef", 1));

    // prefix sığıyor, basamaklar sığmıyor
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(buf, sizeof(buf), "n0.val=", 1));
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(buf, sizeof(buf), "ab=", 12345));

    // buffer yeterli ama NEXTION_CMD_MAX_LEN aşılıyor
    (void)memset(longPrefix, 'a', NEXTION_CMD_MAX_LEN - 2U);
    longPrefix[NEXTION_CMD_MAX_LEN - 2U] = '\0';
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendNum(bigBuf, sizeof(bigBuf), longPrefix, 12345));
}

/**
 * @brief Komut sırası dolu olduğunda NEXTION_STATUS_QUEUE_FULL döndürmeli
 */
void test_nextion_sendNum_queueFull_returnsQueueFull(void) {
// -- Test için hazırlık adımları ------------------------------
    char buf[16];
// -------------------------------------------------------------

    for (uint32_t i = 0U; i < (NEXTION_CMD_QUEUE_DEPTH - 1U); i++) {
        TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
    }

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, nextion_sendNum(buf, sizeof(buf), "x=", 1));
}


/* =========================================================================
 * nextion_sendCmdBlocking() testleri
 * ========================================================================= */

/**
 * @brief Başarı durumunda NEXTION_STATUS_OK döndürmeli
 */
void test_nextion_sendCmdBlocking_success_returnsOk(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_sendCmdBlocking("j0.val=1", 1000U);
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, status);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
    TEST_ASSERT_EQUAL_UINT32(8U + NEXTION_TERM_LEN, gSimTxLen);
}

/**
 * @brief Komut sırası dolu olduğunda NEXTION_STATUS_QUEUE_FULL döndürmeli
 */
void test_nextion_sendCmdBlocking_nullCmd_returnsInvalidParam(void) {

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_sendCmdBlocking(NULL, 100U));
}

/**
 * @brief İzin verilen uzunluktan uzun olan komutları göndermeye çalışıldığında 
 *      NEXTION_STATUS_QUEUE_FULL döndürmelidir ('\0' karakteri ile bitmediğinden dolayı)
 */
void test_nextion_sendCmdBlocking_overLength_returnsQueueFull(void) {
// -- Test için hazırlık adımları ------------------------------
    char oversized[NEXTION_CMD_MAX_LEN + 2U];
    (void)memset(oversized, 'n', NEXTION_CMD_MAX_LEN + 1U);
    oversized[NEXTION_CMD_MAX_LEN + 1U] = '\0';
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, nextion_sendCmdBlocking(oversized, 100U));
}

/**
 * @brief İzin verilen uzunluktan uzun olan komutları göndermeye çalışıldığında 
 *      NEXTION_STATUS_QUEUE_FULL döndürmelidir ('\0' karakteri ile bitmediğinden dolayı)
 */
void test_nextion_sendCmdBlocking_errorAck_returnsError(void) {
    gSimAckFailCount = 1U;
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_sendCmdBlocking("j0.val=1", 1000U);
    gSimDisplayDoStuff = false;

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_ERROR, status);
}

/**
 * @brief Bloklayıcı fonksiyon zaman aşımı durumunda NEXTION_STATUS_TIMEOUT döndürmeli
 */
void test_nextion_sendCmdBlocking_waitBusyTimeout_returnsTimeout(void) {
// -- Test için hazırlık adımları ------------------------------
    // TX hiç boşalmaz, waitBusy kendi süresini aşar
    gFakeTickStepMs = 1U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_TIMEOUT, nextion_sendCmdBlocking("j0.val=1", 50U));
}


/**
 * @brief Ayarlanan zaman içinde ACK geri gelmez ise, sürücü zaman aşımı bayrağı setleyip 
 *      NEXTION_STATUS_TIMEOUT -> NEXTION_STATE_IDLE durumuna geçmelidir
 */
void test_nextion_sendCmdBlocking_noAck_driverTimesOutAndRecovers(void) {
// -- Test için hazırlık adımları ------------------------------
    gSimSuppressAck = true;
    gSimDisplayDoStuff = true;
    gFakeTickStepMs = 1U;
// -------------------------------------------------------------

    Nextion_Status_e status = nextion_sendCmdBlocking("j0.val=1", 1000U);

    gSimDisplayDoStuff = false;
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_TIMEOUT, status);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
}


/* =========================================================================
 * nextion_getVarBlocking() testleri
 * ========================================================================= */

/**
 * @brief Geçersiz argümanlar girildiğinde NEXTION_STATUS_INVALID_PARAM döndürmeli
 */
void test_nextion_getVarBlocking_invalidArguments_returnInvalidParam(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t v = 0U;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_getVarBlocking(&v, NULL, sizeof(v)));
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_getVarBlocking(NULL, "get j0.val", 4U));
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_getVarBlocking(&v, "get j0.val", 0U));
}

/**
 * @brief "get " öneki prefixin önünde yoksa, sadece istenecek olan değişkenin 
 *      ismi varsa, fonksiyon "get " önekini kendisi eklemeli
 */
void test_nextion_getVarBlocking_withoutGetPrefix_prependsGet(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t numeric[5] = {0x71U, 0x01U, 0x00U, 0x00U, 0x00U};
    uint32_t v = 0U;

    gSimRespFrame = numeric;
    gSimRespFrameLen = sizeof(numeric);
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_getVarBlocking(&v, "n0.val", sizeof(v));
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, status);
    TEST_ASSERT_EQUAL_UINT32(1U, v);
    TEST_ASSERT_EQUAL_UINT32(10U + NEXTION_TERM_LEN, gSimTxLen);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"get n0.val", gSimTxLog, 10U);
}

/**
 * @brief Değişken ismi prefixin içine sığmıyorsa NEXTION_STATUS_INVALID_PARAM döndürmeli
 */
void test_nextion_getVarBlocking_cmdTooLongForPrefix_returnsInvalidParam(void) {
// -- Test için hazırlık adımları ------------------------------
    char cmd[61];
    uint32_t v;

    (void)memset(cmd, 'a', 60U);    // get öneki ile birlikte sığmaz
    cmd[60] = '\0';
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, nextion_getVarBlocking(&v, cmd, sizeof(v)));
}

/**
 * @brief Komut sırası dolu olduğunda bu fonksiyon da NEXTION_STATUS_QUEUE_FULL döndürmeli
 */
void test_nextion_getVarBlocking_queueFull_returnsQueueFull(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t v;
// -------------------------------------------------------------

    for (uint32_t i = 0U; i < (NEXTION_CMD_QUEUE_DEPTH - 1U); i++) {

        TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
    }

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_QUEUE_FULL, nextion_getVarBlocking(&v, "get j0.val", sizeof(v)));
}

/**
 * @brief NEXTION_TIMEOUT_MS süresi boyunca display cevap döndürmez ise 
 *      NEXTION_STATUS_TIMEOUT döndürülmeli
 */
void test_nextion_getVarBlocking_noResponse_returnsTimeout(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t v;
// -------------------------------------------------------------

    gFakeTickStepMs = 1U;   // ekran simülasyonu kapalı
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_TIMEOUT, nextion_getVarBlocking(&v, "get j0.val", sizeof(v)));
}

/**
 * @brief NUMERIC olan değişkenin değerinin okunması için kullanılan buffer boyutu kısa ise 
 *      NEXTION_STATUS_INVALID_PARAM döndürülmeli
 */
void test_nextion_getVarBlocking_numericBufferTooSmall_returnsInvalidParam(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t numeric[5] = {0x71U, 0x01U, 0x02U, 0x03U, 0x04U};
    uint16_t small = 0U;

    gSimRespFrame = numeric;
    gSimRespFrameLen = sizeof(numeric);
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_getVarBlocking(&small, "get j0.val", sizeof(small));
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, status);
}

/**
 * @brief Karakter dizini olan değişkenin değerinin okunması için kullanılan buffer boyutu kısa ise 
 *      NEXTION_STATUS_INVALID_PARAM döndürülmeli
 */
void test_nextion_getVarBlocking_stringBufferTooSmall_returnsInvalidParam(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t str[5] = {0x70U, 't', 'e', 's', 't'};
    char text[4];       // "test" + '\0' = 5 > 4

    gSimRespFrame = str;
    gSimRespFrameLen = sizeof(str);
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_getVarBlocking(text, "get t0.txt", sizeof(text));
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, status);
}

/**
 * @brief Değişken değeri yerine ACK tipi bir cevap gelirse NEXTION_STATUS_INVALID_PARAM döndürülmeli
 */
void test_nextion_getVarBlocking_ackInsteadOfValue_returnsInvalidParam(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t v = 0U;

    // gSimRespFrame == NULL ise düz ACK döner
    gSimDisplayDoStuff = true;
    Nextion_Status_e status = nextion_getVarBlocking(&v, "get j0.val", sizeof(v));
    gSimDisplayDoStuff = false;
// -------------------------------------------------------------

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, status);
}


/* =========================================================================
 * nextion_process() testleri
 * ========================================================================= */
 
/**
 * @brief nextion_isBusy() durum geçişleri boyunca doğru değeri döndürmelidir
 */
void test_nextion_isBusy_betweenStates(void) {
// -- Test için hazırlık adımları ------------------------------     
    uint8_t txBuff[32];
// -------------------------------------------------------------

    // IDLE
    TEST_ASSERT_FALSE(nextion_isBusy());

    Nextion_Status_e sendStatus = nextion_sendCmd("j0.val=1");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);
    TEST_ASSERT_TRUE(nextion_isBusy());

    (void)simTransmit(txBuff, sizeof(txBuff));
    (void)nextion_process();
    // TX bitti ama ACK henüz gelmedi -> WAIT_ACK hala meşgul
    TEST_ASSERT_TRUE(nextion_isBusy());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());

    uint8_t ack[1] = {(uint8_t)NEXTION_RESP_SUCCESS};
    simRxFrame(ack, 1);
    (void)nextion_process();

    TEST_ASSERT_FALSE(nextion_isBusy());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
}

/**
 * @brief Birden fazla sıraya alınmış komut FIFO sırasıyla iletilmelidir
 */
void test_nextion_multipleCmds_fifoOrder(void) {
// -- Test için hazırlık adımları ------------------------------        
    uint8_t txBuffA[32];
    uint8_t txBuffB[32];
// -------------------------------------------------------------

    Nextion_Status_e sendStatusA = nextion_sendCmd("t0.txt=\"A\"");
    Nextion_Status_e sendStatusB = nextion_sendCmd("t0.txt=\"B\"");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatusA);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatusB);

    uint32_t lenA = simTransmit(txBuffA, sizeof(txBuffA));
    TEST_ASSERT_EQUAL_UINT32(13U, lenA);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"t0.txt=\"A\"", txBuffA, 10U);

    uint8_t ack[1] = {(uint8_t)NEXTION_RESP_SUCCESS};
    simRxFrame(ack, 1);
    nextion_process();

    uint32_t lenB = simTransmit(txBuffB, sizeof(txBuffB));
    TEST_ASSERT_EQUAL_UINT32(13U, lenB);
    TEST_ASSERT_EQUAL_UINT8_ARRAY((const uint8_t *)"t0.txt=\"B\"", txBuffB, 10U);

    simRxFrame(ack, 1);
    nextion_process();

    TEST_ASSERT_FALSE(nextion_isBusy());
}

/**
 * @brief Hata koduyla gelen bir ACK, komut durumunu FAULTED olarak
 *      işaretlemeli ve IDLE'a geri dönmelidir
 */
void test_nextion_errorAck_setsErrorStatus(void) {
// -- Test için hazırlık adımları ------------------------------ 
    uint8_t txBuff[32];
// -------------------------------------------------------------

    Nextion_Status_e sendStatus = nextion_sendCmd("j0.val=99");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);
    (void)simTransmit(txBuff, sizeof(txBuff));

    uint8_t badAck[1] = {(uint8_t)NEXTION_RESP_INVALID_CMP_ID};
    simRxFrame(badAck, 1U);
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_ERROR, nextion_getLastCommandStatus());
    TEST_ASSERT_EQUAL_UINT8(NEXTION_RESP_INVALID_CMP_ID, nextion_getLastResponse());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_FAULTED, nextion_getState());

    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
}

/**
 * @brief NEXTION_STATUS_OK  beklerken başka bir şey gelirse NEXTION_STATE_FAULTED durumuna geçilmeli
 */
void test_nextion_errorAckWhileWaiting_setsFaulted(void) {
// -- Test için hazırlık adımları ------------------------------ 
    uint8_t txBuff[32];
// -------------------------------------------------------------

    Nextion_Status_e sendStatus = nextion_sendCmd("j0.val=99");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);

    (void)simTransmit(txBuff, sizeof(txBuff));

    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());

    uint8_t badAck[1] = {(uint8_t)NEXTION_RESP_INVALID_CMP_ID};
    simRxFrame(badAck, 1U);

    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_FAULTED, nextion_getState());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_ERROR, nextion_getLastCommandStatus());
    TEST_ASSERT_EQUAL_UINT8(NEXTION_RESP_INVALID_CMP_ID, nextion_getLastResponse());
}

/**
 * @brief ACK zaman aşımına uğradığında sürücü TIMEOUT durumuna geçmeli ve 
 *      ardından IDLE'a dönmelidir
 */
void test_nextion_ackTimeout_setsTimeoutStatus(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t txBuff[32];
// -------------------------------------------------------------

    Nextion_Status_e sendStatus = nextion_sendCmd("j0.val=1");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);
    (void)simTransmit(txBuff, sizeof(txBuff));
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());

    gFakeTickMs = TEST_ACK_TIMEOUT_MS + 1U;
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_TIMEOUT, nextion_getLastCommandStatus());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_TIMEOUT, nextion_getState());

    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE,nextion_getState());
    TEST_ASSERT_FALSE(nextion_isBusy());
}

/**
 * @brief TX aktif olduğu sürece durum NEXTION_STATE_SENDING olarak kalmalı
 */
void test_nextion_process_sending_staysWhileTxBusy(void) {
// -- Test için hazırlık adımları ------------------------------
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_SENDING, nextion_getState());
// -------------------------------------------------------------

    nextion_process();      // TX henüz boşalmadı
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_SENDING, nextion_getState());
    TEST_ASSERT_TRUE(nextion_isBusy());
}

/**
 * @brief Zaman aşımına uğrayana kadar durum NEXTION_STATE_WAIT_ACK olarak kalmalı
 */
void test_nextion_process_waitAck_staysUntilTimeoutBoundary(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t tx[32];

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
    (void)simTransmit(tx, sizeof(tx));
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());
// -------------------------------------------------------------

    gFakeTickMs = TEST_ACK_TIMEOUT_MS - 1U;
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_BUSY, nextion_getLastCommandStatus());

    gFakeTickMs = TEST_ACK_TIMEOUT_MS;      // >= sınırı
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_TIMEOUT, nextion_getState());
}

/**
 * @brief UNKNOWN durumdan IDLE durumuna geçmeli
 */
void test_nextion_process_unknownState_recoversToIdle(void) {
    gState = (Nextion_State_e)0xFFU;

    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, gState);
}

/**
 * @brief TxBusy setli olduğu sürece yeni bir göndermeyi başlatmamalı
 */
void test_nextion_idleWithTxBusy_isBusyAndDoesNotStartTx(void) {
    gTxBusy = true;
    TEST_ASSERT_TRUE(nextion_isBusy());
    (void)cmdQueue_push("j0.val=1");

    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, gState);
    TEST_ASSERT_FALSE(cmdQueue_isEmpty());
}

/**
 * @brief startTx() fonksiyonu, yer yetersizliği nedeniyle false döndürmeli
 */
void test_nextion_startTx_noTxSpace_staysIdle(void) {
    gTxBuff.head = 0U;
    gTxBuff.tail = 5U;
    (void)cmdQueue_push("j0.val=1");

    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, gState);
    TEST_ASSERT_FALSE(gTxBusy);
}

/* =========================================================================
 * nextion_readFrame() testleri
 * ========================================================================= */

/**
 * @brief Bir touch frame'i doğru şekilde ayrıştırılmalı
 */
void test_nextion_rxTouchFrame_parsedCorrectly(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t touchPayload[4] = {0x65U, 0x00U, 0x01U, 0x01U};
    simRxFrame(touchPayload, sizeof(touchPayload));
    Nextion_Frame_t frame;
    nextion_process();
// -------------------------------------------------------------
    
    Nextion_Status_e readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, readStatus);
    TEST_ASSERT_EQUAL_INT(NEXTION_FRAME_TOUCH, frame.type);
    TEST_ASSERT_EQUAL_UINT8(0x65U, frame.code);
    TEST_ASSERT_EQUAL_UINT16(3U, frame.length);
    TEST_ASSERT_EQUAL_UINT8(0x00U, frame.data[0]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, frame.data[1]);
    TEST_ASSERT_EQUAL_UINT8(0x01U, frame.data[2]);

    readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, readStatus);
}

/**
 * @brief Bir page frame'i doğru şekilde ayrıştırılmalı
 */
void test_nextion_rxPageFrame_parsedCorrectly(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t pagePayload[2] = {0x66U, 0x01U};
    Nextion_Frame_t frame;
    simRxFrame(pagePayload, sizeof(pagePayload));
    nextion_process();
// -------------------------------------------------------------

    Nextion_Status_e readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, readStatus);
    TEST_ASSERT_EQUAL_INT(NEXTION_FRAME_PAGE, frame.type);
    TEST_ASSERT_EQUAL_UINT8(0x66U, frame.code);
    TEST_ASSERT_EQUAL_UINT16(1U, frame.length);
    TEST_ASSERT_EQUAL_UINT8(0x01U, frame.data[0]);
}

/**
 * @brief Numeric cevap, getVarBlocking ile doğru değere çevrilmelidir
 * @retval None
 */
void test_nextion_getVarBlocking_numericFrame_returnsValue(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t numericPayload[5] = {0x71U, 0x01U, 0x02U, 0x03U, 0x04U};
    uint32_t value = 0U;
// -------------------------------------------------------------

    gSimRespFrame = numericPayload;
    gSimRespFrameLen = sizeof(numericPayload);
    gSimDisplayDoStuff = true;

    Nextion_Status_e status = nextion_getVarBlocking(&value, "get j0.val", sizeof(value));

    gSimDisplayDoStuff = false;
    gSimRespFrame = NULL;

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, status);
    TEST_ASSERT_EQUAL_HEX32(0x04030201UL, value);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
}

/**
 * @brief String cevap, getVarBlocking ile doğru metne çevrilmelidir
 * @retval None
 */
void test_nextion_getVarBlocking_stringFrame_returnsText(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t stringPayload[5] = {0x70U, 't', 'e', 's', 't'};
    char text[8] = {0};
// -------------------------------------------------------------

    gSimRespFrame = stringPayload;
    gSimRespFrameLen = sizeof(stringPayload);
    gSimDisplayDoStuff = true;

    Nextion_Status_e status = nextion_getVarBlocking(text, "get t0.txt", sizeof(text));

    gSimDisplayDoStuff = false;
    gSimRespFrame = NULL;

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, status);
    TEST_ASSERT_EQUAL_STRING("test", text);
}

/**
 * @brief Bir bilinmeyen format kodu ile başlayan frame'i doğru 
 *      şekilde ayrıştırılmalı
 */
void test_nextion_rxUnknownFrame_parsedAsUnknown(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t unknownPayload[3] = {0x5AU, 0x06U, 0xB5U};
    Nextion_Frame_t frame;
    simRxFrame(unknownPayload, sizeof(unknownPayload));
    nextion_process();
// -------------------------------------------------------------

    Nextion_Status_e readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, readStatus);
    TEST_ASSERT_EQUAL_INT(NEXTION_FRAME_UNKNOWN, frame.type);
    TEST_ASSERT_EQUAL_UINT8(0x5AU, frame.code);
    TEST_ASSERT_EQUAL_UINT16(2U, frame.length);
    TEST_ASSERT_EQUAL_UINT8(0x06U, frame.data[0]);
    TEST_ASSERT_EQUAL_UINT8(0xB5U, frame.data[1]);
}

/**
 * @brief Frame kuyruğu dolduğunda en eski frame'ler atılmalı, en yeni olanlar korunmalıdır
 */
void test_nextion_frameQueueOverflow_fifoTest(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t pushedFrameCount = NEXTION_FRAME_QUEUE_DEPTH + 3U;
    uint32_t i;

    for (i = 0U; i < pushedFrameCount; i++) {
        uint8_t payload[1];
        payload[0] = (uint8_t)(0x5AU + i);
        simRxFrame(payload, sizeof(payload));
    }
    nextion_process();

    // Sıra taştıktan sonra ilk gelen frame
    uint32_t expectedInQueue = NEXTION_FRAME_QUEUE_DEPTH - 1U;
    uint32_t firstExpectedCode = 0x5AU + (pushedFrameCount - expectedInQueue);

    Nextion_Frame_t frame;
// -------------------------------------------------------------

    for (i = 0U; i < expectedInQueue; i++) {
        Nextion_Status_e readStatus = nextion_readFrame(&frame);
        TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, readStatus);
        TEST_ASSERT_EQUAL_INT(NEXTION_FRAME_EVENT, frame.type);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)(firstExpectedCode + i), frame.code);
    }

    Nextion_Status_e readStatus = nextion_readFrame(&frame);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, readStatus);
}

/**
 * @brief NULL işaretcisi girildiğinde NEXTION_STATUS_INVALID döndürmeli
 */
void test_nextion_frameNull_returnsInvalidParam(void) {

    Nextion_Status_e readStatus = nextion_readFrame(NULL);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_INVALID_PARAM, readStatus);
}

/**
 * @brief Numeric cevap bekleyen komutu başarılı olarak tamamlamalıdır
 */
void test_nextion_numericResponse_completesPendingCommand(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t txBuff[32];
    uint8_t numericPayload[5] = {0x71U, 0x01U, 0x02U, 0x03U, 0x04U};
    Nextion_Status_e sendStatus = nextion_sendCmd("get j0.val");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);
    (void)simTransmit(txBuff, sizeof(txBuff));
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());
// -------------------------------------------------------------

    simRxFrame(numericPayload, sizeof(numericPayload));
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_getLastCommandStatus());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
    TEST_ASSERT_FALSE(nextion_isBusy());
}

/**
 * @brief String cevap bekleyen komutu başarılı olarak tamamlamalıdır
 */
void test_nextion_stringResponse_completesPendingCommand(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t txBuff[32];
    uint8_t stringPayload[5] = {0x70U,'t','e','s','t'};
    Nextion_Status_e sendStatus = nextion_sendCmd("get t0.txt");
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, sendStatus);
    (void)simTransmit(txBuff, sizeof(txBuff));
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());
// -------------------------------------------------------------

    simRxFrame(stringPayload, sizeof(stringPayload));
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_getLastCommandStatus());
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
    TEST_ASSERT_FALSE(nextion_isBusy());
}

/**
 * @brief Payload içermeyen yalnızca terminatörden oluşan frame atılmalıdır
 */
void test_nextion_emptyRxFrame_isIgnored(void) {
// -- Test için hazırlık adımları ------------------------------
    Nextion_Frame_t frame;
// -------------------------------------------------------------

    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);

    nextion_process();

    Nextion_Status_e readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, readStatus);
}

/**
 * @brief RX frame kapasitesi aşıldığında taşan frame atılmalıdır
 */
void test_nextion_rxFrameOverflow_frameIsDiscarded(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t i;
    Nextion_Frame_t frame;
    for (i = 0U; i < NEXTION_RX_FRAME_MAX_LEN; i++) {
        simRxByte(0x55U);

        if (0U == ((i + 1U) % 64U)) {
            nextion_process();
        }
    }
    nextion_process();
    simRxByte(0x55U);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
// -------------------------------------------------------------

    nextion_process();

    Nextion_Status_e readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, readStatus);
}

/**
 * @brief RX taşmasından sonra parser sonraki geçerli frame'i ayrıştırabilmelidir
 */
void test_nextion_rxFrameOverflow_parserRecovers(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t i;
    uint8_t pagePayload[2] = {0x66U, 0x03U};
    Nextion_Frame_t frame;
    for (i = 0U; i < NEXTION_RX_FRAME_MAX_LEN; i++) {
        simRxByte(0x55U);

        if (0U == ((i + 1U) % 64U)) {
            nextion_process();
        }
    }
    nextion_process();
    simRxByte(0x55U);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    nextion_process();
// -------------------------------------------------------------

    simRxFrame(pagePayload, sizeof(pagePayload));
    nextion_process();

    Nextion_Status_e readStatus = nextion_readFrame(&frame);

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, readStatus);
    TEST_ASSERT_EQUAL_INT(NEXTION_FRAME_PAGE, frame.type);
    TEST_ASSERT_EQUAL_UINT8(0x66U, frame.code);
    TEST_ASSERT_EQUAL_UINT16(1U, frame.length);
    TEST_ASSERT_EQUAL_UINT8(0x03U, frame.data[0]);

    readStatus = nextion_readFrame(&frame);
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, readStatus);
}

/* =========================================================================
 * isAckCode() testleri
 * ========================================================================= */

/**
 * @brief Bütün tanımlı olan cevap kodları doğru şekilde ayrıştırılmalı
 */
void test_nextion_ackCodes_allDefinedCodesRecognised(void) {
// -- Test için hazırlık adımları ------------------------------
    static const uint8_t codes[] = {
        0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U, 0x06U, 0x09U, 
        0x11U, 0x12U,0x1AU, 0x1BU, 0x1CU, 0x1DU, 0x1EU, 0x1FU, 
        0x20U, 0x23U, 0x24U
    };
    Nextion_Frame_t frame;
// -------------------------------------------------------------

    // Bu test aynı zamanda IDLE durumunda gelen ACK dalını da kapsar
    for (uint32_t i = 0U; i < (sizeof(codes) / sizeof(codes[0])); i++) {
        
        uint8_t c = codes[i];

        simRxFrame(&c, 1U);
        nextion_process();

        TEST_ASSERT_EQUAL_HEX8(c, nextion_getLastResponse());
        TEST_ASSERT_EQUAL_INT((0x01U == c) ? NEXTION_STATUS_OK : NEXTION_STATUS_ERROR,
                              nextion_getLastCommandStatus());
        TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
        TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, nextion_readFrame(&frame));
    }
}

/**
 * @brief Başlığı doğru olup uzunluğu kısa olan TOUCH frame'i bilinmeyen frame olarak atılmalı
 */
void test_nextion_rxTouchFrameTooShort_isUnknown(void) {
    uint8_t p[3] = {0x65U, 0x00U, 0x01U};
    feedAndExpectFrame(p, sizeof(p), NEXTION_FRAME_UNKNOWN, 2U);
}

/**
 * @brief Başlığı TOUCH frame'inin başlığından olsa bile tek bayttan oluşan frame EVENT olarak algılanmalı
 */
void test_nextion_rxSingleByteTouchCode_isEvent(void) {
    uint8_t p[1] = {0x65U};
    feedAndExpectFrame(p, sizeof(p), NEXTION_FRAME_EVENT, 0U);
}

/**
 * @brief Başlığı PAGE frame'inin başlığından olsa bile tek bayttan oluşan frame EVENT olarak algılanmalı
 */
void test_nextion_rxSingleBytePageCode_isEvent(void) {
    uint8_t p[1] = {0x66U};
    feedAndExpectFrame(p, sizeof(p), NEXTION_FRAME_EVENT, 0U);
}

/**
 * @brief NUMERIC frame'inin uzunluğu uyumsuz olduğunda bilinmeyen olarak atılmalı
 */
void test_nextion_rxNumericWrongLength_isUnknown(void) {
    uint8_t p[3] = {0x71U, 0x01U, 0x02U};
    feedAndExpectFrame(p, sizeof(p), NEXTION_FRAME_UNKNOWN, 2U);
}

/* =========================================================================
 * nextion_irqHandler() testleri
 * ========================================================================= */

/**
 * @brief Eğer SR'de hata bit alanları setli ise DR'deki veriler ring bufferın 
        için yazılmayıp ilk önce hata bayrakları DR'yi boşuna okuyarak temizlenmeli
 */
void test_nextion_irq_errorFlags_clearedWithoutPushingByte(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t page[2] = {0x66U, 0x04U};
    Nextion_Frame_t frame;
    gTestCfg.pUsart->DR = 0x5AU;
    gTestCfg.pUsart->SR |= USART_SR_ORE | USART_SR_NE | USART_SR_FE;
    nextion_irqHandler();
    gTestCfg.pUsart->SR &= ~(USART_SR_ORE | USART_SR_NE | USART_SR_FE);
// -------------------------------------------------------------

    // 0x5A ring buffera girseydi sonraki frame UNKNOWN olurdu
    simRxFrame(page, sizeof(page));
    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_readFrame(&frame));
    TEST_ASSERT_EQUAL_INT(NEXTION_FRAME_PAGE, frame.type);
    TEST_ASSERT_EQUAL_UINT8(0x04U, frame.data[0]);
}

/**
 * @brief TXEIE biti setli olmadığında, gelen veriler gerçekten de göz ardı edilmelidir.
 */
void test_nextion_irq_txeieWithoutTxe_doesNotTransmit(void) {
// -- Test için hazırlık adımları ------------------------------
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
    nextion_process();
    TEST_ASSERT_BITS_HIGH(USART_CR1_TXEIE, gTestCfg.pUsart->CR1);
// -------------------------------------------------------------

    gTestCfg.pUsart->SR &= ~USART_SR_TXE;
    gTestCfg.pUsart->DR = 0xABU;
    nextion_irqHandler();

    TEST_ASSERT_EQUAL_HEX32(0xABU, gTestCfg.pUsart->DR);   // bayt iletilmedi
    TEST_ASSERT_BITS_HIGH(USART_CR1_TXEIE, gTestCfg.pUsart->CR1);
}

/**
 * @brief TXIE setli olduğu sürece TC yokken gTxBusy == true olmalı
 */
void test_nextion_irq_tcieWithoutTc_keepsTxBusy(void) {
// -- Test için hazırlık adımları ------------------------------
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
    nextion_process();
// -------------------------------------------------------------

    while (0U != (gTestCfg.pUsart->CR1 & USART_CR1_TXEIE)) {
        gTestCfg.pUsart->SR |= USART_SR_TXE;
        nextion_irqHandler();
    }
    TEST_ASSERT_BITS_HIGH(USART_CR1_TCIE, gTestCfg.pUsart->CR1);

    gTestCfg.pUsart->SR &= ~USART_SR_TC;
    nextion_irqHandler();   // TC yok -> gTxBusy hala true
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_SENDING, nextion_getState());

    gTestCfg.pUsart->SR |= USART_SR_TC;
    nextion_irqHandler();
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_WAIT_ACK, nextion_getState());
}

/**
 * @brief Ring buffer bir seferde tamamen doldu ise nextion_process() 
        çağrılmadan önce yeni gelecek baytlar atılmalı
 */
void test_nextion_rxRingBuffer_full_dropsNewBytes(void) {
// -- Test için hazırlık adımları ------------------------------
    uint8_t dropped[2] = {0x66U, 0x07U};
    uint8_t valid[2]   = {0x66U, 0x02U};
    Nextion_Frame_t frame;
// -------------------------------------------------------------

    for (uint32_t i = 0U; i < (NEXTION_RING_BUFF_SIZE - 1U); i++) {
        simRxByte(0x55U); // nextion_process() çağrılmadan -> ring dolu
    }
    simRxFrame(dropped, sizeof(dropped));   // tamamen atılmalı
    nextion_process();

    // Parser'daki 255 baytlık çöp, sonlandırıcıyle birlikte taşıp atılır
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    simRxByte(NEXTION_TERM_BYTE);
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, nextion_readFrame(&frame));

    simRxFrame(valid, sizeof(valid));
    nextion_process();
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_readFrame(&frame));
    TEST_ASSERT_EQUAL_UINT8(0x02U, frame.data[0]);  // 0x07 olsaydı atılan frame geçmiş demektir
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, nextion_readFrame(&frame));
}

/* =========================================================================
 * Beyaz kutu testleri (nextion.c doğrudan include edilmiştir)
 * ========================================================================= */

/**
 * @brief Boş kuyrukta cmdQueue_peek() NULL döndürmeli
 */
void test_nextion_cmdQueuePeek_emptyQueue_returnsNull(void) {

    TEST_ASSERT_TRUE(cmdQueue_isEmpty());
    TEST_ASSERT_NULL(cmdQueue_peek());
}

/**
 * @brief Boş kuyrukta startTx() false döndürmeli ve TX'i başlatmamalı
 */
void test_nextion_startTx_emptyQueue_returnsFalse(void) {

    TEST_ASSERT_TRUE(cmdQueue_isEmpty());
    TEST_ASSERT_FALSE(nextion_startTx());

    TEST_ASSERT_FALSE(gTxBusy);
    TEST_ASSERT_BITS_LOW(USART_CR1_TXEIE, gTestCfg.pUsart->CR1);
}

/**
 * @brief Boş kuyrukta cmdQueue_pop() tail'i değiştirmemeli
 */
void test_nextion_cmdQueuePop_emptyQueue_keepsTail(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t tailBefore = gCmdQueue.tail;
// -------------------------------------------------------------

    TEST_ASSERT_TRUE(cmdQueue_isEmpty());
    cmdQueue_pop();

    TEST_ASSERT_EQUAL_UINT32(tailBefore, gCmdQueue.tail);
    TEST_ASSERT_TRUE(cmdQueue_isEmpty());
}

/**
 * @brief Dolu TX ring buffer'a bayt yazılamamalı ve head değişmemeli
 */
void test_nextion_txBuffPush_fullBuffer_returnsFalse(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t headBefore;

    for (uint32_t i = 0U; i < (NEXTION_RING_BUFF_SIZE - 1U); i++) {
        TEST_ASSERT_TRUE(txBuff_push(0xAAU));
    }
    TEST_ASSERT_TRUE(txBuff_isFull());
    headBefore = gTxBuff.head;
// -------------------------------------------------------------

    TEST_ASSERT_FALSE(txBuff_push(0x55U));
    TEST_ASSERT_EQUAL_UINT32(headBefore, gTxBuff.head);
}

/**
 * @brief '\0' ile bitmeyen komut slotu startTx() tarafından reddedilmeli
 */
void test_nextion_startTx_unterminatedSlot_returnsFalse(void) {
// -- Test için hazırlık adımları ------------------------------
    // Slot boyutu NEXTION_CMD_MAX_LEN + 1: hepsini 'a' ile doldur, '\0' yok
    (void)memset(gCmdQueue.slots[gCmdQueue.head], 'a', NEXTION_CMD_MAX_LEN + 1U);
    gCmdQueue.head = (gCmdQueue.head + 1U) % NEXTION_CMD_QUEUE_DEPTH;
// -------------------------------------------------------------

    TEST_ASSERT_FALSE(cmdQueue_isEmpty());
    TEST_ASSERT_FALSE(nextion_startTx());

    TEST_ASSERT_FALSE(gTxBusy);
    TEST_ASSERT_FALSE(cmdQueue_isEmpty());     // komut kuyruktan çıkarılmadı
}

/**
 * @brief TX buffer'da yeterli yer yoksa startTx() false döndürmeli, 
 *      durum IDLE kalmalı (429, 1027)
 */
void test_nextion_process_noTxSpace_staysIdle(void) {
// -- Test için hazırlık adımları ------------------------------
    gTxBuff.head = 0U;
    gTxBuff.tail = 5U; // boş yer = 4 < 8 + 3
    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_OK, nextion_sendCmd("j0.val=1"));
// -------------------------------------------------------------

    nextion_process();

    TEST_ASSERT_EQUAL_INT(NEXTION_STATE_IDLE, nextion_getState());
    TEST_ASSERT_FALSE(gTxBusy);
    TEST_ASSERT_FALSE(cmdQueue_isEmpty());
}

/**
 * @brief Payload'ı terminatörden kısa olan RX frame'i saklanmamalı
 */
void test_nextion_storeFrame_lengthBelowTerminator_isDropped(void) {

    gRxFrameLen = NEXTION_TERM_LEN - 1U;
    nextion_storeFrameFromRxBuffer();

    TEST_ASSERT_TRUE(nextion_frameQueueIsEmpty());
    TEST_ASSERT_FALSE(gRespFramePending);

    nextion_resetRxParser();
}

/**
 * @brief ACK alındı ama cevap deposu boşsa getVarBlocking EMPTY döndürmeli
 */
void test_nextion_getVarBlocking_respFrameMissing_returnsEmpty(void) {
// -- Test için hazırlık adımları ------------------------------
    uint32_t v = 0U;

    gSimDisplayDoStuff = true;
    gSimStealRespFrame = true;
// -------------------------------------------------------------

    Nextion_Status_e status = nextion_getVarBlocking(&v, "get j0.val", sizeof(v));

    gSimDisplayDoStuff = false;
    gSimStealRespFrame = false;

    TEST_ASSERT_EQUAL_INT(NEXTION_STATUS_EMPTY, status);
}