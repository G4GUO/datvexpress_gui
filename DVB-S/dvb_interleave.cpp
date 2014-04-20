#include "memory.h"
#include "dvb.h"

#define I_MOD  17
#define I_ROWS 12

uchar d_array[2000];//where data is stored
uchar *rows[I_ROWS];// data rows
unsigned long i_mc[I_ROWS];// current row
int i_mods[I_ROWS];//length of each row

void dvb_interleave_init( void )
{
    int i;
    int m = 0;
    rows[0] = &d_array[m];
    m++;
    for( i = 1; i < I_ROWS; i++ )
    {
        rows[i]   = &d_array[m];
        i_mods[i] = (I_MOD*i);
        m        += i_mods[i];
        i_mc[i]   = 0;
    }
    //printf("%d \n",m );
}
inline void dvb_interleave_sub( uchar *inout )
{
    int   i;
    int   off;
    uchar   out;

//	rows[0][0] = inout[0];
//	inout[0]   = rows[0][0];

    for( i = 1; i < I_ROWS; i++ )
    {
        off          = i_mc[i];
        out          = rows[i][off];
        rows[i][off] = inout[i];
        inout[i]     = out;
        // Update modulo counter
        i_mc[i]      = (i_mc[i]+1)%i_mods[i];
    }
}
//
// Do this inplace, interleave the bytes
//
void dvb_convolutional_interleave( uchar *inout )
{
    int i;

    for( i = 0; i < DVBS_T_CODED_FRAME_LEN; i+= I_ROWS )
    {
        dvb_interleave_sub( &inout[i] );
    }
}
