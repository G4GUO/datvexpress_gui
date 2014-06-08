#include "dvb_gen.h"
#include "dvb_types.h"
#include "dvb.h"
#include "dvb_c.h"

extern scmplx dvb_c_qam16[16];
extern scmplx dvb_c_qam32[32];
extern scmplx dvb_c_qam64[64];
extern scmplx dvb_c_qam128[128];
extern scmplx dvb_c_qam256[256];
extern uchar  dvb_c_diff_enc[16];

static int m_b_count;
static uchar m_b[10];
static uchar m_d;
static int m_fc;

void dvb_c_transmit_symbol( scmplx sym)
{
    sym.re = sym.re;
}

void dvb_c_256qam( uchar *b, int len )
{
    uchar tmp;
    for( int i = 0; i < len; i++ )
    {
        tmp = b[i];
        m_d = dvb_c_diff_enc[(m_d)|(tmp>>6)];
        tmp = (m_d<<4)|(tmp&0x3F);
        dvb_c_transmit_symbol(dvb_c_qam256[tmp]);
    }
}
#define c128enc() { m_d = dvb_c_diff_enc[(m_d)|(tmp>>5)]; \
                    tmp = (m_d<<3)|(tmp&0x1F); \
                    dvb_c_transmit_symbol(dvb_c_qam128[tmp]);}

void dvb_c_128qam( uchar *b, int len )
{
    uchar tmp;
    for( int i = 0; i < len; i++)
    {
        m_b[m_b_count] = b[i];
        m_b_count = (m_b_count + 1)%7;
        if( m_b_count == 0 )
        {
            tmp = (m_b[0]>>1);
            c128enc();
            tmp = ((m_b[0]<<6)&0x7F)|(m_b[1]>>2);
            c128enc();
            tmp = ((m_b[1]<<5)&0x7F)|(m_b[2]>>3);
            c128enc();
            tmp = ((m_b[2]<<4)&0x7F)|(m_b[3]>>4);
            c128enc();
            tmp = ((m_b[3]<<3)&0x7F)|(m_b[4]>>5);
            c128enc();
            tmp = ((m_b[4]<<2)&0x7F)|(m_b[5]>>6);
            c128enc();
            tmp = ((m_b[5]<<1)&0x7F)|(m_b[6]>>7);
            c128enc();
            tmp = (m_b[6]&0x7F);
            c128enc();
        }
    }
}

#define c64enc() { m_d = dvb_c_diff_enc[(m_d)|(tmp>>4)]; \
                    tmp = (m_d<<2)|(tmp&0x0F); \
                    dvb_c_transmit_symbol(dvb_c_qam64[tmp]);}

void dvb_c_64qam( uchar *b, int len )
{
    uchar tmp;
    for( int i = 0; i < len; i++)
    {
        m_b[m_b_count] = b[i];
        m_b_count = (m_b_count + 1)%3;
        if( m_b_count == 0 )
        {
            tmp = m_b[0]>>2;
            c64enc();
            tmp = ((m_b[0]&0x03)<<4)|(m_b[1]>>4);
            c64enc();
            tmp = ((m_b[1]&0x0F)<<2)|(m_b[2]>>6);
            c64enc();
            tmp = m_b[2]&0x3F;
            c64enc();
        }
    }
}

#define c32enc() { m_d = dvb_c_diff_enc[(m_d)|(tmp>>3)]; \
                    tmp = (m_d<<1)|(tmp&0x07); \
                    dvb_c_transmit_symbol(dvb_c_qam32[tmp]);}


void dvb_c_32qam( uchar *b, int len )
{
    uchar tmp;
    for( int i = 0; i < len; i++)
    {
        m_b[m_b_count] = b[i];
        m_b_count = (m_b_count + 1)%5;
        if( m_b_count == 0 )
        {
            tmp = m_b[0]>>3;
            c32enc();
            tmp = ((m_b[0]&0x07)<<2) | (m_b[1]>>6);
            c32enc();
            tmp = ((m_b[1]>>1)&0x1F);
            c32enc();
            tmp = ((m_b[1]<<4)&0x1F)|(m_b[2]>>4);
            c32enc();
            tmp = ((m_b[2]<<1)&0x1F)|(m_b[3]>>7);
            c32enc();
            tmp = ((m_b[3]>>2)&0x1F);
            c32enc();
            tmp = ((m_b[3]<<3)&0x1F)|(m_b[4]>>5);
            c32enc();
            tmp = (m_b[4]&0x1F);
            c32enc();
        }
    }
}
#define c16enc() { m_d = dvb_c_diff_enc[(m_d)|(tmp>>2)]; \
                    tmp = (m_d)|(tmp&0x03); \
                    dvb_c_transmit_symbol(dvb_c_qam16[tmp]);}

void dvb_c_16qam( uchar *b, int len )
{
    uchar tmp;
    for( int i = 0; i < len; i++)
    {
        tmp = b[i]>>4;
        c16enc();
        tmp = b[i]&0x0F;
        c16enc();
    }
}
void dvb_c_encode_and_modulate( uchar *tp )
{
    uchar pkt[DVBS_RS_BLOCK];

    if( m_fc == 0 )
    {
        // Add the inverted sync byte and do the first frame
        // Transport multiplex adaption
        pkt[0] = DVBS_T_ISYNC;
        // Reset the scrambler
        dvb_reset_scrambler();
    }
    else
    {
         pkt[0] = MP_T_SYNC;
    }
    m_fc = (m_fc+1)%DVBS_T_FRAMES_IN_BLOCK;
    // Apply the scrambler
    dvb_scramble_transport_packet( tp, pkt );
    // Reed Solomon Encode the whole frame
    dvb_rs_encode( pkt );
    // Apply the Interleaver
    dvb_convolutional_interleave( pkt );
    // Call the correct DVB-C encoding scheme
    int mode = DVB_C_64QAM_MODE;

    switch(mode)
    {
        case DVB_C_16QAM_MODE:
            dvb_c_16qam( pkt, DVBS_T_CODED_FRAME_LEN );
            break;
        case DVB_C_32QAM_MODE:
            dvb_c_32qam( pkt, DVBS_T_CODED_FRAME_LEN );
            break;
        case DVB_C_64QAM_MODE:
            dvb_c_64qam( pkt, DVBS_T_CODED_FRAME_LEN );
            break;
        case DVB_C_128QAM_MODE:
            dvb_c_128qam( pkt, DVBS_T_CODED_FRAME_LEN );
            break;
        case DVB_C_256QAM_MODE:
            dvb_c_256qam( pkt, DVBS_T_CODED_FRAME_LEN );
            break;
        default:
            break;
    }
}
