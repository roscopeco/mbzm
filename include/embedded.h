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
 * Replacement for stdlib routines in freestanding envs.
 * Replace these with your own if you already have libc.
 * ------------------------------------------------------------
 */

#ifndef __ROSCO_M68K_ZEMBEDDED_H
#define __ROSCO_M68K_ZEMBEDDED_H

#include <stddef.h>

void *memset (void *mem, int val, size_t len);
extern int strcmp (const char *s1, const char *s2);

#endif /* __ROSCO_M68K_ZEMBEDDED_H */
