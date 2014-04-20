#include <stdio.h>
#include "dvb_gen.h"
#include "dvb_t.h"

// Symbol interleaver arrays
int gb_h2k[M2SI];
int gb_h8k[M8SI];

// Bit interleaver arrays
int gb_bi[6][BIL];

// PRBS seq 
int m_sync_prbs[M8KS];

int permutate_bits_8k( int in )
{
	int out = 0;
	out |= in&0x001 ? (1<<7) : 0;
	out |= in&0x002 ? (1<<1) : 0;
	out |= in&0x004 ? (1<<4) : 0;
	out |= in&0x008 ? (1<<2) : 0;
	out |= in&0x010 ? (1<<9) : 0;
	out |= in&0x020 ? (1<<6) : 0;
	out |= in&0x040 ? (1<<8) : 0;
	out |= in&0x080 ? (1<<10): 0;
	out |= in&0x100 ? (1<<0) : 0;
	out |= in&0x200 ? (1<<3) : 0;
	out |= in&0x400 ? (1<<11): 0;
	out |= in&0x800 ? (1<<5) : 0;
	return out;
}
int permutate_bits_2k( int in )
{
	int out = 0;
	out |= in&0x001 ? (1<<4) : 0;
	out |= in&0x002 ? (1<<3) : 0;
	out |= in&0x004 ? (1<<9) : 0;
	out |= in&0x008 ? (1<<6) : 0;
	out |= in&0x010 ? (1<<2) : 0;
	out |= in&0x020 ? (1<<8) : 0;
	out |= in&0x040 ? (1<<1) : 0;
	out |= in&0x080 ? (1<<5) : 0;
	out |= in&0x100 ? (1<<7) : 0;
	out |= in&0x200 ? (1<<0) : 0;
	return out;
}
//
// Build the symbol permutation array
//
// Called only once
void build_h_2k( void )
{
    int i,b9,b0,b3,q;
    int sr;
    int tmp[M2KS];

    // Create R'
    tmp[0] = 0;
    tmp[1] = 0;
    tmp[2] = 1;
    sr = 1;

    for( i = 3; i < M2KS; i++ )
	{
        b0 = sr&0x01;
        b3 = (sr>>3)&0x01;
        b9 = b0^b3?0x200:0;
        sr = (sr>>1);
        sr |= b9;
        tmp[i] = sr;
	}
	// Create R
    for( i = 0; i < M2KS; i++ )
	{
        tmp[i] = ((i%2)<<10) + permutate_bits_2k( tmp[i] );
    }
    // Build iterpolation lookup table

    for( i = 0, q = 0; i < M2KS; i++ )
    {
        if( tmp[i] < M2SI)
        {
            gb_h2k[q] = tmp[i];
            q++;
        }
    }
}
//
// Build the symbol permutation array
//
// Called only once
void build_h_8k( void )
{
    int i,b11,b0,b1,b4,b6,q;
    int sr;
    int tmp[M8KS];

    // Create R'
    tmp[0] = 0;
    tmp[1] = 0;
    tmp[2] = 1;
    sr = 1;

    for( i = 3; i < M8KS; i++ )
    {
        b0 = sr&0x01;
        b1 = (sr>>1)&0x01;
        b4 = (sr>>4)&0x01;
        b6 = (sr>>6)&0x01;
        b11 = b0^b1^b4^b6 ?0x800:0;
        sr = (sr>>1);
        sr |= b11;
        tmp[i] = sr;
    }
    // Create R
    for( i = 0; i < M8KS; i++ )
    {
        tmp[i] = ((i%2)<<12) + permutate_bits_8k( tmp[i] );
    }
    // Build iterpolation lookup table

    for( i = 0, q = 0; i < M8KS; i++ )
    {
        if( tmp[i] < M8SI)
        {
            gb_h8k[q] = tmp[i];
            q++;
        }
    }
}
// Called only once
void build_bit( void )
{
    int w;
    for( w = 0; w < BIL; w++ )
    {
        gb_bi[0][w] = w;
        gb_bi[1][w] = (w+63)%BIL;
        gb_bi[2][w] = (w+105)%BIL;
        gb_bi[3][w] = (w+42)%BIL;
        gb_bi[4][w] = (w+21)%BIL;
        gb_bi[5][w] = (w+84)%BIL;
    }
}
void build_prbs_seq( void )
{
    int i;
    int s = 0x7FF;

    for( i = 0; i < M8KS; i++ )
    {
        m_sync_prbs[i] = s&1;
        s |= (s&1)^((s>>2)&1) ? 0x800 : 0;
        s >>= 1;
    }
}
void dvb_t_build_p_tables( void )
{
    build_h_8k();
    build_h_2k();
    build_bit();
    build_prbs_seq();
}
