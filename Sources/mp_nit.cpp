#include <stdio.h>
#include <string.h>
#include "mp_ts_ids.h"
#include "dvb.h"
#include "dvb_gen.h"
#include "mp_tp.h"
#include "mp_si_desc.h"

uchar nit_pkt[DVBS_T_CODED_FRAME_LEN];

int f_ts_info( uchar *b, transport_stream_info *p )
{
    int len,i,pos,start;
    b[0] = p->transport_stream_id>>8;
    b[1] = p->transport_stream_id&0xFF;
    b[2] = p->original_network_id>>8;
    b[3] = p->original_network_id&0xFF;
    b[4] = 0xF0;
    pos   = 4;
    len   = 6;
    start = len;
    for( i = 0; i < p->nr_descriptors; i++ )
    {
        len += add_si_descriptor( &b[len], &p->desc[i] );
    }
    b[pos++] |= (len-start)>>8;
    b[pos++]  = (len-start)&0xFF;
    return len;
}
int f_nit( uchar *b, network_information_section *p )
{	
    int i,len,pos,start;
    b[0] = 0x40;
    b[1] = 0;
    if( p->section_syntax_indicator ) b[1] |= 0x80;
    b[1] |= 0x40;
    b[1] |= 0x30;
    b[3] = p->network_id>>8;
    b[4] = p->network_id&0xFF;
    b[5] = 0xC0;
    b[5] |= p->version_number<<1;
    if( p->current_next_indicator ) b[5] |= 0x01;
    b[6]  = p->section_number;
    b[7]  = p->last_section_number;
    b[8]  = 0xF0;
    pos   = 8;
    start = 10;
    len   = 10;
    // Add Network descriptors
    for( i = 0; i < p->nr_network_descriptors; i++ )
    {
        len += add_si_descriptor( &b[len], &p->n_desc[i] );
    }
    // Update the length field for the network descriptors
    b[pos++] |= (len-start)>>8;
    b[pos++]  = (len-start)&0xFF;
    // This is where the transport descriptors length will be
    b[len] = 0xF0;
    pos = len;
    len += 2;
    start = len;
    // Add transport descriptors
    for( i = 0; i < p->nr_transport_streams; i++ )
    {
        len += f_ts_info( &b[len], &p->ts_info[i] );
    }
    // Update the length field for the transport descriptors
    b[pos++] |= (len-start)>>8;
    b[pos++]  = (len-start)&0xFF;
    // length from the field after the length and including the CRC
    b[1]  |= ((len-3+CRC_32_LEN)>>8);
    b[2]   = (len-3+CRC_32_LEN)&0xFF;
    len = crc32_add( b, len );

    return len;
}
//
// Format the header and payload of the NIT packet
//
void nit_fmt( void )
{
    int i,len;
    tp_hdr hdr;
    network_information_section nis;
    sys_config info;
    dvb_config_get( &info );

    hdr.transport_error_indicator    = TRANSPORT_ERROR_FALSE;
    hdr.payload_unit_start_indicator = PAYLOAD_START_TRUE;
    hdr.transport_priority           = TRANSPORT_PRIORITY_LOW;
    hdr.pid                          = info.nit_pid;
    hdr.transport_scrambling_control = SCRAMBLING_OFF;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = 0;
    len = tp_fmt( nit_pkt, &hdr );

    // Payload start immediately
    nit_pkt[len++] = 0;

    // Add the payload
    nis.section_syntax_indicator  = 1;
    nis.network_id                = info.network_id;
    nis.version_number            = 2;
    nis.current_next_indicator    = 1;
    nis.section_number            = 0;
    nis.last_section_number       = 0;
    nis.nr_network_descriptors    = 1;

    // First Network descriptor
    nis.n_desc[0].tag = SI_DESC_NET_NAME;
    memcpy(nis.n_desc[0].nnd.name,"Amateur Television",18);
    nis.n_desc[0].nnd.name_length = 18;

    // Transport information
    nis.nr_transport_streams      = 1;

    // First transport stream
    nis.ts_info[0].transport_stream_id = info.stream_id;
    nis.ts_info[0].original_network_id = info.network_id;
    nis.ts_info[0].nr_descriptors      = 1;

    // Transport stream descriptors
    nis.ts_info[0].desc[0].sld.tag                   = SI_DESC_SVC_LST;
    nis.ts_info[0].desc[0].sld.table_length          = 1;
    nis.ts_info[0].desc[0].sld.entry[0].service_id   = info.service_id;
    nis.ts_info[0].desc[0].sld.entry[0].service_type = SVC_DIGITAL_TV;
    len += f_nit( &nit_pkt[len], &nis );

    // PAD out the unused bytes
    for( i = len; i < TP_LEN; i++ )
    {
        nit_pkt[i] = 0xFF;
    }
}
void nit_dvb( void )
{
    ts_write_transport_queue( nit_pkt );
    update_cont_counter( nit_pkt );
}
