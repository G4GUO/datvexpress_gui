#include "dvb_gen.h"
#include "mp_tp.h"

unsigned long dvb_crc32_calc( uchar *b, int len )
{
    int i,n,bit;
    unsigned long crc = 0xFFFFFFFF;

    for( n = 0; n < len; n++ )
    {
        for( i=0x80; i; i>>=1 )
        {
            bit = ( ( ( crc&0x80000000 ) ? 1 : 0 ) ^ ( (b[n]&i) ? 1 : 0 ) );
            crc <<= 1;
            if(bit) crc ^= 0x04C11DB7;
        }
    }
    return crc;
}

int crc32_add( uchar *b, int len )
{
    unsigned long crc = dvb_crc32_calc( b, len );

    b[len++] = (crc>>24)&0xFF;
    b[len++] = (crc>>16)&0xFF;
    b[len++] = (crc>>8)&0xFF;
    b[len++] = (crc&0xFF);
    return len;
}
