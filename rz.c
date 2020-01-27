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
  .type = 0x01,
  .f0 = 0x00,
  .f1 = 0x00,
  .f2 = 0x00,
  .f3 = 0x00
};

ZRESULT recv() {
  register int result = fgetc(com);

  if (result == EOF) {
    return GOT_EOF;
  } else {
    return result;
  }
}

ZRESULT send(uint8_t chr) {
  register int result = putc((char)chr, com);

  if (result == EOF) {
    return GOT_EOF;
  } else {
    return OK;
  }
}

int main() {
  uint8_t rzr_buf[4];
  uint8_t zrinit_buf[HEX_HDR_STR_LEN + 1];
  zrinit_buf[HEX_HDR_STR_LEN] = 0;
  ZHDR hdr;

  // Set up zrinit for later use...
  calc_hdr_crc(&hdr_zrinit);
  if (IS_ERROR(to_hex_header(&hdr_zrinit, zrinit_buf, HEX_HDR_STR_LEN))) {
    printf("Failed to make zrinit; Bailing...\n");
    return 1;
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
            } else if(result == GOT_EOF) {
              printf("Got EOF; Breaking loop...");
              goto cleanup;
            }

            continue;

          case ZFILE:
            DEBUGF("Is ZFILE\n");


          default:
            DEBUGF("Isn't ZRQINIT - is 0x%02x instead :S\n", hdr.type);
          }

          break;
        case BAD_CRC:
          DEBUGF("Didn't get valid header - CRC Check failed\n");
        default:
          DEBUGF("Didn't get valid header - result is 0x%04x\n", result);
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


