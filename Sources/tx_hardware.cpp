#include <iostream>
#include <complex>
#include <pthread.h>
#include <memory.h>
//#include <boost/program_options.hpp>
//#include <boost/format.hpp>
#include "stdio.h"
#include "dvb.h"
#include "dvb_uhd.h"
#include "dvb_gen.h"

static double m_tx_rate;
static sys_config m_config;
int m_carrier; // If set will transmit only a carrier
//
// Transmit a tone at 1/4 the symbol rate on LSB
// This is used for unwanted sideband testing
//

void transmit_sideband( scmplx *samples, int len )
{
    short vals[4] = {
        (short)0x7FFF,
        (short)0x0000,
        (short)0x8000,
        (short)0x0000
    };

    static int p;

    for( int i = 0; i < len ;i++)
    {
        samples[i].re = vals[p];
        p = (p+1)%4;
        samples[i].im = vals[p];
    }
}

//
// Transmit processing thread, called from main dvb module.
//
void hw_thread( void )
{
    // Read from the queue
//    struct timespec tim;

//    tim.tv_sec = 0;
//    tim.tv_nsec = 1000;

    dvb_buffer *b = read_final_tx_queue();

    if( b == NULL ) return;

    if( b->type == BUF_TS )
    {
        express_send_dvb_buffer( b );
        return;
    }

    if( b->type == BUF_UDP )
    {
        udp_send_tp( b );
        return;
    }

    if(b->type == BUF_SCMPLX )
    {
        if( b->len > 0 )
        {
            // This will block on the USB queue until done
            express_send_dvb_buffer( b );
        }
    }
}
//
// Sample rate being sent to Hardware
//
double hw_uniterpolated_sample_rate(void)
{
    return m_tx_rate;
}
//
// Number of samples outstranding on the Hardware
//
double hw_outstanding_queue_size( void )
{
    return express_outstanding_queue_size();
}

//
// Transmit control routines
//
void hw_freq( double freq )
{
    switch(m_config.tx_hardware)
    {
    case HW_EXPRESS_AUTO:
    case HW_EXPRESS_16:
    case HW_EXPRESS_8:
        express_set_freq( freq );
        break;
    case HW_EXPRESS_TS:
        break;
    }
    // select the right amplifiers etc
    eq_change_frequency( freq );
}

void hw_level( float gain )
{
    switch(m_config.tx_hardware)
    {
    case HW_EXPRESS_AUTO:
    case HW_EXPRESS_16:
    case HW_EXPRESS_8:
        express_set_level((int)gain);
        break;
    case HW_EXPRESS_TS:
        break;
    }
}

void hw_sample_rate( double rate )
{
    switch(m_config.tx_hardware)
    {
    case HW_EXPRESS_AUTO:
    case HW_EXPRESS_16:
    case HW_EXPRESS_8:
        express_set_sr( rate );
        break;
    case HW_EXPRESS_TS:
        break;
    }
}


void hw_config( double freq, float lvl)
{
    switch(m_config.tx_hardware)
    {
    case HW_EXPRESS_AUTO:
    case HW_EXPRESS_16:
    case HW_EXPRESS_8:
        express_set_freq( freq );
        express_set_level((int)lvl);
        break;
    case HW_EXPRESS_TS:
        break;
    }
}
//
// Sets the appropriate filter in the FPGA for the mode  in use
//
void hw_set_interp_and_filter( int rate )
{
    if((m_config.tx_hardware == HW_EXPRESS_16) ||
       (m_config.tx_hardware == HW_EXPRESS_AUTO) ||
       (m_config.tx_hardware == HW_EXPRESS_8) )
    {
        if( m_config.dvb_mode == MODE_DVBS )
        {
            express_set_interp( rate );
            express_set_filter( 0 );
            express_set_fec( m_config.dvbs_fmt.fec );
        }

        if( m_config.dvb_mode == MODE_DVBS2 )
        {
            express_set_interp( rate );
            if(m_config.dvbs2_fmt.roll_off == RO_0_35 ) express_set_filter( 0 );
            if(m_config.dvbs2_fmt.roll_off == RO_0_25 ) express_set_filter( 1 );
            if(m_config.dvbs2_fmt.roll_off == RO_0_20 ) express_set_filter( 2 );
        }

        if( m_config.dvb_mode == MODE_DVBT )
        {
            express_set_interp( rate );
            express_set_filter( 3 );
        }

        if( m_config.dvb_mode == MODE_DVBT2 )
        {
            express_set_interp( rate );
            express_set_filter( 3 );
        }
    }
}
int hw_set_dvbt_sr( double &srate )
{
    int irate;
    srate = dvb_t_get_sample_rate();
    irate = express_set_sr(srate);
    return irate;
}
void hw_setup_channel(void)
{
    int irate = 0;
    double srate = 0;

    // Put the FPGA into a known state
    express_fpga_reset();

    // Call the right configuration dependent on hardware
    if((m_config.tx_hardware == HW_EXPRESS_16)   ||
       (m_config.tx_hardware == HW_EXPRESS_AUTO) ||
       (m_config.tx_hardware == HW_EXPRESS_8) )
     {
        if( m_config.dvb_mode == MODE_DVBS )
        {
            srate = m_config.sr_mem[m_config.sr_mem_nr];
            irate = express_set_sr( srate );
            hw_set_interp_and_filter( irate );
        }

        if( m_config.dvb_mode == MODE_DVBS2 )
        {
            srate = m_config.sr_mem[m_config.sr_mem_nr];
            irate = express_set_sr(srate);
            hw_set_interp_and_filter( irate );
        }

        if( m_config.dvb_mode == MODE_DVBT )
        {
            irate = hw_set_dvbt_sr( srate );
            hw_set_interp_and_filter( irate );
        }

        if( m_config.dvb_mode == MODE_DVBT2 )
        {
            irate = hw_set_dvbt_sr( srate );
            hw_set_interp_and_filter( irate );
        }
    }
    m_tx_rate = srate;
}

int hw_tx_init( void )
{
    int res  = -1;

    // Get information about the system
    dvb_config_get( &m_config );

    // Call the right configuration dependent on hardware
    switch( m_config.tx_hardware )
    {
    case HW_EXPRESS_AUTO :
        if(m_config.dvb_mode == MODE_DVBS )
            if((res = express_init("datvexpress8.ihx","datvexpressdvbs.rbf"))== EXP_OK) hw_setup_channel();
        if(m_config.dvb_mode == MODE_DVBS2 )
            if((res = express_init("datvexpress16.ihx","datvexpress.rbf"))   == EXP_OK) hw_setup_channel();
        if(m_config.dvb_mode == MODE_DVBT )
            if((res = express_init("datvexpress16.ihx","datvexpress.rbf"))   == EXP_OK) hw_setup_channel();
        break;
    case HW_EXPRESS_16 :
    case HW_EXPRESS_8  :
        if((res = express_init("datvexpress16.ihx","datvexpress.rbf"))==EXP_OK) hw_setup_channel();
        break;
    case HW_EXPRESS_TS:
        if((res = express_init("datvexpress8.ihx","datvexpressdvbs.rbf"))==EXP_OK) hw_setup_channel();
        break;
    case HW_EXPRESS_UDP:
        res= udp_tx_init();
        if(res==0)
        {
            res = EXP_OK;
            hw_setup_channel();
        }
        break;
    }
    m_carrier = 0;
    return res;
}

void hw_set_carrier( int status )
{
    if(status)
        express_set_carrier(true);
    else
        express_set_carrier(false);

    m_carrier = status;
}

int hw_get_carrier( void )
{
    return m_carrier;
}
void hw_tx( void )
{
    if((m_config.tx_hardware == HW_EXPRESS_16) ||
       (m_config.tx_hardware == HW_EXPRESS_AUTO) ||
       (m_config.tx_hardware == HW_EXPRESS_8) )
    {
        express_transmit();
    }
}
void hw_rx( void )
{
    if((m_config.tx_hardware == HW_EXPRESS_16) ||
       (m_config.tx_hardware == HW_EXPRESS_AUTO) ||
       (m_config.tx_hardware == HW_EXPRESS_8) )
    {
        express_receive();
    }
}
void hw_load_calibration( void )
{
    express_load_calibration();
}
