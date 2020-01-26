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
 * Routines for working with Zmodem headers
 * ------------------------------------------------------------
 */

#include "zheaders.h"
#include "znumbers.h"
#include "xmodemcrc.h"

void calc_hdr_crc(ZHDR *hdr) {
  uint16_t crc = updcrc(hdr->type, 0);
  crc = updcrc(hdr->f0, crc);
  crc = updcrc(hdr->f1, crc);
  crc = updcrc(hdr->f2, crc);
  crc = updcrc(hdr->f3, crc);
  crc = updcrc(0, crc);
  crc = updcrc(0, crc);

  hdr->crc1 = CRC_MSB(crc);
  hdr->crc2 = CRC_LSB(crc);
}

uint16_t to_hex_header(ZHDR *hdr, uint8_t *buf, int max_len) {
  DEBUGF("Converting header to hex; Dump is:\n");
  DEBUG_DUMPHDR(hdr);

  if (max_len < HEX_HDR_STR_LEN) {
    return OUT_OF_SPACE;
  } else {
    *buf++ = 'B';                   // 01

    byte_to_hex(hdr->type, buf);    // 03
    buf += 2;
    byte_to_hex(hdr->f0, buf);      // 05
    buf += 2;
    byte_to_hex(hdr->f1, buf);      // 07
    buf += 2;
    byte_to_hex(hdr->f2, buf);      // 09
    buf += 2;
    byte_to_hex(hdr->f3, buf);      // 0b
    buf += 2;
    byte_to_hex(hdr->crc1, buf);    // 0d
    buf += 2;
    byte_to_hex(hdr->crc2, buf);    // 0f
    buf += 2;
    *buf++ = CR;                    // 10
    *buf++ = LF | 0x80;             // 11

    return HEX_HDR_STR_LEN;
  }
}

