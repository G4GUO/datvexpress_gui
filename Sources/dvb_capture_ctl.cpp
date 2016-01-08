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
#include <pthread.h>
#include <iostream>
#include <linux/videodev2.h>
#ifdef _USE_SW_CODECS
#include <an_capture.h>
#endif
#include "dvb_capture_ctl.h"
#include "mp_ts_ids.h"
#include "dvb_gui_if.h"
#include "DVB-T/dvb_t.h"
#include "dvb_s2_if.h"
#include "dvb.h"
#include "an_capture.h"
#include "bm_mod_interface.h"

// Externally needed variables
int m_i_fd;

#ifdef _USE_SW_CODECS
snd_pcm_t *m_audio_handle;
#endif

// Local variables
static bool   m_cap_update;
static double m_raw_bitrate;
static double m_bits_in_transport_packet;
static int    m_cap_status;
static char   m_v4l_capture_device[80];

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

    switch(info.dvb_mode)
    {
    case MODE_DVBS:
        s_rate = calculate_s_raw_bitrate( &info );
        m_raw_bitrate = s_rate;
        m_bits_in_transport_packet = 204.0;
        // Approx Overhead due to transport packets
        s_rate = s_rate*((188.0-5.0)/204.0);
        twiddle = 0.92;
        break;
    case MODE_DVBS2:
        s_rate = calculate_s2_raw_bitrate( &info );
        // Approx Overhead due to transport packets (NO RS)
        m_raw_bitrate = s_rate;
        m_bits_in_transport_packet = 188.0;
       s_rate = s_rate*((188.0-5.0)/m_bits_in_transport_packet);
        twiddle = 0.92;
        break;
    case MODE_DVBT:
        s_rate = dvb_t_raw_bitrate();
        m_raw_bitrate = s_rate;
        m_bits_in_transport_packet = 188.0;
        // Approx Overhead due to transport packets
        s_rate = s_rate*((188.0-5.0)/m_bits_in_transport_packet);
        twiddle = 0.92;
        break;
    default:
        twiddle = 1.0;
        s_rate  = 0;
    }
    if(info.cap_dev_type == CAP_DEV_TYPE_HD_HAUP) twiddle *= 1.12;
    // Fixed audio bitrate
    s_rate -= 192000.0;
    // SI overhead
    s_rate -= 100000;
    // PCR overhead if using seperate stream
    if(info.pcr_pid != info.video_pid) s_rate -= 50000;

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

    if( std ==  V4L2_STD_PAL )
    {
        std_id = V4L2_STD_PAL;

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
// Open the named capture device
//
int open_named_capture_device( const char *name )
{
    int fd;
    struct v4l2_capability cap;

    if(strcmp(name,"UDP") == 0 )
    {
        return 0;
    }
    if(strcmp(name,"FIREWIRE") == 0 )
    {
        return 0;
    }
    if(strcmp(name,"DeckLink Mini Recorder") == 0 )
    {
        return 0;
    }
    // Must be a /dev/video device
    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        sprintf(m_v4l_capture_device,"/dev/video%d",i);
        if((fd = open( m_v4l_capture_device, O_RDWR )) > 0 )
        {
            if( ioctl(fd,VIDIOC_QUERYCAP, &cap) >= 0 )
            {
                if(strcmp(name,(const char*)cap.card) == 0)
                {
                    // Device found
                    m_i_fd = fd;
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
// Get the device type from the driver
//
CapDevType get_video_device_type_from_driver( const char *driver )
{
    CapDevType type = CAP_DEV_TYPE_NONE;

    if(strncmp((const char*)driver,"pvrusb2",7) == 0)
    {
        type = CAP_DEV_TYPE_SD_HAUP;
    }

    if(strncmp((const char*)driver,"ivtv",7) == 0)
    {
        type = CAP_DEV_TYPE_SD_HAUP;
    }

    if(strncmp((const char*)driver,"saa7134",7) == 0)
    {
        type = CAP_DEV_TYPE_SA7134;
    }
    if(strncmp((const char*)driver,"em28xx",6) == 0)
    {
        type = CAP_DEV_TYPE_SA7113;
    }
    if(strncmp((const char*)driver,"sonixj",5) == 0)
    {
        type = CAP_DEV_TYPE_SONIXJ;
    }

    if(strncmp((const char*)driver,"uvcvideo",5) == 0)
    {
        type = CAP_DEV_TYPE_UVCVIDEO;
    }

    if(strncmp((const char*)driver,"hdpvr",5) == 0)
    {
        type = CAP_DEV_TYPE_HD_HAUP;
    }
    if(strncmp((const char*)driver,"udpts",7) == 0)
    {
        type = CAP_DEV_TYPE_UDP;
    }
    if(strncmp((const char*)driver,"firewire",8) == 0)
    {
        type = CAP_DEV_TYPE_FIREWIRE;
    }
    return type;
}

CapDevType get_device_type_from_name( const char *name )
{
    int fd;
    CapDevType type;
    struct v4l2_capability cap;
    char devname[40];

    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        sprintf(devname,"/dev/video%d",i);
        if((fd = open( devname, O_RDWR )) > 0 )
        {
            if( ioctl(fd,VIDIOC_QUERYCAP, &cap) >= 0 )
            {
                if(strcmp(name,(const char*)cap.card) == 0)
                {
                    // Device found
                    type = get_video_device_type_from_driver( (const char*)cap.driver );
                    close(fd);
                    return type;
                }
            }
            close(fd);
        }
    }
    // See if it is a blackmagic card
    if(strcmp(name,(const char*)"DeckLink Mini Recorder") == 0){
        return CAP_DEV_TYPE_DL_MINI_RECORDER;
    }

    // We have not found a valid device
    return CAP_DEV_TYPE_NONE;
}
//
// Given a name get the file handle for that device
//
int get_device_id_from_name( const char *name )
{
    int fd;
    CapDevType type;
    struct v4l2_capability cap;
    char devname[40];

    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        sprintf(devname,"/dev/video%d",i);
        if((fd = open( devname, O_RDWR )) > 0 )
        {
            if( ioctl(fd,VIDIOC_QUERYCAP, &cap) >= 0 )
            {
                if(strcmp(name,(const char*)cap.card) == 0)
                {
                    // Device found
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
// This gets a list of the capture devices on the system
//
void populate_video_capture_list( CaptureCardList *list )
{
    char text[80];
    int fd;
    struct v4l2_capability cap;
    list->items = 0;

    sprintf(list->item[list->items],S_NONE);
    list->items++;
    sprintf(list->item[list->items],S_UDP_TS);
    list->items++;
    sprintf(list->item[list->items],S_FIREWIRE);
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
        if(list->items >= MAX_CAPTURE_LIST_ITEMS ) return;
    }
#ifdef _USE_SW_CODECS
    // Do the same for Black magic cards
    CaptureCardList bmcards;
    int num = dl_list_devices( &bmcards );
    for( int i = 0; i < num; i++ ){
        sprintf(list->item[list->items],"%s",bmcards.item[i]);
        list->items++;
        if(list->items >= MAX_CAPTURE_LIST_ITEMS ) return;
    }
#endif
}
//
// Given a file id get the list of input names
//
void populate_video_input_list( int fd, CaptureInputList *list )
{
    struct v4l2_input input;
    list->items = 0;

    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        input.index = i;
        if( ioctl(fd, VIDIOC_ENUMINPUT, &input) >= 0 )
        {
            sprintf(list->item[list->items],"%s",input.name);
            list->items++;
        }
    }
}
//
// Get the input names for the currently open device
//
void populate_video_input_list_from_open_fd( CaptureInputList *list )
{
    list->items = 0;
    populate_video_input_list( m_i_fd, list );
}
//
// Given a name get the list of input names
//
int populate_video_input_list( const char *name, CaptureInputList *list )
{
    int fd;
    list->items = 0;
    if((fd=get_device_id_from_name( name ))>0)
    {
        populate_video_input_list( fd, list );
        close(fd);
        return 0;
    }
    return -1;
}

#ifdef _USE_SW_CODECS
void populate_audio_capture_list( CaptureCardList *list )
{
    void **hints;
    char *name;
    char *iod;
//    char *desc;
    list->items = 0;
    hints = NULL;

    snd_config_update();

    // This should only list the input PCM devices
    snd_device_name_hint(-1, "pcm", &hints );

    for( int i = 0; i < MAX_CAPTURE_LIST_ITEMS; i++ )
    {
        if( hints[i] )
        {
            name = snd_device_name_get_hint(hints[i],"NAME");
           // desc = snd_device_name_get_hint(hints[i],"DESC");
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
                        strcpy(list->item[list->items++],iod);
                    }
                    free(iod);
                }
            }
            free(name);
        }
    }
}
#endif
//
// Program or re-program the capture device
// This is semaphore protected.
//
void dvb_cap_ctl( void )
{
    struct v4l2_format fmt;
    struct v4l2_control ctl;
    int input;
    int video_bitrate;
    sys_config info;

    memset(&fmt,0,sizeof(fmt));
    memset(&ctl,0,sizeof(ctl));
    dvb_config_get( &info );

    video_bitrate = calculate_video_bitrate();

    //
    // Get the capabilities
    //
    if( m_i_fd > 0 ){
        struct v4l2_capability cap;
        if( ioctl(	m_i_fd,VIDIOC_QUERYCAP, &cap) < 0 ){
            logger("CAP Query Error");
        }
        else{
            loggerf("Driver: %s, Card: %s, Device: %s",cap.driver,cap.card,m_v4l_capture_device);
//            if( cap.capabilities & V4L2_CAP_READWRITE) loggerf("R/W supported");
        }
    }

    if( info.video_capture_device_class == DVB_FIREWIRE  ){
        // Set up the transcoder
        //transcoder.ConfigureOutput( info.video_bitrate, 64000, 720, 576 );
        info.video_bitrate = video_bitrate;
        dvb_config_save_and_update( &info );
    }

/////////////////////////////
// PVR XXX
/////////////////////////////

    if( info.cap_dev_type == CAP_DEV_TYPE_SD_HAUP )
    {

//        dvb_pvrxxx_set_analog_standard( fd, V4L2_STD_PAL_BG );

        // Set priority
        v4l2_priority priority = V4L2_PRIORITY_RECORD;

        if( ioctl( m_i_fd, VIDIOC_S_PRIORITY, &priority ) < 0 ){
            logger("CAP Error V4L2_PRIORITY_RECORD");
        }

        //
        // Confugure the appropriate controls on the capture card
        //
        // v4l2-ctl -c stream_type=0 #MPEG-2 DVD = 3, MPEG-2 Program Stream=0
        ctl.id    = V4L2_CID_MPEG_STREAM_TYPE;
        ctl.value = V4L2_MPEG_STREAM_TYPE_MPEG2_PS;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_STREAM_TYPE");
        }

        // Sample frequency 48K
        ctl.id    = V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ;
        ctl.value = V4L2_MPEG_AUDIO_SAMPLING_FREQ_48000;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_AUDIO_SAMPLING_FREQ");
        }

        // v4l2-ctl -c audio_stereo_mode=0 		#Stereo
        ctl.id    = V4L2_CID_MPEG_AUDIO_MODE;
        ctl.value = V4L2_MPEG_AUDIO_MODE_STEREO;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
                logger("CAP Error V4L2_CID_MPEG_AUDIO_MODE");
        }

        // v4l2-ctl -c audio_layer_ii_bitrate=9 		#192Kbps
        ctl.id    = V4L2_CID_MPEG_AUDIO_L2_BITRATE;
        ctl.value = V4L2_MPEG_AUDIO_L2_BITRATE_192K;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_AUDIO_L2_BITRATE");
        }

        ctl.id       = V4L2_CID_MPEG_AUDIO_ENCODING;
        ctl.value    = V4L2_MPEG_AUDIO_ENCODING_LAYER_2;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_AUDIO_ENCODING");
        }

        // v4l2-ctl -c video_bitrate=3100000		#Set Video Bitrate
        ctl.id    = V4L2_CID_MPEG_VIDEO_BITRATE;
        ctl.value = video_bitrate;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_VIDEO_BITRATE");
        }

//        loggerf("Video bitrate fixed : %d\n",ctl.value);

        // v4l2-ctl -c video_bitrate_mode=1		#COnstant Bitrate = 1, Variable Bitrate = 0
        ctl.id    = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
        ctl.value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_VIDEO_BITRATE_MODE");
        }

        // v4l2-ctl -c video_aspect=1			#4:3
        ctl.id    = V4L2_CID_MPEG_VIDEO_ASPECT;
        ctl.value = V4L2_MPEG_VIDEO_ASPECT_4x3;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_VIDEO_ASPECT");
        }

        ctl.id    = V4L2_CID_MPEG_VIDEO_GOP_SIZE;
        ctl.value = 10;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_VIDEO_GOP_SIZE");
        }
        ctl.id    = V4L2_CID_MPEG_VIDEO_B_FRAMES;
        ctl.value = 1;
        if( ioctl( m_i_fd, VIDIOC_S_CTRL, &ctl) < 0 ){
            loggerf("CAP Error V4L2_CID_MPEG_VIDEO_B_FRAMES");
        }

// v4l2-ctl --set-input=2				#Composite input for PVR-150 = 2

//	input = V4L2_INPUT_TYPE_CAMERA;
        input = info.video_capture_device_input;

        if( ioctl( m_i_fd, VIDIOC_S_INPUT, &input) < 0 ){
            loggerf("CAP Error VIDIOC_S_INPUT %d",input);
        }

        // v4l2-ctl --set-fmt-video=width=720,height=576
        // Use the defaults for your region determined by Linux
        fmt.type                = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
        fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
        fmt.fmt.pix.width       = 720;

        if(info.cap_format.video_format == CAP_PAL ){
            fmt.fmt.pix.height = 576;
            if( ioctl( m_i_fd, VIDIOC_S_FMT, &fmt ) < 0 )
                loggerf("Video format error %d",fmt.fmt.pix.height);
            dvb_cap_set_analog_standard( m_i_fd, V4L2_STD_PAL );
        }
        if(info.cap_format.video_format == CAP_NTSC ){
            fmt.fmt.pix.height = 480;
            if( ioctl( m_i_fd, VIDIOC_S_FMT, &fmt ) < 0 )
                loggerf("Video format error %d",fmt.fmt.pix.height);
            dvb_cap_set_analog_standard( m_i_fd, V4L2_STD_NTSC );
        }

        info.video_bitrate = video_bitrate;
        info.audio_bitrate = 192000;
        info.sw_codec.video_encoder_type = CODEC_MPEG2;
        info.sw_codec.audio_encoder_type = CODEC_13818_3;
        info.sw_codec.using_sw_codec = false;
        dvb_config_save_and_update( &info );
    }

///////////////////
// HD PVR
///////////////////

    if( info.cap_dev_type == CAP_DEV_TYPE_HD_HAUP ){
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

        if( ioctl( m_i_fd, VIDIOC_S_EXT_CTRLS, &ec )<0){
            logger("CAP Error Extended V4L2_CID_MPEG_VIDEO_CONFIGURATION");
        }
        video_bitrate = ct[0].value;

        // v4l2-ctl -c video_bitrate_mode=1		#COnstant Bitrate = 1, Variable Bitrate = 0
        ec.ctrl_class = V4L2_CTRL_CLASS_MPEG;
        ec.controls = ct;
        ec.count    = 1;

        ct[0].id    = V4L2_CID_MPEG_VIDEO_BITRATE_MODE;
        ct[0].value = V4L2_MPEG_VIDEO_BITRATE_MODE_CBR;

        if( ioctl( m_i_fd, VIDIOC_S_EXT_CTRLS, &ec) < 0 ){
            logger("CAP Error V4L2_CID_MPEG_VIDEO_BITRATE_MODE");
        }

        // v4l2-ctl --set-input=2
        memset( &input,0,sizeof( input ));
         // 0 = component, 1 = S-Video, 2 = Composite

        input = info.video_capture_device_input;
        if( ioctl( m_i_fd, VIDIOC_S_INPUT, &input) < 0 ){
            logger("CAP Error VIDIOC_S_INPUT");
        }
        info.video_bitrate = video_bitrate;
        info.audio_bitrate = 192000;
        info.sw_codec.video_encoder_type = CODEC_MPEG4;
        info.sw_codec.audio_encoder_type = CODEC_LAOS;
        info.sw_codec.using_sw_codec = false;

        dvb_config_save_and_update( &info );
    }

    if( info.video_capture_device_class == DVB_UDP_TS ){
       // Do nothing
        info.video_bitrate = video_bitrate;
        info.audio_bitrate = 192000;
        dvb_config_save_and_update( &info );
    }

///////////////////
// ANALOGUE CAPTURE
///////////////////
#ifdef _USE_SW_CODECS

    if(info.cap_dev_type == CAP_DEV_TYPE_SA7134 ){
        info.video_bitrate     = calculate_video_bitrate();
        info.audio_bitrate     = 192000;
        info.sw_codec.using_sw_codec = true;
        info.sw_codec.aspect[0] = 4;
        info.sw_codec.aspect[1] = 3;
        dvb_config_save_and_update( &info );
        an_configure_capture_card(CAP_DEV_TYPE_SA7134);
    }

    if(info.cap_dev_type == CAP_DEV_TYPE_SA7113 ){
        info.video_bitrate     = calculate_video_bitrate();
        info.audio_bitrate     = 192000;
        info.sw_codec.using_sw_codec = true;
        info.sw_codec.aspect[0] = 4;
        info.sw_codec.aspect[1] = 3;
        dvb_config_save_and_update( &info );
        an_configure_capture_card(CAP_DEV_TYPE_SA7113);
    }

    if(info.cap_dev_type == CAP_DEV_TYPE_SONIXJ ){
        info.video_bitrate     = calculate_video_bitrate();
        info.audio_bitrate     = 192000;
        info.sw_codec.using_sw_codec = true;
        info.sw_codec.aspect[0] = 4;
        info.sw_codec.aspect[1] = 3;
        dvb_config_save_and_update( &info );
        an_configure_capture_card(CAP_DEV_TYPE_SONIXJ);
    }
    if(info.cap_dev_type == CAP_DEV_TYPE_UVCVIDEO ){
        info.video_bitrate     = calculate_video_bitrate();
        info.audio_bitrate     = 192000;
        info.sw_codec.using_sw_codec = true;
        info.sw_codec.aspect[0] = 4;
        info.sw_codec.aspect[1] = 3;
        dvb_config_save_and_update( &info );
        an_configure_capture_card(CAP_DEV_TYPE_UVCVIDEO);
    }

    if(info.cap_dev_type == CAP_DEV_TYPE_DL_MINI_RECORDER){
        info.video_bitrate     = calculate_video_bitrate();
        info.audio_bitrate     = 192000;
        info.sw_codec.using_sw_codec = true;
        info.sw_codec.aspect[0] = 16;
        info.sw_codec.aspect[1] = 9;
        dvb_config_save_and_update( &info );
        an_configure_capture_card(CAP_DEV_TYPE_DL_MINI_RECORDER);
    }

    if(info.cap_format.video_format == CAP_PAL ){
       // dvb_cap_set_analog_standard( m_i_fd, V4L2_STD_PAL );
       //info.sw_codec.aspect[0] = 4;
       //info.sw_codec.aspect[1] = 3;
    }

    if(info.cap_format.video_format == CAP_NTSC ){
        dvb_cap_set_analog_standard( m_i_fd, V4L2_STD_NTSC );
        info.sw_codec.aspect[0] = 4;
        info.sw_codec.aspect[1] = 3;
    }

#endif

    m_cap_update = false;
}
//
// Called from the capturing process to see if the video/audio capture parameters
// need changing.
//
void dvb_cap_check_for_update(void)
{
    // Get exclusive access
    if( m_cap_update == true )
    {
        dvb_cap_ctl();
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

    if( open_named_capture_device( info.video_capture_device_name ) < 0 )
    {
        loggerf("Video Capture device open failed");
        return -1;
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
    if(info.cap_dev_type == CAP_DEV_TYPE_DL_MINI_RECORDER){
        dl_close();
        return;
    }

    // No need to close the device if it is a UDP connection
    if(info.video_capture_device_class == DVB_UDP_TS)
        return;

    if( m_i_fd > 0 )
    {
       close( m_i_fd );
    }

#ifdef _USE_SW_CODECS
    if(info.video_capture_device_class == DVB_YUV )
        snd_pcm_close(m_audio_handle);
    an_stop_capture();
#endif
    m_i_fd = 0;
}
//
// Initialisation of the capture device
//
int dvb_cap_init( void )
{
    pes_process();// reset the modules
    pes_reset();
    if( dvb_open_capture_device() >= 0 )
    {
        m_cap_status = 0;
        dvb_cap_ctl();
        cap_purge();
    }
    else
    {
        m_cap_status = -1;
    }
    return m_cap_status;
}
