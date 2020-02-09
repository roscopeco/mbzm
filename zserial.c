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

#ifdef ZDEBUG
#include <stdio.h>
#endif

#ifndef ZEMBEDDED
#include <string.h>
#else
#include "embedded.h"
#endif

#include "zserial.h"
#include "zheaders.h"
#include "znumbers.h"
#include "checksum.h"

ZRESULT zm_read_crlf() {
  uint16_t c = zm_recv();

  if (IS_ERROR(c)) {
    DEBUGF("CRLF: Got error on first character: 0x%04x\n", c);
    return c;
  } else if (c == LF || c == (LF | 0x80)) {
    TRACEF("CRLF: Got LF on first character: OK\n");
    return OK;
  } else if (c == CR || c == (CR | 0x80)) {
    TRACEF("CRLF: Got CR on first character, await LF\n");
    c = zm_recv();

    if (IS_ERROR(c)) {
      return c;
      DEBUGF("CRLF: Got error on second character: 0x%04x\n", c);
    } else if (c == LF || c == (LF | 0x80)) {
      TRACEF("CRLF: Got LF on second character: OK\n");
      return OK;
    } else {
      TRACEF("CRLF: Got corruption on second character: 0x%04x\n", c);
      return CORRUPTED;
    }
  } else {
    DEBUGF("CRLF: Got corruption on first character: 0x%04x\n", c);
    return CORRUPTED;
  }
}

ZRESULT zm_read_hex_byte() {
  int c1 = zm_recv(), c2;

  if (IS_ERROR(c1)) {
    return c1;
  } else {
    c2 = zm_recv();
    if (IS_ERROR(c2)) {
      return c2;
    } else {
      return zm_hex_to_byte(c1, c2);
    }
  }
}

ZRESULT zm_read_escaped(bool noHdr) {
  ZRESULT c;

  while (true) {
    c = zm_recv();

    // Return immediately if non-control character or error
    if (NONCONTROL(c) || IS_ERROR(c) || IS_CTL(c)) {
      return c;
    }

    switch (c) {
    case XON:
    case XON | 0x80:
    case XOFF:
    case XOFF | 0x80:
    case ZPAD:
    case ZPAD | 0200:
      continue;
      continue;
    case ZDLE:
      TRACEF("  >> READ_ESCAPED: Got ZDLE\n");
      goto gotzdle;
    default:
      // TODO support control escaping?
      TRACEF("  >> READ_ESCAPED: Got 0x%02x [%c]\n", c, c & 0xff);
      return c;
    }
  }

  gotzdle:

  /* Picked up this trick from the original ZM implementation. Took me a few
   * minutes to figure it out, so here's what's going on.
   *
   * At the begining, c is **definitely** ZDLE (a.k.a. CAN, important to note that
   * they're the same!) so we can say we've already received exactly one CAN before
   * we enter this section.
   *
   * (We know this because the condition for entering this section was that we
   * received that ZDLE (a.k.a. CAN), and anything other than ZDLE was just
   * returned, give or take some swallowing of software flow control characters).
   *
   * 1. The first 'if' reads the next character, and immediately returns on error.
   *    If no error, c is set to the next character.
   *
   * 2. We check the character read in 1. If it **was** CAN (making two CANs so far):
   *        * Read the next character into c, immediately return on error.
   *
   *    If it **wasn't** CAN, this step does nothing (i.e. nothing is read), and
   *    (crucially) the rest of these steps will be skipped too, since c never changes.
   *    **This part is important**.
   *
   * 3. We check the character read in 2. If it was CAN (making three CANs so far):
   *        * Read the next character into c, immediately return on error.
   *
   *    This is the clever part - if the character in read in 1 **was not** CAN,
   *    then no character will have been read in step 2. So we're checking the
   *    character read in 1 again, and it still cannot be CAN so again we won't read
   *    another character.
   *
   *    Furthermore, if the character read in 1 **was** CAN, but the character read
   *    in step 2 **was not** CAN, then we won't see CAN in this step, or in any of
   *    the following steps, and they'll all be skipped.
   *
   * 4. We check the character read in 3. If it was CAN (making four CANs so far):
   *        * Read the next character into c, immediately return on error.
   *
   *    Again, if the character we read in any of the preceeding steps **was not** CAN,
   *    then c still hasn't changed because we haven't read any more characters.
   *
   * 5. Now, we enter the switch. At this point, we know the following to be true:
   *
   *        * If c is CAN, we have **definitely** seen five CANs, indicating
   *          the other end wants to cancel the transfer, **or**
   *
   *        * c is definitely **not** CAN, but WAS preceeded by exactly one ZDLE,
   *          so is either protocol control, or an escaped character.
   *
   */
  if (IS_ERROR(c = zm_recv()))
    return c;
  if (c == CAN && IS_ERROR(c = zm_recv()))
    return c;
  if (c == CAN && IS_ERROR(c = zm_recv()))
    return c;
  if (c == CAN && IS_ERROR(c = zm_recv()))
    return c;

  switch (c) {
  case CAN:
    DEBUGF("  >> READ_ESCAPED: Got five CANs!\n");
    return CANCELLED;
  case ZCRCE:
    DEBUGF("  >> READ_ESCAPED: Got ZCRCE\n");
    return GOT_CRCE;
  case ZCRCG:
    DEBUGF("  >> READ_ESCAPED: Got ZCRCG\n");
    return GOT_CRCG;
  case ZCRCQ:
    DEBUGF("  >> READ_ESCAPED: Got ZCRCQ\n");
    return GOT_CRCQ;
  case ZCRCW:
    DEBUGF("  >> READ_ESCAPED: Got ZCRCW\n");
    return GOT_CRCW;
  case ZHEX:
    DEBUGF("  >> READ_ESCAPED: Got ZHEX\n");
    return noHdr ? ZHEX : GOT_HDR_HEX;
  case ZBIN16:
    DEBUGF("  >> READ_ESCAPED: Got ZBIN16\n");
    return noHdr ? ZBIN16 : GOT_HDR_BIN16;
  case ZBIN32:
    DEBUGF("  >> READ_ESCAPED: Got ZBIN32\n");
    return noHdr ? ZBIN32 : GOT_HDR_BIN32;
  case TIMEOUT:
    return TIMEOUT;
  default:
    if ((c & 0x60) == 0x40) {
      TRACEF("  >> READ_ESCAPED: Got escaped character: 0x%02x\n", (c ^ 0x40));
      return c ^ 0x40;
    }
  }

  DEBUGF("  >> READ_ESCAPED: Got bad control character 0x%02x", (c & 0xff));
  return BAD_ESCAPE;
}

/* Just read a data block - no CRC checking is done; see read_data_block */
static ZRESULT recv_data_block(uint8_t *buf, uint16_t *len) {
  uint16_t max = *len;
  *len = 0;

  while (*len < max) {
    ZRESULT c = zm_read_escaped(true);

    if (IS_ERROR(c)) {
      DEBUGF("  >> RECV_BLOCK: GOT ERROR: 0x%04x\n", c);
      return c;
    } else if (IS_CTL(c)) {
      DEBUGF("  >> RECV_BLOCK: GOT CONTROL (probably cancel / timeout): 0x%04x\n", c);
      return c;
    } else {
      // Always add, even if frameend, as CRC takes that into account...
      buf[(*len)++] = ZVALUE(c);

      if (IS_FIN(c)) {
        return c;
      }
    }
  }

  return OUT_OF_SPACE;
}

ZRESULT zm_read_data_block(uint8_t *buf, uint16_t *len) {
  const ZRESULT result = recv_data_block(buf, len);
  ZRESULT crc_result;
  DEBUGF("  >> READ_BLOCK: Result of data block recv is [0x%04x] (got %d character(s))\n", result, *len);

  if (IS_ERROR(result) || IS_CTL(result)) {
    return result;
  } else {
    // CRC bytes are ZDLE escaped!
    crc_result = zm_read_escaped(true);
    if (IS_ERROR(crc_result)) {
      DEBUGF("  >> READ_BLOCK: Error while reading crc1: 0x%04x\n", crc_result);
      return crc_result;
    }

    uint8_t crc1 = ((uint8_t)crc_result) & 0xff;

    crc_result = zm_read_escaped(true);
    if (IS_ERROR(crc_result)) {
      return crc_result;
      DEBUGF("  >> READ_BLOCK: Error while reading crc2: 0x%04x\n", crc_result);
    }

    uint8_t crc2 = ((uint8_t)crc_result) & 0xff;

    uint16_t recv_crc = CRC(crc1, crc2);
    uint16_t calc_crc = zm_calc_data_crc(buf, *len);

    if (recv_crc == calc_crc) {
      return result;
    } else {
      DEBUGF("  >> READ_BLOCK: CRC is borked (recv: 0x%04x; calc: 0x%04x)\n", recv_crc, calc_crc);
      return BAD_CRC;
    }
  }
}


ZRESULT zm_await(char *str, char *buf, int buf_size) {
  memset(buf, 0, 4);
  int ptr = 0;

  while(true) {
    int c = zm_recv();

    if (IS_ERROR(c)) {
      return c;
    } else if(c == TIMEOUT) {
      continue;
    } else {
      // shift buffer if necessary
      if (ptr == buf_size - 1) {
        for (int i = 0; i < buf_size - 2; i++) {
          buf[i] = buf[i+1];
        }
        buf[buf_size - 2] = c;
      } else {
        buf[ptr++] = c;
      }

#ifdef ZTRACE
      TRACEF("Buf is [");
      for (int i = 0; i < buf_size; i++) {
        TRACEF("%02x ", buf[i]);
      }
      TRACEF("]\n");
#endif

      // TODO if we don't use strcmp, we don't need buf to be one char longer...
      if (strcmp(str, buf) == 0) {
        DEBUGF("AWAIT: Target received; Await completed...\n");
        return OK;
      }
    }
  }
}

ZRESULT zm_read_hex_header(ZHDR *hdr) {
  uint8_t *ptr = (uint8_t*)hdr;
  memset(hdr, 0xc0, sizeof(ZHDR));
  uint16_t crc = CRC_START_XMODEM;

  // TODO maybe don't treat header as a stream of bytes, which would remove
  //      the need to have the all-byte layout in ZHDR struct...
  for (int i = 0; i < ZHDR_SIZE; i++) {
    // TODO use read_hex_byte here...
    uint16_t c1 = zm_recv();

    if (IS_CTL(c1)) {
      DEBUGF("READ_HEX: Character %d/1 is control: 0x%04x\n", i, c1);
      return c1;
    } else if (IS_ERROR(c1)) {
      DEBUGF("READ_HEX: Character %d/1 is Error: 0x%04x\n", i, c1);
      return CLOSED;
    } else {
      TRACEF("READ_HEX: Character %d/1 is good: 0x%02x\n", i, c1);
      uint16_t c2 = zm_recv();

      if (IS_CTL(c2)) {
        DEBUGF("READ_HEX: Character %d/2 is control: 0x%04x\n", i, c2);
        return c2;
      } else if (IS_ERROR(c2)) {
        DEBUGF("READ_HEX: Character %d/2 is Error: 0x%04x\n", i, c2);
        return CLOSED;
      } else {
        TRACEF("READ_HEX: Character %d/2 is good: 0x%02x\n", i, c2);
        uint16_t b = zm_hex_to_byte(c1,c2);

        if (IS_ERROR(b)) {
          DEBUGF("READ_HEX: hex_to_byte %d is error: 0x%04x\n", i, b);
          return b;
        } else {
          TRACEF("READ_HEX: Byte %d is good: 0x%02x\n", i, b);
          TRACEF("Byte %d; hdr at 0x%0lx; ptr at 0x%0lx\n", i, (uint64_t)hdr, (uint64_t)ptr);
          *ptr++ = (uint8_t)b;

          if (i < ZHDR_SIZE - 2) {
            TRACEF("Will update CRC for byte %d\n", i);
            crc = update_crc_ccitt(crc, b);
          } else {
            TRACEF("Won't update CRC for byte %d\n", i);
          }

          TRACEF("READ_HEX: CRC after byte %d is 0x%04x\n", i, crc);
        }
      }
    }
  }

  DEBUG_DUMPHDR(hdr);

  DEBUGF("READ_HEX: All read; check CRC (Received: 0x%04x; Computed: 0x%04x)\n", CRC(hdr->crc1, hdr->crc2), crc);
  return zm_check_header_crc16(hdr, crc);
}

ZRESULT zm_read_binary16_header(ZHDR *hdr) {
  uint8_t *ptr = (uint8_t*)hdr;
  memset(hdr, 0xc0, sizeof(ZHDR));
  uint16_t crc = CRC_START_XMODEM;

  for (int i = 0; i < ZHDR_SIZE; i++) {
    uint16_t b = zm_recv();

    if (IS_CTL(b)) {
      DEBUGF("READ_BIN16: Character %d/1 is control: 0x%04x\n", i, b);
      return b;
    } else if (IS_ERROR(b)) {
      DEBUGF("READ_BIN16: Character %d/1 is error: 0x%04x\n", i, b);
      return b;
    } else {
      TRACEF("READ_BIN16: Byte %d is good: 0x%02x\n", i, b);
      TRACEF("Byte %d; hdr at 0x%0lx; ptr at 0x%0lx\n", i, (uint64_t)hdr, (uint64_t)ptr);
      *ptr++ = (uint8_t)b;

      if (i < ZHDR_SIZE - 2) {
        TRACEF("Will update CRC for byte %d\n", i);
        crc = update_crc_ccitt(crc, b);
      } else {
        TRACEF("Won't update CRC for byte %d\n", i);
      }

      TRACEF("READ_BIN16: CRC after byte %d is 0x%04x\n", i, crc);
    }
  }

  DEBUG_DUMPHDR(hdr);

  DEBUGF("READ_BIN16: All read; check CRC (Received: 0x%04x; Computed: 0x%04x)\n", CRC(hdr->crc1, hdr->crc2), crc);
  return zm_check_header_crc16(hdr, crc);
}

ZRESULT zm_await_header(ZHDR *hdr) {
  DEBUGF(">> ZM_AWAIT_HEADER\n");

  while (true) {
    uint16_t result = zm_read_escaped(false);

    if (IS_ERROR(result)) {
      DEBUGF("ZM_AWAIT_HEADER: Error while reading escaped 0x%04x...\n", result);
      return result;
    } else if (IS_CTL(result)) {
      DEBUGF("ZM_AWAIT_HEADER: Control received while waiting; probable timeout...\n");
      return result;
    } else {
      switch (result) {
      case ZPAD:
        DEBUGF("Got ZPAD while waiting for header; Skipping...\n");
        continue;

      case GOT_HDR_HEX:
        DEBUGF("ZM_AWAIT_HEADER: Reading HEX header\n");
        result = zm_read_hex_header(hdr);

        if (result == OK) {
          DEBUGF("ZM_AWAIT_HEADER: Got valid header\n");
          return zm_read_crlf();
        } else {
          DEBUGF("ZM_AWAIT_HEADER: Didn't get valid header [0x%02x]\n", result);
          return result;
        }

      case GOT_HDR_BIN16:
        DEBUGF("ZM_AWAIT_HEADER: Reading BIN16 header\n");
        result = zm_read_binary16_header(hdr);

        if (result == OK) {
          DEBUGF("ZM_AWAIT_HEADER: Got valid header\n");
          return OK;
        } else {
          DEBUGF("ZM_AWAIT_HEADER: Didn't get valid header [0x%02x]\n", result);
          return result;
        }

      case GOT_HDR_BIN32:
        DEBUGF("ZM_AWAIT_HEADER: Cannot handle 32-bit CRC frames yet\n");
        return UNSUPPORTED;

      default:
        DEBUGF("ZM_AWAIT_HEADER: Got bad frame type 0x%02x\n", ZVALUE(result));
        return BAD_FRAME_TYPE;
//        continue;
      }
    }
  }
}

ZRESULT zm_send_sz(uint8_t *data) {
  register uint16_t result;

  while (*data) {
    result = zm_send(*data++);

    if (IS_ERROR(result)) {
      return result;
    }
  }

  return OK;
}

static uint8_t *last_sent_buf = NULL;

static ZRESULT zm_send_hex_hdr(uint8_t *buf) {
  zm_send(ZPAD);
  zm_send(ZPAD);
  zm_send(ZDLE);

  return zm_send_sz(last_sent_buf = buf);
}

ZRESULT zm_send_hdr(ZHDR *hdr) {
  static uint8_t buf[HEX_HDR_STR_LEN + 1];

  DEBUGF(">> SEND HEADER\n");
  DEBUG_DUMPHDR(hdr);

  buf[HEX_HDR_STR_LEN] = 0;
  zm_calc_hdr_crc(hdr);

  ZRESULT result = zm_to_hex_header(hdr, buf, HEX_HDR_STR_LEN);

  if (IS_ERROR(result) || ZVALUE(result) != HEX_HDR_STR_LEN) {
    DEBUGF("ZM_SEND_HDR: Failed to hex-encode header: 0x%04x\n", result);
    return result;
  } else {
    return zm_send_hex_hdr(buf);
  }
}

ZRESULT zm_send_hdr_flags(uint8_t type, uint8_t f0, uint8_t f1, uint8_t f2, uint8_t f3) {
  ZHDR hdr = {
      .type = type,
      .flags = {
          .f0 = f0,
          .f1 = f1,
          .f2 = f2,
          .f3 = f3
      }
  };

  return zm_send_hdr(&hdr);
}

ZRESULT zm_send_hdr_pos(uint8_t type, uint8_t p0, uint8_t p1, uint8_t p2, uint8_t p3) {
  ZHDR hdr = {
      .type = type,
      .position = {
          .p0 = p0,
          .p1 = p1,
          .p2 = p2,
          .p3 = p3
      }
  };

  return zm_send_hdr(&hdr);
}

ZRESULT zm_send_hdr_pos32(uint8_t type, uint32_t pos) {
  ZHDR hdr = {
      .type = type,
  };

  *((uint32_t*)&hdr.position) = pos;

  return zm_send_hdr(&hdr);
}



ZRESULT zm_resend_last_header() {
  if (last_sent_buf) {
    return zm_send_sz(last_sent_buf);
  } else {
    return NO_DATA;
  }
}
