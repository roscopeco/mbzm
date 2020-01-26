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

#ifndef __ROSCO_M68K_ZHEADERS_H
#define __ROSCO_M68K_ZHEADERS_H

#include <stdint.h>
#include "ztypes.h"

#ifdef __cplusplus
extern 'C' {
#endif

void calc_hdr_crc(ZHDR *hdr);

/*
 * Converts ZHDR to wire-format hex header. Expects CRC is already
 * computed. Result placed in the supplied buffer.
 *
 * The encoded header includes the 'B' header-type character and
 * trailing CRLF, but does not include other Zmodem control
 * characters (e.g. leading ZBUF/ZDLE etc).
 *
 * Returns actual used length (max 0xff bytes), or OUT_OF_SPACE
 * if the supplied buffer is not large enough.
 */
uint16_t to_hex_header(ZHDR *hdr, uint8_t *buf, int max_len);

#ifdef ZDEBUG
#define DEBUG_DUMPHDR(hdr)                      \
  DEBUGF("READ_HEX: Header read:\n");           \
  DEBUGF("  type: 0x%02x\n", hdr->type);        \
  DEBUGF("    f0: 0x%02x\n", hdr->f0);          \
  DEBUGF("    f1: 0x%02x\n", hdr->f1);          \
  DEBUGF("    f2: 0x%02x\n", hdr->f2);          \
  DEBUGF("    f3: 0x%02x\n", hdr->f3);          \
  DEBUGF("  crc1: 0x%02x\n", hdr->crc1);        \
  DEBUGF("  crc2: 0x%02x\n", hdr->crc2);        \
  DEBUGF("   RES: 0x%02x\n", hdr->PADDING);     \
  DEBUGF("\n");
#else
#define DEBUG_DUMPHDR(hdr)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ROSCO_M68K_ZHEADERS_H */
