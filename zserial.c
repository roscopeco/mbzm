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

#include <stdio.h>
#include <string.h>
#include "zserial.h"
#include "zheaders.h"
#include "znumbers.h"
#include "xmodemcrc.h"

uint16_t read_crlf() {
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

uint16_t read_hex_byte() {
  int c1 = recv(), c2;

  if (c1 == EOF) {
    return GOT_EOF;
  } else {
    c2 = recv();
    if (c2 == EOF) {
      return GOT_EOF;
    } else {
      return hex_to_byte(c1, c2);
    }
  }
}

bool await(char *str, char *buf, int buf_size) {
  memset(buf, 0, 4);
  int ptr = 0;

  while(true) {
    int c = recv();

    if (c == EOF) {
      return false;
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
        return true;
      }
    }
  }
}

bool await_zdle() {
  while (true) {
    int c = recv();

    switch (c) {
    case EOF:
      DEBUGF("Got EOF :(\n");
      return false;
    case ZPAD:
    case ZPAD | 0200:
      DEBUGF("Got ZPAD...\n");
      continue;
    case ZDLE:
      DEBUGF("Got ZDLE\n");
      return true;
    default:
      DEBUGF("Got unknown (%08x)\n", c);
      return false;
    }
  }
}

uint16_t read_hex_header(ZHDR *hdr) {
  uint8_t *ptr = (uint8_t*)hdr;
  memset(hdr, 0xc0, sizeof(ZHDR));
  uint16_t crc = 0;

  // TODO maybe don't treat header as a stream of bytes, which would remove
  //      the need to have the all-byte layout in ZHDR struct...
  for (int i = 0; i < ZHDR_SIZE; i++) {
    // TODO use read_hex_byte here...
    uint16_t c1 = recv();

    if (IS_ERROR(c1)) {
      DEBUGF("READ_HEX: Character %d/1 is error: 0x%04x\n", i, c1);
      return c1;
    } else if (c1 == EOF) {
      DEBUGF("READ_HEX: Character %d/1 is EOF\n", i);
      return GOT_EOF;
    } else {
      DEBUGF("READ_HEX: Character %d/1 is good: 0x%02x\n", i, c1);
      uint16_t c2 = recv();

      if (IS_ERROR(c2)) {
        DEBUGF("READ_HEX: Character %d/2 is error: 0x%04x\n", i, c2);
        return c2;
      } else if (c2 == EOF) {
        DEBUGF("READ_HEX: Character %d/2 is EOF\n", i);
        return GOT_EOF;
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
            crc = updcrc(b, crc);
          } else {
            DEBUGF("Won't update CRC for byte %d\n", i);
          }

          DEBUGF("READ_HEX: CRC after byte %d is 0x%04x\n", i, crc);
        }
      }
    }
  }

  DEBUG_DUMPHDR(hdr);

  DEBUGF("READ_HEX: All read; check CRC (0x%04x)\n", CRC(hdr->crc1, hdr->crc2));
  if (CRC(hdr->crc1, hdr->crc2)  == crc) {
    return OK;
  } else {
    return BAD_CRC;
  }
}

uint16_t await_header(ZHDR *hdr) {
  uint16_t result;

  if (await_zdle()) {
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
    case ZBIN32:
      DEBUGF("Cannot handle frame type '%c' :(\n", frame_type);
      return UNSUPPORTED;
    default:
      DEBUGF("Got bad frame type '%c'\n", frame_type);
      return BAD_FRAME_TYPE;
    }
  } else {
    return GOT_EOF;
  }
}


bool await_zrqinit() {
  ZHDR hdr;

  uint16_t result = await_header(&hdr);

  switch (result) {
  case OK:
    DEBUGF("Got valid header\n");
    if (hdr.type == ZRQINIT) {
      DEBUGF("Is ZRQINIT!\n");
      return true;
    } else {
      DEBUGF("Isn't ZRQINIT - is 0x%02x instead :S\n", hdr.type);
      return false;
    }
  case BAD_CRC:
    DEBUGF("Didn't get valid header - CRC Check failed\n");
  default:
    DEBUGF("Didn't get valid header - result is 0x%04x\n", result);
    return false;
  }

  return true;

  char buf[4];

  if (await_zdle()) {
    DEBUGF("Got ZDLE, waiting for init...\n");

    if (await("B00", (char*)buf, 4)) {
      DEBUGF("Got ZRQINIT :)\n");
      return true;
    } else {
      DEBUGF("Didn't get ZRQINIT - Must've gotten EOF\n");
      return false;
    }




    int c = recv();

    switch (c) {
    case EOF:
      DEBUGF("Got EOF :(\n");
      return false;
    case ZRQINIT:
      DEBUGF("Got ZRQINIT! :)\n");
      return true;
    default:
      DEBUGF("Got unknown (%08x)\n", c);
      return false;
    }
  } else {
    DEBUGF("No ZDLE before ZRQINIT, bailing...\n");
  }
}

uint16_t send_sz(uint8_t *data) {
  while (*data) {
    if (send(*data++) == EOF) {
      return GOT_EOF;
    }
  }

  return OK;
}

