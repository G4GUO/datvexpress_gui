#include "dvb.h"
#include "dvb_gen.h"
#include "mp_tp.h"
#include "mp_ts_ids.h"

uchar null_pkt[DVBS_T_CODED_FRAME_LEN];

void null_fmt( void )
{
    int len,i;
    tp_hdr hdr;

    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 0;
    hdr.transport_priority           = 0;
    hdr.pid                          = NULL_PID;
    hdr.transport_scrambling_control = 0;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = 0;
    len = tp_fmt( null_pkt, &hdr );
    // PAD out the unused bytes
    for( i = len; i < TP_LEN; i++ )
    {
        null_pkt[i] = 0xFF;
    }
}
//
// Send a NULL transport packet via dvb encoder
// NULLS are only used with DVB-S and DVB-T
void padding_null_dvb( void )
{
    update_cont_counter( null_pkt );
    increment_null_count();
    tx_write_transport_queue( null_pkt );
}
uchar *get_padding_null_dvb( void )
{
    update_cont_counter( null_pkt );
    increment_null_count();
    return null_pkt;
}
