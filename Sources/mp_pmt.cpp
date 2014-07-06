#include <stdio.h>
#include "mp_ts_ids.h"
#include "dvb.h"
#include "dvb_gen.h"
#include "mp_tp.h"
//
// This module formats and sends PMT tables
//
static uchar m_pmt_seq;
static uchar pmt_pkt[DVBS_T_CODED_FRAME_LEN];

int tp_pmt_add_section( uchar *b, tp_pmt_section *s )
{
    int len  = 0;
    int l_field;
    b[len++] = s->stream_type;
    b[len]   = 0xE0;
    b[len++] |= s->elementary_pid>>8;
    b[len++]  = s->elementary_pid&0xFF;
    l_field = 3;//Start of length filed
    len    += 2;//Skip over length
    // Fill in the descriptors
    for( int i = 0; i < s->nr_descriptors; i++ )
    {
        len += add_si_descriptor( &b[len], &s->descriptor[i] );
    }
    // Fill in the ES info length field
    b[l_field]   = (0xF0) | (len - 5)>>8;
    b[l_field+1] = (len - 5)&0xFF;
    return len;
}

int tp_pmt_fmt( uchar *b, tp_pmt *p )
{
   int len;

    b[0] = 0x02;
    b[1] = 0x30;
    if( p->section_syntax_indicator ) b[1] |= 0x80;
    b[3]  =  (p->program_number>>8);
    b[4]  = (p->program_number&0xFF);
    b[5]  = 0xC0;
    b[5]  |= (p->version_number<<1);
    if(p->current_next_indicator) b[5] |= 0x01;
    b[6] = p->section_number;
    b[7] = p->last_section_number;

    // PCR PID
    b[8] = 0xE0;
    b[8] |= (p->pcr_pid>>8);
    b[9] = (p->pcr_pid&0xFF);

    // Program info length = 0
    b[10] = 0xF0;
    b[11] = 0x00;

    len = 12;

    for( int i = 0; i < p->nr_elementary_streams; i++ )
    {
        len += tp_pmt_add_section( &b[len], &p->stream[i] );
    }

    // Add the length field
    b[1] |= ((len+1)>>8);
    b[2]  = ((len+1)&0xFF);
    len = crc32_add( b, len );
    return len;
}
void pmt_fmt( int video_stream_type, int audio_stream_type )
{
    int len,i;
    tp_hdr hdr;
    tp_pmt pmt;

    sys_config info;
    dvb_config_get( &info );

    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 1;
    hdr.transport_priority           = 0;
    hdr.pid                          = info.pmt_pid;//PAT table pid
    hdr.transport_scrambling_control = 0;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = m_pmt_seq = 0;
    len = tp_fmt( pmt_pkt, &hdr );

    pmt_pkt[len++]               = 0; // Table starts immediately

    pmt.section_syntax_indicator = 1;
    pmt.program_number           = info.program_nr;
    pmt.version_number           = 2;
    pmt.current_next_indicator   = 1;
    pmt.section_number           = 0;
    pmt.last_section_number      = 0;

    pmt.pcr_pid                  = info.pcr_pid;
    pmt.nr_elementary_streams    = 2;

    pmt.stream[0].stream_type    = video_stream_type;
    pmt.stream[0].elementary_pid = info.video_pid;
    pmt.stream[0].nr_descriptors = 0;

    pmt.stream[1].stream_type    = audio_stream_type;
    pmt.stream[1].elementary_pid = info.audio_pid;
    pmt.stream[1].nr_descriptors = 0;

/*
    if( info.ebu_data_enabled )
    {
        pmt.stream[2].stream_type    = 0x06; // EBU private data
        pmt.stream[2].elementary_pid = info.ebu_data_pid;
        pmt.stream[2].nr_descriptors = 1;
        pmt.stream[2].descriptor[0].tag          = SI_DESC_TELETEXT;
        pmt.stream[2].descriptor[0].ttd.nr_items = 0;
        pmt.nr_elementary_streams    = 3;
    }
*/
/*
    // Video information
    pmt.desc[0].table_id                       = 0x02;
    pmt.desc[0].video.multiple_frame_rate_flag = 0;
    pmt.desc[0].video.frame_rate_code          = 3; // 25 frames per sec
    pmt.desc[0].video.mpeg_1_only_flag         = 0;
    pmt.desc[0].video.constrained_parameter_flag = 0;
    pmt.desc[0].video.still_picture_flag       = 0;
    pmt.desc[0].video.profile_and_level_indication = 0xC8;// MP@ML esc = 1
    pmt.desc[0].video.chroma_format             = 2;// 4:2:2, 4:2:0 == 1
    pmt.desc[0].video.frame_rate_extension_flag = 1;//not sure

    // Audio information
    pmt.desc[1].table_id                       = 0x03;
    pmt.desc[1].audio.free_format_flag         = 1;//dont know
    pmt.desc[1].audio.id                       = 0;// don't know
    pmt.desc[1].audio.layer                    = 0;//don't know
    pmt.desc[1].audio.variable_rate_audio_indicator = 0;
*/
    len+= tp_pmt_fmt( &pmt_pkt[len], &pmt );

    // PAD out the unused bytes
    for( i = len; i < TP_LEN; i++ )
    {
            pmt_pkt[i] = 0xFF;
    }

}

void pmt_fmt( void )
{
    sys_config cfg;
    dvb_config_get( &cfg );


    if( cfg.video_codec_class == CODEC_MPEG2 )
    {
        //ISO 13818-2 Video (02)
        //ISO 13818-3 Audio (04)
        //ISO 11172-3 Audio (03)
        pmt_fmt( 0x02, 0x04 );
    }

    if( cfg.video_codec_class == CODEC_MPEG4 )
    {
        //ISO 13818-7 Video MPEG4
        //ISO 14496-3 Audio LATM/LAOS AAC?
        pmt_fmt( 0x10, 0x11 );
    }
}
//
// Send a PMT transport packet via dvb encoder
//
void pmt_dvb( void )
{
    ts_write_transport_queue( pmt_pkt );
    update_cont_counter( pmt_pkt );
}

