#ifdef _USE_SW_CODECS
#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <alsa/asoundlib.h>
#include <semaphore.h>
#include "dvb.h"
#include "an_capture.h"
#include "dvb_capture_ctl.h"
#include "mp_tp.h"

#define ENVC 0
#define ENAC 1
#define INBUF_SIZE 200000


// Externally located variables
extern int m_i_fd;
extern snd_pcm_t *m_audio_handle;
// Local variables
static int m_image_size;
static AVPacket        m_avpkt[4];
static AVCodecContext *m_pC[4];
static AVFrame *m_pFrameVideo;
static AVFrame *m_pFrameAudio;
static uint8_t m_eb[2][INBUF_SIZE];
static int64_t m_video_timestamp;
static int64_t m_audio_timestamp;
static int64_t m_pcr_timestamp;
static int m_video_timestamp_delta;
static int m_pcr_timestamp_delta;
static int m_audio_timestamp_delta;
//1920 samples, 2 channels, 2 bytes per sample, maximum size
static unsigned char m_audio_buffer[PAL_SOUND_CAPTURE_SIZE*40];
static int m_sound_capture_buf_size;
static bool m_capturing;
static pthread_t an_thread[2];
static sem_t m_an_sem;
static int m_video_flag;
static int m_audio_flag;

void an_set_image_size( v4l2_format *fmt  )
{
    // Video
    m_pFrameVideo = av_frame_alloc();
    m_image_size  = fmt->fmt.pix.sizeimage;
    av_image_alloc( m_pFrameVideo->data, m_pFrameVideo->linesize, fmt->fmt.pix.width, \
                    fmt->fmt.pix.height, AV_PIX_FMT_YUV420P, 32);
}
void an_set_audio_size( void  )
{
    // Audio
    m_pFrameAudio = av_frame_alloc();
    avcodec_get_frame_defaults(m_pFrameAudio);
    m_pFrameAudio->nb_samples = m_pC[ENAC]->frame_size;
    avcodec_fill_audio_frame(m_pFrameAudio,2,AV_SAMPLE_FMT_S16, \
                             m_audio_buffer,m_sound_capture_buf_size,2);
}
//
// Capture analoge sound
//
void an_capture_audio(void)
{
    int got_packet;

    m_avpkt[ENAC].data = m_eb[ENAC];
    m_avpkt[ENAC].size = INBUF_SIZE;

    if(avcodec_encode_audio2( m_pC[ENAC], &m_avpkt[ENAC], m_pFrameAudio, &got_packet ) == 0 )
    {
        if(got_packet)
        {
//          printf("Audio Size %d\n",m_avpkt[ENAC].size);
            int64_t pts = m_audio_timestamp*m_audio_timestamp_delta;
            pes_audio_el_to_pes( m_avpkt[ENAC].data, m_avpkt[ENAC].size, pts, -1 );
            cap_audio_pes_to_ts();
            cap_audio_present();
            pes_reset();
        }
    }
}
void an_capture_video(void)
{
    int got_packet;

    m_avpkt[ENVC].size = INBUF_SIZE;
    m_avpkt[ENVC].data = m_eb[ENVC];

    force_pcr(m_pcr_timestamp*m_pcr_timestamp_delta);
    m_pcr_timestamp++;

    if(avcodec_encode_video2( m_pC[ENVC], &m_avpkt[ENVC], m_pFrameVideo, &got_packet ) == 0 )
    {
        if( got_packet )
        {
            int64_t pts = m_avpkt[ENVC].pts*m_video_timestamp_delta;
            int64_t dts = m_avpkt[ENVC].dts*m_video_timestamp_delta;
//                printf("Video size %d\n",m_avpkt[ENVC].size*8*25);
//                printf("A %d %d\n",m_avpkt[ENVC].pts,m_avpkt[ENVC].dts);
            pes_video_el_to_pes( m_avpkt[ENVC].data, m_avpkt[ENVC].size, pts, dts );
            // Now encode into transport packets
            cap_video_pes_to_ts();
            cap_video_present();
            pes_reset();
        }
    }
}
//
// Capture new image and associated sound
// Interval depends on whether PAL or NTSC
//
void *an_video_capturing_thread( void *arg )
{
    while( m_capturing == true )
    {
        if(read( m_i_fd, m_pFrameVideo->data[0], m_image_size) == m_image_size)
        {
            sem_wait( &m_an_sem );
            an_capture_video();
            sem_post( &m_an_sem );
        }
    }
    return arg;
}
void *an_audio_capturing_thread( void *arg )
{
    while( m_capturing == true )
    {
        if(snd_pcm_readi( m_audio_handle, m_audio_buffer, m_sound_capture_buf_size) > 0)
        {
            sem_wait( &m_an_sem );
            an_capture_audio();
            sem_post( &m_an_sem );
        }
    }
    return arg;
}

//
// Initilaise all the software codecs
//
int an_init( v4l2_format *fmt )
{
    av_register_all();
//    avfilter_register_all();
    av_init_packet(&m_avpkt[ENVC]);
    m_avpkt[ENVC].data = NULL;
    m_avpkt[ENVC].size = 0;

    // 25 frames per sec, every 40 ms
    m_video_timestamp_delta = ((0.04*27000000.0)/300);
    // 40 ms
    m_pcr_timestamp_delta = (0.04*27000000.0);
    // New audio packet sent every 24 ms
    m_audio_timestamp_delta = (0.024*27000000.0)/300;

    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
    if(codec != NULL)
    {
        m_pC[ENVC]                     = avcodec_alloc_context3(codec);
        m_pC[ENVC]->bit_rate           = calculate_video_bitrate();
        m_pC[ENVC]->bit_rate_tolerance = calculate_video_bitrate()/10;
        m_pC[ENVC]->width              = PAL_WIDTH;
        m_pC[ENVC]->height             = PAL_HEIGHT;
        m_pC[ENVC]->gop_size           = 10;
        m_pC[ENVC]->max_b_frames       = 10;
        m_pC[ENVC]->me_method          = 5;
        m_pC[ENVC]->pix_fmt            = AV_PIX_FMT_YUV420P;
        m_pC[ENVC]->time_base          = (AVRational){1,25};
        m_pC[ENVC]->profile            = FF_PROFILE_MPEG2_MAIN;
        m_pC[ENVC]->thread_count       = 4;

        if(avcodec_open2(m_pC[ENVC], codec, NULL)<0)
        {
            loggerf("Unable to open MPEG2 Codec");
            return -1;
        }
        an_set_image_size( fmt );
    }
    else
    {
        loggerf("MPEG2 Codec not found");
        return -1;
    }
    //
    // Audio
    //
    av_init_packet(&m_avpkt[ENAC]);
    //
    // Must be set to 48000, 2 chan
    //
    // Size in bytes 2 channels, 16 bits 1/25 sec
    m_audio_timestamp = 0;
    codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
    if( codec != NULL )
    {
        m_pC[ENAC] = avcodec_alloc_context3(codec);
        m_pC[ENAC]->bits_per_raw_sample = 16;
        m_pC[ENAC]->bit_rate            = 192000;
        m_pC[ENAC]->sample_rate         = 48000;
        m_pC[ENAC]->channels            = 2;
        m_pC[ENAC]->sample_fmt          = AV_SAMPLE_FMT_S16;
        m_pC[ENAC]->channel_layout      = AV_CH_LAYOUT_STEREO;
        if(avcodec_open2(m_pC[ENAC], codec, NULL)<0 )
        {
            loggerf("Unable to open MPEG1 codec");
            return -1;
        }
        // 16 bit samples & stereo so multiply by 4
        m_sound_capture_buf_size = m_pC[ENAC]->frame_size*4;
        an_set_audio_size();
    }
    return 0;
}
void an_start_capture(void)
{
    m_capturing = true;
    m_audio_flag = 0;
    m_video_flag = 0;
    m_pcr_timestamp = 0;

    sem_init( &m_an_sem, 0, 0 );
    sem_post( &m_an_sem );

    pthread_create( &an_thread[1], NULL, an_audio_capturing_thread, NULL );
    pthread_create( &an_thread[0], NULL, an_video_capturing_thread, NULL );
}
void an_stop_capture(void)
{
    m_capturing = false;
}
#endif
