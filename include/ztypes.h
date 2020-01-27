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
 * Types and defines for Zmodem implementation
 * ------------------------------------------------------------
 */


#ifndef __ROSCO_M68K_ZTYPES_H
#define __ROSCO_M68K_ZTYPES_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern 'C' {
#endif

// Control characters
#define XON         0x11
#define XOFF        0x13
#define LF          0x0a
#define CR          0x0d
#define ZPAD        '*'
#define ZDLE        0x18

// Header types
#define ZBIN16      'A'
#define ZHEX        'B'
#define ZBIN32      'C'

// Frame types
#define ZRQINIT     0x00
#define ZFILE       0x04

// Result codes
#define VALUE_MASK        0x00ff        /* Mask used to extract value from ZRESULT          */
#define ERROR_MASK        0xf000        /* Mask used to determine if result is an error     */
#define OK                0x0100        /* Return code for "all is well"                    */
#define BAD_DIGIT         0x1000        /* Bad digit when converting from hex               */
#define GOT_EOF           0x2000        /* Got EOF when reading from stream                 */
#define BAD_HEADER_TYPE   0x3000        /* Bad header type in stream (probably noise)       */
#define BAD_FRAME_TYPE    0x4000        /* Bad frame type in stream (probably noise)        */
#define CORRUPTED         0x5000        /* Corruption detected in header (probably noise)   */
#define BAD_CRC           0x6000        /* Header did not match CRC (probably noise)        */
#define OUT_OF_RANGE      0x7000        /* Conversion attempted for out-of-range number     */
#define OUT_OF_SPACE      0x8000        /* Supplied buffer is not big enough                */
#define UNSUPPORTED       0xf000        /* Attempted to use an unsupported protocol feature */

#define ERROR_CODE(x)     (x & ERROR_MASK)
#define IS_ERROR(x)       ((bool)(ERROR_CODE((x)) != 0))
#define ZVALUE(x)         ((uint8_t)(x & 0xff))

// Nybble to byte / byte to word / vice-versa
// TODO check - still shl on LE plaf?
#define NTOB(n1, n2)      (n1 << 4 | n2)        /* 2 nybbles    -> byte                     */
#define BTOW(b1, b2)      (b1 << 8 | b2)        /* 2 bytes      -> word                     */
#define BMSN(b)           ((b & 0xf0) >> 4)     /* byte         -> most-significant nybble  */
#define BLSN(b)           (b & 0x0f)            /* byte         -> least-significant nybble */
#define WMSB(w)           ((w & 0xff00) >> 8)   /* word         -> most-significant byte    */
#define WLSB(w)           (w & 0x00ff)          /* word         -> least-significant byte   */

// CRC manipulation
#define CRC               BTOW                  /* Convert two-bytes to 16-bit CRC          */
#define CRC_MSB           WMSB                  /* Get most-significant byte of 16-bit CRC  */
#define CRC_LSB           WLSB                  /* Get least-significant byte of 16-bit CRC */

// Various sizes
#define ZHDR_SIZE         0x07                  /* Size of ZHDR (excluding padding)         */
#define HEX_HDR_STR_LEN   0x11                  /* Total size of a ZHDR encoded as hex      */

/*
 * The ZRESULT type is the general return type for functions in this library.
 *
 */
typedef uint16_t ZRESULT;

typedef struct {
  uint8_t   type;
  uint8_t   f0;
  uint8_t   f1;
  uint8_t   f2;
  uint8_t   f3;
  uint8_t   crc1;         /* keep these byte-sized to avoid alignment */
  uint8_t   crc2;         /* issues with 16-bit reads on m68k         */
  uint8_t   PADDING;
} ZHDR;

#ifdef ZDEBUG
#define DEBUGF(...)       printf(__VA_ARGS__)

#else
#define DEBUGF(...)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __ROSCO_M68K_ZTYPES_H */
