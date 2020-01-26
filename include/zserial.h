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

/*
 * The lib doesn't implement these - they need to be provided.
 */
int recv();
int send(uint8_t chr);

/*
 * Receive CR/LF (with CR being optional).
 */
uint16_t read_crlf();

uint16_t read_hex_byte();

/*
 * buf must be one character longer than the string...
 * Trashes buf, for obvious reasons.
 */
bool await(char *str, char *buf, int buf_size);

bool await_zdle();

uint16_t read_hex_header(ZHDR *hdr);

uint16_t await_header(ZHDR *hdr);

bool await_zrqinit();

/*
 * Send a null-terminated string.
 */
uint16_t send_sz(uint8_t *data);

#ifdef __cplusplus
}
#endif

#endif /* __ROSCO_M68K_ZSERIAL_H */
