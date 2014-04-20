// 
// This is the interface module in and out of the dvb_t
// encoder
//
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/types.h>
#include <unistd.h>
#include "dvb_gen.h"
#include "dvb_t.h"
#include "dvb.h"
#include "dvb_capture_ctl.h"

#define COFFS_PRESENT  1
#define INPUT_PRESENT  2
#define OUTPUT_PRESENT 4

extern int m_i_fd;
extern int m_o_fd;
DVBTFormat m_format;

//
// Called externally to send the next
// transport frame. It encodes the frame
// then calls the modulator if a complete frame is ready
//
void dvb_t_encode_and_modulate( uchar *tp, uchar *dibit )
{
    int len;
    len =  dvb_encode_frame( tp, dibit  );
    dvb_t_enc_dibit( dibit, len );
}
long double dvb_t_get_sample_rate( void )
{
    long double srate = 8000000.0/7.0;

    switch( m_format.chan )
    {
    case CH_8:
        srate = srate*8.0;
        break;
    case CH_7:
        srate = srate*7.0;
        break;
    case CH_6:
        srate = srate*6.0;
        break;
    case CH_4:
        srate = srate*4.0*2.0;
        break;
    case CH_3:
        srate = srate*3.0*2.0;
        break;
    case CH_2:
        srate = srate*2.0*2.0;
         break;
    default:
        break;
    }
    return srate;
}
double dvb_t_get_symbol_rate( void )
{
    double symbol_len = 1;//default value
    double srate = dvb_t_get_sample_rate();
    if( m_format.tm == TM_2K ) symbol_len = M2KS;
    if( m_format.tm == TM_8K ) symbol_len = M8KS;
    switch( m_format.gi )
    {
        case GI_132:
            symbol_len += symbol_len/32;
            break;
        case GI_116:
            symbol_len += symbol_len/16;
            break;
        case GI_18:
            symbol_len += symbol_len/8;
            break;
        case GI_14:
            symbol_len += symbol_len/4;
            break;
        default:
            symbol_len += symbol_len/4;
            break;
    }
    if(( m_format.chan == CH_4 ) ||
       ( m_format.chan == CH_3 ) ||
       ( m_format.chan == CH_2 ))
    {
        srate = srate/2;
    }
    return srate/symbol_len;
}
//
// Initialise the module
//
//
void dvb_t_init( void )
{
    sys_config info;
    // Encode the correct parameters
    dvb_config_get( &info );
    m_format = info.dvbt_fmt;
    build_tx_sym_tabs();
    dvb_t_build_p_tables();
    init_dvb_t_fft();
    init_dvb_t_enc();
    build_tp_block();
    init_reference_frames();
    dvb_t_modulate_init();
}
//
// Called when altering the DVB-T mode form the GUI
//
// This does not work yet.
//
void dvb_t_re_init( void )
{
    dvb_t_init();
}
