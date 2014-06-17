//
// Configuration
//
#ifndef DVB_CONFIG_H
#define DVB_CONFIG_H

#include "DVB-T/dvb_t.h"
#include "DVB-S2/DVBS2.h"

typedef struct{
    int  event_duration;
    char event_title[2048];
    char event_text[2048];
}epg_event;

typedef struct{
    int fec;
}DVBSFormat;

#define MAX_CAPTURE_LIST_ITEMS 10

typedef struct{
    int items;
    char item[MAX_CAPTURE_LIST_ITEMS][80];
}CaptureList;

// Capture device configuration
typedef enum{CAP_AUTO, CAP_720X576X25, CAP_720X480X30 }CapVideoFormat;

typedef struct{
    CapVideoFormat video_format;
}CaptureFormat;

#define N_SR_MEMS 12

enum{ MODE_DVBS=0, MODE_DVBS2=1, MODE_DVBC = 2, MODE_DVBT=3, MODE_DVBT2=4, MODE_UDP=5 };
enum{ FEC_RATE_12 = 0, FEC_RATE_23 = 1, FEC_RATE_34 = 2, FEC_RATE_56 = 3, FEC_RATE_78 = 4};
enum{ DVB_PROGRAM = 0, DVB_TRANSPORT = 1, DVB_DV = 2, DVB_YUV = 3};
enum{ DVB_V4L = 0, DVB_UDP_TS = 2, DVB_FIREWIRE = 3, DVB_NONE = 4 };
enum{ CODEC_MPEG2 = 0, CODEC_MPEG4 = 1 };
enum{ HW_EXPRESS_AUTO = 0, HW_EXPRESS_16 = 1, HW_EXPRESS_8 = 2, HW_EXPRESS_TS = 3, HW_EXPRESS_UDP = 4 };
enum{ SAMPLEMODE_8_BITS = 0, SAMPLEMODE_16_BITS = 1 };
enum{ DVB_C_16QAM_MODE = 0, DVB_C_32QAM_MODE, DVB_C_64QAM_MODE, DVB_C_128QAM_MODE, DVB_C_256QAM_MODE};

typedef struct {
    char version[80];
    int dvb_mode;
    DVBSFormat      dvbs_fmt;
    DVBTFormat      dvbt_fmt;
    DVB2FrameFormat dvbs2_fmt;
    CaptureFormat   cap_format;
    int    sr_mem_nr;
    int    sr_mem[N_SR_MEMS];
    double tx_frequency;
    float  tx_level;
    u_int16_t video_pid;
    u_int16_t audio_pid;
    u_int16_t pcr_pid;
    u_int16_t pmt_pid;
    u_int16_t nit_pid;
    u_int16_t network_id;
    u_int16_t stream_id;
    u_int16_t service_id;
    u_int16_t program_nr;
    int  ebu_data_enabled;
    int  ebu_data_pid;
    int  capture_stream_type;
    int  capture_device_type;
    int  capture_device_input;
    int  video_codec_type;
    char ebu_teletext_file_name[2048];
    char capture_device_name[256];
    char capture_device_type_name[256];
    char service_provider_name[256];
    char service_name[256];
    epg_event event;
    int video_bitrate;
    int tx_hardware;
    char tx_hardware_type_name[256];
    char server_ip_address[256];
    int  server_socket_number;
    int i_chan_dc_offset;
    int q_chan_dc_offset;
    double i_chan_gain;
    double q_chan_gain;
    double chan_raw_bitrate;
}sys_config;

typedef struct {
    unsigned long crc;
}Crc;

typedef struct {
    sys_config cfg;
    Crc        crc;
}SysConfigRecord;


void dvb_config_save( sys_config *cfg );
void dvb_config_save_and_update( sys_config *cfg );
void dvb_config_save_to_local_memory( sys_config *cfg );
void dvb_config_get( sys_config *cfg );
const sys_config *dvb_config_get( void );
void dvb_config_get_disk( sys_config *cfg );
void dvb_config_save_to_disk(sys_config *cfg);
void dvb_config_retrieve_from_disk(sys_config *cfg);
void dvb_config_retrieve_from_disk(void);
const char *dvb_config_get_path( const char *filename );

#endif
