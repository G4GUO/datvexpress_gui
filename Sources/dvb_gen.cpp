//
// General routines for DVB transmitter
//
#include <stdio.h>
#include "dvb_gen.h"
#include "mp_tp.h"
#include "mp_config.h"

void dvb_si_init( void )
{
	null_fmt();
	pat_fmt();
	pmt_fmt();
	sdt_fmt();
	eit_fmt();
	nit_fmt();
}
void dvb_refresh_epg( void )
{
    sdt_fmt();
    eit_fmt();
}
//
// Print a packet
//
void print_tp( uchar *b )
{
	int i;
	for( i = 0; i < 188; i++ )
	{
		printf("%.2x ",b[i] );
	}
	printf("\n");
}

