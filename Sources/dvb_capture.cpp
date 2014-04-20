#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <semaphore.h>
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#ifdef _USE_SW_CODECS
#include <alsa/asoundlib.h>
#endif
#include "mp_tp.h"
#include "mp_ts_ids.h"
#include "dvb.h"
#include "dvb_capture_ctl.h"
//#include "Transcoder.h"
#include "an_capture.h"

extern int m_i_fd;
#ifdef _USE_SW_CODECS
extern snd_pcm_t *m_audio_handle;
#endif
extern sem_t capture_sem;
extern int m_video_seq;
extern int m_audio_seq;

uchar m_b[2048];

//extern Transcoder transcoder;
static int m_video_present;
static int m_audio_present;

#define VIDEO_B_SIZE 2000
static VideoBuffer m_vbuf[VIDEO_B_SIZE];
static int m_vbp;

VideoBuffer *video_delay(uchar *h, int l)
{
    static int delay = 40;
    // Save the header
    memcpy( m_vbuf[m_vbp].b, h, 6);
    m_vbuf[m_vbp].l = l + 6;
    // Save the body
    cap_rd_bytes( &m_vbuf[m_vbp].b[6], l );
    // Increment the pointer
    m_vbp = (m_vbp + 1)%VIDEO_B_SIZE;
    // Return a delayed version
    return &m_vbuf[(m_vbp+(VIDEO_B_SIZE-delay))%VIDEO_B_SIZE];
}

//
// This buffers the input stream
//
#define CAP_BUFF_LEN 64
uchar cap_buffer[CAP_BUFF_LEN*2];
int cap_offset;
int buffered_read( int fd, uchar *b, int len )
{
    for( int i = 0; i < len; i++ )
    {
        b[i] = cap_buffer[cap_offset++];
        if( cap_offset == CAP_BUFF_LEN)
        {
            read( fd, cap_buffer, CAP_BUFF_LEN );
        }
        if( cap_offset == (CAP_BUFF_LEN*2))
        {
            read( fd, &cap_buffer[CAP_BUFF_LEN], CAP_BUFF_LEN );
            cap_offset = 0;
        }
    }
    return len;
}

void cap_purge(void)
{
    lseek(m_i_fd,0,SEEK_END);
}

//
// read until we have the correct number of bytes
//
void cap_rd_bytes( uchar *b, int len )
{
    int bytes;
    int offset = 0;
    int req    = len;

    if( len == 0 ) return;
    int flag = 1;

    // Need to know the source of the video/audio
    sys_config info;
    dvb_config_get( &info );

    while( flag )
    {
        if(info.capture_device_type == DVB_UDP_TS)
        {
            bytes=udp_read( &b[offset], req  );
            dvb_cap_check_for_update();
        }
        else
        {
            if( m_i_fd > 0 )
            {
                dvb_get_cap_sem();
//               bytes = buffered_read( m_i_fd, &b[offset], req );
                bytes=read( m_i_fd, &b[offset], req );
                dvb_release_cap_sem();

                if( bytes > 0 )
                {
                    offset += bytes;
                    if( offset == len )
                        flag = 0;
                    else
                        req = len - offset;
                }
            }
        }
    }
}

void cap_trace( uchar *b, int len )
{
    int i;
    for( i = 0; i < len; i++ )
    {
        printf("%.2X ",b[i]);
    }
    printf("\n\n");
}
//
// Blocks until it sees a program packet sync sequence
//
void cap_wait_sync( void )
{
    while(1)
    {
        m_b[0] = m_b[1];
        m_b[1] = m_b[2];
        cap_rd_bytes( &m_b[2], 1 );
        if((m_b[0] == 0x00)&&(m_b[1]==0x00)&&(m_b[2] == 0x01))
        {
            return;
        }
    }
}
/*
//
// Send a sequence of video transport packets from memory
//
void cap_video_pes_trans( int len )
{
    int payload_remaining;
    int offset;
    sys_config info;
    dvb_config_get( &info );

    // First packet in sequence
    f_send_pes_first( &m_b[0], info.video_pid, m_video_seq, 0 );
    payload_remaining = len - PES_PAYLOAD_LENGTH;
    offset            = PES_PAYLOAD_LENGTH;
    m_video_seq = (m_video_seq+1)&0x0F;

    while( payload_remaining >= PES_PAYLOAD_LENGTH )
    {
        f_send_pes_next( &m_b[offset], info.video_pid, m_video_seq );
        m_video_seq = (m_video_seq+1)&0x0F;
        payload_remaining -= PES_PAYLOAD_LENGTH;
        offset            += PES_PAYLOAD_LENGTH;
    }

    if( payload_remaining > 0 )
    {
        f_send_pes_last( &m_b[offset], payload_remaining, info.video_pid, m_video_seq );
        m_video_seq = (m_video_seq+1)&0x0F;
    }
}
//
// Send a sequence of audio transport packets from memory
//
void cap_audio_pes_trans( int len )
{
    int payload_remaining;
    int offset;
    sys_config info;
    dvb_config_get( &info );

    // First packet in sequence
    f_send_pes_first( &m_b[0], info.audio_pid, m_audio_seq, 0 );
    payload_remaining = len - PES_PAYLOAD_LENGTH;
    offset            = PES_PAYLOAD_LENGTH;
    m_audio_seq = (m_audio_seq+1)&0x0F;

    while( payload_remaining >= PES_PAYLOAD_LENGTH )
    {
        f_send_pes_next( &m_b[offset], info.audio_pid, m_audio_seq );
        m_audio_seq = (m_audio_seq+1)&0x0F;
        payload_remaining -= PES_PAYLOAD_LENGTH;
        offset            += PES_PAYLOAD_LENGTH;
    }

    if( payload_remaining > 0 )
    {
        f_send_pes_last( &m_b[offset], payload_remaining, info.audio_pid, m_audio_seq );
        m_audio_seq = (m_audio_seq+1)&0x0F;
    }
}
*/
/*
//
// Send a Transport packet with a PCR in it and no payload
//
int send_pcr_tp(void)
{
    sys_config info;
    dvb_config_get( &info );
return 0;
    if(is_pcr_update())
    {
        // adaption field required
info.pcr_pid = info.video_pid;
        if(info.pcr_pid == info.video_pid)
        {
            f_send_tp_with_adaption_no_payload( info.video_pid, m_video_seq );
            m_video_seq = (m_video_seq+1)&0x0F;
        }
        else
        {
            f_send_tp_with_adaption_no_payload( info.pcr_pid, m_pcr_seq );
            m_pcr_seq = (m_pcr_seq+1)&0x0F;
        }
        return 1;
    }
    return 0;
}
*/
//
// Create transport packets from the program stream pulled from the video input
// len is the total length of the program packet.
//
// Returns the number of stuff bytes that have been added
//
int cap_video_pes_to_ts( void )
{
    int payload_remaining;
    int stuff;
    sys_config info;
    uchar b[188];
    dvb_config_get( &info );
    stuff = 0;
    int len = pes_get_length();
    // First packet in sequence
    // The bytes read (from the capture device) is the number of bytes available for a payload
    // in the first transport packet. this will include an adaptation field
    // containing the PCR derived from the last SCR and bitstream updates
    int overhead;
    bool ph = is_pcr_update();
    if( ph == true )
        overhead = (4 + 8);
    else
        overhead = (4);
    pes_read( b, TP_LEN - overhead);

    payload_remaining = len - f_send_pes_first( b, info.video_pid, m_video_seq, ph);// Add adaption field
    m_video_seq = (m_video_seq+1)&0x0F;

    // Send the middle
    while( payload_remaining >= PES_PAYLOAD_LENGTH )
    {
        bool ph = is_pcr_update();
        if( ph == true )
            overhead = (4 + 8);
        else
            overhead = (4);

        pes_read( b, TP_LEN - overhead);
        payload_remaining -= f_send_pes_next( b, info.video_pid, m_video_seq, ph );
        m_video_seq = (m_video_seq+1)&0x0F;
    }
    // Send the last
    if( payload_remaining > 0 )
    {
        pes_read( b, payload_remaining);
        stuff = f_send_pes_last( b, payload_remaining, info.video_pid, m_video_seq );
        m_video_seq = (m_video_seq+1)&0x0F;
    }
    return stuff;
}
//
// Create transport packets from the program stream pulled from the audio input
//
// Returns the number of stuff bytes that have been added.
//
int cap_audio_pes_to_ts( void )
{
	int payload_remaining;
    int stuff;
    sys_config info;
    uchar b[188];
    dvb_config_get( &info );
    stuff = 0;
    int len = pes_get_length();
    // First packet in sequence
    int overhead = 4;
    pes_read( b, TP_LEN - overhead);
    payload_remaining = len - (TP_LEN - overhead);

    f_send_pes_first( b, info.audio_pid, m_audio_seq, false );
    m_audio_seq = (m_audio_seq+1)&0x0F;

	while( payload_remaining >= PES_PAYLOAD_LENGTH )
	{
        pes_read( b,  PES_PAYLOAD_LENGTH );
        f_send_pes_next( b, info.audio_pid, m_audio_seq, false );
        m_audio_seq = (m_audio_seq+1)&0x0F;
        payload_remaining -= PES_PAYLOAD_LENGTH;
	}

	if( payload_remaining > 0 )
	{
        pes_read( b, payload_remaining );
        stuff = f_send_pes_last( b, payload_remaining, info.audio_pid, m_audio_seq );
        m_audio_seq = (m_audio_seq+1)&0x0F;
	}
    return stuff;
}
int last_stream;
//
// Parse the program stream coming from the capture card
//
int cap_get_len( uchar *b )
{
    int len = b[0];
    len = (len<<8) | b[1];
    return len;
}
int cap_parse_program_instream( void )
{
    int len;
    VideoBuffer *v;
    //    double old_scr,new_scr;
    // Wait for a new sync sequence
    cap_wait_sync();
    // Get the type
    cap_rd_bytes( &m_b[3], 1 );
    // Do the appropriate action
    switch( m_b[3] )
    {
        case 0xBA:
        //printf("Pack header\n");
            // Pack header
            // Read it in
            cap_rd_bytes( &m_b[4], 10 );
            // Variable length stuff field
            len = m_b[13]&0x07;
            // remove stuff bytes
            cap_rd_bytes( &m_b[14], len );
//            printf("Pack header len %d\n",len+14);
            len += 14;
            extract_scr_from_pack_header( m_b, len );
            post_scr_actions();
            break;
        case 0xBB:
            // System Header
//            printf("System header %d bytes\n",len);
            // Get the length
            cap_rd_bytes( &m_b[4], 2 );
            len = cap_get_len( &m_b[4] );
            // Discard the contents
            cap_rd_bytes( &m_b[6], len );
            len += 6;
            break;
        case 0xBE:
            // Padding Stream
//            printf("Padding %d\n", len + 6);
            cap_rd_bytes( &m_b[4], 2 );
            len = cap_get_len( &m_b[4] );
            cap_rd_bytes( &m_b[6], len );
            len += 6;
            break;
        case 0xE0:
            // Video PES
            cap_rd_bytes( &m_b[4], 2 );
            len = cap_get_len( &m_b[4] );
//printf("Video %d bytes\n",len);
            // Save the header
            v = video_delay( m_b, len );
//            pes_write_from_memory( m_b, 6 );
            // Save the elementary payload
            pes_write_from_memory( v->b, v->l );
//            pes_write_from_capture( len );
//            len += 6;
            // Do any required extra processing
            haup_pvr_video_packet();
            pes_process();
            // Add a PCR when needed
            cap_video_pes_to_ts();
            cap_video_present();
            // Get ready for next PES packet
            pes_reset();
            break;
        case 0xC0:
            // Audio PES
            cap_rd_bytes( &m_b[4], 2 );
            len = cap_get_len( &m_b[4] );
//printf("  Audio %d bytes\n",len);
            // Save the header
          pes_write_from_memory( m_b, 6 );
            // Save the elementary payload
//            pes_write_from_memory( a->b, a->l );
            pes_write_from_capture( len );
//            len += 6;
            // Do any required extra processing
            haup_pvr_audio_packet();
            pes_process();
            cap_audio_pes_to_ts();
            cap_audio_present();
            // Get ready for next PES packet
            pes_reset();
            break;
        default:
            cap_rd_bytes( &m_b[4], 2 );
            len = cap_get_len( &m_b[4] );
            loggerf("Unknown Header %.2x len %d",m_b[3],len);
            break;
    }
    return 0;
}
//
// Process a transport stream coming from the card.
//
void cap_parse_transport_instream( void )
{
    uchar b[MP_T_FRAME_LEN];
    cap_rd_bytes( b, MP_T_FRAME_LEN );
    if( b[0] != MP_T_SYNC )
    {
        //loggerf("No frame sync transport\n");
        cap_rd_bytes( b, 1 );
    }
    else
    {
        // We should have sync so process it
        dvb_ts_if( b );
    }
}
//
// This handles the FIREWIRE input
//
/*
int cap_parse_dv_dif_instream( void )
{
    if(firewire_read_dv_frame( m_b, DV_PAL_FRAME_LENGTH ))
    {
        int vl,al;
        //transcoder.DvInput( m_b, DV_PAL_FRAME_LENGTH, vl, al );
        if( vl > 0)
        {
            vl = pes_video_el_to_pes( vl );
            if( final_tx_queue_percentage() > 80 )
            {
                // Prevent overflow at low rates
                //transcoder.DropVideoFrames(1);
            }
            cap_video_pes_trans( vl );
        }
        if( al > 0)
        {
            al = pes_audio_el_to_pes( al );
            cap_audio_pes_trans( al );
        }
    }
    return 0;
}
*/
//
// Parse the input stream.
//
int cap_parse_instream( void )
{
    // Need to know the stream type
    sys_config info;
    dvb_config_get( &info );
    if( info.capture_stream_type == DVB_PROGRAM   ) cap_parse_program_instream();
    if( info.capture_stream_type == DVB_TRANSPORT ) cap_parse_transport_instream();
//    if( info.capture_stream_type == DVB_DV )        cap_parse_dv_dif_instream();
    return 0;
}
//
// This gets a list of the capture devices on the system
//
void populate_video_capture_list( CaptureList *list )
{
    char text[80];
    int fd;
    struct v4l2_capability cap;
    list->items = 0;

    sprintf(list->item[list->items],"None");
    list->items++;
    sprintf(list->item[list->items],"UDP");
    list->items++;

    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        sprintf(text,"/dev/video%d",i);
        if((fd = open( text, O_RDWR  )) > 0 )
        {
            if( ioctl(fd,VIDIOC_QUERYCAP, &cap) >= 0 )
            {
                if(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)
                {
                    sprintf(list->item[list->items],"%s",cap.card);
                    list->items++;
                    //printf("%s\n",cap.driver);
                }
            }
            close(fd);
        }
        if( list->items == MAX_CAPTURE_LIST_ITEMS ) return;
    }
}
void populate_audio_capture_list( CaptureList *list )
{
#ifdef _USE_SW_CODECS
    char **hints;
    char *name;
    char *iod;
    list->items = 0;


    // This should only list the input PCM devices
    snd_device_name_hint(-1, "pcm", (void***)&hints );

    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        if( hints[i] != NULL)
        {
            name = snd_device_name_get_hint(hints[i],"NAME");
            if(strcmp("null",name) != 0)
            {
                iod = snd_device_name_get_hint(hints[i],"IOID");
                if( iod == NULL)
                {
                    strcpy(list->item[list->items++],name);
                }
                else
                {
                    if(strcmp("Input",iod) == 0)
                    {
                        strcpy(list->item[list->items++],name);
                        free(iod);
                    }
                }
            }
            free(name);
        }
    }
#endif
}

//
// Set the stream and device type from the driver
//
void video_capture_stream_and_device_type( const char *driver )
{
    sys_config info;
    dvb_config_get( &info );

    // Default
    info.capture_stream_type = DVB_PROGRAM;
    info.capture_device_type = DVB_V4L;
    info.video_codec_type    = CODEC_MPEG2;

    if(strncmp((const char*)driver,"pvrusb2",7) == 0)
    {
        info.capture_stream_type = DVB_PROGRAM;
        info.capture_device_type = DVB_V4L;
        info.video_codec_type    = CODEC_MPEG2;
    }

    if(strncmp((const char*)driver,"saa7134",7) == 0)
    {
        info.capture_stream_type = DVB_YUV;
        info.capture_device_type = DVB_V4L;
        info.video_codec_type    = CODEC_MPEG2;
    }

    if(strncmp((const char*)driver,"hdpvr",5) == 0)
    {
        info.capture_stream_type = DVB_TRANSPORT;
        info.capture_device_type = DVB_V4L;
        info.video_codec_type    = CODEC_MPEG4;
    }
    if(strncmp((const char*)driver,"udpts",7) == 0)
    {
        info.capture_stream_type = DVB_TRANSPORT;
        info.capture_device_type = DVB_UDP_TS;
        info.video_codec_type    = CODEC_MPEG2;
    }
    if(strncmp((const char*)driver,"firewire",8) == 0)
    {
        info.capture_stream_type = DVB_DV;
        info.capture_device_type = DVB_FIREWIRE;
        info.video_codec_type    = CODEC_MPEG2;
    }
    dvb_config_save_to_local_memory( &info );
}
//
// Open the named capture device
//
int open_named_capture_device( const char *name )
{
    char text[80];
    int fd;
    struct v4l2_capability cap;

    if(strcmp(name,"UDP") == 0 )
    {
        video_capture_stream_and_device_type( "udpts" );
        return 0;
    }
    if(strcmp(name,"FIREWIRE") == 0 )
    {
        video_capture_stream_and_device_type( "firewire" );
        return 0;
    }
    // Must be a /dev/video device
    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        sprintf(text,"/dev/video%d",i);
        if((fd = open( text, O_RDWR  )) > 0 )
        {
            if( ioctl(fd,VIDIOC_QUERYCAP, &cap) >= 0 )
            {
                if(strcmp(name,(const char*)cap.card) == 0)
                {
                    // Device found
                    m_i_fd = fd;
                    video_capture_stream_and_device_type( (const char*)cap.driver );
                    return fd;
                }
            }
            close(fd);
        }
    }
    // We have not found a valid device
    return -1;
}
//
// Signals whether audio and video elemetary stream is being
// captured.
//
void cap_video_present(void)
{
    if(m_video_present < 20) m_video_present++;
}
void cap_audio_present(void)
{
    if(m_audio_present < 20) m_audio_present++;
}
//
// looks at status then resets it, called by GUI
//
bool cap_check_video_present(void)
{
    if( m_video_present > 0) m_video_present--;
    if( m_video_present == 0 )
        return false;
    else
        return true;
}
bool cap_check_audio_present(void)
{
    if( m_audio_present > 0) m_audio_present--;
    if( m_audio_present == 0 )
        return false;
    else
        return true;
}

//
// Semaphore handlers
//
void dvb_get_cap_sem(void)
{
    //sem_wait( &capture_sem );
}
void dvb_release_cap_sem(void)
{
    //sem_post( &capture_sem );
}
