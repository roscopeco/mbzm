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

ZRESULT read_crlf() {
  uint16_t c = recv();

  if (IS_ERROR(c)) {
    DEBUGF("CRLF: Got error on first character: 0x%04x\n", c);
    return c;
  } else if (c == LF || c == (LF | 0x80)) {
    DEBUGF("CRLF: Got LF on first character: OK\n");
    return OK;
  } else if (c == CR || c == (CR | 0x80)) {
    DEBUGF("CRLF: Got CR on first character, await LF\n");
    c = recv();

    if (IS_ERROR(c)) {
      return c;
      DEBUGF("CRLF: Got error on second character: 0x%04x\n", c);
    } else if (c == LF || c == (LF | 0x80)) {
      DEBUGF("CRLF: Got LF on second character: OK\n");
      return OK;
    } else {
      DEBUGF("CRLF: Got corruption on second character: 0x%04x\n", c);
      return CORRUPTED;
    }
  } else {
    DEBUGF("CRLF: Got corruption on first character: 0x%04x\n", c);
    return CORRUPTED;
  }
}

ZRESULT read_hex_byte() {
  int c1 = recv(), c2;

  if (IS_ERROR(c1)) {
    return c1;
  } else {
    c2 = recv();
    if (IS_ERROR(c2)) {
      return c2;
    } else {
      return hex_to_byte(c1, c2);
    }
  }
}

ZRESULT read_escaped() {
  while (true) {
    ZRESULT c = recv();

    // Return immediately if non-control character or error
    if (NONCONTROL(c) || IS_ERROR(c)) {
      return c;
    }

    switch (c) {


    }
  }
}

ZRESULT await(char *str, char *buf, int buf_size) {
  memset(buf, 0, 4);
  int ptr = 0;

  while(true) {
    int c = recv();

    if (IS_ERROR(c)) {
      return c;
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

      DEBUGF("Buf is [");
      for (int i = 0; i < buf_size; i++) {
        DEBUGF("%02x ", buf[i]);
      }
      DEBUGF("]\n");

      // TODO if we don't use strcmp, we don't need buf to be one char longer...
      if (strcmp(str, buf) == 0) {
        DEBUGF("Target received; Await completed...\n");
        return OK;
      }
    }
  }
}

ZRESULT await_zdle() {
  while (true) {
    int c = recv();

    if (IS_ERROR(c)) {
      DEBUGF("Got error :(\n");
      return c;
    } else {
      switch (c) {
      case ZPAD:
      case ZPAD | 0200:
        DEBUGF("Got ZPAD...\n");
        continue;
      case ZDLE:
        DEBUGF("Got ZDLE\n");
        return OK;
      default:
        DEBUGF("Got unknown (%08x)\n", c);
        continue;
      }
    }
  }
}

ZRESULT read_hex_header(ZHDR *hdr) {
  uint8_t *ptr = (uint8_t*)hdr;
  memset(hdr, 0xc0, sizeof(ZHDR));
  uint16_t crc = CRC_START_XMODEM;

  // TODO maybe don't treat header as a stream of bytes, which would remove
  //      the need to have the all-byte layout in ZHDR struct...
  for (int i = 0; i < ZHDR_SIZE; i++) {
    // TODO use read_hex_byte here...
    uint16_t c1 = recv();

    if (IS_ERROR(c1)) {
      DEBUGF("READ_HEX: Character %d/1 is error: 0x%04x\n", i, c1);
      return c1;
    } else if (IS_ERROR(c1)) {
      DEBUGF("READ_HEX: Character %d/1 is EOF\n", i);
      return CLOSED;
    } else {
      DEBUGF("READ_HEX: Character %d/1 is good: 0x%02x\n", i, c1);
      uint16_t c2 = recv();

      if (IS_ERROR(c2)) {
        DEBUGF("READ_HEX: Character %d/2 is error: 0x%04x\n", i, c2);
        return c2;
      } else if (IS_ERROR(c2)) {
        DEBUGF("READ_HEX: Character %d/2 is EOF\n", i);
        return CLOSED;
      } else {
        DEBUGF("READ_HEX: Character %d/2 is good: 0x%02x\n", i, c2);
        uint16_t b = hex_to_byte(c1,c2);

        if (IS_ERROR(b)) {
          DEBUGF("READ_HEX: hex_to_byte %d is error: 0x%04x\n", i, b);
          return b;
        } else {
          DEBUGF("READ_HEX: Byte %d is good: 0x%02x\n", i, b);
          DEBUGF("Byte %d; hdr at 0x%0lx; ptr at 0x%0lx\n", i, (uint64_t)hdr, (uint64_t)ptr);
          *ptr++ = (uint8_t)b;

          if (i < ZHDR_SIZE - 2) {
            DEBUGF("Will update CRC for byte %d\n", i);
            crc = update_crc_ccitt(crc, b);
          } else {
            DEBUGF("Won't update CRC for byte %d\n", i);
          }

          DEBUGF("READ_HEX: CRC after byte %d is 0x%04x\n", i, crc);
        }
      }
    }
  }

  DEBUG_DUMPHDR(hdr);

  DEBUGF("READ_HEX: All read; check CRC (Received: 0x%04x; Computed: 0x%04x)\n", CRC(hdr->crc1, hdr->crc2), crc);
  return check_header_crc16(hdr, crc);
}

ZRESULT read_binary16_header(ZHDR *hdr) {
  uint8_t *ptr = (uint8_t*)hdr;
  memset(hdr, 0xc0, sizeof(ZHDR));
  uint16_t crc = CRC_START_XMODEM;

  for (int i = 0; i < ZHDR_SIZE; i++) {
    uint16_t b = recv();

    if (IS_ERROR(b)) {
      DEBUGF("READ_BIN16: Character %d/1 is error: 0x%04x\n", i, b);
      return b;
    } else if (IS_ERROR(b)) {
      DEBUGF("READ_BIN16: Character %d/1 is EOF\n", i);
      return CLOSED;
    } else {
      DEBUGF("READ_BIN16: Byte %d is good: 0x%02x\n", i, b);
      DEBUGF("Byte %d; hdr at 0x%0lx; ptr at 0x%0lx\n", i, (uint64_t)hdr, (uint64_t)ptr);
      *ptr++ = (uint8_t)b;

      if (i < ZHDR_SIZE - 2) {
        DEBUGF("Will update CRC for byte %d\n", i);
        crc = update_crc_ccitt(crc, b);
      } else {
        DEBUGF("Won't update CRC for byte %d\n", i);
      }

      DEBUGF("READ_BIN16: CRC after byte %d is 0x%04x\n", i, crc);
    }
  }

  DEBUG_DUMPHDR(hdr);

  DEBUGF("READ_BIN16: All read; check CRC (Received: 0x%04x; Computed: 0x%04x)\n", CRC(hdr->crc1, hdr->crc2), crc);
  return check_header_crc16(hdr, crc);
}

ZRESULT await_header(ZHDR *hdr) {
  uint16_t result;

  if (await_zdle() == OK) {
    DEBUGF("Got ZDLE, awaiting type...\n");
    char frame_type = recv();

    switch (frame_type) {
    case ZHEX:
      DEBUGF("Reading HEX header\n");
      result = read_hex_header(hdr);

      if (result == OK) {
        DEBUGF("Got valid header :)\n");
        return read_crlf();
      } else {
        DEBUGF("Didn't get valid header...\n");
        return result;
      }
    case ZBIN16:
      DEBUGF("Reading BIN16 header\n");
      result = read_binary16_header(hdr);

      if (result == OK) {
        DEBUGF("Got valid header :)\n");
        return OK;
      } else {
        DEBUGF("Didn't get valid header...\n");
        return result;
      }
    case ZBIN32:
      DEBUGF("Cannot handle frame type '%c' :(\n", frame_type);
      return UNSUPPORTED;
    default:
      DEBUGF("Got bad frame type '%c'\n", frame_type);
      return BAD_FRAME_TYPE;
    }
  } else {
    return CLOSED;
  }
}

ZRESULT send_sz(uint8_t *data) {
  register uint16_t result;

  while (*data) {
    result = send(*data++);

    if (IS_ERROR(result)) {
      return result;
    }
  }

  return OK;
}

