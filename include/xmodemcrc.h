/*
 *------------------------------------------------------------
 *                                  ___ ___ _
 *  ___ ___ ___ ___ ___       _____|  _| . | |_
 * |  _| . |_ -|  _| . |     |     | . | . | '_|
 * |_| |___|___|___|___|_____|_|_|_|___|___|_,_|
 *                     |_____|       firmware v1
 * ------------------------------------------------------------
 * See individual copyright notices, below.
 *
 * CRC Calculation Routines from various authors.
 * ------------------------------------------------------------
 */
/*
 * Copyright (C) 1986 Gary S. Brown.  You may use this program, or
 * code or tables extracted from it, as desired without restriction.
 */
/*
 * updcrc macro derived from article Copyright (C) 1986 Stephen Satchell.
 *  NOTE: First srgument must be in range 0 to 255.
 *        Second argument is referenced twice.
 *
 * Programmers may incorporate any or all code into their programs,
 * giving proper credit within the source. Publication of the
 * source routines is permitted so long as proper credit is given
 * to Stephen Satchell, Satchell Evaluations and Chuck Forsberg,
 * Omen Technology.
 */

#ifndef __ROSCO_M68K_XMODEMCRC_H
#define __ROSCO_M68K_XMODEMCRC_H

#ifdef __cplusplus
extern 'C' {
#endif

extern unsigned short crctab[256];
extern long cr3tab[];

/*
 *  Crc calculation stuff
 */

#define updcrc(cp, crc) ( crctab[((crc >> 8) & 255)] ^ (crc << 8) ^ cp)

#ifdef NFGM
long
UPDC32(b, c)
long c;
{
    return (cr3tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF));
}

#else

#define UPDC32(b, c) (cr3tab[((int)c ^ b) & 0xff] ^ ((c >> 8) & 0x00FFFFFF))
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ROSCO_M68K_XMODEMCRC_H */
