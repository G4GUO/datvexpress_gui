#include <linux/videodev2.h>
#include <sys/ioctl.h>
#include <memory.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <iostream>
#include <semaphore.h>
#include <sys/mman.h>
#include "dvb.h"
#include "dvb_capture_ctl.h"
#include "mp_tp.h"
#ifdef _USE_SW_CODECS
#include "an_capture.h"

#define ENVC 0
#define ENAC 1
#define INBUF_SIZE 1000000

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
static int m_video_timestamp_delta;
static int m_audio_timestamp_delta;
static int64_t m_audio_pts;
//1920 samples, 2 channels, 2 bytes per sample, maximum size
static unsigned char m_audio_buffer[PAL_SOUND_CAPTURE_SIZE*40];
static int m_sound_capture_buf_size;
static bool m_capturing;
static pthread_mutex_t mutex;
static pthread_t an_thread[2];
static int m_video_flag;
static int m_audio_flag;
// Picture withd and height
static int m_height;
static int m_width;

#define CLEAR(x) memset(&(x), 0, sizeof(x))

struct buffer
{
    void *start;
    size_t length;
};
static struct buffer *m_buffers;
static unsigned int   m_n_buffers;

void an_set_image_size( void  )
{
    // Video
    m_pFrameVideo         = av_frame_alloc();
    m_pFrameVideo->width  = m_width;
    m_pFrameVideo->height = m_height;
    m_pFrameVideo->format = AV_PIX_FMT_YUV420P;

    avpicture_alloc((AVPicture*) m_pFrameVideo, \
                   AV_PIX_FMT_YUV420P,\
                   m_width, \
                   m_height );

    m_image_size = avpicture_get_size(AV_PIX_FMT_YUV420P, \
                                      m_width, \
                                      m_height );
}

void an_set_audio_size( void  )
{
    // Audio
    m_pFrameAudio = av_frame_alloc();
    av_frame_unref(m_pFrameAudio);
    m_pFrameAudio->nb_samples = m_pC[ENAC]->frame_size;
    avcodec_fill_audio_frame(m_pFrameAudio,2,AV_SAMPLE_FMT_S16, \
                             m_audio_buffer,m_sound_capture_buf_size,0);
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
            int64_t pts = m_audio_pts*m_audio_timestamp_delta;
            m_audio_pts++;
            pthread_mutex_lock( &mutex );
            pes_audio_el_to_pes( m_avpkt[ENAC].data, m_avpkt[ENAC].size, pts, -1 );
            cap_audio_pes_to_ts();
            cap_audio_present();
            force_pcr( pts );
            pthread_mutex_unlock( &mutex );
        }
    }
}
void an_capture_video(void)
{
    int got_packet;

    m_avpkt[ENVC].size = INBUF_SIZE;
    m_avpkt[ENVC].data = m_eb[ENVC];

    if(avcodec_encode_video2( m_pC[ENVC], &m_avpkt[ENVC], m_pFrameVideo, &got_packet ) == 0 )
    {
        if(got_packet)
        {
            int64_t pts = m_avpkt[ENVC].pts*m_video_timestamp_delta;
            int64_t dts = m_avpkt[ENVC].dts*m_video_timestamp_delta;
            // Sync the audio and video streams
            if(pts == 0 ) m_audio_pts = 0;
//                printf("Video size %d\n",m_avpkt[ENVC].size*8*25);
//                printf("A %d %d\n",m_avpkt[ENVC].pts,m_avpkt[ENVC].dts);
            pthread_mutex_lock( &mutex );
            pes_video_el_to_pes( m_avpkt[ENVC].data, m_avpkt[ENVC].size, pts, dts );
            // Now encode into transport packets
            cap_video_pes_to_ts();
            cap_video_present();
            pthread_mutex_unlock( &mutex );
        }
    }
}
void an_dummy_frame(AVFrame *frame, int h, int w )
{
    int y,x;
    static int i;

    /* prepare a dummy image */
   /* Y */

    for(y=0;y<h;y++) {
             for(x=0;x<w;x++) {
                 frame->data[0][y * frame->linesize[0] + x] = x + y + i * 3;
         }
     }

         /* Cb and Cr */
         for(y=0;y<h/2;y++) {
             for(x=0;x<w/2;x++) {
                 frame->data[1][y * frame->linesize[1] + x] = 128 + y + i * 2;
                 frame->data[2][y * frame->linesize[2] + x] = 64 + x + i * 5;
         }
    }
    frame->pts = i++;

}

//
// Capture new image and associated sound
// Interval depends on whether PAL or NTSC
//
// No matter how large a read is made only 1 picture is returned
//

#define VIDEO_CAPTURE_SIZE 1000000

void *an_video_io_capturing_thread( void *arg )
{
    int val,bytes_left;
    u_int8_t *b;
    int policy = 0;

//   b = m_pFrameVideo->data[0];
     b = (u_int8_t*)av_malloc(VIDEO_CAPTURE_SIZE);

    while( m_capturing == true )
    {
        val = read( m_i_fd, b, VIDEO_CAPTURE_SIZE );
//        printf("VAL %d\n",val);
        avpicture_fill((AVPicture*)m_pFrameVideo, b, AV_PIX_FMT_YUV420P, m_width, m_height);
     //   m_pFrameVideo->data[0] = b;
     //   m_pFrameVideo->data[1] = m_pFrameVideo->data[0] + size;
     //   m_pFrameVideo->data[2] = m_pFrameVideo->data[1] + size/4;
     //   m_pFrameVideo->linesize[0] = width;
     //   m_pFrameVideo->linesize[1] = width/2;
     //   m_pFrameVideo->linesize[2] = width/2;

      //  an_dummy_frame( m_pFrameVideo, PAL_HEIGHT_CAPTURE, PAL_WIDTH_CAPTURE );
        an_capture_video();
      //  an_dummy_frame( m_pFrameVideo, PAL_HEIGHT_CAPTURE, PAL_WIDTH_CAPTURE );
//       an_capture_video();
//            printf("Image size %d\n",val);

    }
    av_free(b);

    return arg;
}

void *an_video_mmap_capturing_thread( void *arg )
{
    fd_set fds;
    struct timeval tv;
    int r;

    while ( m_capturing == true )
    {
        FD_ZERO(&fds);
        FD_SET(m_i_fd, &fds);

        /* 2 second Timeout.(in case something goes wrong) */
        tv.tv_sec = 2;
        tv.tv_usec = 0;

        // Wait for something to process
        r = select( m_i_fd + 1, &fds, NULL, NULL, &tv);

        if( r > 0 )
        {
            // Something ready to process
            struct v4l2_buffer buf;

            CLEAR(buf);

            buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            buf.memory = V4L2_MEMORY_MMAP;
            if(ioctl( m_i_fd, VIDIOC_DQBUF, &buf)>= 0)
            {
                avpicture_fill((AVPicture*)m_pFrameVideo, (uint8_t *)m_buffers[buf.index].start, AV_PIX_FMT_YUV420P, m_width, m_height);
                an_capture_video();
//                process_image(buffers[buf.index].start, buf.bytesused);
                ioctl(m_i_fd, VIDIOC_QBUF, &buf);
            }
        }
    }
    return arg;
}

void *an_audio_capturing_thread( void *arg )
{
    while( m_capturing == true )
    {
        if(snd_pcm_readi( m_audio_handle, m_audio_buffer, m_sound_capture_buf_size/4) > 0)
        {
            an_capture_audio();
        }
    }
    return arg;
}
//
// Start video capturing using IO
//
void an_start_io_capture(void)
{

    m_audio_flag = 0;
    m_video_flag = 0;
    m_audio_pts  = 0;
    // Create the mutex
    pthread_mutex_init( &mutex, NULL );
    int ret;
    struct sched_param params;
    pthread_t this_thread = pthread_self();

    // We'll set the priority to the maximum.
//    struct sched_param params;
    params.sched_priority = 50;//sched_get_priority_max(SCHED_FIFO);
    ret = pthread_setschedparam(this_thread, SCHED_FIFO, &params);
//           printf("VAL %d\n",ret);

    pthread_create( &an_thread[0], NULL, an_video_io_capturing_thread, NULL );
}
//
// Start capturing using streaming
//
void an_start_streaming_capture(int fd)
{
    struct v4l2_requestbuffers req;

    CLEAR(req);

    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;

    if (ioctl(fd, VIDIOC_REQBUFS, &req)<0)
    {
        loggerf("CAP does not support MM");
        return;
    }

    if (req.count < 2)
    {
        loggerf("CAP not enough MM buffers");
        return;
    }

    m_buffers = (struct buffer*)calloc(req.count, sizeof(*m_buffers));

    if (!m_buffers)
    {
        loggerf("Not enough buffer memory");
        return;
    }

    for (m_n_buffers = 0; m_n_buffers < req.count; ++m_n_buffers)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);

        buf.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index  = m_n_buffers;

        if ( ioctl(fd, VIDIOC_QUERYBUF, &buf)<0)
        {
            return;
        }

        m_buffers[m_n_buffers].length = buf.length;
        m_buffers[m_n_buffers].start =
        mmap(NULL /* start anywhere */,
        buf.length,
        PROT_READ | PROT_WRITE /* required */,
        MAP_SHARED /* recommended */,
        fd, buf.m.offset);

        if (MAP_FAILED == m_buffers[m_n_buffers].start) return;
    }
    //
    // Start the capturing
    //
    for (int i = 0; i < m_n_buffers; ++i)
    {
        struct v4l2_buffer buf;

        CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;

        if (ioctl(fd, VIDIOC_QBUF, &buf) < 0) return;
    }
    v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type)<0) return;

    pthread_create( &an_thread[0], NULL, an_video_mmap_capturing_thread, NULL );
}
//
// Initilaise all the software codecs
//
int an_init_codecs( void )
{
    sys_config info;

    av_register_all();
    avfilter_register_all();
    dvb_config_get( &info );

    av_init_packet(&m_avpkt[ENVC]);

    m_avpkt[ENVC].data = NULL;
    m_avpkt[ENVC].size = 0;

    // 25 frames per sec, every 40 ms
    m_video_timestamp_delta = ((0.04*27000000.0)/300.0);
    // New audio packet sent every 24 ms
    m_audio_timestamp_delta = ((0.024*27000000.0)/300.0);

    AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_MPEG2VIDEO);
    if(codec != NULL)
    {
        m_pC[ENVC]                     = avcodec_alloc_context3(codec);
        m_pC[ENVC]->bit_rate           = info.video_bitrate;
        m_pC[ENVC]->bit_rate_tolerance = info.video_bitrate/10;
        m_pC[ENVC]->width              = m_width;
        m_pC[ENVC]->height             = m_height;
        m_pC[ENVC]->gop_size           = 12;
        m_pC[ENVC]->max_b_frames       = 0;
        m_pC[ENVC]->me_method          = 5;
        m_pC[ENVC]->pix_fmt            = AV_PIX_FMT_YUV420P;
        m_pC[ENVC]->time_base          = (AVRational){1,25};
        m_pC[ENVC]->ticks_per_frame    = 2;// MPEG2 & 4
        m_pC[ENVC]->profile            = FF_PROFILE_MPEG2_MAIN;
        m_pC[ENVC]->thread_count       = 1;

        if(avcodec_open2(m_pC[ENVC], codec, NULL)<0)
        {
            loggerf("Unable to open MPEG2 Codec");
            return -1;
        }
        an_set_image_size();
    }
    else
    {
        loggerf("MPEG2 Codec not found");
        return -1;
    }
    //
    // Audio
    //
    av_init_packet( &m_avpkt[ENAC] );
    //
    // Must be set to 48000, 2 chan
    //
    // Size in bytes 2 channels, 16 bits 1/25 sec
    codec = avcodec_find_encoder(AV_CODEC_ID_MP2);
    if( codec != NULL )
    {
        m_pC[ENAC] = avcodec_alloc_context3(codec);
        m_pC[ENAC]->bit_rate            = info.audio_bitrate;
        m_pC[ENAC]->bit_rate_tolerance  = 0;
        m_pC[ENAC]->bits_per_raw_sample = 16;
        m_pC[ENAC]->sample_rate         = 48000;
        m_pC[ENAC]->channels            = 2;
        m_pC[ENAC]->sample_fmt          = AV_SAMPLE_FMT_S16;
        m_pC[ENAC]->channel_layout      = AV_CH_LAYOUT_STEREO;
        m_pC[ENAC]->thread_count        = 1;

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

void an_setup_video_capturing( int fd )
{
   struct v4l2_capability cap;

   if ( ioctl(fd, VIDIOC_QUERYCAP, &cap) < 0 ) return;

   if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE)) return;


   if ( cap.capabilities & V4L2_CAP_STREAMING)
   {
      // printf("Streaming supported\n");
       an_start_streaming_capture( fd );
       return;
   }

   if ( cap.capabilities & V4L2_CAP_READWRITE )
   {
      //printf("IO supported\n");
      an_start_io_capture();
      return;
   }
}
void an_setup_audio_capturing( void )
{
    pthread_create( &an_thread[1], NULL, an_audio_capturing_thread, NULL );
}
//
// Set up both the audio and video capturing on the card
//
void an_configure_capture_card( void )
{
    struct v4l2_cropcap cropcap;
    struct v4l2_crop    crop;
    struct v4l2_format  fmt;
    sys_config info;
    int input;

    dvb_config_get( &info );

    CLEAR(cropcap);
    CLEAR(crop);
    CLEAR(fmt);

    m_width  = PAL_WIDTH_CAPTURE;
    m_height = PAL_HEIGHT_CAPTURE;

    // Set the cropping, ignore any errors
    cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (ioctl(m_i_fd, VIDIOC_CROPCAP, &cropcap)==0) {
            crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            crop.c    = cropcap.defrect; /* reset to default */

            ioctl(m_i_fd, VIDIOC_S_CROP, &crop);
    }

    // Analogue Video capture
    fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = m_width;
    fmt.fmt.pix.height      = m_height;
    fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    if (ioctl(m_i_fd, VIDIOC_S_FMT, &fmt) < 0)
    {
        logger("CAP ANALOGUE FORMAT NOT SUPPORTED");
    }
    if(ioctl(m_i_fd, VIDIOC_G_FMT, &fmt)<0 )
        logger("can't get format");

    //	input = V4L2_INPUT_TYPE_CAMERA;
    input = info.video_capture_device_input;

    if( ioctl( m_i_fd, VIDIOC_S_INPUT, &input) < 0 )
    {
        loggerf("CAP Error VIDIOC_S_INPUT %d",input);
    }

/*
    v4l2_streamparm parm;
    memset(&parm,0,sizeof(v4l2_streamparm));
    parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    parm.parm.capture.capturemode = 0;
    parm.parm.capture.readbuffers = 0;

    if( ioctl( m_i_fd, VIDIOC_S_PARM, &parm) < 0 )
    {
        loggerf("CAP Error VIDIOC_S_PARM");
    }
*/
    info.video_bitrate = calculate_video_bitrate();

    //
    // Analogue sound capture
    //
    snd_pcm_hw_params_t *hw_params;


    if(snd_pcm_open(&m_audio_handle, "pulse", SND_PCM_STREAM_CAPTURE, 0)< 0 )
    {
        loggerf("Unable to open sound device");
        return;
    }
    unsigned int rate = 48000;
    snd_pcm_hw_params_malloc(&hw_params);
    snd_pcm_hw_params_any(m_audio_handle, hw_params);
    snd_pcm_hw_params_set_access(m_audio_handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(m_audio_handle, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_rate_near(m_audio_handle, hw_params, &rate, 0);
    snd_pcm_hw_params_set_channels(m_audio_handle, hw_params, 2);
    snd_pcm_hw_params(m_audio_handle, hw_params);
    snd_pcm_hw_params_free(hw_params);
    snd_pcm_prepare(m_audio_handle);

    an_init_codecs();
    m_capturing = true;
    an_setup_video_capturing( m_i_fd );
    an_setup_audio_capturing();
}

void an_stop_capture(void)
{
    if( m_capturing == true )
    {
        m_capturing = false;
    }
}
#endif
