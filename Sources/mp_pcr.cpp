#include <stdio.h>
#include <stdlib.h>
#include "dvb.h"
#include "dvb_gen.h"
#include "mp_ts_ids.h"
#include "mp_tp.h"
//
// This module formats and sends PCR packets
// The PCR field is sent in the adaption field and can be added to any
// transport frame.

uchar m_pcr_seq;
uchar pcr_pkt[DVBS_T_CODED_FRAME_LEN+10];

int adaption_fmt( uchar *b, tp_adaption *a )
{
    b[0] = 0;

    if(a->discontinuity_ind)     b[0] |= 0x80;
    if(a->random_access_ind)     b[0] |= 0x40;
    if(a->elem_stream_pr_ind)    b[0] |= 0x20;
    if(a->PCR_flag)              b[0] |= 0x10;
    if(a->OPCR_flag)             b[0] |= 0x08;
    if(a->splicing_point_flag)   b[0] |= 0x04;
    if(a->trans_priv_data_flag)  b[0] |= 0x02;
    if(a->adapt_field_extn_flag) b[0] |= 0x01;

    return 1;
}
int pcr_fmt( uchar *b, int stuff )
{
    int len;
    tp_adaption apt;

    len = 1;//length field

    apt.discontinuity_ind     = 0;
    apt.random_access_ind     = 0;
    apt.elem_stream_pr_ind    = 0;
    apt.PCR_flag              = 1;
    apt.OPCR_flag             = 0;
    apt.splicing_point_flag   = 0;
    apt.trans_priv_data_flag  = 0;
    apt.adapt_field_extn_flag = 0;

    // First byte will contain the adaption field length
    len += adaption_fmt( &b[len], &apt );

    // Now add the PCR field
    len += add_pcr_field( &b[len] );

    // PAD out the stuff bytes
    for( int i = 0; i < stuff; i++ )
    {
         b[len++] = 0xFF;
    }
    b[0] = len-1;// Encode the length of the adaption field
    return len;
}
//
// This inspects a transport packet to see whether a PCR field
// is present and if it is re-timestamps it
//
void pcr_timestamp( uchar *b )
{
    if((b[3]&0xC0)==0xC0)
    {
        if(b[5]&0x10)
        {
            // PCR field present
            add_pcr_field( &b[6] );
        }
    }
}
