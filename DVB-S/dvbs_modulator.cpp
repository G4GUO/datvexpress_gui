#include <math.h>
#include "memory.h"
#include "dvb.h"

// DVB-S encoding table

#define SMAG (0x7FFF)

const scmplx tx_lookup[4] = {
    {  SMAG,  SMAG},
    { -SMAG,  SMAG},
    {  SMAG, -SMAG},
    { -SMAG, -SMAG}
};
static scmplx m_sams[20000];
static int    m_tx_hardware;

//
// It all starts here
//
void dvbs_modulate_init( void )
{
}
//
// Input transport packet
//
// tp -> dibit -> samples -> queue
//
void dvb_s_encode_and_modulate( uchar *tp, uchar *dibit )
{
    // Samples passed to Express

    int len = dvb_encode_frame( tp, dibit  );

    for( int i = 0; i < len; i++ )
    {
        m_sams[i].re = tx_lookup[dibit[i]].re;
        m_sams[i].im = tx_lookup[dibit[i]].im;
    }
    write_final_tx_queue( m_sams, len );
}
//
// Input IQ samples float,
// length is the number of complex samples
// Output complex samples short
//

void dvbt_modulate( fft_complex *in, int length )
{
    // Convert to 16 bit fixed point and apply clipping where required

    for( int i = 0; i < length; i++)
    {

        if(fabs(in[i].re)>0.707)
        {
            if(in[i].re > 0 )
                in[i].re =  0.707;
            else
                in[i].re = -0.707;
        }
        if(fabs(in[i].im) > 0.707)
        {
            if(in[i].im > 0 )
                in[i].im =  0.707;
            else
                in[i].im = -0.707;
        }

        m_sams[i].re = (short)(in[i].re*0x7FFF);
        m_sams[i].im = (short)(in[i].im*0x7FFF);
    }
    // We should now queue the symbols
    write_final_tx_queue( m_sams, length);
}
void dvbs2_modulate( scmplx *in, int length )
{
    // We should now queue the symbols
    write_final_tx_queue( in, length );
}
void dvb_modulate_init(void)
{
    sys_config info;
    dvb_config_get( &info );
    m_tx_hardware = info.tx_hardware;
}
