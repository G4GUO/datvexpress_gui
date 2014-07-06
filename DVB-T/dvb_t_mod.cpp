//
// This is the dvb modulator.
//
// It takes the pre-built reference symbols
// add the unknown data, calls the FFT
// attaches the guard samples and sends
// the result to the transmitter
//
#include <stdio.h>
#include <memory.h>
#include "dvb_t.h"
#include "dvb.h"

// External 
extern DVBTFormat m_format;

// Constellation tables
extern fft_complex a1_qpsk[4];
extern fft_complex a1_qam16[16];
extern fft_complex a1_qam64[64];
extern fft_complex a2_qam16[16];
extern fft_complex a2_qam64[64];
extern fft_complex a4_qam16[16];
extern fft_complex a4_qam64[64];

extern fft_complex rt_2k[SYMS_IN_FRAME*SF_NR][M2KS];
extern int    dt_2k[SYMS_IN_FRAME*SF_NR][M2KS];
extern fft_complex rt_8k[SYMS_IN_FRAME*SF_NR][M8KS];
extern int    dt_8k[SYMS_IN_FRAME*SF_NR][M8KS];

// Local

// Where it is all at, choose the largest size.
static fft_complex m_tx_in_frame[M8KS];

// Table used to map binary onto constellation
const fft_complex *m_co_tab;

// Number of guard symbols
int m_guard;

//
// Calculate the number of samples to add for the guard period.
//
void dvb_t_calculate_guard_period( void )
{
    int d;
	
    switch( m_format.gi )
    {
        case GI_132:
            d = 32;
            break;
        case GI_116:
            d = 16;
            break;
        case GI_18:
            d = 8;
            break;
        case GI_14:
            d = 4;
            break;
        default:
            d = 4;
            break;
    }

    if( m_format.tm == TM_2K )
    {
        m_guard = M2KS/d;
    }

    if( m_format.tm == TM_8K )
    {
        m_guard = M8KS/d;
    }
}

//
// Select the required constellation for transmission.
//
void dvb_t_select_constellation_table( void )
{
    if(m_format.co == CO_QPSK )
    {
        m_co_tab = a1_qpsk;
    }

    if(m_format.co == CO_16QAM )
    {
        if((m_format.sf == SF_NH)||(m_format.sf == SF_A1))
        {
             m_co_tab = a1_qam16;
        }
        if(m_format.sf == SF_A2)
        {
             m_co_tab = a2_qam16;
        }
        if( m_format.sf == SF_A4 )
        {
             m_co_tab = a4_qam16;
        }
    }

    if(m_format.co == CO_64QAM )
    {
        if((m_format.sf == SF_NH)||(m_format.sf == SF_A1))
        {
            m_co_tab = a1_qam64;
        }
        if(m_format.sf == SF_A2)
        {
            m_co_tab = a2_qam64;
        }
        if( m_format.sf == SF_A4 )
        {
            m_co_tab = a4_qam64;
        }
    }
}
// Compensate for the filter response
void dvb_t_2k_compensation( fft_complex *s )
{
   fft_complex *f,*l;
   float div;
    f = &s[M2KSTART];
    l = &s[M2KSTART+K2MAX-1];

    div = 2;

    for( int i = 0; i < 300; i++ )
    {
        div *= 0.997;
        f->re*=div;
        f->im*=div;
        l->re*=div;
        l->im*=div;
        f++;l--;
        if( div <= 1.0 ) break;
    }
}

//
// An array of binary symbols are passed to this
// routine. These are converted to QPSK/QAM. IFFTed
// guard period added and then sent for sample 
// rate conversion before being written to the USRP2.
//
void dvb_t_modulate( uchar *syms )
{
    int i,r;
    fft_complex *fm;

    r = reference_symbol_seq_update();

    if( m_format.tm == TM_2K )
    {
        fm = &m_tx_in_frame[M2KSTART];
        // Add the reference tones
        memcpy( fm, rt_2k[r], sizeof(fft_complex)*(K2MAX+1));
        // add the data tsymbols
        for( i = 0; i < M2SI; i++ ) fm[dt_2k[r][i]] = m_co_tab[syms[i]];
//        dvb_t_2k_compensation( m_tx_in_frame );
//       fft_2k_test( m_tx_out_frame );
         dvbt_fft_modulate( m_tx_in_frame, m_guard );
    }

    if( m_format.tm == TM_8K )
    {
        fm = &m_tx_in_frame[M8KSTART];
        // Add the reference tones
        memcpy( fm, rt_8k[r], sizeof(fft_complex)*(K8MAX+1));
        // add the data symbols
        for( i = 0; i < M8SI; i++ ) fm[dt_8k[r][i]] = m_co_tab[syms[i]];
        dvbt_fft_modulate( m_tx_in_frame, m_guard );
    }
}
void dvb_t_modulate_init( void )
{
    // Clear the transmit frame
    memset( m_tx_in_frame, 0, sizeof(scmplx)*M8KS);
    dvb_t_calculate_guard_period();
    dvb_t_select_constellation_table();
}
