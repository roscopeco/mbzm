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
 * Example usage of Zmodem implementation
 * ------------------------------------------------------------
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "zmodem.h"

static FILE *com;

static ZHDR hdr_zrinit = {
  .type = ZRINIT,
  .flags = {
      .f0 = 0,
      .f1 = 0
  },
  .position = {
      .p0 = 0x00,
      .p1 = 0x00
  }
};

static ZHDR hdr_znak = {
    .type = ZNAK,
    .flags = {
        .f0 = 0,
        .f1 = 0,
        .f2 = 0,
        .f3 = 0
    }
};

static ZHDR hdr_zrpos = {
    .type = ZRPOS,
    .position = {
        .p0 = 0,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0
    }
};

static ZHDR hdr_zabort = {
    .type = ZABORT,
    .position = {
        .p0 = 0,
        .p1 = 0,
        .p2 = 0,
        .p3 = 0
    }
};

static uint8_t zrinit_buf[HEX_HDR_STR_LEN + 1];
static uint8_t znak_buf[HEX_HDR_STR_LEN + 1];
static uint8_t zrpos_buf[HEX_HDR_STR_LEN + 1];
static uint8_t zabort_buf[HEX_HDR_STR_LEN + 1];

static ZRESULT init_hdr_buf(ZHDR *hdr, uint8_t *buf) {
  buf[HEX_HDR_STR_LEN] = 0;
  calc_hdr_crc(hdr);
  return to_hex_header(hdr, buf, HEX_HDR_STR_LEN);
}


ZRESULT recv() {
  register int result = fgetc(com);

  if (result == EOF) {
    return CLOSED;
  } else {
    return result;
  }
}

ZRESULT send(uint8_t chr) {
  register int result = putc((char)chr, com);

  if (result == EOF) {
    return CLOSED;
  } else {
    return OK;
  }
}

static ZRESULT zabort() {
  DEBUGF("Aborting transfer...\n");

  if (send_sz(zabort_buf) != OK) {
    DEBUGF("Send ZABORT failed!\n");
  }

  return OK;
}

static ZRESULT znak() {
  DEBUGF("Sending ZNAK...\n");

  ZRESULT result = send_sz(zabort_buf);
  if (result != OK) {
    DEBUGF("Send ZNAK failed!\n");
  }

  return result;
}

int main() {
  uint8_t rzr_buf[4];
  ZHDR hdr;

  // Set up static header buffers for later use...
  if (IS_ERROR(init_hdr_buf(&hdr_zrinit, zrinit_buf))) {
    printf("Failed to initialize ZRINIT buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_znak, znak_buf))) {
    printf("Failed to initialize ZNAK buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_zrpos, zrpos_buf))) {
    printf("Failed to initialize ZRPOS buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_zabort, zabort_buf))) {
    printf("Failed to initialize ZABORT buffer; Bailing...");
    return 3;
  }

  com = fopen("/dev/pts/2", "a+");

  if (com != NULL) {
    printf("Opened port just fine\n");

    if (await("rz\r", (char*)rzr_buf, 4) == OK) {
      printf("Got rzr...\n");

      while (true) {
        uint16_t result = await_header(&hdr);

        switch (result) {
        case OK:
          DEBUGF("Got valid header\n");

          switch (hdr.type) {
          case ZRQINIT:
            DEBUGF("Is ZRQINIT\n");
            send(ZPAD);
            send(ZPAD);
            send(ZDLE);

            result = send_sz(zrinit_buf);

            if (result == OK) {
              printf("Send ZRINIT was OK\n");
            } else if(result == CLOSED) {
              printf("Got EOF; Breaking loop...");
              goto cleanup;
            }

            continue;

          case ZFILE:
            DEBUGF("Is ZFILE\n");

            switch (hdr.flags.f0) {
            case ZCBIN:
              DEBUGF("--> Binary receive\n");
              break;
            case ZCNL:
              DEBUGF("--> ASCII Receive; Fix newlines (IGNORED - NOT SUPPORTED)\n");
              break;
            case ZCRESUM:
              DEBUGF("--> Resume interrupted transfer (IGNORED - NOT SUPPORTED)\n");
              break;
            default:
              DEBUGF("--> Invalid conversion flag!\n");
              if (znak() != OK) {
                DEBUGF("Failed to NAK; Bailing...\n");
                zabort();
                goto cleanup;
              }
            }

            // TODO handle this :)

            continue;

          default:
            DEBUGF("Completely unhandled header - is 0x%02x :S\n", hdr.type);
            continue;
          }

          break;
        case BAD_CRC:
          DEBUGF("Didn't get valid header - CRC Check failed\n");
          // TODO send NAK
          continue;
        default:
          DEBUGF("Didn't get valid header - result is 0x%04x\n", result);
          // TODO what?
          return false;
        }
      }
    }

    cleanup:
    
    if (fclose(com)) {
      printf("Failed to close file\n");
      return 1;
    } else {
      return 0;
    }

  } else {
    printf("Unable to open port\n");
    return 2;
  }
}


