#ifndef HARDWARE_H
#define HARDWARE_H
//
// Transmit control routines
//
void   hw_freq( double freq );
void   hw_level( float gain );
void   hw_sample_rate( double rate );
void   hw_config( double freq, float lvl );
void   hw_setup_channel(void);
int    hw_init( void );
void   hw_samples( scmplx *samples, int length );
double hw_uniterpolated_sample_rate(void);
void   hw_set_carrier( int status );
int    hw_get_carrier( void );
void   hw_thread( void );
void   hw_tx( void );
void   hw_rx( void );
void   hw_set_interp_and_filter( int rate );
void   hw_set_dvbt_sr(unsigned char chan);
void   hw_load_calibration( void );
double hw_outstanding_queue_size(void);

#endif // HARDWARE_H
