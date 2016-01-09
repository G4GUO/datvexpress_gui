#ifndef AN_CAPTURE_H
#define AN_CAPTURE_H

#define  __STDC_CONSTANT_MACROS
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <queue>
#include <deque>
#include <linux/videodev2.h>
#include "dvb.h"


using namespace std ;

#ifdef _USE_SW_CODECS
// AVCONV is a C application so we need to tell everyone
// so the linker knows what we are talking about
//
#ifdef HAVE_AV_CONFIG_H
#undef HAVE_AV_CONFIG_H
#endif

extern "C"
{
#include <alsa/asoundlib.h>
#include <libavutil/mem.h>
#include <libavformat/avformat.h>
#include <libavfilter/avfiltergraph.h>
#include <libavutil/pixfmt.h>
#include <libavutil/pixdesc.h>
#include <libavfilter/buffersink.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/opt.h>
#include <libavcodec/avcodec.h>
#include <libavutil/channel_layout.h>
#include <libavutil/common.h>
#include <libavutil/imgutils.h>
#include <libavutil/mathematics.h>
#include <libavutil/samplefmt.h>
}
#endif

// Set the capture size of the image
void an_configure_capture_card( int dev );
// Initialise this module
int an_init( v4l2_format *fmt );
// capture an image and sound
void an_start_capture(void);
void an_stop_capture(void);
void an_process_captured_video_buffer( uint8_t *b, AVPixelFormat fmt);
void an_process_capture_audio(uint8_t *b, int bytes);

#endif // AN_CAPTURE_H
