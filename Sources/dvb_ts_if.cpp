//
// This modules processes input bitstreams that are encoded in
// transport packet format.
//
#include <stdio.h>
#include "dvb.h"
#include "dvb_gen.h"
#include "dvb_config.h"
#include "mp_ts_ids.h"
#include "mp_tp.h"
#include "dvb_capture_ctl.h"

#include <iostream>

static ts_info t_info;
static pat_info p_info[10];
static tp_pmt pmt;

void dvb_video_tp( uchar *tp )
{
    tp[1] = (tp[1]&0xE0) | (t_info.required_video_id>>8);
    tp[2] = t_info.required_video_id&0xFF;
    ts_write_transport_queue( tp );
}

void dvb_audio_tp( uchar *tp )
{
    tp[1] = (tp[1]&0xE0) | (t_info.required_audio_id>>8);
    tp[2] = t_info.required_audio_id&0xFF;
    ts_write_transport_queue( tp );
}

void dvb_pcr_tp( uchar *tp )
{
    tp[1] = (tp[1]&0xE0) | (t_info.required_pcr_id>>8);
    tp[2] = t_info.required_pcr_id&0xFF;
    ts_write_transport_queue( tp );
}

void dvb_parse_pmt( uchar *b )
{
     int n = 1;
     pmt.section_syntax_indicator = b[n]>>7;
     int len = (b[n++]&0x0F)<<4;
     len |= b[n++];
     pmt.program_number = b[n++];
     pmt.program_number <<= 8;
     pmt.program_number |= b[n++];

     pmt.version_number = (b[n]>>1)&0x1F;
     pmt.current_next_indicator = b[n++]&0x01;
     pmt.section_number = b[n++];
     pmt.last_section_number = b[n++];

     pmt.pcr_pid   = b[n++]&0x1F;
     pmt.pcr_pid <<= 8;
     pmt.pcr_pid  |= b[n++];

     int pi_len = b[n++]&0x0F;
     pi_len <<= 8;
     pi_len |= b[n++];
     for( int i = 0; i < pi_len; i++ ) n++;
     int nr_streams = 0;
     while( n < (len - 4))
     {
         pmt.stream[nr_streams].stream_type = b[n++];
         pmt.stream[nr_streams].elementary_pid = b[n++]&0x1F;
         pmt.stream[nr_streams].elementary_pid <<= 8;
         pmt.stream[nr_streams].elementary_pid |= b[n++];
         int es_len = b[n++]&0x0F;
         es_len <<= 8;
         es_len |= b[n++];
         for( int i = 0; i < es_len; i++ ) n++;
         nr_streams++;
     }
     // finished parsing now use the information
     for( int i = 0; i < nr_streams; i++ )
     {
         if( pmt.stream[i].stream_type == 0x02 )// ISO 13818-2 Video
         {
            t_info.video_detected = 1;
            t_info.video_type     = pmt.stream[i].stream_type;
            t_info.video_id       = pmt.stream[i].elementary_pid;
         }
         if( pmt.stream[i].stream_type == 0x1B ) // H.264 Video
         {
             t_info.video_detected = 1;
             t_info.video_type     = pmt.stream[i].stream_type;
             t_info.video_id       = pmt.stream[i].elementary_pid;
         }
         if( pmt.stream[i].stream_type == 0x03 ) // ISO 11172-3 Audio
         {
             t_info.audio_detected = 1;
             t_info.audio_type     = pmt.stream[i].stream_type;
             t_info.audio_id       = pmt.stream[i].elementary_pid;
         }
         if( pmt.stream[i].stream_type == 0x0F ) // ISO 13818-7 Audio ADTS syntax
         {
             t_info.audio_detected = 1;
             t_info.audio_type     = pmt.stream[i].stream_type;
             t_info.audio_id       = pmt.stream[i].elementary_pid;
         }
         if( pmt.stream[i].stream_type == 0x06 ) // EBU private data
         {

         }
         //loggerf( "Stream type = %d %x\n",pmt.stream[i].stream_type,pmt.stream[i].stream_type);
     }
     if( t_info.video_detected && t_info.audio_detected)
     {
         pmt_fmt( t_info.video_type, t_info.audio_type );
     }
}
//
// We need to find the id of the video and audio and then process them
//
void dvb_ts_if( uchar *b )
{
    uint id = b[1]&0x1F;
    id <<= 8;
    id |= b[2];

    if( t_info.video_detected )
    {
        if( id == t_info.video_id )
        {
            dvb_video_tp( b );
            cap_video_present();
            return;
        }
    }

    if( t_info.audio_detected )
    {
        if( id == t_info.audio_id )
        {
            dvb_audio_tp( b );
            cap_audio_present();
            return;
        }
    }

    if( t_info.pmt_parsed )
    {
        // Add re-constructed SI packets
        ts_single_stream();
        ts_multi_stream();

        if( id == t_info.pcr_id )
        {
            dvb_pcr_tp( b );
            return;
        }
    }

    if( t_info.pat_detected == 0 )
    {
        if( id == PAT_PID )
        {
            // PAT detected find id of PMT
            int n = 13;
            t_info.pat_detected = 1;
            for( int i = 0; i < 2; i++ )
            {
                p_info[i].pgm_nr    = b[n++];
                p_info[i].pgm_nr  <<= 8;
                p_info[i].pgm_nr   |= b[n++];

                p_info[i].pgm_id    = b[n++]&0x1F;
                p_info[i].pgm_id  <<= 8;
                p_info[i].pgm_id   |= b[n++];

                if( p_info[i].pgm_nr != 0 )
                {
                    t_info.pmt_detected = 1;
                    t_info.pmt_id = p_info[i].pgm_id;
                }
            }
            return;
        }
   }

   if((t_info.pmt_detected)&&(id == t_info.pmt_id)&&(t_info.pmt_parsed == 0))
   {
       // We have found the PMT table
       //printf("PMT detected\n");
       dvb_parse_pmt( &b[5] );
       t_info.pmt_parsed   = 1;
       t_info.pcr_id       = pmt.pcr_pid;
       //printf("PCR detected %d\n",pmt.pcr_pid);
       return;
   }
}
void dvb_ts_if_init( void )
{
    sys_config info;
    dvb_config_get( &info );
    t_info.pat_detected      = 0;
    t_info.pmt_detected      = 0;
    t_info.pmt_parsed        = 0;
    t_info.video_detected    = 0;
    t_info.audio_detected    = 0;
    t_info.video_id          = 0;
    t_info.audio_id          = 0;
    t_info.pcr_id            = 0;
    t_info.required_video_id = info.video_pid;
    t_info.required_audio_id = info.audio_pid;
    t_info.required_pcr_id   = info.pcr_pid;
}
