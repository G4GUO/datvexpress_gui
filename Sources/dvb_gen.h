#ifndef GEN_H
#define GEN_H

#include "dvb_buffer.h"

typedef unsigned char uchar;
typedef unsigned int  uint;

#define MP_T_SYNC 0x47
#define DVBS_T_ISYNC 0xB8
#define DVBS_T_PAYLOAD_LEN 187
#define MP_T_FRAME_LEN 188
#define DVBS_RS_BLOCK_DATA 239
#define DVBS_RS_BLOCK_PARITY 16
#define DVBS_RS_BLOCK (DVBS_RS_BLOCK_DATA+DVBS_RS_BLOCK_PARITY)
#define DVBS_PARITY_LEN 16
#define DVBS_T_CODED_FRAME_LEN (MP_T_FRAME_LEN+DVBS_PARITY_LEN)
#define DVBS_T_FRAMES_IN_BLOCK 8
#define DVBS_T_BIT_WIDTH 8
#define DVBS_T_SCRAM_SEQ_LENGTH 1503

typedef struct{
        int length;
        uchar *b;
}Buffer;

//#define __TEST__

#define CRC_32_LEN 4

void dvb_si_init( void );
void dvb_si_refresh( void );
int crc32_add( uchar *b, int len );
unsigned long dvb_crc32_calc( uchar *b, int len );
void rd_pes( uchar *b, int c );
int tdt_time_fmt( uchar *b );

// gen.c
void print_tp( uchar *b );

// dvb.c
void dvb_encode_init( void );
int dvb_encode_frame( uchar *tp, uchar *dibit  );
void dvb_reset_scrambler( void );
void dvb_scramble_transport_packet( uchar *in, uchar *out );

// tx_flow.c
void tx_flow_init( void );

// dvb_timer.c
void *timer_proc( void *arg);

// capture.c
int cap_parse_instream( void );
int dvb_cap_init( void );
void cap_purge(void);

// dvb_viewport.cpp
void vt_init( void );
void vt_queue_tp( uchar *b  );

// udp
void *udp_proc( void *args );
int udp_send_tp( dvb_buffer *b );
// used to read from the UDP socket
int udp_read( uchar *b, int length );
uchar *udp_get_transport_packet(void);
// Initialise UDP sender
int udp_tx_init( void );
// Initialise UDP receiver
int udp_rx_init( void );

#endif
