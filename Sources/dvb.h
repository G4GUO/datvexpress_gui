
#ifndef DVB_H_
#define DVB_H_

#include <limits.h>
#include "express.h"
#include "dvb_gen.h"
#include "dvb_config.h"
#include "dvb_buffer.h"

#define TAPS 12
#define ITP 5
#define BLOCKS 80
#define TMP_CSIZE 200

#define DVB_FILTER_BLK_LEN (TAPS*BLOCKS)

//#define _USE_SW_CODECS

#define S_VERSION    "2.02"

#define S_DVB_S    "DVB-S"
#define S_DVB_S2   "DVB-S2"
#define S_DVB_C    "DVB-C"
#define S_DVB_T    "DVB-T"
#define S_DVB_T2   "DVB-T2"
#define S_EXPRESS_AUTO "Express Auto"
#define S_EXPRESS_16 "Express 16 bit"
#define S_EXPRESS_8  "Express 8 bit"
#define S_EXPRESS_TS "Express TS"
#define S_EXPRESS_UDP "Express UDP"
#define S_USRP2    "USRP2"
#define S_NONE     "NONE"
#define S_DIGILITE "Digilite"
#define S_UDP_TS   "UDP TS"
#define S_UDP_PS   "UDP PS"
#define S_PVRXXX   "PVRXXX"
#define S_PVRHD    "PVRHD"
#define S_FIREWIRE "FIREWIRE"
#define S_FEC_1_2  "1/2"
#define S_FEC_2_3  "2/3"
#define S_FEC_3_4  "3/4"
#define S_FEC_5_6  "5/6"
#define S_FEC_7_8  "7/8"
#define S_FEC_1_4  "1/4"
#define S_FEC_1_3  "1/3"
#define S_FEC_2_5  "2/5"
#define S_FEC_3_5  "3/5"
#define S_FEC_4_5  "4/5"
#define S_FEC_5_6  "5/6"
#define S_FEC_8_9  "8/9"
#define S_FEC_9_10 "9/10"
#define S_M_QPSK   "QPSK"
#define S_M_8PSK   "8PSK"
#define S_M_16APSK "16APSK"
#define S_M_16QAM  "16QAM"
#define S_M_32APSK "32APSK"
#define S_M_64QAM  "64QAM"
#define S_YES      "Yes"
#define S_NO       "No"
#define S_RO_0_35  "0.35"
#define S_RO_0_25  "0.25"
#define S_RO_0_20  "0.20"
#define S_FFT_2K   "2K"
#define S_FFT_8K   "8K"
#define S_GP_1_4   "1/4"
#define S_GP_1_8   "1/8"
#define S_GP_1_16  "1/16"
#define S_GP_1_32  "1/32"
#define S_CH_8MHZ  "8MHz"
#define S_CH_7MHZ  "7MHz"
#define S_CH_6MHZ  "6MHz"
#define S_CH_4MHZ  "4MHz"
#define S_CH_3MHZ  "3MHz"
#define S_CH_2MHZ  "2MHz"
#define S_FM_NORMAL "Normal"
#define S_FM_SHORT  "Short"
#define S_ALPHA_NH  "Alpha NH"
#define S_ALPHA_1   "Alpha 1"
#define S_ALPHA_2   "Alpha 2"
#define S_ALPHA_4   "Alpha 4"


// Final buffering queue. these are blocks of samples
#define TX_BUFFER_LENGTH 512
// Delays to get the HAUPAUGE to work
#define PVR_PCR_DELAY   0.08

// PTT status
#define DVB_RECEIVING           0
#define DVB_TRANSMITTING        1
#define DVB_CALIBRATING         2
#define DVB_CALIBRATING_OFFSET  0
#define DVB_CALIBRATING_GAIN    1

#define MB_BUFFER_SIZE 65000

// DVB Module
void dvb_get_sem( void );
void dvb_release_sem( void );
void dvb_refresh_epg( void );
int dvb_is_system_running( void );

// RS functions
void dvb_rs_init( void );
void dvb_rs_encode( uchar *inout );

// Interleave
void dvb_interleave_init( void );
void dvb_convolutional_interleave( uchar *inout );

// Convolutional coder
void dvb_conv_ctl( sys_config *info );
void dvb_conv_init( void );
int  dvb_conv_encode_frame( uchar *in, uchar *out, int len );


// Modulators
void dvbs_modulate_init( void );
void dvb_s_encode_and_modulate( uchar *tp, uchar *dibit );
void dvbt_modulate( fftw_complex *in, int length );
void dvbs2_modulate( scmplx *in, int length );
void dvb_modulate_init(void);

// tx_frame
void dvb_tx_frame_ctl( sys_config *info );
void dvb_tx_frame_init(void);
void dvb_tx_frame( uchar *tp );
void dvb_tx_encode_and_transmit_tp_raw( uchar *tp );
void dvb_tx_encode_and_transmit_tp( uchar *tp );

// dvbs_receive.cpp
void dvbs_receive( short int *samples, int length );

// Teletext
void dvb_send_teletext_file( void );
void dvb_teletext_init( void );
void tt_text_encode( void );

// TS parser
void dvb_ts_if_init(void);
void dvb_ts_if( uchar *b );

// Final TX queue
int     final_tx_queue_size( void );
int     final_tx_queue_percentage_unprotected( void );
void    write_final_tx_queue( scmplx *samples, int length );
void    write_final_tx_queue_ts( uchar* tp );
void    write_final_tx_queue_udp( uchar* tp );
dvb_buffer *read_final_tx_queue(void);
void    create_final_tx_queue(void);
double  final_txq_time_delay( void );

// TX flow (transport queue functions)
int ts_single_stream( void );
int ts_multi_stream( void );
int ts_multi_pad( void );
void ts_enable_si( bool status );
void increment_null_count( void );
int flow_read_null_count( void );
void ts_write_transport_queue( uchar *tp );
void ts_write_transport_queue_elementary( uchar *tp );
uchar *ts_read_transport_queue( void );
void ts_init_transport_flow( void );
int ts_queue_percentage(void);

// DVB
double dvb_get_tx_interpolator_rate( void );

// TX status
int dvb_get_major_txrx_status( void );
int dvb_get_minor_txrx_status( void );
void dvb_set_major_txrx_status( int status );
void dvb_set_minor_txrx_status( int status );
void dvb_set_testcard( int status );
void dvb_block_rx_check( void );

// Equipment
int  eq_initialise( void );
void eq_change_frequency( double freq );
void eq_transmit( void );
void eq_receive( void );

// Elementary stream
int pes_find_payload( uchar *b );
void pes_video_el_to_pes( uchar *b, int length, int64_t pts, int64_t dts );
void pes_audio_el_to_pes( uchar *b, int length, int64_t pts, int64_t dts );

// Firewire
#define DV_PAL_FRAME_LENGTH 144000
int init_firewire(void);
int firewire_read_dv_frame( uchar *b, int len );

// Log functions will need major update
const char *logger_get_text( void );
void logger( char const *text );
void loggerf( const char *fmt, ... );
void loggerf( char *fmt, ... );
int logger_updated( void );
void logger_released( void );
void increment_display_log_index(void);
void display_logger_init( void );

// Error checking routine
#define dvberrorchk( a ) if(a<0) return-1

// Various file paths

const char *dvb_firmware_get_path( const char *filename, char *pathname );

// Transport packet logging routines
void tp_file_logger_log( uchar *b, int length);// Transport stream pointer and length
void tp_file_logger_start(void);
void tp_file_logger_stop(void);
void tp_file_logger_init(void);

// PES process
void pes_write_from_capture( int len );
void pes_write_from_memory( uchar *b, int len );
void pes_read( uchar *b, int len );
int  pes_process( void );
int  pes_get_length( void );
void pes_reset( void );
int  pes_get_length(void);
uchar *pes_get_packet(void);

#endif
