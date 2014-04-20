#include <stdio.h>
#include <memory.h>
#include "DVB-S2/DVBS2.h"
#include "dvb.h"
#include "dvb_s2_if.h"
#include "dvb_uhd.h"

// DVBS2 Class
static DVBS2 *m_dvbs2;

void dvb_s2_start(void)
{
    m_dvbs2 = new DVBS2;

    // Get the Configuration from the database
    sys_config cfg;
    dvb_config_get( &cfg );
    m_dvbs2->s2_set_configure( &cfg.dvbs2_fmt );

}
void dvb_s2_stop(void)
{
    delete m_dvbs2;
}

void dvb_s2_encode_tp( uchar *b )
{
    int len;

    if((len = m_dvbs2->s2_add_ts_frame( b )) > 0 )
    {
        dvbs2_modulate( m_dvbs2->pl_get_frame(), len );
        //printf("SVBS2 TS %d %d %d\n", in[11].re, in[11].im, len );
    }
}
void dvb_s2_encode_dummy( void )
{
    scmplx *in;
    int len;
    in = m_dvbs2->pl_get_dummy( len );
    dvbs2_modulate( in, len );
}
int dvb_s2_re_configure( DVB2FrameFormat *f )
{
    return m_dvbs2->s2_set_configure( f );
}
double dvb_s2_code_rate( void )
{
    return m_dvbs2->s2_get_efficiency();
}
