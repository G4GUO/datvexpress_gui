//
// Format and send a Transport packet
//
#include <memory.h>
#include <stdio.h>
#include "dvb.h"
#include "mp_tp.h"

uchar m_tp_pkt[DVBS_T_CODED_FRAME_LEN];

//
// Format a transport packet
//
int tp_fmt( uchar *b, tp_hdr *hdr )
{
        int len;

        b[0] = TP_SYNC;
        b[1] = 0;
        if( hdr->transport_error_indicator)     b[1] |= 0x80;
        if( hdr->payload_unit_start_indicator ) b[1] |= 0x40;
        if( hdr->transport_priority )           b[1] |= 0x20;
        // Add the 13 bit pid
        b[1] |= (hdr->pid >> 8 );
        b[2]  = (hdr->pid & 0xFF);
        b[3]  = (hdr->transport_scrambling_control << 6);
        b[3] |= (hdr->adaption_field_control << 4);
        b[3] |= (hdr->continuity_counter);
        len = 4;
        if((hdr->adaption_field_control == 0x02)||(hdr->adaption_field_control == 0x03))
        {

        }
        return len;
}
void update_cont_counter( uchar *b )
{
        uchar c;

        c = b[3]&0x0F;
        c = (c+1)&0x0F;
        b[3] = (b[3]&0xF0) | c;
}
void set_cont_counter( uchar *b, uchar c )
{
        b[3] = (b[3]&0xF0) | (c&0x0F);
}
//
// Prepare the start and continuation packets
//
void f_tp_init( void )
{
}
//
// Alter the adaption field flag
// length and add n stuff bytes
void add_pes_stuff_bytes( uchar *b, int n )
{
     int i;

     if( n == 1)
     {
         // only one byte required so set the
         // length field to 0
         b[4] = 0;
     }
     else
     {
         b[4] = n-1;// set length field
         b[5] = 0;// Clear adaption flags
         for( i = 0; i < n-2; i++ )
         {
             b[6+i] = 0xFF;
         }
    }
}

//
// Format and send a Transport packet 188.
// Pass the buffer to send, the length of the buffer
// and the type (audio, video, system)
//
// This is for PES packets in transport streams
//

int f_send_pes_first_tp( uchar *b, int pid, uchar c, bool pcr )
{
    int  len;
    tp_hdr hdr;

    // Header
    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 1;
    hdr.transport_priority           = 0 ;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = 0;
    hdr.continuity_counter           = c;

    if( pcr == true )
    {
        // Adaption and payload (PCR)
        hdr.adaption_field_control  = 0x03;
        len = tp_fmt( m_tp_pkt, &hdr );
        // Add PCR field
        len += pcr_fmt( &m_tp_pkt[len], 0 );
        // Add the payload
        memcpy( &m_tp_pkt[len], b, TP_LEN-len );
    }
    else
    {
        // Payload only
        hdr.adaption_field_control  = 0x01;
        len = tp_fmt( m_tp_pkt, &hdr );
        memcpy( &m_tp_pkt[len], b, TP_LEN-len );
    }
    tx_write_transport_queue_elementary( m_tp_pkt );
    return (TP_LEN-len);
}
int f_send_pes_next_tp( uchar *b, int pid, uchar c, bool pcr )
{
    int  len;
    tp_hdr hdr;

    // Header
    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 0;
    hdr.transport_priority           = 0 ;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = 0;
    hdr.adaption_field_control       = 0x01;
    hdr.continuity_counter           = c;

    if( pcr == true )
    {
        // Adaption and payload (PCR)
        hdr.adaption_field_control  = 0x03;
        len = tp_fmt( m_tp_pkt, &hdr );
        // Add PCR field
        len += pcr_fmt( &m_tp_pkt[len], 0 );
        // Add the payload
        memcpy( &m_tp_pkt[len], b, TP_LEN-len );
    }
    else
    {
        // Payload only
        hdr.adaption_field_control  = 0x01;
        len = tp_fmt( m_tp_pkt, &hdr );
        memcpy( &m_tp_pkt[len], b, TP_LEN-len );
    }
    tx_write_transport_queue_elementary( m_tp_pkt );
    return (TP_LEN-len);
}

int f_send_pes_last_tp( uchar *b, int bytes, int pid, uchar c, bool pcr )
{
    int  len,stuff;
    tp_hdr hdr;

    // Header
    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 0;
    hdr.transport_priority           = 0 ;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = 0;
    hdr.adaption_field_control       = 0x03;//Adaption and payload
    hdr.continuity_counter           = c;

    if( pcr == true )
    {
        // Adaption and payload (PCR)
        hdr.adaption_field_control  = 0x03;
        len = tp_fmt( m_tp_pkt, &hdr );
        // Add PCR field
        len += pcr_fmt( &m_tp_pkt[len], 0 );
    }
    else
    {
        // Payload only
        hdr.adaption_field_control  = 0x01;
        len = tp_fmt( m_tp_pkt, &hdr );
    }
    // Add the stuff
    stuff = TP_LEN-(len+bytes);
    add_pes_stuff_bytes( m_tp_pkt, stuff );
    len += stuff;
    // Add the payload
    memcpy( &m_tp_pkt[len], b, bytes );
    tx_write_transport_queue_elementary( m_tp_pkt );
    return stuff;
}
void f_send_tp_with_adaption_no_payload( int pid, int c )
{
    int  len;
    tp_hdr hdr;
    // Header
    hdr.transport_error_indicator    = 0;
    hdr.payload_unit_start_indicator = 0;
    hdr.transport_priority           = 0 ;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = 0;
    hdr.continuity_counter           = c;
    // Adaption and payload (PCR)
    hdr.adaption_field_control  = 0x02;
    len = tp_fmt( m_tp_pkt, &hdr );
    // Add PCR field and pad it out
    len += pcr_fmt( &m_tp_pkt[len], 176 );
    tx_write_transport_queue_elementary( m_tp_pkt );
}
//
// Send a sequence of PES packets, normally from a file
//
void f_send_pes_seq( uchar *b, int length, int pid, uchar &count )
{
    int payload_remaining;
    int offset;
    // First packet in sequence
    payload_remaining = length;
    offset            = 0;
    f_send_pes_first_tp( b, pid, count, 0 );
    count = (count+1)%16;
    payload_remaining = length - 178;
    offset            += (178);

    while( payload_remaining >= PES_PAYLOAD_LENGTH )
    {
        f_send_pes_next_tp( &b[offset], pid, count, false );
        payload_remaining -= PES_PAYLOAD_LENGTH;
        offset            += PES_PAYLOAD_LENGTH;
        count = (count+1)%16;
    }

    if( payload_remaining > 0 )
    {
        f_send_pes_last_tp( &b[offset], payload_remaining, pid, count, false );
        count = (count+1)%16;
    }
}

//
// Format and send a Transport packet 188.
// Pass the buffer to send, the length of the buffer
//
// This is for SI packets
//

void f_create_si_first( uchar *pkt, uchar *b, int pid, int len )
{
    int  l;
    tp_hdr hdr;

    // Header
    hdr.transport_error_indicator    = TRANSPORT_ERROR_FALSE;
    hdr.payload_unit_start_indicator = PAYLOAD_START_TRUE;
    hdr.transport_priority           = TRANSPORT_PRIORITY_LOW;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = SCRAMBLING_OFF;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = 0;
    l = tp_fmt( pkt, &hdr );
    pkt[l++] = 0;
    if( len > SI_PAYLOAD_LENGTH-1 ) len = SI_PAYLOAD_LENGTH-1;
    memcpy( &pkt[l], b, len );
    l += len;
    for( int i = l; i < TP_LEN; i++ )
    {
        pkt[i] = 0xFF;
    }
}
void f_create_si_next( uchar *pkt, uchar *b, int pid )
{
    int  l;
    tp_hdr hdr;

    // Header
    hdr.transport_error_indicator    = TRANSPORT_ERROR_FALSE;
    hdr.payload_unit_start_indicator = PAYLOAD_START_FALSE;
    hdr.transport_priority           = TRANSPORT_PRIORITY_LOW;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = SCRAMBLING_OFF;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = 0;
    l   = tp_fmt( pkt, &hdr );
    //pkt[l++] = 0;
    memcpy( &pkt[l], b, SI_PAYLOAD_LENGTH );
}

void f_create_si_last( uchar *pkt, uchar *b, int pid, int len )
{
    int  l;
    tp_hdr hdr;

    // Header
    hdr.transport_error_indicator    = TRANSPORT_ERROR_FALSE;
    hdr.payload_unit_start_indicator = PAYLOAD_START_FALSE;
    hdr.transport_priority           = TRANSPORT_PRIORITY_LOW;
    hdr.pid                          = pid;
    hdr.transport_scrambling_control = SCRAMBLING_OFF;
    hdr.adaption_field_control       = ADAPTION_PAYLOAD_ONLY;
    hdr.continuity_counter           = 0;
    l = tp_fmt( pkt, &hdr );
    //pkt[l++] = 0;
    memcpy( &pkt[l], b, len );
    l += len;
    for( int i = l; i < TP_LEN; i++ )
    {
        pkt[i] = 0xFF;
    }
}
//
// Send a sequence of SI packets.
// This allows larger tables i.e for EPG
// The sequence number will be set at transmission time
//
void f_create_si_seq( tp_si_seq *cblk )
{
    int payload_remaining;
    int offset;
    // First packet in sequence
    payload_remaining = cblk->ibuff_len;
    cblk->nr_frames   = 0;
    offset            = 0;
    f_create_si_first( cblk->frames[cblk->nr_frames], &cblk->inbuff[offset], cblk->pid, payload_remaining );
    //cblk->seq_count = 0;
    cblk->nr_frames++;
    payload_remaining -= (SI_PAYLOAD_LENGTH-1);
    offset            += (SI_PAYLOAD_LENGTH-1);

    while( payload_remaining >= SI_PAYLOAD_LENGTH )
    {
        f_create_si_next( cblk->frames[cblk->nr_frames], &cblk->inbuff[offset], cblk->pid );
        cblk->nr_frames++;
        payload_remaining -= SI_PAYLOAD_LENGTH;
        offset            += SI_PAYLOAD_LENGTH;
    }

    if( payload_remaining > 0 )
    {
        f_create_si_last( cblk->frames[cblk->nr_frames], &cblk->inbuff[offset], cblk->pid, payload_remaining );
        cblk->nr_frames++;
        payload_remaining -= SI_PAYLOAD_LENGTH;
    }
}

