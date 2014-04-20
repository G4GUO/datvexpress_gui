#ifndef SI570_H
#define SI570_H

#include <math.h>
#include "express.h"
#include "dvb_gen.h"

#define MIN_DCO 4850000000.0L
#define MAX_DCO 5670000000.0L
#define FCLK    114285000.0L
#define N_HSDIV 6

typedef struct{
    uchar r;
    uchar n;
}HsDiv;

void si570_set_clock( long double fclk );
void si570_rx( uchar rer, uchar val );
void si570_query( uchar reg);
void si570_initialise( void );

#endif // SI570_H
