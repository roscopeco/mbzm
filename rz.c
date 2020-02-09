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
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/select.h>
#include <errno.h>
#include "zmodem.h"

#include "rzinit.c"

//static FILE *com;
static int com;
static FILE *out;

/*
 * Implementation-defined receive character function.
 */
ZRESULT zm_recv() {
  time_t start, now;
  uint8_t buf;

  time(&start);

  while (true) {
    time(&now);

    if (now - start > 5) {
      DEBUGF("ZM_RECV: TIMEOUT");
      return TIMEOUT;
    } else {
      if (read(com, &buf, 1) == 1) {
        int rnd = rand();

        if (rnd < RAND_MAX / 1000) {
          return rnd % 255;
        } else {
          return buf;
        }
      } else {
        continue;
      }
    }
  }


//  fd_set set;
//  struct timeval timeout;
//  uint8_t buf = 0;
//
//  FD_ZERO(&set);
//  FD_SET(com, &set);
//
//  timeout.tv_sec = 5;
//  timeout.tv_usec = 0;
//
//  int result = select(com + 1, &set, NULL, NULL, &timeout);
//
//  if (result == -1) {
//    DEBUGF("Select failed (-1)\n");
//    return CLOSED;
//  } else if (result == 0) {
//    DEBUGF("Select failed (timeout)\n");
//    return TIMEOUT;
//  } else {
//    if (read(com, &buf, 1) == 1) {
//      int rnd = rand();
//
//      if (rnd < RAND_MAX / 1000) {
//        return rnd % 255;
//      } else {
//        return buf;
//      }
//    } else {
//      DEBUGF("read failed!\n");
//      return CLOSED;
//    }
//  }



  //  register int result = fgetc(com);
//
//  if (result == EOF) {
//    return CLOSED;
//  } else {
//    int rnd = rand();
//    if (rnd < RAND_MAX / 1000) {
//      return rnd % 255;
//    } else {
//      return result;
//    }
//  }
}

/*
 * Implementation-defined send character function.
 */
ZRESULT zm_send(uint8_t chr) {
  int times = 0;

  while (true) {
    if (times > 1) {
      DEBUGF("ZM_SEND: Write timeout; Giving up...\n");
      return TIMEOUT;
    }

    register int result = write(com, &chr, 1);
    if (result != 1) {
      DEBUGF("ZM_SEND: Write failed (%s)!\n", strerror(errno));
      times++;
      sleep(1);
      continue;
    } else {
      return OK;
    }
  }

  if (fsync(com) != 0) {
    DEBUGF("WARN: FSYNC FAILED!\n");
  }
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

//  com = fopen("/dev/pts/2", "a+");
  com = open("/dev/pts/7", O_RDWR | O_NONBLOCK);

  if (com > 0) {
    DEBUGF("Opened port just fine\n");

    PRINTF("rosco_m68k ZMODEM receive example v0.01 - Awaiting remote transfer initiation...\n");

    if (zm_await("rz\r", (char*)rzr_buf, 4) == OK) {
      DEBUGF("Got rzr...\n");

      while (true) {
        uint16_t result = zm_await_header(&hdr);

        switch (result) {
        case TIMEOUT:
          DEBUGF("Timeout; Resend last header\n");
          zm_resend_last_header();
          continue;

        case OK:
          DEBUGF("Got valid header\n");

          switch (hdr.type) {
          case ZRQINIT:
          case ZEOF:
            DEBUGF("Is ZRQINIT or ZEOF\n");

            result = zm_send_hdr(&hdr_zrinit);

            if (result == OK) {
              DEBUGF("Send ZRINIT was OK\n");
            } else if(result == CLOSED) {
              FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
              goto cleanup;
            }

            continue;

          case ZFIN:
            DEBUGF("Is ZFIN\n");

            result = zm_send_hdr(&hdr_zfin);

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
            case 0:     /* no special treatment - default to ZCBIN */
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
            result = zm_read_data_block(data_buf, &count);
            DEBUGF("Result of data block read is [0x%04x] (got %d character(s))\n", result, count);

            if (!IS_ERROR(result) && result == GOT_CRCW) {
              PRINTF("Receiving file: '%s'\n", data_buf);

              out = fopen((char*)data_buf, "wb");
              if (out == NULL) {
                FPRINTF(stderr, "Error opening file for output; Bailing...\n");
                goto cleanup;
              }

              result = zm_send_hdr(&hdr_zrpos);

              if (result == OK) {
                DEBUGF("Send ZRPOS was OK\n");
              } else if(result == CLOSED) {
                FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                goto cleanup;
              }
            } else {
              DEBUGF("Error receiving ZFILE data block (0x%04x); Sending ZRINIT\n", result);
              zm_send_hdr(&hdr_zrinit);
            }

            // TODO care about XON that will follow?

            continue;

          case ZNAK:
            DEBUGF("FATAL: Don't know what to do with a ZNAK...");
            zm_send_hdr_pos32(ZABORT, 0);
            goto cleanup;

          case ZDATA:
            DEBUGF("Is ZDATA\n");

            while (true) {
              count = DATA_BUF_LEN;
              result = zm_read_data_block(data_buf, &count);
              DEBUGF("Result of data block read is [0x%04x] (got %d character(s))\n", result, count);

              if (!IS_ERROR(result)) {

                DEBUGF("Received %d byte(s) of data\n", count);
                received_data_size += (count - 1);
                if (out != NULL) {
                  fwrite(data_buf, count - 1, 1, out);
                } else {
                  FPRINTF(stderr, "Received data before open file; Bailing...\n");
                  goto cleanup;
                }

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

                  result = zm_send_hdr_pos32(ZACK, received_data_size);

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

                  result = zm_send_hdr_pos32(ZACK, received_data_size);

                  if (result == OK) {
                    DEBUGF("Send ZACK was OK\n");
                  } else if(result == CLOSED) {
                    FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                    goto cleanup;
                  }

                  break;
                }

              } else if (result == CORRUPTED || result == BAD_CRC || result == BAD_ESCAPE) {
                DEBUGF("Bad CRC while receiving block: 0x%04x\n", result);

                result = zm_send_hdr_pos32(ZRPOS, received_data_size);

                if (result == OK) {
                  DEBUGF("Send ZRPOS was OK\n");
                } else if(result == CLOSED) {
                  FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
                  goto cleanup;
                }

//                DEBUGF("NOTICE:: Continuing data block loop after error!!!\n");
//                continue;
                break;

              } else {
                DEBUGF("FATAL: Error 0x%04x while receiving block; Bailing...\n", result);
                goto cleanup;
              }
            }

            break;

          default:
            FPRINTF(stderr, "WARN: Ignoring unknown header type 0x%02x\n", hdr.type);
            DEBUGF("FATAL: Dying\n");
            goto cleanup;
            //continue;
          }

          break;

        case CANCELLED:
          DEBUGF("Cancelled by remote; Bailing...\n");
          goto cleanup;

        case BAD_CRC:
          DEBUGF("BADCRC\n");
          goto zrpos;
        case BAD_FRAME_TYPE:
          DEBUGF("BADFRAME\n");
          goto zrpos;
        default:
          zrpos:
          DEBUGF("Didn't get valid header - CRC Check failed\n");

          result = zm_send_hdr_pos32(ZRPOS, received_data_size);

          if (result == OK) {
            DEBUGF("Send ZRPOS was OK\n");
          } else if(result == CLOSED) {
            FPRINTF(stderr, "Connection closed prematurely; Bailing...\n");
            goto cleanup;
          }

          continue;
        }
      }
    }

    cleanup:
    
    DEBUGF("Cleaning up...\n");

    if (out != NULL && fclose(out)) {
      FPRINTF(stderr, "Failed to close output file\n");
    }
    if (com != 0 && close(com)) {
      FPRINTF(stderr, "Failed to close serial port\n");
    }

  } else {
    PRINTF("Unable to open port\n");
    return 2;
  }
}


