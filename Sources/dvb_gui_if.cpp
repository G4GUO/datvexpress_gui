//
// The interface between GUI and DVB
//
#include <syscall.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <memory.h>
#include "dvb_gen.h"
#include "dvb_gui_if.h"
#include "dvb.h"
#include "dvb_config.h"
#include "dvb_capture_ctl.h"
#include "DVB-T/dvb_t.h"
#include "dvb_s2_if.h"
#include "mp_tp.h"
#include "tx_hardware.h"

#define PID_MASK(a) (a&0xFFF)

void dvb_get_config( sys_config *cfg )
{
    dvb_config_get( cfg );
}

int dvb_set_dvbs_fec_rate( int rate )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.dvbs_fmt.fec = rate;
    dvb_config_save( &info );
    return 0;
}
int dvb_set_dvbs_symbol_rate( int mem_nr, int rate )
{
    sys_config info;

    dvb_config_retrieve_from_disk(&info);
    if( rate == 0 ) rate = 4000000;//Default value
    if((mem_nr > 11)||(mem_nr<0)) mem_nr = 11;//Defensive
    info.sr_mem_nr = mem_nr;
    info.sr_mem[mem_nr] = rate;
    dvb_config_save( &info );
    return 0;
}
void dvb_set_dvbt_params( sys_config *cfg )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.dvbt_fmt = cfg->dvbt_fmt;
    dvb_config_save( &info );
}

void dvb_set_tx_freq( double freq )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.tx_frequency = freq;
    dvb_config_save_and_update( &info );
    hw_freq( freq );
}

void dvb_set_tx_lvl( float lvl )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.tx_level = lvl;
    dvb_config_save_and_update( &info );
    hw_level( lvl );
}

void dvb_set_service_provider_name( const char *txt )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    strcpy( info.service_provider_name, txt );
    dvb_config_save( &info );
    dvb_si_refresh();
}

void dvb_set_service_name( const char *txt )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    strcpy( info.service_name, txt );
    dvb_config_save( &info );
    dvb_si_refresh();
}
//
// Type of capture device, only devices that are V4L compliant will be shown.
//
void dvb_set_video_capture_device( const char *txt )
{
    sys_config info;

    dvb_config_retrieve_from_disk(&info);

    int type = DVB_V4L;

    if(strcmp(txt,S_UDP_TS)   == 0 )
    {
        type = DVB_UDP_TS;
        info.cap_dev_type = CAP_DEV_TYPE_UDP;
    }
    if(strcmp(txt,S_FIREWIRE) == 0 )
    {
        type = DVB_FIREWIRE;
        info.cap_dev_type = CAP_DEV_TYPE_FIREWIRE;
    }
    if(strcmp(txt,S_NONE)   == 0 )
    {
        type = DVB_NONE;
        info.cap_dev_type = CAP_DEV_TYPE_NONE;
    }

    if( type == DVB_V4L )
    {
        info.cap_dev_type = get_device_type_from_name( txt );
    }

    strcpy( info.video_capture_device_name, txt );
    info.video_capture_device_class = type;
    dvb_config_save( &info );
}
void dvb_set_audio_capture_device( const char *txt )
{
    sys_config info;
    int type = DVB_PCM;

    dvb_config_retrieve_from_disk(&info);
    strcpy( info.audio_capture_device_name, txt );
    info.audio_capture_device_class = DVB_PCM;
    dvb_config_save( &info );
}
//
// This can only be DATV-Express now
//
void dvb_set_tx_hardware_type( int type, const char *name )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.tx_hardware = type;
    strcpy(info.tx_hardware_type_name,name);
    dvb_config_save( &info );
}
//
// Input port to use on video device
//
void dvb_set_video_capture_device_input( int input, const char *name )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.video_capture_device_input = input;
    strcpy(info.video_capture_input_name, name );
    dvb_config_save( &info );
}
int dvb_set_dvb_mode( int mode )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.dvb_mode = mode;
    dvb_config_save( &info );
    return 0;
}
//
// IDs
//
void dvb_set_PmtPid( u_int16_t pid )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.pmt_pid = PID_MASK(pid);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_VideoPid( u_int16_t pid )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.video_pid = PID_MASK(pid);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_AudioPid( u_int16_t pid )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.audio_pid = PID_MASK(pid);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_PcrPid( u_int16_t pid )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.pcr_pid = PID_MASK(pid);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_NitPid( u_int16_t pid )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.nit_pid = PID_MASK(pid);
    dvb_config_save( &info );
    dvb_si_refresh();
}

void dvb_set_NetworkId( u_int16_t id )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.network_id = PID_MASK(id);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_StreamId( u_int16_t id )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.stream_id = PID_MASK(id);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_ServiceId( u_int16_t id )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.service_id = PID_MASK(id);
    info.program_nr = PID_MASK(id);
    dvb_config_save( &info );
    dvb_si_refresh();
}
void dvb_set_ProgramNr( u_int16_t id )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.program_nr = PID_MASK(id);
    info.service_id = PID_MASK(id);
    dvb_config_save( &info );
    dvb_si_refresh();
}

void dvb_set_epg_event_duration( int duration )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.event.event_duration = duration;
    dvb_config_save( &info );
    dvb_refresh_epg();
}

void dvb_set_epg_event_title( const char *txt )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    strcpy( info.event.event_title, txt );
    dvb_config_save( &info );
    dvb_refresh_epg();
}

void dvb_set_epg_event_text( const char *txt )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    strcpy( info.event.event_text, txt );
    dvb_config_save( &info );
    dvb_refresh_epg();
}
int dvb_set_s2_configuration( DVB2FrameFormat *f )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.dvbs2_fmt = *f;

    if(dvb_s2_re_configure( f ) == 0 )
    {
        dvb_config_save( &info );
        return 0;
    }
    return -1;
}
void dvb_set_server_ip_address( const char *text )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    strcpy( info.server_ip_address, text );
    dvb_config_save( &info );
}

void dvb_set_server_socket_number( const char *text )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    info.server_socket_number = atoi( text );
    dvb_config_save( &info );
}
//
// Calibration routine
//
void dvb_calibration(bool status, int mode)
{
    if( status == true )
    {
        dvb_set_major_txrx_status( DVB_CALIBRATING );
        dvb_set_minor_txrx_status( mode );
        express_set_calibrate(true);
    }
    else
    {
        express_set_calibrate(false);
    }
}

void dvb_i_chan_dac_offset_changed( int val )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    if( dvb_get_major_txrx_status() == DVB_CALIBRATING )
    {
        info.i_chan_dc_offset = val;
        dvb_config_save_and_update( &info );
        hw_load_calibration();
    }
}

void dvb_q_chan_dac_offset_changed( int val )
{
    sys_config info;
    dvb_config_retrieve_from_disk(&info);
    if( dvb_get_major_txrx_status() == DVB_CALIBRATING )
    {
        info.q_chan_dc_offset = val;
        dvb_config_save_and_update( &info );
        hw_load_calibration();
    }
}
void dvb_dac_offset_selected(void)
{
    dvb_set_minor_txrx_status( DVB_CALIBRATING_OFFSET );
}
