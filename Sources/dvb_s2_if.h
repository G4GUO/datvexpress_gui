#ifndef DVB_S2_IF_H
#define DVB_S2_IF_H
#include "dvb_gen.h"

void dvb_s2_start( void );
void dvb_s2_stop( void );
void dvb_s2_encode_tp( uchar *buffer );
void dvb_s2_encode_dummy( void );
int  dvb_s2_re_configure( DVB2FrameFormat *f );
double dvb_s2_code_rate( void );

#endif // DVB_S2_IF_H
