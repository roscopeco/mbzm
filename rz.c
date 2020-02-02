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

#include "rzinit.c"

static FILE *com;
static FILE *out;

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

static ZRESULT send_sz_hex_hdr(uint8_t *buf) {
  send(ZPAD);
  send(ZPAD);
  send(ZDLE);

  return send_sz(buf);
}

int main() {
  uint8_t rzr_buf[4];
  uint8_t data_buf[DATA_BUF_LEN];
  uint16_t count;
  uint32_t received_data_size = 0;
  ZHDR hdr;

  // Set up static header buffers for later use...
  if (IS_ERROR(init_hdr_buf(&hdr_zrinit, zrinit_buf))) {
    FPRINTF(stderr, "Failed to initialize ZRINIT buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_znak, znak_buf))) {
    FPRINTF(stderr, "Failed to initialize ZNAK buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_zrpos, zrpos_buf))) {
    FPRINTF(stderr, "Failed to initialize ZRPOS buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_zabort, zabort_buf))) {
    FPRINTF(stderr, "Failed to initialize ZABORT buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_zack, zack_buf))) {
    FPRINTF(stderr, "Failed to initialize ZACK buffer; Bailing...");
    return 3;
  }
  if (IS_ERROR(init_hdr_buf(&hdr_zfin, zfin_buf))) {
    FPRINTF(stderr, "Failed to initialize ZACK buffer; Bailing...");
    return 3;
  }

  com = fopen("/dev/pts/2", "a+");

  if (com != NULL) {
    DEBUGF("Opened port just fine\n");

    PRINTF("rosco_m68k ZMODEM receive example v0.01 - Awaiting remote transfer initiation...\n");

    if (await("rz\r", (char*)rzr_buf, 4) == OK) {
      DEBUGF("Got rzr...\n");

      while (true) {
        uint16_t result = await_header(&hdr);

        switch (result) {
        case OK:
          DEBUGF("Got valid header\n");

          switch (hdr.type) {
          case ZRQINIT:
          case ZEOF:
            DEBUGF("Is ZRQINIT or ZEOF\n");

            result = send_sz_hex_hdr(zrinit_buf);

            if (result == OK) {
              DEBUGF("Send ZRINIT was OK\n");
            } else if(result == CLOSED) {
              FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
              goto cleanup;
            }

            continue;

          case ZFIN:
            DEBUGF("Is ZFIN\n");

            result = send_sz_hex_hdr(zfin_buf);

            if (result == OK) {
              DEBUGF("Send ZFIN was OK\n");
            } else if(result == CLOSED) {
              FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
            }

            PRINTF("Transfer complete; Received %0d byte(s)\n", received_data_size);
            goto cleanup;

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
              FPRINTF(stderr, "WARN: Invalid conversion flag [0x%02x] (IGNORED - Assuming Binary)\n", hdr.flags.f0);
            }

            count = DATA_BUF_LEN;
            result = read_data_block(data_buf, &count);
            DEBUGF("Result of data block read is [0x%04x] (got %d character(s))\n", result, count);

            if (!IS_ERROR(result)) {
              PRINTF("Receiving file: '%s'\n", data_buf);

              out = fopen((char*)data_buf, "wb");
              if (out == NULL) {
                FPRINTF(stderr, "Error opening file for output; Bailing...\n");
                goto cleanup;
              }

              result = send_sz_hex_hdr(zrpos_buf);

              if (result == OK) {
                DEBUGF("Send ZRPOS was OK\n");
              } else if(result == CLOSED) {
                FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                goto cleanup;
              }
            }

            // TODO care about XON that will follow?

            continue;

          case ZDATA:
            DEBUGF("Is ZDATA\n");

            while (true) {
              count = DATA_BUF_LEN;
              result = read_data_block(data_buf, &count);
              DEBUGF("Result of data block read is [0x%04x] (got %d character(s))\n", result, count);

              received_data_size += (count - 1);
              if (out != NULL) {
                fwrite(data_buf, count - 1, 1, out);
              } else {
                FPRINTF(stderr, "Received data before open file; Bailing...\n");
                goto cleanup;
              }

              if (!IS_ERROR(result)) {
                DEBUGF("Received %d byte(s) of data\n", count);

                if (result == GOT_CRCE) {
                  // End of frame, header follows, no ZACK expected.
                  DEBUGF("Got CRCE; Frame done [NOACK]\n");
                  break;
                } else if (result == GOT_CRCG) {
                  // Frame continues, non-stop (another data packet follows)
                  DEBUGF("Got CRCG; Frame continues [NOACK]\n");
                  continue;
                } else if (result == GOT_CRCQ) {
                  // Frame continues, ACK required
                  DEBUGF("Got CRCQ; Frame continues [ACK]\n");

                  result = send_sz_hex_hdr(zack_buf);

                  if (result == OK) {
                    DEBUGF("Send ZACK was OK\n");
                  } else if(result == CLOSED) {
                    FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                    goto cleanup;
                  }

                  continue;
                } else if (result == GOT_CRCW) {
                  // End of frame, header follows, ZACK expected.
                  DEBUGF("Got CRCW; Frame done [ACK]\n");

                  result = send_sz_hex_hdr(zack_buf);

                  if (result == OK) {
                    DEBUGF("Send ZACK was OK\n");
                  } else if(result == CLOSED) {
                    FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                    goto cleanup;
                  }

                  break;
                }

              } else {
                DEBUGF("Error while receiving block: 0x%04x\n", result);

                result = send_sz_hex_hdr(znak_buf);

                if (result == OK) {
                  DEBUGF("Send ZNACK was OK\n");
                } else if(result == CLOSED) {
                  FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                  goto cleanup;
                }
              }
            }

            continue;

          default:
            FPRINTF(stderr, "WARN: Ignoring unknown header type 0x%02x\n", hdr.type);
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
    
    if (out != NULL && fclose(out)) {
      FPRINTF(stderr, "Failed to close output file\n");
    }
    if (com != NULL && fclose(com)) {
      FPRINTF(stderr, "Failed to close serial port\n");
    }

  } else {
    PRINTF("Unable to open port\n");
    return 2;
  }
}


