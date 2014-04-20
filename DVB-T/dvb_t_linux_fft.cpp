#include "math.h"
#include <stdio.h>
#include <memory.h>
#include "dvb.h"
#include "dvb_t.h"

extern DVBTFormat m_format;

static fftw_plan    m_fftw_2k_plan;
static fftw_plan    m_fftw_4k_plan;
static fftw_plan    m_fftw_8k_plan;
static fftw_plan    m_fftw_16k_plan;
static fftw_complex m_fftw_in[M16KS];
static fftw_complex m_fftw_out[M16KS];

static void fft_2k( fftw_complex *in, fftw_complex *out )
{
    int i,m;
    m = (M2KS/2);
    for( i = 0; i < (M2KS); i++ )
    {
        m_fftw_in[m] = in[i];
        m = (m+1)%(M2KS);
    }
    fftw_one( m_fftw_2k_plan, m_fftw_in, out );
    return;
}
static void fft_4k( fftw_complex *in, fftw_complex *out )
{
    int i,m;
    m = (M2KS)+(M2KS/2);
    for( i = 0; i < (M2KS); i++ )
    {
        m_fftw_in[m] = in[i];
        m = (m+1)%(M2KS*2);
    }
    fftw_one( m_fftw_4k_plan, m_fftw_in, out );
    return;
}
static void fft_8k( fftw_complex *in, fftw_complex *out )
{
    int i,m;
    m = (M8KS/2);
    for( i = 0; i < (M8KS); i++ )
    {
        m_fftw_in[m] = in[i];
        m = (m+1)%M8KS;
    }
    fftw_one( m_fftw_8k_plan, m_fftw_in, out );
}
static void fft_16k( fftw_complex *in, fftw_complex *out )
{
    int i,m;
    m = (M8KS)+(M8KS/2);
    for( i = 0; i < (M8KS); i++ )
    {
        m_fftw_in[m] = in[i];
        m = (m+1)%(M8KS*2);
    }
    fftw_one( m_fftw_16k_plan, m_fftw_in, out );
}
//
// Chooses the corect FFT, adds the guard interval and sends to the modulator
//
void dvbt_fft_modulate( fftw_complex *in, int guard )
{
    int size = 0;
    if( m_format.tm == TM_2K)
    {
        switch( m_format.chan )
        {
        case CH_8:
        case CH_7:
        case CH_6:
            fft_2k( in, m_fftw_out );
            size = M2KS;
            break;
        case CH_4:
        case CH_3:
        case CH_2:
            fft_4k( in, m_fftw_out );
            size  = M4KS;
            guard = guard*2;
            break;
        }
    }
    if( m_format.tm == TM_8K)
    {
        switch( m_format.chan )
        {
        case CH_8:
        case CH_7:
        case CH_6:
            fft_8k( in, m_fftw_out );
            size = M8KS;
            break;
        case CH_4:
        case CH_3:
        case CH_2:
            fft_16k( in, m_fftw_out );
            size = M16KS;
            guard = guard*2;
            break;
        }
    }
    // Guard
    dvbt_modulate( &m_fftw_out[size-guard], guard);
    // Data
    dvbt_modulate( m_fftw_out, size );
}
/*
void fft_2k_test(  fftw_complex *out )
{
    memset(fftw_in, 0, sizeof(fftw_complex)*M2KS);
    int m = (M2KS/2)+32;//1704;
    fftw_in[m].re =  0.7;

    fftw_one( m_fftw_2k_plan, fftw_in, out );
    return;
}
*/
void init_dvb_t_fft( void )
{
    //
    // Plans
    //
    m_fftw_2k_plan = fftw_create_plan(M2KS,FFTW_BACKWARD,FFTW_ESTIMATE );
    m_fftw_4k_plan = fftw_create_plan(M4KS,FFTW_BACKWARD,FFTW_ESTIMATE );
    m_fftw_8k_plan = fftw_create_plan(M8KS,FFTW_BACKWARD,FFTW_ESTIMATE);
    m_fftw_16k_plan = fftw_create_plan(M16KS,FFTW_BACKWARD,FFTW_ESTIMATE);
}
