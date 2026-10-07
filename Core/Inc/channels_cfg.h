/**
 * ╔═══════════════════════════════════════════════════════════════════════╗
 * ║                              GENX-400HF-L                             ║
 * ╚═══════════════════════════════════════════════════════════════════════╝
 *
 * @file    channels_cfg.h
 * @brief   Kanal yolu başına mod açma/kapama anahtarları.
 *          Bir modu açmak/kapatmak için ilgili satırdaki 1U/0U değerini değiştirmek yeterlidir
 *
 * @author  destrocore
 * @date    2026
 */

#ifndef CHANNELS_CFG_H
#define CHANNELS_CFG_H

#ifdef __cplusplus
extern "C" {
#endif

/* =========================================================================
 * Mod anahtarları
 * ========================================================================= */

// -- MONO1 CUT ------------------------------------------------
#define MONO1_CUT_EN_CUT            1U
#define MONO1_CUT_EN_BLEND1         1U
#define MONO1_CUT_EN_BLEND2         1U
#define MONO1_CUT_EN_BLEND3         1U
#define MONO1_CUT_EN_POLYPECTOMY    1U
#define MONO1_CUT_EN_PAPILLOTOMES   1U
#define MONO1_CUT_EN_RESECTION1     1U
#define MONO1_CUT_EN_RESECTION2     1U

// -- MONO1 COAG -----------------------------------------------
#define MONO1_COAG_EN_CONTACT       1U
#define MONO1_COAG_EN_SPRAY1        1U
#define MONO1_COAG_EN_SPRAY2        1U
#define MONO1_COAG_EN_SPRAY3        1U
#define MONO1_COAG_EN_FULGURATION   1U

// -- MONO2 CUT ------------------------------------------------
#define MONO2_CUT_EN_CUT            1U
#define MONO2_CUT_EN_BLEND1         1U
#define MONO2_CUT_EN_BLEND2         1U
#define MONO2_CUT_EN_BLEND3         1U
#define MONO2_CUT_EN_POLYPECTOMY    0U
#define MONO2_CUT_EN_PAPILLOTOMES   0U
#define MONO2_CUT_EN_RESECTION1     1U
#define MONO2_CUT_EN_RESECTION2     1U

// -- MONO2 COAG -----------------------------------------------
#define MONO2_COAG_EN_CONTACT       1U
#define MONO2_COAG_EN_SPRAY1        1U
#define MONO2_COAG_EN_SPRAY2        1U
#define MONO2_COAG_EN_SPRAY3        1U
#define MONO2_COAG_EN_FULGURATION   1U

// -- BIPOLAR1 CUT ---------------------------------------------
#define BIPOLAR1_CUT_EN_CUTTING     1U
#define BIPOLAR1_CUT_EN_SCISSORS    1U
#define BIPOLAR1_CUT_EN_BIVAPO      1U
#define BIPOLAR1_CUT_EN_BIREZO      1U

// -- BIPOLAR1 COAG --------------------------------------------
#define BIPOLAR1_COAG_EN_STANDARD   1U
#define BIPOLAR1_COAG_EN_FORCED     1U
#define BIPOLAR1_COAG_EN_STOP       1U
#define BIPOLAR1_COAG_EN_START      1U
#define BIPOLAR1_COAG_EN_LIGATION   0U
#define BIPOLAR1_COAG_EN_SEALSURE   0U
#define BIPOLAR1_COAG_EN_TISSUELOCK 0U

// -- BIPOLAR2 (seal) CUT --------------------------------------
#define BIPOLAR2_CUT_EN_CUTTING     1U
#define BIPOLAR2_CUT_EN_SCISSORS    1U
#define BIPOLAR2_CUT_EN_BIVAPO      1U
#define BIPOLAR2_CUT_EN_BIREZO      1U

// -- BIPOLAR2 (seal) COAG -------------------------------------
#define BIPOLAR2_COAG_EN_STANDARD   0U
#define BIPOLAR2_COAG_EN_FORCED     0U
#define BIPOLAR2_COAG_EN_STOP       0U
#define BIPOLAR2_COAG_EN_START      0U
#define BIPOLAR2_COAG_EN_LIGATION   1U
#define BIPOLAR2_COAG_EN_SEALSURE   1U
#define BIPOLAR2_COAG_EN_TISSUELOCK 1U

#ifdef __cplusplus
}
#endif

#endif /* CHANNELS_CFG_H */