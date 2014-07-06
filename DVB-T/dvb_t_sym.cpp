//
// This module builds reference super frames for both modes
// and is used as a template when transmitting actual frames
//
#include <memory.h>
#include <stdio.h>

#include "dvb_gen.h"
#include "dvb_t.h"
#include "dvb_t_sym.h"

extern int   sptab_2k[PPS_2K_TAB_LEN];
extern int   sttab_2k[TPS_2K_TAB_LEN];

extern int   sptab_8k[PPS_8K_TAB_LEN];
extern int   sttab_8k[TPS_8K_TAB_LEN];

// Pilot tone BPSK
extern FLOAT pc_stab_cont[2];
extern FLOAT pc_stab_scat[2];
extern FLOAT pc_stab_tps[2];

extern int   m_sync_prbs[M8KS];

// first index is whether the seq starts with a 0 or 1 and
// Which frame number it is in a superframe
// 2nd index is the symbol number
// output is the index into the modulation table 0,1

extern uchar sstd[4][SYMS_IN_FRAME];
extern DVBTFormat m_format;

fft_complex rt_2k[SYMS_IN_FRAME*SF_NR][M2KS];
int          dt_2k[SYMS_IN_FRAME*SF_NR][M2KS];
fft_complex rt_8k[SYMS_IN_FRAME*SF_NR][M8KS];
int          dt_8k[SYMS_IN_FRAME*SF_NR][M8KS];

int m_l_count;

//
// Build the pilot tone reference table
//
void build_2k_sf_ref( void )
{
    int l,k,i,p,s;
    // Zap everything
    memset( rt_2k, 0,sizeof(scmplx)*SYMS_IN_FRAME*SF_NR*M2KS);

    // Add continuous pilot tones to all symbols in frame
    for( l = 0; l < (SYMS_IN_FRAME*SF_NR); l++ )
    {
        for( i = 0; i < PPS_2K_TAB_LEN; i++ )
        {
            k = sptab_2k[i];// Tone to do
            rt_2k[l][k].re = pc_stab_cont[m_sync_prbs[k]];
        }
    }

    // Add the scattered pilot tones
    for( l = 0; l < (SYMS_IN_FRAME*SF_NR); l++ )
    {
        for( p = 0; p < K2MAX; p++ )
        {
            k = K2MIN + (3*(l%4))+(12*p);
            if( k < K2MAX )
            {
                rt_2k[l][k].re = pc_stab_scat[m_sync_prbs[k]];
            }
        }
    }

    // Add the TP information to the 4 frames in the superframe
    // Frame 1
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l;
        for( i = 0; i < TPS_2K_TAB_LEN; i++ )
        {
            k = sttab_2k[i];// tone to do
            rt_2k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]][l]];
        }
    }
    // Frame 2
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l+SYMS_IN_FRAME;
        for( i = 0; i < TPS_2K_TAB_LEN; i++ )
        {
            k = sttab_2k[i];//tone to do
            rt_2k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]+2][l]];
        }
    }
    // Frame 3
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l+(SYMS_IN_FRAME*2);
        for( i = 0; i < TPS_2K_TAB_LEN; i++ )
        {
            k = sttab_2k[i];// tone to do
            rt_2k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]+4][l]];
        }
    }
    // Frame 4
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l+(SYMS_IN_FRAME*3);
        for( i = 0; i < TPS_2K_TAB_LEN; i++ )
        {
            k = sttab_2k[i];// tone to do
            rt_2k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]+6][l]];
        }
    }
    // Now build the data tone reference table
    for( l = 0, i = 0; l < (SYMS_IN_FRAME*SF_NR); l++ )
    {
        i = 0;
        for( k = K2MIN; k <= K2MAX; k++ )
        {
            if(rt_2k[l][k].re == 0 ) dt_2k[l][i++]=k;
        }
    }
}

void build_8k_sf_ref( void )
{
    int l,k,i,p,s;
    // Zap everything
    memset( rt_8k, 0,sizeof(scmplx)*SYMS_IN_FRAME*SF_NR*M8KS);

    // Add continuos pilot tones
    for( l = 0; l < (SYMS_IN_FRAME*SF_NR); l++ )
    {
       for( i = 0; i < PPS_8K_TAB_LEN; i++ )
       {
           k = sptab_8k[i];//Tone to do
           rt_8k[l][k].re = pc_stab_cont[m_sync_prbs[k]];
       }
    }
    // Add the scattered pilot tones
    for( l = 0; l < (SYMS_IN_FRAME*SF_NR); l++ )
    {
       for( p = 0; p < K8MAX; p++ )
       {
          k = K8MIN + (3*(l%4))+(12*p);
          if( k < K8MAX )
          {
             rt_8k[l][k].re = pc_stab_scat[m_sync_prbs[k]];
          }
        }
    }
    // Add the TP information to the 4 frames in the superframe
    // Frame 0
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l;
        for( i = 0; i < TPS_8K_TAB_LEN; i++ )
        {
           k = sttab_8k[i];
           rt_8k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]][l]];
        }
    }
    // Frame 2
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l+SYMS_IN_FRAME;
        for( i = 0; i < TPS_8K_TAB_LEN; i++ )
        {
            k = sttab_8k[i];
            rt_8k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]+2][l]];
        }
    }
    // Frame 3
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l+(SYMS_IN_FRAME*2);
        for( i = 0; i < TPS_8K_TAB_LEN; i++ )
        {
           k = sttab_8k[i];
           rt_8k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]+4][l]];
        }
    }
    // Frame 4
    for( l = 0; l < SYMS_IN_FRAME; l++ )
    {
        s = l+(SYMS_IN_FRAME*3);
        for( i = 0; i < TPS_8K_TAB_LEN; i++ )
        {
            k = sttab_8k[i];
            rt_8k[s][k].re = pc_stab_tps[sstd[m_sync_prbs[k]+6][l]];
        }
    }
    // Now build the data tone reference table
    for( l = 0, i = 0; l < (SYMS_IN_FRAME*SF_NR); l++ )
    {
        i = 0;
        for( k = K8MIN; k <= K8MAX; k++ )
        {
            if(rt_8k[l][k].re == 0 )dt_8k[l][i++]=k;
        }
    }
}
void reference_symbol_reset( void )
{
    m_l_count = 0;
}
//
// This is required by the symbol interleaver
//

int reference_symbol_seq_get( void )
{
    return m_l_count;
}

int reference_symbol_seq_update( void )
{
    int c = m_l_count;
    m_l_count = (m_l_count+1)%(SYMS_IN_FRAME*SF_NR);
    return c;
}

//
// Build a reference super frame, this only needs to be done
// at program start-up and when the transmitter mode is changed.
//
void init_reference_frames( void )
{
    build_2k_sf_ref();
    build_8k_sf_ref();
    reference_symbol_reset();
}
