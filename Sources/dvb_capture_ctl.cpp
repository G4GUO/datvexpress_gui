//
// This module controls the capture card.
//
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <math.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <semaphore.h>
#include <iostream>
#include <linux/videodev2.h>
#ifdef _USE_SW_CODECS
#include <alsa/asoundlib.h>
#endif
#include "dvb_capture_ctl.h"
#include "mp_ts_ids.h"
#include "dvb_gui_if.h"
#include "DVB-T/dvb_t.h"
#include "dvb_s2_if.h"
#include "dvb.h"
#include "an_capture.h"

// Externally needed variables
int m_i_fd;

#ifdef _USE_SW_CODECS
snd_pcm_t *m_audio_handle;
#endif

sem_t capture_sem;

// Local variables
static bool m_cap_update;
static double m_raw_bitrate;
static double m_bits_in_transport_packet;
static int m_cap_status;

double get_raw_bitrate( void )
{
    // This should be an accurate value of the bitrate
    // for the 188 byte TS packet data it takes into
    // account the underlying FEC
    return m_raw_bitrate;
}
double get_bits_in_transport_packet( void )
{
    // This should be an accurate value of the bitrate
    // for the 188 byte TS packet data it takes into
    // account the underlying FEC
    return m_bits_in_transport_packet*8.0;
}
//
// Symbol rate for DVB-S and DVB-S2
//
double get_symbol_rate( sys_config *info )
{
    double s_rate = info->sr_mem[info->sr_mem_nr];
    return s_rate;
}

//
// Calculate the raw bitrate for the waveform
//
double calculate_s2_raw_bitrate( sys_config *info )
{
    double s_rate;
    s_rate = get_symbol_rate( info );
    s_rate = s_rate*dvb_s2_code_rate();
    return s_rate;
}
//
// Calculate the raw bitrate for the waveform
//
double calculate_s_raw_bitrate( sys_config *info )
{
    double s_rate;
    s_rate = get_symbol_rate( info );
    // It can only be QPSK
    s_rate *= 2.0;
    // Now the FEC
    switch(info->dvbs_fmt.fec)
    {
    case FEC_RATE_12:
        s_rate = s_rate/2.0;
        break;
    case FEC_RATE_23:
        s_rate = s_rate*2.0/3.0;
        break;
    case FEC_RATE_34:
        s_rate = s_rate*3.0/4.0;
        break;
    case FEC_RATE_56:
        s_rate = s_rate*5.0/6.0;
        break;
    case FEC_RATE_78:
        s_rate = s_rate*7.0/8.0;
        break;
    default:
        s_rate = s_rate/2.0;
        break;
    }
    return s_rate;
}

int calculate_video_bitrate( void )
{
    sys_config info;
    dvb_config_get( &info );
    double s_rate = 0;
    double twiddle;

    switch( info.dvb_mode)
    {
    case MODE_DVBS:
        s_rate = calculate_s_raw_bitrate( &info );
        m_raw_bitrate = s_rate;
        m_bits_in_transport_packet = 204.0;
        // Approx Overhead due to transport packets
        s_rate = s_rate*((188.0-5.0)/204.0);
        twiddle = 0.90;
        break;
    case MODE_DVBS2:
        s_rate = calculate_s2_raw_bitrate( &info );
        // Approx Overhead due to transport packets (NO RS)
        m_raw_bitrate = s_rate;
        m_bits_in_transport_packet = 188.0;
        s_rate = s_rate*((188.0-5.0)/m_bits_in_transport_packet);
        twiddle = 0.90;
        break;
    case MODE_DVBT:
        s_rate = dvb_t_raw_bitrate();
        m_raw_bitrate = s_rate;
        m_bits_in_transport_packet = 188.0;
        // Approx Overhead due to transport packets
        s_rate = s_rate*((188.0-5.0)/m_bits_in_transport_packet);
        twiddle = 0.90;
        break;
    default:
        twiddle = 1.0;
        s_rate  = 0;
    }
    // Fixed audio bitrate
    s_rate -= 192000.0;
    // SI overhead
    s_rate -= 35000;
    // Twiddle
    s_rate = s_rate*twiddle;
    return (int)s_rate;
}

void dvb_cap_set_analog_standard( int fd, int std )
{
    struct v4l2_input input;
    v4l2_std_id std_id;

    memset (&input, 0, sizeof (input));

    if (-1 == ioctl (fd, VIDIOC_G_INPUT, &input.index)) {
        printf("VIDIOC_G_INPUT Error");
        return;
    }

    if (-1 == ioctl (fd, VIDIOC_ENUMINPUT, &input)) {
        loggerf("VIDIOC_ENUM_INPUT Error");
        return;
    }

    if( std ==  V4L2_STD_PAL_BG )
    {
        std_id = V4L2_STD_PAL_BG;

        if (-1 == ioctl (fd, VIDIOC_S_STD, &std_id)) {
            loggerf("VIDIOC_S_STD Err");
            return;
        }
    }

    if( std ==  V4L2_STD_SECAM_H )
    {
        std_id = V4L2_STD_SECAM_H;

        if (-1 == ioctl (fd, VIDIOC_S_STD, &std_id)) {
            loggerf("VIDIOC_S_STD Err");
            return;
        }
    }

    if( std ==  V4L2_STD_NTSC_M )
    {
        std_id = V4L2_STD_NTSC_M;

        if (-1 == ioctl (fd, VIDIOC_S_STD, &std_id)) {
            loggerf("VIDIOC_S_STD Err");
            return;
        }
    }

    struct v4l2_standard standard;

    if (-1 == ioctl (fd, VIDIOC_G_STD, &std_id)) {
        /* Note when VIDIOC_ENUMSTD always returns ENOTTY this
           is no video device or it falls under the USB exception,
           and VIDIOC_G_STD returning ENOTTY is no error. */

        loggerf("VIDIOC_G_STD");
//            exit (EXIT_FAILURE);
    }

    memset (&standard, 0, sizeof (standard));
    standard.index = 0;


    while (0 == ioctl (fd, VIDIOC_ENUMSTD, &standard)) {
        if (standard.id & std_id) {
               loggerf("Current video standard: %s", standard.name);
               break;
        }
        standard.index++;
    }
}

//
// Program or re-program the capture device
// This is semaphore protected.
//
void dvb_cap_ctl( int fd )
{
    struct v4l2_format fmt;
    struct v4l2_control ctl;
    int input;
    int video_bitrate;
    sys_config info;

    dvb_get_cap_sem();
    lseek( fd, 0, SEEK_END);

    memset(&fmt,0,sizeof(fmt));
    memset(&ctl,0,sizeof(ctl));
    dvb_config_get( &info );

    video_bitrate = calculate_video_bitrate();

    //
    // Get the capabilities
    //
    struct v4l2_capability cap;
    if( ioctl(	fd,VIDIOC_QUERYCAP, &cap) < 0 )
    {
        logger("CAP Query Error");
    }
    else
    {
        loggerf("Driver %s Card %s",cap.driver,cap.card);
//            if( cap.capabilities & V4L2_CAP_READWRITE) loggerf("R/W supported");
    }

    if( info.capture_device_type == DVB_FIREWIRE  )
    {
        // Set up the transcoder
        //transcoder.ConfigureOutput( info.video_bitrate, 64000, 720, 576 );
        info.video_bitrate = video_bitrate;
        dvb_config_save_and_update( &info );
    }

/////////////////////////////
// PVR XXX
/////////////////////////////

    if((info.capture_device_type == DVB_V4L)&&(info.capture_stream_type  == DVB_PROGRAM) )
    {

//        dvb_pvrxxx_set_analog_standard( fd, V4L2_STD_PAL_BG );

        // Set priority
        v4l2_priority priority = V4L2_PRIORITY_RECORD;

        if( ioctl( fd, VIDIOC_S_PRIORITY, &priority ) < 0 )
        {
            logger("CAP Error V4L2_PRIORITY_RECORD");
        }

        //
        // Confugure the appropriate controls on the capture card
        //
        // v4l2-ctl -c stream_type=0 #MPEG-2 DVD = 3, MPEG-2 Program Stream=0
        ctl.id    = V4L2_CID_MPEG_STREAM_TYPE;
        ctl.value = V4L2_MPEG_STREAM_TYPE_MPEG2_PS;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_STREAM_TYPE");
        }

        // Sample frequency 48K
        ctl.id    = V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ;
        ctl.value = V4L2_MPEG_AUDIO_SAMPLING_FREQ_48000;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ");
        }

        // v4l2-ctl -c audio_stereo_mode=0 		#Stereo
        ctl.id    = V4L2_CID_MPEG_AUDIO_MODE;
        ctl.value = V4L2_MPEG_AUDIO_MODE_STEREO;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
                logger("CAP Error V4L2_CID_MPEG_AUDIO_MODE");
        }

        // v4l2-ctl -c audio_layer_ii_bitrate=9 		#192Kbps
        ctl.id    = V4L2_CID_MPEG_AUDIO_L2_BITRATE;
        ctl.value = V4L2_MPEG_AUDIO_L2_BITRATE_192K;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_AUDIO_L2_BITRATE");
        }

        ctl.id       = V4L2_CID_MPEG_AUDIO_ENCODING;
        ctl.value    = V4L2_MPEG_AUDIO_ENCODING_LAYER_2;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_AUDIO_ENCODING");
        }

        // v4l2-ctl -c video_bitrate=3100000		#Set Video Bitrate
        ctl.id    = V4L2_CID_MPEG_VIDEO_BITRATE;
        ctl.value = video_bitrate;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_VIDEO_BITRATE");
        }

//        loggerf("Video bitrate fixed : %d\n",ctl.value);

        // v4l2-ctl -c video_bitrate_mode=1		#COnstant Bitrate = 1, Variable Bitrate = 0
        ctl.id    = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
        ctl.value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_VIDEO_BITRATE_MODE");
        }

        // v4l2-ctl -c video_aspect=1			#4:3
        ctl.id    = V4L2_CID_MPEG_VIDEO_ASPECT;
        ctl.value = V4L2_MPEG_VIDEO_ASPECT_4x3;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_VIDEO_ASPECT");
        }

        ctl.id    = V4L2_CID_MPEG_VIDEO_GOP_SIZE;
        ctl.value = 10;
        if( ioctl( fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_VIDEO_GOP_SIZE");
        }
        ctl.id    = V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ctl.value = 1;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 )
        {
            loggerf("CAP Error V4L2_CID_MPEG_VIDEO_B_FRAMES\n");
        }

// v4l2-ctl --set-input=2				#Composite input for PVR-150 = 2

//	input = V4L2_INPUT_TYPE_CAMERA;
        input = info.capture_device_input;

        if( ioctl( fd, VIDIOC_S_INPUT, &input) < 0 )
        {
            loggerf("CAP Error VIDIOC_S_INPUT %d",input);
        }
/*
        // v4l2-ctl --set-fmt-video=width=720,height=576
        // Use the defaults for your region determined by Linux
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width  = 720;
        fmt.fmt.pix.height = 576;//480;//576;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;

        if( ioctl( fd, VIDIOC_S_FMT, &fmt ) < 0 )
            logger("Video format error\n");
*/
        info.video_bitrate = video_bitrate;
        dvb_config_save_and_update( &info );
    }

///////////////////
// HD PVR
///////////////////

    if((info.capture_device_type == DVB_V4L)&&(info.capture_stream_type  == DVB_TRANSPORT))
    {
        v4l2_ext_controls ec;
        v4l2_ext_control  ct[6];
        memset( &ec,0,sizeof(v4l2_ext_controls));

//        loggerf("HD-PVR Video Bitrate fixed : %d",video_bitrate);

        // v4l2-ctl -c video_bitrate=3100000		#Set Video Bitrate
        ec.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ec.controls = ct;
        ec.count    = 2;

        // Control value
        ct[0].id    = V4L2_CID_MPEG_VIDEO_BITRATE;
        ct[0].value = video_bitrate;

        ct[1].id    = V4L2_CID_MPEG_AUDIO_ENCODING;
        ct[1].value = V4L2_MPEG_AUDIO_ENCODING_AAC;


// These don't appear to work.
//        ct[2].id       = V4L2_CID_MPEG_STREAM_PID_PMT;
//        ct[2].value    = info.pmt_pid;

//        ct[3].id       = V4L2_CID_MPEG_STREAM_PID_AUDIO;
//        ct[3].value    = info.audio_pid;

//        ct[4].id       = V4L2_CID_MPEG_STREAM_PID_VIDEO;
//        ct[4].value    = info.video_pid;

//        ct[5].id       = V4L2_CID_MPEG_STREAM_PID_PCR;
//        ct[5].value    = info.pcr_pid;

        if( ioctl( fd, VIDIOC_S_EXT_CTRLS, &ec )<0)
        {
            logger("CAP Error Extended V4L2_CID_MPEG_VIDEO_CONFIGURATION");
        }
        video_bitrate = ct[0].value;

        // v4l2-ctl -c video_bitrate_mode=1		#COnstant Bitrate = 1, Variable Bitrate = 0
        ec.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ec.controls = ct;
        ec.count    = 1;

        ct[0].id    = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
        ct[0].value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;

        if( ioctl( fd, VIDIOC_S_EXT_CTRLS, &ec) < 0 )
        {
            logger("CAP Error V4L2_CID_MPEG_VIDEO_BITRATE_MODE");
        }

        // v4l2-ctl --set-input=2
        memset( &input,0,sizeof( input ));
         // 0 = component, 1 = S-Video, 2 = Composite

        input = info.capture_device_input;
        if( ioctl( fd, VIDIOC_S_INPUT, &input) < 0 )
        {
            logger("CAP Error VIDIOC_S_INPUT");
        }
        info.video_bitrate = video_bitrate;
        dvb_config_save_and_update( &info );
    }

    if( info.capture_device_type == DVB_UDP_TS )
    {
       // Do nothing
        info.video_bitrate = video_bitrate;
        dvb_config_save_and_update( &info );
    }

///////////////////
// ANALOGUE CAPTURE
///////////////////
#ifdef _USE_SW_CODECS
    if((info.capture_device_type == DVB_V4L)&&(info.capture_stream_type  == DVB_YUV))
    {
        // Analogue Video capture
        dvb_cap_set_analog_standard( fd, V4L2_STD_PAL_BG );
        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(v4l2_format));
        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width       = PAL_WIDTH;
        fmt.fmt.pix.height      = PAL_HEIGHT;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
        fmt.fmt.pix.field       = V4L2_FIELD_ANY;
        if (ioctl(fd, VIDIOC_S_FMT, &fmt) < 0)
        {
            logger("CAP ANALOGUE FORMAT NOT SUPPORTED");
        }
        //	input = V4L2_INPUT_TYPE_CAMERA;
        input = info.capture_device_input;

        if( ioctl( fd, VIDIOC_S_INPUT, &input) < 0 )
        {
            loggerf("CAP Error VIDIOC_S_INPUT %d",input);
        }
        info.video_bitrate = calculate_video_bitrate();

        // Analogue sound capture
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

        an_init( &fmt );

        dvb_config_save_and_update( &info );

    }
#endif
    dvb_release_cap_sem();
    m_cap_update = false;
}
//
// Called from the capturing process to see if the video/audio capture parameters
// need changing.
//
void dvb_cap_check_for_update(void)
{
    if( m_cap_update == true )
    {
        dvb_cap_ctl( m_i_fd );
    }
}
//
// Called from the GUI to request new video capture charatersistics
//
void dvb_cap_request_param_update( void )
{
    m_cap_update = true;
}
//
// Open the Video/Audio Capture device
//
int dvb_open_capture_device(void)
{
    sys_config info;
    dvb_config_get( &info );

    if( open_named_capture_device( info.capture_device_name ) < 0 )
    {
        loggerf("Video Capture device open failed");
        return -1;
    }
    else
    {
//        loggerf("Using Video capture device %s\n",info.capture_device_name);
    }
    return m_i_fd;
}
//
// Close the video/audio capture device.
//
void dvb_close_capture_device(void)
{
    sys_config info;
    dvb_config_get( &info );

    // No need to close the device if it is a UDP connection
    if(info.capture_device_type == DVB_UDP_TS)
        return;

    dvb_get_cap_sem();

    if( m_i_fd > 0 )
    {
       close( m_i_fd );
    }
#ifdef _USE_SW_CODECS
    if(info.capture_device_type == DVB_ANALOG )
        snd_pcm_close(m_audio_handle);
#endif
    m_i_fd = 0;
    dvb_release_cap_sem();
}
//
// Initialisation of the capture device
//
int dvb_cap_init( void )
{
    sem_init( &capture_sem, 0, 0 );
    sem_post( &capture_sem );
    pes_process();// reset the modules
    pes_reset();
    if( dvb_open_capture_device() > 0 )
    {
        m_cap_status = 0;
        dvb_cap_ctl(m_i_fd);
        cap_purge();
    }
    else
    {
        m_cap_status = -1;
    }
    return m_cap_status;
}
