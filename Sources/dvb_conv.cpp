#include "memory.h"
#include "dvb.h"
#include "dvb_config.h"

#define CONV_STATES 128

//131 Y
#define C20MASK 0x0001 
#define C21MASK 0x0002 
#define C23MASK 0x0008 
#define C24MASK 0x0010 
#define C26MASK 0x0040 

//171 X
#define C10MASK 0x0001 
#define C13MASK 0x0008 
#define C14MASK 0x0010 
#define C15MASK 0x0020 
#define C16MASK 0x0040 

static uchar conv_tab[CONV_STATES];
static int encode_state;
static int dvbs_code_rate;
// Storage
static uchar mb[10];
static int   mbc;
//
// Calculate the parity
// Bit 0 is G1 (X)
// Bit 1 is G2 (Y)
//
uchar dvb_conv_parity( int state )
{
    uchar count;
    uchar parity;

	count = 0;
	if(state&C20MASK) count++;
	if(state&C21MASK) count++;
	if(state&C23MASK) count++;
	if(state&C24MASK) count++;
	if(state&C26MASK) count++;
	
    parity = (count&1)<<1;

	count = 0;
	if(state&C10MASK) count++;
	if(state&C13MASK) count++;
	if(state&C14MASK) count++;
	if(state&C15MASK) count++;
	if(state&C16MASK) count++;
	
	parity |= (count&1);
		    
	return parity;
}

void dvb_conv_ctl( void )
{
    mbc            = 0;
    encode_state   = 0;
}

void dvb_conv_init( void )
{
    int i;
    sys_config info;

    // Build the encoding Tables
    for( i = 0; i < CONV_STATES; i++)
    {
        conv_tab[i] = dvb_conv_parity( i );
    }
    dvb_config_get( &info );
    dvb_conv_ctl();
    if( info.dvb_mode == MODE_DVBS ) dvbs_code_rate = info.dvbs_fmt.fec;
    if( info.dvb_mode == MODE_DVBT ) dvbs_code_rate = info.dvbt_fmt.fec;

}
inline uchar dvb_conv_encode_bit( uchar in )
{
    encode_state = encode_state>>1;

    if(in) encode_state |= 0x40;

    return conv_tab[encode_state];
}
//
// This punctures the dibits I channel Bit 0, Q channel Bit 1
// The output length varies depending on the puncture rate.
// The input array is overwritten
//
int dvb_puncture( uchar *in, int len )
{
    int i;
    uchar tmp[2000];
    // Output length
    int odx = 0;

    if( dvbs_code_rate == FEC_RATE_12 )
    {
        // Nothing to do
        return len;
    }

    if( dvbs_code_rate == FEC_RATE_23 )
    {
        for( i = 0; i < len; i++ )
        {
            mb[mbc++] = in[i];
            if( mbc == 4 )
            {
                tmp[odx++]  = mb[0];   //(Y1,X1) LSB == X (sent first)
                tmp[odx++]  = ((mb[2]<<1)&0x02)|(mb[1]>>1);//(X3,Y2)
                tmp[odx++]  = (mb[3]&0x2)|(mb[2]>>1);//(Y4,Y3)
                mbc = 0;
            }
        }
    }

    if( dvbs_code_rate == FEC_RATE_34 )
    {
        for( i = 0; i < len; i++ )
        {
            mb[mbc++] = in[i];
            if( mbc == 3 )
            {
                tmp[odx++]  = mb[0];     //(Y1,X1)
                tmp[odx++]  = ((mb[2]<<1)&0x02)|(mb[1]>>1);//(X3,Y2)
                mbc = 0;
            }
        }
    }

    if( dvbs_code_rate == FEC_RATE_56 )
    {
        for( i = 0; i < len; i++ )
        {
            mb[mbc++] = in[i];
            if( mbc == 5 )
            {
                tmp[odx++]  = mb[0];     //(Y1,X1)
                tmp[odx++]  = ((mb[2]<<1)&0x02)|(mb[1]>>1);  //(X3,Y2)
                tmp[odx++]  = ((mb[4]<<1)&0x02)|(mb[3]>>1);  //(X5,Y4)
                mbc = 0;
            }
        }
    }

    if( dvbs_code_rate == FEC_RATE_78 )
    {
        for( i = 0; i < len; i++ )
        {
            mb[mbc++] = in[i];
            if( mbc == 7 )
            {
                tmp[odx++]  = mb[0];     //(Y1,X1)
                tmp[odx++]  = (mb[2]&0x02)|(mb[1]>>1);//(Y3,Y2)
                tmp[odx++]  = ((mb[4]<<1)&0x2)|(mb[3]>>1);//(X5,Y4)
                tmp[odx++]  = ((mb[6]<<1)&0x2)|(mb[5]>>1);//(X7,Y6)
                mbc = 0;
            }
        }
    }
    memcpy(in,tmp,sizeof(uchar)*odx);
    return odx;
}
//
// The input is octets and the output is dibits.
//
int dvb_conv_encode_frame( uchar *in, uchar *out, int len )
{
    int i;
    int odx=0;

    for( i = 0; i < len; i++ )
    {
        out[odx++] = dvb_conv_encode_bit( in[i]&0x80 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x40 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x20 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x10 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x08 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x04 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x02 );
        out[odx++] = dvb_conv_encode_bit( in[i]&0x01 );
    }
    odx = dvb_puncture( out, odx );
    return odx;
}
