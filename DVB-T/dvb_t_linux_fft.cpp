#include "math.h"
#include <stdio.h>
#include <memory.h>
#include "an_capture.h"
#include "dvb.h"
#include "dvb_t.h"

extern DVBTFormat m_format;
extern double m_sample_rate;

#ifdef USE_AVFFT
static FFTContext *m_avfft_2k_context;
static FFTContext *m_avfft_4k_context;
static FFTContext *m_avfft_8k_context;
static FFTContext *m_avfft_16k_context;
static fft_complex *m_fft_in;
static fft_complex *m_fft_out;
#else
static fftw_plan    m_fftw_2k_plan;
static fftw_plan    m_fftw_4k_plan;
static fftw_plan    m_fftw_8k_plan;
static fftw_plan    m_fftw_16k_plan;
static fft_complex *m_fft_in;
static fft_complex *m_fft_out;
#endif
//
// Taper table
//
double m_taper[M16KS+(M16KS/4)];

void create_taper_table(int N, int IR, int GI)
{
    N         = N*IR;// FFT size
    int CP    = N/GI;//Cyclic prefix
    int ALPHA = 32;// Rx Alpha
    int NT    = 2*(N/(ALPHA*2));//Taper length
    int idx   = 0;
    int n     = -NT/2;

    for( int i = 0; i < NT; i++ ){
        m_taper[idx++] = 0.5*(1.0+cos((M_PI*n)/NT));
        n++;
    }
    for( int i = 0; i < (N+CP)-(2*NT); i++ ){
        m_taper[idx++] = 1.0;
    }
    for( int i = 0; i < NT; i++ ){
        m_taper[idx++] = m_taper[i];
    }
}

// sinc correction coefficients
int m_N;
int m_IR;
double m_c[M16KS];

void create_correction_table( int N, int IR )
{
    double x,sinc;
    double f = 0.0;
    double fsr = m_sample_rate;
    double fstep = fsr / (N*IR);

    for( int i = 0; i < N/2; i++ )
    {
        if( f == 0 )
            sinc = 1.0;
        else{
            x = M_PI*f/fsr;
            sinc = sin(x)/x;
        }
        m_c[i+(N/2)]   = 1.0/sinc;
        m_c[(N/2)-i-1] = 1.0/sinc;
        f += fstep;
    }
}

static void fft_2k( fft_complex *in, fft_complex *out )
{

    // Copy the data into the correct bins
//    memcpy(&m_fft_in[M2KS/2], &in[0],      sizeof(fft_complex)*M2KS/2);
//    memcpy(&m_fft_in[0],      &in[M2KS/2], sizeof(fft_complex)*M2KS/2);

    int i,m;
    m = (M2KS/2);
    for( i = 0; i < (M2KS); i++ )
    {
        m_fft_in[m].re = in[i].re*m_c[i];
        m_fft_in[m].im = in[i].im*m_c[i];
        m = (m+1)%(M2KS);
    }

#ifdef USE_AVFFT
    av_fft_permute( m_avfft_2k_context, m_fft_in );
    av_fft_calc(    m_avfft_2k_context, m_fft_in );
#else
    fftw_one( m_fftw_2k_plan, m_fft_in, out );
#endif
    return;
}
static void fft_4k( fft_complex *in, fft_complex *out )
{
    // Zero the unused parts of the array
    memset(&m_fft_in[M4KS/4],0,sizeof(fft_complex)*M4KS/2);
    // Copy the data into the correct bins
    //memcpy(&m_fft_in[M4KS*3/4], &in[0],      sizeof(fft_complex)*M2KS/2);
    //memcpy(&m_fft_in[0],        &in[M2KS/2], sizeof(fft_complex)*M2KS/2);

    int i,m;
    m = (M4KS/2)+(M4KS/4);
    for( i = 0; i < (M2KS); i++ )
    {
        m_fft_in[m].re = in[i].re*m_c[i];
        m_fft_in[m].im = in[i].im*m_c[i];
        m = (m+1)%(M2KS*2);
    }

#ifdef USE_AVFFT
    av_fft_permute( m_avfft_4k_context, m_fft_in );
    av_fft_calc(    m_avfft_4k_context, m_fft_in );
#else
    fftw_one( m_fftw_4k_plan, m_fft_in, out );
#endif
    return;
}
//
// Interpolate by 4 using an 8K FFT
//
static void fft_2k_nb( fft_complex *in, fft_complex *out )
{
    // Zero the unused parts of the array
    memset(&m_fft_in[M8KS/8],0,sizeof(fft_complex)*M8KS*6/8);
    // Copy the data into the correct bins
    // Copy the data into the correct bins
//    memcpy(&m_fft_in[M8KS*7/8], &in[0],      sizeof(fft_complex)*M2KS/2);
//    memcpy(&m_fft_in[0],        &in[M2KS/2], sizeof(fft_complex)*M2KS/2);

    int i,m;
    m = (M8KS)-(M8KS/8);
    for( i = 0; i < (M2KS); i++ )
    {
        m_fft_in[m].re = in[i].re*m_c[i];
        m_fft_in[m].im = in[i].im*m_c[i];
        m = (m+1)%M2KS;
    }

#ifdef USE_AVFFT
    av_fft_permute( m_avfft_8k_context, m_fft_in );
    av_fft_calc(    m_avfft_8k_context, m_fft_in );
#else
    fftw_one( m_fftw_8k_plan, m_fft_in, out );
#endif
}
static void fft_8k( fft_complex *in, fft_complex *out )
{
    // Copy the data into the correct bins
//    memcpy(&m_fft_in[M8KS/2], &in[0],      sizeof(fft_complex)*M8KS/2);
//    memcpy(&m_fft_in[0],      &in[M8KS/2], sizeof(fft_complex)*M8KS/2);

    int i,m;
    m = (M8KS/2);
    for( i = 0; i < (M8KS); i++ )
    {
        m_fft_in[m].re = in[i].re*m_c[i];
        m_fft_in[m].im = in[i].im*m_c[i];
        m = (m+1)%M8KS;
    }

#ifdef USE_AVFFT
    av_fft_permute( m_avfft_8k_context, m_fft_in );
    av_fft_calc(    m_avfft_8k_context, m_fft_in );
#else
    fftw_one( m_fftw_8k_plan, m_fft_in, out );
#endif
}
static void fft_16k( fft_complex *in, fft_complex *out )
{
    // Zero the unused parts of the array
    memset(&m_fft_in[M8KS/2],0,sizeof(fft_complex)*M8KS);
    // Copy the data into the correct bins
//    memcpy(&m_fft_in[M8KS*3/2], &in[0],      sizeof(fft_complex)*M8KS/2);
//    memcpy(&m_fft_in[0],        &in[M8KS/2], sizeof(fft_complex)*M8KS/2);

    int i,m;
    m = (M8KS)+(M8KS/2);
    for( i = 0; i < (M8KS); i++ )
    {
        m_fft_in[m].re = in[i].re*m_c[i];
        m_fft_in[m].im = in[i].im*m_c[i];
        m = (m+1)%(M8KS*2);
    }

#ifdef USE_AVFFT
    av_fft_permute( m_avfft_16k_context, m_fft_in );
    av_fft_calc(    m_avfft_16k_context, m_fft_in );
#else
    fftw_one( m_fftw_16k_plan, m_fft_in, out );
#endif
}
//
// Chooses the corect FFT, adds the guard interval and sends to the modulator
//
void dvbt_fft_modulate( fft_complex *in, int guard )
{
    int size = 0;
    fft_complex *out;

    if( m_format.tm == TM_2K)
    {
        switch( m_format.chan )
        {
        case CH_8M:
        case CH_7M:
        case CH_6M:
            fft_2k( in, m_fft_out );
            size = M2KS;
            break;
        case CH_4M:
        case CH_3M:
        case CH_2M:
        case CH_1M:
            fft_4k( in, m_fft_out );
            size  = M4KS;
            guard = guard*2;
            break;
        case CH_500K:
            fft_2k_nb( in, m_fft_out );
            size  = M8KS;
            guard = guard*4;
#ifdef USE_AVFFT
            // Guard
            dvbt_clip( &m_fft_in, size );
            dvbt_modulate( &m_fft_in[size-guard], guard);
            // Data
            dvbt_modulate( m_fft_in, size );
#else
           // Clip the FFT outputs
           dvbt_clip( m_fft_out, size );
           dvbt_modulate( &m_fft_in[size-guard], guard);
           // Guard
           //out =  dvbt_filter( &m_fft_out[size-guard], guard );
           //dvbt_modulate( out, guard);
           // Data
           //out =  dvbt_filter( m_fft_out, size );
           //dvbt_modulate( out, size );
           dvbt_modulate( m_fft_in, size );
#endif
            return;
            break;
        }
    }
    if( m_format.tm == TM_8K)
    {
        switch( m_format.chan )
        {
        case CH_8M:
        case CH_7M:
        case CH_6M:
            fft_8k( in, m_fft_out );
            size = M8KS;
            break;
        case CH_4M:
        case CH_3M:
        case CH_2M:
        case CH_1M:
//        case CH_500K:
            fft_16k( in, m_fft_out );
            size = M16KS;
            guard = guard*2;
            break;
        }
    }
//    fft_complex *out;
#ifdef USE_AVFFT
    // Guard
    dvbt_clip( &m_fft_in, size );
    dvbt_modulate( &m_fft_in[size-guard], guard);
    // Data
    dvbt_modulate( m_fft_in, size );
#else
    // Clip the FFT outputs
    dvbt_clip( m_fft_out, size );
    // Guard
    //out =  dvbt_filter( &m_fft_out[size-guard], guard );
    //dvbt_modulate( out, guard);

    //dvbt_modulate( &m_fft_out[size-guard], guard);
    dvbt_modulate( &m_fft_out[size-guard], &m_taper[0], guard);
    // Data
    //out =  dvbt_filter( m_fft_out, size );
    //dvbt_modulate( out, size );
    //dvbt_modulate( m_fft_out, size );
    dvbt_modulate( m_fft_out, &m_taper[guard], size );
#endif
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
#ifdef USE_AVFFT
    m_avfft_2k_context  = av_fft_init (11, 1);
    m_avfft_4k_context  = av_fft_init (12, 1);
    m_avfft_8k_context  = av_fft_init (13, 1);
    m_avfft_16k_context = av_fft_init (14, 1);
    m_fft_in  = (fft_complex*)av_malloc(sizeof(fft_complex)*M16KS);
    m_fft_out = (fft_complex*)av_malloc(sizeof(fft_complex)*M16KS);
#else

    FILE *fp;
    if((fp=fopen(dvb_config_get_path("fftw_wisdom"),"r"))!=NULL)
    {
        fftw_import_wisdom_from_file(fp);
        m_fftw_2k_plan  = fftw_create_plan(M2KS,  FFTW_BACKWARD, FFTW_USE_WISDOM);
        m_fftw_4k_plan  = fftw_create_plan(M4KS,  FFTW_BACKWARD, FFTW_USE_WISDOM);
        m_fftw_8k_plan  = fftw_create_plan(M8KS,  FFTW_BACKWARD, FFTW_USE_WISDOM);
        m_fftw_16k_plan = fftw_create_plan(M16KS, FFTW_BACKWARD, FFTW_USE_WISDOM);
        fftw_import_wisdom_from_file(fp);
    }
    else
    {
        if((fp=fopen(dvb_config_get_path("fftw_wisdom"),"w"))!=NULL)
        {
            m_fftw_2k_plan  = fftw_create_plan(M2KS,  FFTW_BACKWARD, FFTW_MEASURE | FFTW_USE_WISDOM);
            m_fftw_4k_plan  = fftw_create_plan(M4KS,  FFTW_BACKWARD, FFTW_MEASURE | FFTW_USE_WISDOM);
            m_fftw_8k_plan  = fftw_create_plan(M8KS,  FFTW_BACKWARD, FFTW_MEASURE | FFTW_USE_WISDOM);
            m_fftw_16k_plan = fftw_create_plan(M16KS, FFTW_BACKWARD, FFTW_MEASURE | FFTW_USE_WISDOM);
            if(fp!=NULL) fftw_export_wisdom_to_file(fp);
        }
    }
    m_fft_in  = (fft_complex*)fftw_malloc(sizeof(fft_complex)*M16KS);
    m_fft_out = (fft_complex*)fftw_malloc(sizeof(fft_complex)*M16KS);
#endif
    if( m_format.tm == TM_2K)
    {
        m_N = M2KS;
        switch( m_format.chan )
        {
        case CH_8M:
        case CH_7M:
        case CH_6M:
            m_IR = 1;
            break;
        case CH_4M:
        case CH_3M:
        case CH_2M:
        case CH_1M:
            m_IR = 2;
            break;
        case CH_500K:
            m_IR = 4;
            break;
        }
    }
    if( m_format.tm == TM_8K)
    {
        m_N = M8KS;
        switch( m_format.chan )
        {
        case CH_8M:
        case CH_7M:
        case CH_6M:
            m_IR = 1;
            break;
        case CH_4M:
        case CH_3M:
        case CH_2M:
        case CH_1M:
            m_IR = 2;
            break;
        }
    }
    int GI;
    switch(m_format.gi)
    {
        case GI_132:
            GI = 32;
            break;
        case GI_116:
            GI = 16;
            break;
        case GI_18:
            GI = 8;
            break;
        case GI_14:
            GI = 4;
            break;
        default:
            GI = 4;
            break;
    }

    create_correction_table( m_N, m_IR );
    create_taper_table(m_N, m_IR, GI );
}
void deinit_dvb_t_fft( void )
{
#ifdef USE_AVFFT
    av_free(m_fft_in);
    av_free(m_fft_out);
#else
    fftw_free(m_fft_in);
    fftw_free(m_fft_out);
#endif
}
