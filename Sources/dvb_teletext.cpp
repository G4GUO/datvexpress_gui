#include <stdio.h>
#include <memory.h>
#include <stdlib.h>
//#include <boost/program_options.hpp>
//#include <boost/format.hpp>
#include <iostream>
#include "dvb.h"
#include "mp_tp.h"
#include "mp_ts_ids.h"

uchar m_seq_count;
uchar *m_buffer;
int m_buffer_length;
int m_pid;
int m_active;

int tt_pts_encode( uchar *b )
{
    b[0]=0x21;
    b[1]=0x00;
    b[2]=0x01;
    b[3]=0x00;
    b[4]=0x01;
    return 5;
}
//
// Take the encoded teletext blocks and turn them into a PES packet
//
int tt_pes_encode( uchar *b, teletext_pes_data_field *tt )
{
    int len = 0;
    int length_field;
    b[len++]  = 0x00;
    b[len++]  = 0x00;
    b[len++]  = 0x01;
    b[len++]  = 0xBD;//Private stream 1
    length_field = len;
    b[len++]  = 0x00;// Will be filled in later
    b[len++]  = 0x00;
    b[len]    = 0xC0;//reserved
    b[len]   |= 0<<4;//scrambling control
    b[len]   |= 0<<3;//PES priority
    b[len]   |= 1<<2;//Data alignment indicator
    b[len]   |= 0<<1;//Copyright
    b[len++] |= 0x01;//Original
    b[len]    = 2<<6;//PTS_DTS_flag
    b[len]   |= 0<<5;//ESCR_flag
    b[len]   |= 0<<4;//ES_rate_flag
    b[len]   |= 0<<3;//DSM_trick_mode_flag
    b[len]   |= 0<<2;//Additional copy info flag
    b[len]   |= 0<<1;//PES_CRC_flag
    b[len++] |= 0;   //PES_extension_flag
    b[len++] = 0x00; //Header length
    b[len++] = 0x24;
    memset( &b[len], 0xFF, 0x24 );// Stuff all
    tt_pts_encode( &b[len] );
    len += 0x24;
    // Add the teletext information Private stream 1
    b[len++] = 0x10;//EBU data EN 300 472 (teletext)
    for( int i = 0; i < tt->nr_fields; i++ )
    {
        b[len++]  = tt->field[i].data_unit_id;
        int pos = len;
        b[len++]  = 0;//length
        b[len]    = 0xC0;
        b[len]   |= tt->field[i].field_parity<<5;
        b[len++] |= tt->field[i].line_offset;
        b[len++] = tt->field[i].framing_code;
        b[len++] = tt->field[i].magazine_address>>8;
        b[len++] = tt->field[i].magazine_address&0xFF;
        memcpy( &b[len], tt->field[i].data_block, 40 );
        len += 40;
        b[pos] = len - pos;
    }
    // Set the length field
    int length = len - 4;
    b[length_field] = length>>8;
    b[length_field+1] = length&0xFF;

    return length;
}
//
// Take an array of teletext blocks and encode them
//
void tt_text_encode( void )
{
    if( m_active )
    {
        teletext_pes_data_field t;
        t.nr_fields = 1;
        t.field[0].data_unit_id = 0x02;
        t.field[0].field_parity = 0;
        t.field[0].line_offset  = 0;
        t.field[0].framing_code = 0xE4;
        t.field[0].magazine_address = 0xC00D;
        sprintf((char*)t.field[0].data_block,"%c%c%c    Hello teletext",0x03,0x1d,0x01);
        int len =  tt_pes_encode( m_buffer, &t );
        f_send_pes_seq( m_buffer, len, m_pid, m_seq_count );
    }
}

void dvb_send_teletext_file( void )
{
    if( m_active )
    {
        f_send_pes_seq( m_buffer, m_buffer_length, m_pid, m_seq_count );
    }
}
void dvb_teletext_init( void )
{
    FILE *fp;
    sys_config info;
    dvb_config_get( &info );
    m_buffer = (uchar*)malloc(4096);
//    m_pid = info.ebu_data_pid;
    m_seq_count = 0;

    m_active = 0;

//    if( info.ebu_data_enabled )
    {
//        if((fp=(fopen(info.ebu_teletext_file_name,"r"))))
        {
            m_buffer_length = fread(m_buffer,4096,1,fp);
            m_active = 1;
        }
//        else
        {
            //std::cout << boost::format("Cannot open Teletext File : %s") % info.ebu_teletext_file_name << std::endl;
        }
    }
}
