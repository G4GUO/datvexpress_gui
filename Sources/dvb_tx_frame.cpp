#include <memory.h>
#include "dvb_config.h"
#include "dvb.h"
#include "DVB-T/dvb_t.h"
#include "dvb_s2_if.h"
#include "mp_tp.h"
#include "dvb_capture_ctl.h"

static uchar m_dibit[2000];
static int m_tx_hardware;
static int m_dvb_mode;
static u_int32_t m_null_seperation;

int below_null_threshold(void)
{
    static unsigned int count;

    if(tx_queue_percentage() < 10) return 1;
    if(tx_queue_percentage() < 30) count++;
    if(count > 100 )
    {
        count = 0;
        return 1;
    }
    return 0;
}

void dvb_tx_frame_init(void)
{
    sys_config info;
    dvb_config_get( &info );
    m_tx_hardware = info.tx_hardware;
    m_dvb_mode = info.dvb_mode;

    dvb_s2_start();
}
void dvb_tx_transmit_tp_dvbs( uchar *tp )
{
    uchar *ntp;
    write_final_tx_queue_ts( tp );
    tp_file_logger_log( tp, 188 );
    if(below_null_threshold())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        tp_file_logger_log( ntp, 188 );
        write_final_tx_queue_ts(ntp);
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            ntp = get_padding_null_dvb();
            // Update the PCR clock
            pcr_transport_packet_clock_update();
            tp_file_logger_log( ntp, 188 );
            write_final_tx_queue_ts(ntp);
            m_null_seperation = 0;
        }
    }
}

void dvb_tx_encode_and_transmit_tp_dvbs( uchar *tp )
{
    uchar *ntp;
    dvb_s_encode_and_modulate( tp, m_dibit );
    tp_file_logger_log( tp, 188 );
    // Encode and queue it for transmission
    if(below_null_threshold())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        // Log if required
        tp_file_logger_log( ntp, 188 );
        // Encode
        dvb_s_encode_and_modulate( ntp, m_dibit);
        // Encode and queue it for transmission
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            ntp = get_padding_null_dvb();
            // Update the PCR clock
            pcr_transport_packet_clock_update();
            tp_file_logger_log( ntp, 188 );
            dvb_s_encode_and_modulate(ntp, m_dibit);
            m_null_seperation = 0;
        }
    }
}
void dvb_tx_encode_and_transmit_tp_dvbs2( uchar *tp )
{
    uchar *ntp;
    dvb_s2_encode_tp( tp );
    tp_file_logger_log( tp, 188 );
    if(below_null_threshold())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        tp_file_logger_log( ntp, 188 );
        dvb_s2_encode_tp(ntp);
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            ntp = get_padding_null_dvb();
            // Update the PCR clock
            pcr_transport_packet_clock_update();
            tp_file_logger_log( ntp, 188 );
            dvb_s2_encode_tp(ntp);
            m_null_seperation = 0;
        }
    }
}
void dvb_tx_encode_and_transmit_tp_dvbt( uchar *tp )
{
    uchar *ntp;
    dvb_t_encode_and_modulate( tp, m_dibit );
    tp_file_logger_log( tp, 188 );
    if(below_null_threshold())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        tp_file_logger_log( ntp, 188 );
        dvb_t_encode_and_modulate(ntp, m_dibit);
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            ntp = get_padding_null_dvb();
            // Update the PCR clock
            pcr_transport_packet_clock_update();
            tp_file_logger_log( ntp, 188 );
            dvb_t_encode_and_modulate(ntp, m_dibit);
            m_null_seperation = 0;
        }
    }
}

//
// Encode and transmit a transport packet
// This accepts a 188 byte packet and is semaphore protected
//
void dvb_tx_encode_and_transmit_tp_raw( uchar *tp )
{
    //
    // Send to monitor port
    //
    //vt_queue_tp( tp );
    //
    // Modulate
    //

    // Update the PCR clock
    pcr_transport_packet_clock_update();

    switch( m_tx_hardware )
    {
    case HW_EXPRESS_AUTO:
        if( m_dvb_mode == MODE_DVBS )  dvb_tx_transmit_tp_dvbs( tp );
        if( m_dvb_mode == MODE_DVBS2 ) dvb_tx_encode_and_transmit_tp_dvbs2( tp );
        if( m_dvb_mode == MODE_DVBT )  dvb_tx_encode_and_transmit_tp_dvbt( tp );
        break;
    case HW_EXPRESS_16:
    case HW_EXPRESS_8:
        {
            switch( m_dvb_mode )
            {
            case MODE_DVBS:
                dvb_tx_encode_and_transmit_tp_dvbs( tp );
                break;
            case MODE_DVBS2:
                dvb_tx_encode_and_transmit_tp_dvbs2( tp );
                break;
            case MODE_DVBT:
                dvb_tx_encode_and_transmit_tp_dvbt( tp );
                break;
            case MODE_DVBT2:
                break;
            default:
                break;
        }
        break;
    case HW_EXPRESS_TS:
        if( m_dvb_mode == MODE_DVBS ) dvb_tx_transmit_tp_dvbs( tp );
        break;
    case HW_EXPRESS_UDP:
        if( m_dvb_mode == MODE_DVBS ) write_final_tx_queue_udp( tp );
        break;
    default:
        break;
        }
    }
}
//
// Add a PCR packet if needed
// Transmit the tp it had been given
//
void dvb_tx_encode_and_transmit_tp( uchar *tp )
{
    // See if a PCR packet on a seperate ts needs to be added
    cap_pcr_to_ts();
    dvb_tx_encode_and_transmit_tp_raw( tp );
}
