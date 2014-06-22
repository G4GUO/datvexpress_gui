// dvbs.cpp : Defines the entry point for the console application.
//
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <fcntl.h>
#include <memory.h>
#include <sys/types.h>
#include <syscall.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <memory.h>
#include <syscall.h>
#include <time.h>

#include "dvb_gen.h"
#include "dvb_gui_if.h"
#include "dvb.h"
#include "dvb_config.h"
#include "dvb_capture_ctl.h"
#include "DVB-T/dvb_t.h"
#include "dvb_gui_if.h"
#include "dvb_s2_if.h"
//#include "testcard.h"
//#include "Transcoder.h"
#include "tx_hardware.h"
#include "mp_tp.h"
#include "an_capture.h"

//Transcoder transcoder;

static int m_dvb_major_txrx_status;
static int m_dvb_minor_txrx_status;

static int m_dvb_running;
static int m_dvb_capture_running;

static pthread_t dvb_thread[5];

static bool m_input_device_ok;
static bool m_output_device_ok;

void dvb_block_rx_check( void )
{
    struct timespec tim;

    tim.tv_sec  = 0;
    tim.tv_nsec = 100000000; // 100 ms
return;
    while((m_dvb_major_txrx_status == DVB_RECEIVING)&&m_dvb_running )
    {
        nanosleep( &tim, NULL);
    }
}

int dvb_is_system_running( void )
{
    return m_dvb_running;
}

//
// This task parses the capture stream
//
void *dvb_task_capture_blocking( void *arg )
{
    // Purge the capture device
    cap_purge();
    // This blocks
    while( m_dvb_capture_running )
    {
        dvb_block_rx_check();
        cap_parse_instream();
    }
    return arg;
}
//
// This task manages the transmit stream
// (tx_hardware)
//
void *dvb_task_transmit( void *arg )
{
    while( m_dvb_running )
    {
        hw_thread();
    }
    return arg;
}
//
// Read transport packets from the UDP socket and queue them
//
void *udp_proc( void *arg )
{
    while( m_dvb_running )
    {
        uchar *tp = udp_get_transport_packet();
        if(tp != NULL) tx_write_transport_queue( tp );
        sleep(0);
    }
    return arg;
}
//
// This process adds the SI tables
//
void *si_proc( void *arg )
{
    struct timespec tim;

    tim.tv_sec  = 0;
    tim.tv_nsec = 10000000; // 10 ms

    while( m_dvb_capture_running )
    {
        ts_multi_stream();
        // So the task does not hog all the CPU
        nanosleep( &tim, NULL);
    }
    return arg;
}
void dvb_serious_error( void )
{
    loggerf("Serious error encountered, please restart the program");
    while(1)
    {
          sleep(1);
    }
}
//
// Initialises the process threads
//
int dvb_initialise_system(void)
{
    sys_config cfg;
    int res;
    dvb_config_get( &cfg );

    //if((res=init_firewire())<0)loggerf("Firewire failed %d\n",res);
    m_dvb_running = 1;// This keeps threads running, assume it is going to work
    m_dvb_capture_running = 1;

    // Set the hw frequency and the level to zero as we are in rx at start
    hw_config( cfg.tx_frequency, 0 );

    if((cfg.tx_hardware == HW_EXPRESS_16)||
       (cfg.tx_hardware == HW_EXPRESS_AUTO)||
       (cfg.tx_hardware == HW_EXPRESS_8)||
       (cfg.tx_hardware == HW_EXPRESS_TS)||
       (cfg.tx_hardware == HW_EXPRESS_UDP))
    {
        // Create the Transmit sample thread
        res = pthread_create( &dvb_thread[0], NULL, dvb_task_transmit, NULL );
        if( res!= 0 )
        {
            logger("DVB Thread transmit creation failed");
        }
    }

    if( cfg.capture_device_type == DVB_UDP_TS )
    {
        // Create the UDP receive socket for capture
        res = pthread_create( &dvb_thread[1], NULL, udp_proc, NULL );
        if( res!= 0 )
        {
            logger("DVB Thread UDP creation failed");
        }
    }

    if( cfg.capture_device_type == DVB_V4L )
    {
        if( cfg.capture_stream_type == DVB_PROGRAM )
        {
            // Create the master thread that reads from the video capture device
            res = pthread_create( &dvb_thread[2], NULL, dvb_task_capture_blocking, NULL );
            if( res!= 0 )
            {
                logger("DVB Thread (Program) capture blocking creation failed");
            }
        }

        if( cfg.capture_stream_type == DVB_TRANSPORT )
        {
            // Create the master thread that reads from the video capture device
            res = pthread_create( &dvb_thread[2], NULL, dvb_task_capture_blocking, NULL );
            if( res!= 0 )
            {
                logger("DVB Thread (Transport) capture blocking creation failed");
            }
        }

#ifdef _USE_SW_CODECS
        if( cfg.capture_stream_type == DVB_YUV )
        {
            // Start the analog capture processes
            an_start_capture();
        }
#endif
    }

    // Create the timer thread, required for all varients
    res = pthread_create( &dvb_thread[3], NULL, timer_proc, NULL );
    if( res!= 0 ) loggerf("DVB Thread Timer creation failed");

    if( cfg.capture_device_type != DVB_UDP_TS )
    {
        // Create the SI thread, required for all varients except UDP capture
        res = pthread_create( &dvb_thread[4], NULL, si_proc, NULL );
        if( res!= 0 ) loggerf("DVB Thread SI creation failed");
    }

    return(0);
}
//
// This starts off the process thread that
// captures video from the card and processes it.
//
int dvb_start( void )
{
    m_dvb_running = 0;// Not running yet
    m_dvb_capture_running = 0;
    m_input_device_ok  = false;
    m_output_device_ok = false;

    // Initial configuration
    dvb_config_retrieve_from_disk();
    sys_config cfg;
    dvb_config_get( &cfg );

    // Run all the module initialisation functions
    dvb_t_init();
    dvb_modulate_init();
    display_logger_init();
    dvb_si_init();
    dvbs_modulate_init();
    dvb_encode_init();
    dvb_interleave_init();
    dvb_conv_init();
    dvb_rs_init();
    dvb_tx_frame_init();
    create_final_tx_queue();
    tx_init_transport_flow();
    dvb_teletext_init();
    dvb_ts_if_init();
    eq_initialise();
    tp_file_logger_init();
    pcr_scr_init();
    dvb_s2_start();// Must be called before cap is initialised

    // Now configure the ouput devices

    if( hw_tx_init() == EXP_OK )
    {
        m_output_device_ok = true;
    }

    // Now configure the capture device

    if( cfg.capture_device_type == DVB_UDP_TS )
    {
        // Using UDP input so no capture device required
        if(udp_rx_init()==0) m_input_device_ok = true;
    }

    if( cfg.capture_device_type == DVB_V4L )
    {
        // Using direct video input, so capture device required
        // This maybe a hardware encoder or raw video input
        if(dvb_cap_init()==0)
        {
            m_input_device_ok = true;
        }            
    }

    if((m_input_device_ok == true)&&( m_output_device_ok == true))
    {
        dvb_initialise_system();
        return 0;
    }
    else
    {
        dvb_close_capture_device();
        loggerf("Running in demo mode");
        m_dvb_running = 0;
    }
    return(-1);
}
//
// Called when program is closing
//
void dvb_stop( void )
{
    struct timespec tim;
    // Terminate video capture
    m_dvb_capture_running = 0;
#ifdef _USE_SW_CODECS
    // Stop the analog capture processes
    an_stop_capture();
#endif
    // Wait
    tim.tv_sec  = 1;//1 sec
    tim.tv_nsec = 0;// s
    nanosleep( &tim, NULL);
    // Terminate all other processes
    m_dvb_running         = 0;
    dvb_t_deinit();

    // Close the logging file
    tp_file_logger_stop();
    // Close the capture device
    dvb_close_capture_device();
    // Close the DATV-Express hardware
    express_deinit();
}
//
// We have requested new parameters
//
int dvb_get_major_txrx_status( void )
{
    return m_dvb_major_txrx_status;
}
int dvb_get_minor_txrx_status( void )
{
    return m_dvb_minor_txrx_status;
}
//
// Set the system to either
// transmitting
// receiving
// calibrating
//
void dvb_set_major_txrx_status( int status )
{
    //struct timespec tim;
    sys_config cfg;
    dvb_config_get( &cfg );

    //tim.tv_sec  = 0;
    //tim.tv_nsec = 50000000; // 50 ms

    if( status == DVB_RECEIVING)
    {
        // Go to receive
        m_dvb_major_txrx_status = status;// Stops output
        hw_level(0);
        hw_rx();
        eq_receive();
        return;
    }

    if( status == DVB_TRANSMITTING )
    {
        // Go to transmit
        //eq_transmit();
        hw_tx();
        // Delay while relays change over
        hw_level(cfg.tx_level);
        hw_freq(cfg.tx_frequency);
        m_dvb_major_txrx_status = status;
        eq_transmit();
        return;
    }

    if( status == DVB_CALIBRATING )
    {
        m_dvb_major_txrx_status = status;// Stops output
        hw_level(47);// Full output
        return;
    }
}
//
// This is a sub status within the main status i.e during calibration
// what it is actually calibrating.
//
void dvb_set_minor_txrx_status( int status )
{
    m_dvb_minor_txrx_status = status;
}

const char *dvb_firmware_get_path( const char *filename, char *pathname )
{
    sprintf(pathname,"%s%s","/lib/firmware/datvexpress/",filename);
    return pathname;
}

void dvb_set_testcard( int status )
{
    status = status;
//    if( status == 1 )
//        transcoder.SetCurrentPicture(1);
//    else
//        transcoder.SetCurrentPicture(0);
}

