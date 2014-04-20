//
// The interface between GUI and DVB
//
#ifndef DVB_GUI_IF_H
#define DVB_GUI_IF_H

#include "dvb_config.h"

int dvb_start( void );
void dvb_stop( void );
void dvb_re_start( void );
void ndvb_set_config( sys_config *cfg );
void dvb_get_config( sys_config *cfg );
void dvb_set_reconfigure( int action );
int  dvb_set_dvbs_fec_rate(int rate );
int  dvb_set_dvbs_symbol_rate( int mem_nt, int rate );
void dvb_set_dvbt_params( sys_config *cfg );
void dvb_set_tx_freq( double freq );
void dvb_set_tx_lvl( float lvl );
void dvb_set_service_provider_name( const char *txt );
void dvb_set_service_name( const char *txt );
void dvb_set_video_capture_device( const char *txt );
int  dvb_set_dvb_mode( int mode );
void dvb_set_PmtPid( int pid );
void dvb_set_VideoPid( int pid );
void dvb_set_AudioPid( int pid );
void dvb_set_PcrPid( int pid );
void dvb_set_DataPid( int pid );
void dvb_set_video_capture_device_input( int input );
void dvb_set_tx_hardware_type( int type, const char *name );

// EVENT INFORMATION
void dvb_set_epg_event_duration( int duration );
void dvb_set_epg_event_title( const char *txt );
void dvb_set_epg_event_text( const char *txt );

// Teletext
void dvb_set_teletext_enabled( bool state );

// DVB S2 configuration
int dvb_set_s2_configuration( DVB2FrameFormat *f );

// IP sever
void dvb_set_server_ip_address( const char *text );
void dvb_set_server_socket_number( const char *text );

// Calibration
void dvb_calibration(bool status, int mode);
void dvb_i_chan_dac_offset_changed( int val );
void dvb_q_chan_dac_offset_changed( int val );
void dvb_dac_offset_selected(void);
void dvb_chan_dac_gain_selected(void);
void dvb_i_chan_dac_gain_changed( double val );
void dvb_q_chan_dac_gain_changed( double val );

#endif
