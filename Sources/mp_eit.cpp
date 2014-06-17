#include <stdio.h>
#include <time.h>
#include <string.h>
#include "mp_ts_ids.h"
#include "dvb.h"
#include "dvb_si.h"
#include "dvb_gen.h"
#include "mp_tp.h"
#include "mp_si_desc.h"

tp_si_seq m_pkts;

int eit_time_fmt( uchar *b )
{
    dvb_si_time st;
    dvb_si_system_time( &st );
    // Round to the start of the last hour
    st.minute = 0;
    st.second = 0;
    return dvb_si_add_time( b, &st);
}

int f_es_info( uchar *b, eis_info *p )
{
    int len,i,pos,start;
    b[0] = p->event_id>>8;
    b[1] = p->event_id&0xFF;
    eit_time_fmt( &b[2] );
    dvb_si_add_duration( &b[7], &p->duration );
    b[10] = (p->running_status<<5);
    if( p->free_ca_mode ) b[10] |= 0x10;
    pos   = 10;
    len   = 12;
    start = len;
    for( i = 0; i < p->nr_descriptors; i++ )
    {
        len += add_si_descriptor( &b[len], &p->descriptors[i] );
    }
    b[pos++] |= (len-start)>>8;
    b[pos++]  = (len-start)&0xFF;
    return len;
}

int f_eit( uchar *b, event_information_section *p )
{	
    int i,len;
    b[0] = 0x4E;// Actual TS
    b[1] = 0;
    if( p->section_syntax_indicator ) b[1] |= 0x80;
    b[1] |= 0x40;
    b[1] |= 0x30;
    // section length 16 bits
    b[3]  = p->service_id>>8;
    b[4]  = p->service_id&0xFF;
    b[5]  = 0xC0;
    b[5] |= p->version_number<<1;
    if( p->current_next_indicator ) b[5] |= 0x01;
    b[6]  = p->section_number;
    b[7]  = p->last_section_number;
    b[8]  = p->transport_stream_id>>8;
    b[9]  = p->transport_stream_id&0xFF;
    b[10] = p->original_network_id>>8;
    b[11] = p->original_network_id&0xFF;
    b[12] = p->segment_last_section_number;
    b[13] = p->last_table_id;
    len   = 14;
    // Add Network descriptors
    for( i = 0; i <= p->segment_last_section_number; i++ )
    {
        len += f_es_info( &b[len], &p->section[i] );
    }
    // length from the field after the length and including the CRC
    b[1]  |= ((len+1)>>8);
    b[2]   = (len+1)&0xFF;
    len = crc32_add( b, len );

    return len;
}
//
// Format the header and payload of the EIT packet
//
void eit_fmt( void )
{
    event_information_section eis;
    uchar b[5000];

    sys_config info;
    dvb_config_get( &info );

    m_pkts.pid = EIT_PID;
    // Add the payload
    eis.section_syntax_indicator  = 1;
    eis.service_id                = info.service_id;
    eis.version_number            = 2;
    eis.current_next_indicator    = 1;
    eis.section_number            = 0;
    eis.last_section_number       = 0;
    eis.transport_stream_id       = info.stream_id;
    eis.original_network_id       = info.network_id;
    eis.segment_last_section_number = 0;
    eis.last_table_id             = 0x4E;


    // Get the sytem time and round to the last hour
    dvb_si_system_time( &eis.section[0].start_time );

    eis.section[0].event_id           = 0;
    eis.section[0].start_time.minute  = 0;
    eis.section[0].start_time.second  = 0;

    eis.section[0].duration.hour      = info.event.event_duration/60;
    eis.section[0].duration.minute    = info.event.event_duration%60;
    eis.section[0].duration.second    = 0;

    eis.section[0].running_status     = 0;
    eis.section[0].free_ca_mode       = 0;
    eis.section[0].nr_descriptors     = 2;

    // Item descriptor
    eis.section[0].descriptors[0].tag            = SI_DESC_SE;
    eis.section[0].descriptors[0].sctd.nr_items  = 1;
    eis.section[0].descriptors[0].sed.iso_639_language_code = ('e'<<16) | ('n'<<8) | ('g');
    eis.section[0].descriptors[0].sed.event_name = info.event.event_title;
    eis.section[0].descriptors[0].sed.event_text = info.event.event_text;

    // Content descriptor
    eis.section[0].descriptors[1].tag            = SI_DESC_CONTENT;
    eis.section[0].descriptors[1].sctd.nr_items  = 2;
    eis.section[0].descriptors[1].sctd.item[0].l1_nibble = 0xA;//Leisure Hobbies
    eis.section[0].descriptors[1].sctd.item[0].l2_nibble = 0x0;
    eis.section[0].descriptors[1].sctd.item[0].u1_nibble = 0x0;
    eis.section[0].descriptors[1].sctd.item[0].u2_nibble = 0x0;

    eis.section[0].descriptors[1].sctd.item[1].l1_nibble = 0x9;//Technology
    eis.section[0].descriptors[1].sctd.item[1].l2_nibble = 0x2;
    eis.section[0].descriptors[1].sctd.item[1].u1_nibble = 0x0;
    eis.section[0].descriptors[1].sctd.item[1].u2_nibble = 0x0;


    // Create the sequence of packets
    m_pkts.ibuff_len = f_eit( &b[0], &eis );
    m_pkts.inbuff    = b;
    m_pkts.seq_count = 0;
    f_create_si_seq( &m_pkts );
}
void eit_dvb( void )
{
    for( int i = 0; i < m_pkts.nr_frames; i++)
    {
        // Update the sequence count field for this packet
        set_cont_counter( m_pkts.frames[i], m_pkts.seq_count );
        // Send the packet
        tx_write_transport_queue( m_pkts.frames[i] );
        // Update the sequence count
        m_pkts.seq_count = (m_pkts.seq_count+1)&0x0F;
    }
}
