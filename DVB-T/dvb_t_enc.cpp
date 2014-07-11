//
// This module encodes and sends the DVB-T signal.
//
//
#include <stdio.h>
#include "dvb_t.h"

extern DVBTFormat m_format;

// Bit Interleaver lookup table
extern int gb_bi[6][BIL];

// Symbol interleaver arrays
extern int gb_h2k[M2SI];
extern int gb_h8k[M8SI];

// Symbol interleaver array
static uchar m_sio[M8SI];
static uchar m_sii[M8SI];
static int m_si_x;
static int m_count;

// Bit interleaver array
static uchar m_bi[6][BIL];
static int   m_i_x;

//
// Symbol interleaver called once per symbol
//
void symbol_interleave( void )
{
    int i;
    int sn = reference_symbol_seq_get();

    if( m_format.tm == TM_2K )
    {
        if((sn&1) == 0 )
        {
            // Even symbols
            for( i = 0; i < M2SI; i++ )
            {
                m_sio[gb_h2k[i]] = m_sii[i];
            }
        }
        else
        {
            // Odd symbols
            for( i = 0; i < M2SI; i++ )
            {
                 m_sio[i] = m_sii[gb_h2k[i]];
            }
        }
    }

    if( m_format.tm == TM_8K )
    {
       if((sn&1) == 0 )
       {
           // Even symbols
           for( i = 0; i < M8SI; i++ )
           {
                m_sio[gb_h8k[i]] = m_sii[i];
            }
        }
        else
        {
            // Odd symbols
            for( i = 0; i < M8SI; i++ )
            {
                m_sio[i] = m_sii[gb_h8k[i]];
            }
        }
    }

    // Call the modulator as a complete symbol is ready
    dvb_t_modulate( m_sio );
    m_si_x = 0;//reset the symbol interleaver input counter
}
//
// Input array, output array, bit interleave lookup table
//
void inline bit_interleave( uchar *in, uchar *out, int *tab )
{
    int i;
    for( i = 0; i < BIL; i++ )
    {
        out[i] = in[tab[i]];
    }
}
//
// Done one full bit interleaver at a time (126 bits)
//
void qpsk_bit_interleave( void )
{
    int i;
    uchar bi[2][BIL];
    bit_interleave( m_bi[0], bi[0], gb_bi[0] );
    bit_interleave( m_bi[1], bi[1], gb_bi[1] );
    // Pack the symbols
    for( i = 0; i < BIL; i++ )
    {
        m_sii[m_si_x]  = bi[0][i]<<1;
        m_sii[m_si_x] |= bi[1][i];
        m_si_x++;
    }
    if((m_format.tm == TM_2K)&&(m_si_x==M2SI)) symbol_interleave();
    if((m_format.tm == TM_8K)&&(m_si_x==M8SI)) symbol_interleave();
}
void qam16_bit_interleave( void )
{
    int i;
    uchar bi[4][BIL];
    bit_interleave( m_bi[0], bi[0], gb_bi[0] );
    bit_interleave( m_bi[1], bi[1], gb_bi[1] );
    bit_interleave( m_bi[2], bi[2], gb_bi[2] );
    bit_interleave( m_bi[3], bi[3], gb_bi[3] );
    // Pack the symbols
    for( i = 0; i < BIL; i++ )
    {
        m_sii[m_si_x]  = bi[0][i]<<3;
        m_sii[m_si_x] |= bi[1][i]<<2;
        m_sii[m_si_x] |= bi[2][i]<<1;
        m_sii[m_si_x] |= bi[3][i];
        m_si_x++;
    }
    if((m_format.tm == TM_2K)&&(m_si_x==M2SI)) symbol_interleave();
    if((m_format.tm == TM_8K)&&(m_si_x==M8SI)) symbol_interleave();
}
void qam64_bit_interleave( void )
{
    int i;
    uchar bi[6][BIL];
    bit_interleave( m_bi[0], bi[0], gb_bi[0] );
    bit_interleave( m_bi[1], bi[1], gb_bi[1] );
    bit_interleave( m_bi[2], bi[2], gb_bi[2] );
    bit_interleave( m_bi[3], bi[3], gb_bi[3] );
    bit_interleave( m_bi[4], bi[4], gb_bi[4] );
    bit_interleave( m_bi[5], bi[5], gb_bi[5] );

    // Pack the symbols
    for( i = 0; i < BIL; i++ )
    {
        m_sii[m_si_x]  = bi[0][i]<<5;
        m_sii[m_si_x] |= bi[1][i]<<4;
        m_sii[m_si_x] |= bi[2][i]<<3;
        m_sii[m_si_x] |= bi[3][i]<<2;
        m_sii[m_si_x] |= bi[4][i]<<1;
        m_sii[m_si_x] |= bi[5][i];
        m_si_x++;
    }
    if((m_format.tm == TM_2K)&&(m_si_x==M2SI)) symbol_interleave();
    if((m_format.tm == TM_8K)&&(m_si_x==M8SI)) symbol_interleave();
}
//
// This currently only supports non H mode
// Input is a dibit array
//
void dvb_t_enc_dibit( uchar *in, int length )
{
    int i;
//    printf("Len %d\n",length);

    // Two bits per symbol

    if( m_format.co == CO_QPSK )
    {
        for( i = 0; i < length; i++ )
        {
            m_bi[0][m_i_x] = in[i]&1;
            m_bi[1][m_i_x] = in[i]>>1;
            m_i_x++;
            if( m_i_x == BIL )
            {
                m_i_x = 0;
                qpsk_bit_interleave();
            }
        }
    }

    // 4 bits per symbol

    if( m_format.co == CO_16QAM )
    {
        for( i = 0; i < length; i++ )
        {
            switch(m_count)
            {
            case 0:
                m_bi[0][m_i_x] = in[i]&1;
                m_bi[2][m_i_x] = in[i]>>1;
                break;
            case 1:
                m_bi[1][m_i_x] = in[i]&1;
                m_bi[3][m_i_x] = in[i]>>1;
                m_i_x++;
                if( m_i_x == BIL )
                {
                    m_i_x = 0;
                    qam16_bit_interleave();
                }
                break;
            default:
                break;
            }
            m_count = (m_count + 1)%2;
        }
    }

    // 6 bits per symbol

    if( m_format.co == CO_64QAM )
    {
        for( i = 0; i < length; i++ )
        {
            switch(m_count)
            {
            case 0:
                m_bi[0][m_i_x] = in[i]&1;
                m_bi[2][m_i_x] = in[i]>>1;
                break;
            case 1:
                m_bi[4][m_i_x] = in[i]&1;
                m_bi[1][m_i_x] = in[i]>>1;
                break;
            case 2:
                m_bi[3][m_i_x] = in[i]&1;
                m_bi[5][m_i_x] = in[i]>>1;
                m_i_x++;
                if( m_i_x == BIL )
                {
                    m_i_x = 0;
                    qam64_bit_interleave();
                }
                break;
            default:
                break;
            }
            m_count = (m_count + 1)%3;
        }
    }
}
void init_dvb_t_enc( void )
{
    m_si_x = 0;
    m_i_x  = 0;
    m_count = 0;
}
