#include <time.h>
#include "dvb.h"
#include "dvb_si.h"

void to_bcd( int v, uchar *b )
{
        b[0]  = ((v/10)<<4)&0xF0;
        b[0] |= (v%10)&0x0F;
}

int  dvb_si_add_time( uchar *b, dvb_si_time *t)
{
    int l,d,m,y;
    int mjd;

    d = t->day;
    m = t->month;
    y = t->year;

    if((m == 1)||(m == 2 ))
            l = 1;
    else
            l = 0;

    mjd = 14956 + d + (int)((y - l)*365.25f) + (int)((m+1+(l*12))*30.6001);
    b[0] = (mjd>>8)&0xFF;
    b[1] = mjd&0xFF;

    to_bcd( t->hour,    &b[2] );
    to_bcd( t->minute,  &b[3] );
    to_bcd( t->second,  &b[4] );
    return 5;
}

int  dvb_si_add_duration( uchar *b, dvb_si_duration *d )
{
    to_bcd( d->hour,    &b[0] );
    to_bcd( d->minute,  &b[1] );
    to_bcd( d->second,  &b[2] );
    return 3;
}

void dvb_si_system_time( dvb_si_time *st )
{
    time_t t;
    struct tm    *gt;

    t  = time(NULL);
    gt = gmtime( &t );
    // modify to correct format
    st->day    = gt->tm_mday;
    st->month  = gt->tm_mon + 1;
    st->year   = gt->tm_year;
    st->hour   = gt->tm_hour;
    st->minute = gt->tm_min;
    st->second = gt->tm_sec;
}
