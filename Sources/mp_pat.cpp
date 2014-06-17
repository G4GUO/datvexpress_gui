#include <stdio.h>
#include <stdlib.h>
#include "dvb.h"
#include "dvb_gen.h"
#include "mp_ts_ids.h"
#include "mp_tp.h"
//
// This module formats and sends PAT tables
//
static uchar m_pat_seq;
static uchar pat_pkt[DVBS_T_CODED_FRAME_LEN];

int tp_pat_fmt( uchar *b, tp_pat *p )
{
    int i,len;

    b[0] = 0x00;
    b[1] = 0;
    if( p->section_syntax_indicator ) b[1] |= 0x80;
    b[1] |= 0x30;

    b[3]  = (p->transport_stream_id>>8);
    b[4]  = (p->transport_stream_id&0xFF);
    b[5]  = 0xC0;
    b[5] |= (p->version_number<<1);
    if(p->current_next_indicator) b[5] |= 0x01;
    b[6]  = p->section_number;
    b[7]  = p->last_section_number;
    len = 8;
    for( i = 0; i < p->nr_table_entries; i++ )
    {
        b[len++] = p->entry[i].program_number>>8;
        b[len++] = p->entry[i].program_number&0xFF;
        b[len] = 0xE0;
        b[len++] |= p->entry[i].pid>>8;
        b[len++]  = p->entry[i].pid&0xFF;
    }
    // length from the field after the length and including the CRC
    b[1]  |= ((len+1)>>8);
    b[2]   = (len+1)&0xFF;
    len = crc32_add( b, len );

    return len;
}
void pat_fmt( void )
{
    int i,len;
    tp_hdr hdr;
    tp_pat pat;

    sys_config info;
    dvb_config_get( &info );

    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 1;
    hdr.transport_priority           = 0;
    hdr.pid                          = PAT_PID;//PAT table pid
    hdr.transport_scrambling_control = 0;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = m_pat_seq = 0;
    len = tp_fmt( pat_pkt, &hdr );

    // Tables follow immediately
    pat_pkt[len++]                   = 0;

    // Add the pat table
    pat.section_syntax_indicator = 1;
    pat.transport_stream_id      = info.stream_id;
    pat.version_number           = 2;
    pat.current_next_indicator   = 1;
    pat.section_number           = 0;
    pat.last_section_number      = 0;
    pat.nr_table_entries         = 2;
    // Entries
    pat.entry[0].program_number  = 0;
    pat.entry[0].pid             = info.nit_pid;//default value

    pat.entry[1].program_number  = info.program_nr;// Only one program
    pat.entry[1].pid             = info.pmt_pid;


    // format
    len += tp_pat_fmt( &pat_pkt[len], &pat );
    // PAD out the unused bytes
    for( i = len; i < TP_LEN; i++ )
    {
        pat_pkt[i] = 0xFF;
    }
}
//
// Send a PAT transport packet via dvb encoder
//
void pat_dvb( void )
{
    tx_write_transport_queue( pat_pkt );
    update_cont_counter( pat_pkt );
}

