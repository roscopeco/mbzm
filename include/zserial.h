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
 * Generic serial routines for Zmodem
 * ------------------------------------------------------------
 */

#ifndef __ROSCO_M68K_ZSERIAL_H
#define __ROSCO_M68K_ZSERIAL_H

#include <stdbool.h>
#include <stdint.h>
#include "ztypes.h"

#ifdef __cplusplus
extern 'C' {
#endif

#define NONCONTROL(c)    ((bool)((uint8_t)(c & 0xe0)))

/*
 * The lib doesn't implement these - they need to be provided.
 */
ZRESULT recv();
ZRESULT send(uint8_t chr);

/*
 * Receive CR/LF (with CR being optional).
 */
ZRESULT read_crlf();

ZRESULT read_hex_byte();

/*
 * buf must be one character longer than the string...
 * Trashes buf, for obvious reasons.
 */
ZRESULT await(char *str, char *buf, int buf_size);
ZRESULT await_zdle();
ZRESULT await_header(ZHDR *hdr);

ZRESULT read_hex_header(ZHDR *hdr);
ZRESULT read_binary16_header(ZHDR *hdr);

ZRESULT read_escaped();

/*
 * Send a null-terminated string.
 */
ZRESULT send_sz(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __ROSCO_M68K_ZSERIAL_H */
