#ifndef MAIN_H
#define MAIN_H

#define SOH 0x01
#define STX 0x02
#define EOT 0x04
#define ACK 0x06
#define NAK 0x15
#define CAN 0x18
#define CTRLZ 0x1A

#ifdef XMODEM_1K
/* 1024 for XModem 1k + 3 head chars + 2 crc */
#define XBUF_SIZE ( 1024 + 3 + 2 )
#else
/* 128 for XModem + 3 head chars + 2 crc */
#define XBUF_SIZE ( 128 + 3 + 2 )
#endif

#define DLY_1S 1000
#define MAXRETRANS 25

/* ERROR CODE */
#define ERR_OK 0
#define ERR_OPEN_PORT -1
#define ERR_OPEN_FILE -2
#define ERR_NO_BOOT -3
#define ERR_XMODEM -4
#define ERR_SDK_718 -5
#define ERR_ARG -6

/** End Config Section ************************************************************************************************/

#define MAX_BUFF_LEN 2000
#define MIN_BUFF_LEN 2000

#define TIME_TO_READ 2000

#define TIME_INBYTE 1000

#endif  // MAIN_H
