#ifndef DVB_SI_H
#define DVB_SI_H

typedef struct{
    int year;
    int month;
    int day;
    int hour;
    int minute;
    int second;
}dvb_si_time;

typedef struct{
    int hour;
    int minute;
    int second;
}dvb_si_duration;

int  dvb_si_add_time( uchar *b, dvb_si_time *t);
int  dvb_si_add_duration( uchar *b, dvb_si_duration *d);
void dvb_si_system_time( dvb_si_time *t );

#endif // DVB_SI_H
