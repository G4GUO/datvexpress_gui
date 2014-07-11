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
uchar gmult_tab[G_SIZE][RS_LEN];
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
    int a,b;

    for( a = 0; a < G_SIZE; a++ )
    {
        for( b = 0; b < RS_LEN; b++ )
        {
            gmult_tab[a][b] = gmult( a, gpoly[b] );
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
/*
inline uchar old_rs_round( void )
{
    int i;
    uchar sum = 0;

    for( i = 0; i < RS_LEN; i++ )
    {
        sum = gadd(sum,gmul(l_shift[i],gpoly[i]));
    }
    return sum;
}
*/
inline uchar rs_round( uchar val )
{
    uchar sum = 0;

    sum = gadd(sum,gmul(l_shift[0],0));
    l_shift[0] = l_shift[1];
    sum = gadd(sum,gmul(l_shift[1],1));
    l_shift[1] = l_shift[2];
    sum = gadd(sum,gmul(l_shift[2],2));
    l_shift[2] = l_shift[3];
    sum = gadd(sum,gmul(l_shift[3],3));
    l_shift[3] = l_shift[4];
    sum = gadd(sum,gmul(l_shift[4],4));
    l_shift[4] = l_shift[5];
    sum = gadd(sum,gmul(l_shift[5],5));
    l_shift[5] = l_shift[6];
    sum = gadd(sum,gmul(l_shift[6],6));
    l_shift[6] = l_shift[7];
    sum = gadd(sum,gmul(l_shift[7],7));
    l_shift[7] = l_shift[8];
    sum = gadd(sum,gmul(l_shift[8],8));
    l_shift[8] = l_shift[9];
    sum = gadd(sum,gmul(l_shift[9],9));
    l_shift[9] = l_shift[10];
    sum = gadd(sum,gmul(l_shift[10],10));
    l_shift[10] = l_shift[11];
    sum = gadd(sum,gmul(l_shift[11],11));
    l_shift[11] = l_shift[12];
    sum = gadd(sum,gmul(l_shift[12],12));
    l_shift[12] = l_shift[13];
    sum = gadd(sum,gmul(l_shift[13],13));
    l_shift[13] = l_shift[14];
    sum = gadd(sum,gmul(l_shift[14],14));
    l_shift[14] = l_shift[15];
    sum = gadd(sum,gmul(l_shift[15],15));
    l_shift[15] = sum ^ val;
    return sum;
}
inline uchar rs_round( void )
{
    uchar sum;

    sum = gadd(0,gmul(l_shift[0],0));
    l_shift[0] = l_shift[1];
    sum = gadd(sum,gmul(l_shift[1],1));
    l_shift[1] = l_shift[2];
    sum = gadd(sum,gmul(l_shift[2],2));
    l_shift[2] = l_shift[3];
    sum = gadd(sum,gmul(l_shift[3],3));
    l_shift[3] = l_shift[4];
    sum = gadd(sum,gmul(l_shift[4],4));
    l_shift[4] = l_shift[5];
    sum = gadd(sum,gmul(l_shift[5],5));
    l_shift[5] = l_shift[6];
    sum = gadd(sum,gmul(l_shift[6],6));
    l_shift[6] = l_shift[7];
    sum = gadd(sum,gmul(l_shift[7],7));
    l_shift[7] = l_shift[8];
    sum = gadd(sum,gmul(l_shift[8],8));
    l_shift[8] = l_shift[9];
    sum = gadd(sum,gmul(l_shift[9],9));
    l_shift[9] = l_shift[10];
    sum = gadd(sum,gmul(l_shift[10],10));
    l_shift[10] = l_shift[11];
    sum = gadd(sum,gmul(l_shift[11],11));
    l_shift[11] = l_shift[12];
    sum = gadd(sum,gmul(l_shift[12],12));
    l_shift[12] = l_shift[13];
    sum = gadd(sum,gmul(l_shift[13],13));
    l_shift[13] = l_shift[14];
    sum = gadd(sum,gmul(l_shift[14],14));
    l_shift[14] = l_shift[15];
    sum = gadd(sum,gmul(l_shift[15],15));
    l_shift[15] = 0;
    return sum;
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
    // The shift register has been zeroed by  this point
    uchar *b = inout;
    // Data phase use feedback
    int i;

    for( i = 0; i < M_LEN; i++ ) rs_round(b[i]);

    // Parity phase use 16 zeros
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
    b[i++] = rs_round();
}
