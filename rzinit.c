/*
 *------------------------------------------------------------
 *                                  ___ ___ _
 *  ___ ___ ___ ___ ___       _____|  _| . | |_
 * |  _| . |_ -|  _| . |     |     | . | . | '_|
 * |_| |___|___|___|___|_____|_|_|_|___|___|_,_|
 *                     |_____|       firmware v1
 * ------------------------------------------------------------
 * Copyright (c)2020 Ross Bamford
 * See top-level LICENSE.md for licence information.
 *
 * Initialisation for rz.c example program.
 * This is directly included by rz.c.
 * ------------------------------------------------------------
 */

#include "zmodem.h"

#ifdef ZEMBEDDED
#define PRINTF(...)
#define FPRINTF(...)
#else
#define PRINTF(...) printf(__VA_ARGS__)
#define FPRINTF(...) fprintf(__VA_ARGS__)
#endif

// Spec says a data packet is max 1024 bytes, but add some headroom...
#define DATA_BUF_LEN    2048

static ZHDR hdr_zrinit = {
  .type = ZRINIT,
  .flags = {
      .f0 = 0, //CANOVIO | CANFC32 | CANFDX,
      .f1 = 0,
      .f2 = 0,
      .f3 = 0
  },
};

static ZHDR hdr_znak = {
    .type = ZNAK,
    .flags = {
        .f0 = 0,
        .f1 = 0,
        .f2 = 0,
        .f3 = 0
    }
};

static ZHDR hdr_zrpos = {
    .type = ZRPOS,
    .position = {
        .p0 = 0,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0
    }
};

static ZHDR hdr_zabort = {
    .type = ZABORT,
    .position = {
        .p0 = 0,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0
    }
};

static ZHDR hdr_zack = {
    .type = ZACK,
    .position = {
        .p0 = 0,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0
    }
};

static ZHDR hdr_zfin = {
    .type = ZFIN,
    .position = {
        .p0 = 0,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0
    }
};

static uint8_t zrinit_buf[HEX_HDR_STR_LEN + 1];
static uint8_t znak_buf[HEX_HDR_STR_LEN + 1];
static uint8_t zrpos_buf[HEX_HDR_STR_LEN + 1];
static uint8_t zabort_buf[HEX_HDR_STR_LEN + 1];
static uint8_t zack_buf[HEX_HDR_STR_LEN + 1];
static uint8_t zfin_buf[HEX_HDR_STR_LEN + 1];

static ZRESULT init_hdr_buf(ZHDR *hdr, uint8_t *buf) {
  buf[HEX_HDR_STR_LEN] = 0;
  zm_calc_hdr_crc(hdr);
  return zm_to_hex_header(hdr, buf, HEX_HDR_STR_LEN);
}
