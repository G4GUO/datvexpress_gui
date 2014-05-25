//
// This module controls the capture card.
//
#ifndef DVB_CAPTURE_CTL_H
#define DVB_CAPTURE_CTL_H

#include "dvb_config.h"

#define PAL_WIDTH  704
#define PAL_HEIGHT 576
#define PAL_SOUND_CAPTURE_SIZE 1920

//#define PAL_WIDTH  640
//#define PAL_HEIGHT 480

//
// Video save buffer
//
typedef struct{
    uchar b[4096];
    int   l;
}VideoBuffer;

void dvb_cap_ctl( int fd );
int dvb_open_capture_device(void);
void dvb_close_capture_device(void);
void dvb_cap_input( int input );
void dvb_get_cap_sem(void);
void dvb_release_cap_sem(void);
void dvb_cap_check_for_update(void);
void dvb_cap_request_param_update( void );
void cap_video_present(void);
void cap_audio_present(void);
bool cap_check_video_present(void);
void cap_video_pes_to_ts( void );
void cap_audio_pes_to_ts( void );
void cap_pcr_to_ts( void );
bool cap_check_audio_present(void);
double get_raw_bitrate( void );
double get_bits_in_transport_packet( void );
void cap_rd_bytes( uchar *b, int len );
void populate_video_capture_list( CaptureList *list );
void populate_audio_capture_list( CaptureList *list );
int open_named_capture_device( const char *name );
int calculate_video_bitrate(void);

#endif
