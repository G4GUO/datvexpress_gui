#include <stdio.h>
#include "memory.h"
#include "dvb.h"

#ifdef __TEST__

// Shorter test RS code
#define G_POLY 0x3
#define G_BITS 4
#define G_MASK 0xF
#define G_OVR  0x10
#define G_MSB  0x8
#define G_SIZE 16
#define RS_LEN 4
#define M_LEN  11
#define P_LEN  4
#define gadd(a,b) (a^b)
#define gsub(a,b) (a^b)
#define gmul(a,b) (gmult_tab[a][b])

uchar gpoly[RS_LEN]={12,1,3,15};
#else

// DVB-S  RS code
#define G_POLY 0x1D
#define G_BITS 8
#define G_MASK 0xFF
#define G_OVR  0x100
#define G_MSB  0x80
#define G_SIZE 256
#define RS_LEN 16
#define M_LEN  188
#define P_LEN  16
#define gadd(a,b) (a^b)
#define gsub(a,b) (a^b)
#define gmul(a,b) (gmult_tab[a][b])

// Generator Polynomial (reverse order, highest order ignored)
uchar gpoly[RS_LEN]={59,36,50,98,229,41,65,163,8,30,209,68,189,104,13,59};

#endif

uchar gf[G_SIZE];
uchar gmult_tab[G_SIZE][G_SIZE];
uchar l_shift[RS_LEN];

inline uchar gmult(uchar a, uchar b)
{
	uchar p = 0;
	int i;
	uchar hbs;
	for( i = 0; i < G_BITS; i++) 
	{
		if((b & 1) == 1) 
		    p ^= a;
		hbs = (a & G_MSB);
		a <<= 1;
		if(hbs == G_MSB)
		{
			a ^= G_POLY;
			a &= G_MASK;
		}
		b >>= 1;
	}
	return p;
}
//
// Fast multiply table
//
void build_gf_mult_tab( void )
{
    int i,j;

    for( i = 0; i < G_SIZE; i++ )
    {
        for( j = 0; j < G_SIZE; j++ )
        {
            gmult_tab[i][j] = gmult( i, j );
        }
    }
}
//
// Construct the Galois field from the generator poly.
//
void build_gf_tab( void )
{
    int i;
    int n = 1;
    int temp[1000];
    temp[0] = 1;
    for( i = 1; i < G_MASK; i++ )
    {
        temp[i] = temp[i-1]<<1;
        if(temp[i] >= G_OVR )
        {
            temp[i] &= G_MASK;
            temp[i] ^= G_POLY;
        }
    }
    gf[0] = 0;
    for( i = 0; i < G_SIZE; i++ )
    {
        gf[n++] = temp[i];
    }

#ifdef __TEST__

	for( i = 0; i < G_SIZE; i++ )
	{
		//printf("%d %.2X\n",i,gf[i]);
		printf("%d %d\n",i,gf[i]);
	}
	uchar b = gf[2];
	uchar v = gmult( 0x53, 0xCA );
	printf("\nRes %.2X\n",v );

#endif 
}
inline uchar rs_round( void )
{
    int i;
    uchar sum = 0;

    for( i = 0; i < RS_LEN; i++ )
    {
        sum = gadd(sum,gmul(l_shift[i],gpoly[i]));
    }
    return sum;
}
inline void l_update( uchar val )
{
    int i;
    for( i = 0; i < RS_LEN-1; i++ )
    {
        l_shift[i] = l_shift[i+1];
    }
    l_shift[RS_LEN-1] = val;
}
//
// Build all the required tables
//
void dvb_rs_init( void )
{
	build_gf_tab();
	build_gf_mult_tab();

#ifdef __TEST__

	// Test code
	int i;
	uchar in[15]={1,2,3,4,5,6,7,8,9,10,11,0,0,0,0};
	printf("\n");
	dvb_rs_encode( in );
	for( i = 0; i < 15; i++ )
	{
		printf(" %d,",in[i] );
	}
	printf("\n");

#endif

}
//
// This is a RS(255,239,8) RS encoder
// The 188 byte blocks are padded out to 239 bytes
// by adding 51 zero bytes at the beginning and then removing
// them before transmission.
//
// Operates in place
//
void dvb_rs_encode( uchar *inout )
{
    // Clear the shift register
    memset(l_shift,0,sizeof(uchar)*RS_LEN);
    // Data phase use feedback
    int op = 0;int i;

    for( i = 0; i < M_LEN; i++ )
    {
        l_update( gadd(inout[op], rs_round()));
        op++;
    }
    // Parity phase use zeros
    for( i = 0; i < P_LEN; i++ )
    {
        inout[op] = rs_round();
        l_update( 0 );
        op++;
    }
}
