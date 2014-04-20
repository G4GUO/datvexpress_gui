#ifndef DVB_UHD_H
#define DVB_UHD_H
#include "DVB-T/dvb_t.h"

void dvb_uhd_init( void );
void dvb_uhd_set_tx_freq( double freq );
void dvb_uhd_set_tx_gain( float gain );
void dvb_set_tx_rate( double rate );
void dvb_uhd_set_rx_freq( double freq );
void dvb_uhd_set_rx_gain( float gain );
void dvb_set_rx_rate( double rate );
void dvb_uhd_write( scmplx *samples, int length );
void dvb_uhd_tx_config( double freq, float lvl, double rate );
double uhd_txq_delay(void);
#endif // DVB_UHD_H
