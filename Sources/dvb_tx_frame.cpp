#include <memory.h>
#include "dvb_config.h"
#include "dvb.h"
#include "DVB-T/dvb_t.h"
#include "dvb_s2_if.h"
#include "mp_tp.h"
#include "dvb_capture_ctl.h"

static sys_config m_info;
static uchar m_dibit[2000];
static u_int32_t m_null_seperation;
static u_int32_t m_null_seperation_limit;
static u_int32_t m_null_spacing;
static u_int32_t m_null_spacing_limit;
// NULLS cannot be closer than this
#define NULL_SPACING_LIMIT 20
// Maximum NULL packet seperation
#define NULL_SEPERATION_LIMIT 500

void dvb_tx_frame_init(void)
{
    calculate_video_bitrate();
    // This amounts to a 10 mS spacing
    m_null_spacing_limit    = get_raw_bitrate()/(204*8*100);
    m_null_seperation_limit = NULL_SEPERATION_LIMIT;
    dvb_config_get( &m_info );
    dvb_s2_start();
}

bool below_null_threshold(void)
{
    if(ts_queue_percentage() < 50) return true;
    return false;
}

bool dvb_tx_time_to_send_null(void)
{
    if(below_null_threshold())
    {
        if( m_null_spacing == 0 )
        {
            m_null_seperation = 0;
            m_null_spacing    = m_null_spacing_limit;
            return true;
        }
        else
        {
            if( m_null_spacing > 0 ) m_null_spacing--;
        }
    }

    m_null_seperation++;

    if( m_null_seperation >= m_null_seperation_limit)
    {
        m_null_seperation = 0;
        m_null_spacing    = m_null_spacing_limit;
        return true;
    }
    return false;
}

void dvb_tx_transmit_tp_dvbs( uchar *tp )
{
    uchar *ntp;
    write_final_tx_queue_ts( tp );
    tp_file_logger_log( tp, 188 );
    if(dvb_tx_time_to_send_null())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        tp_file_logger_log( ntp, 188 );
        write_final_tx_queue_ts(ntp);
    }
}

void dvb_tx_encode_and_transmit_tp_dvbs( uchar *tp )
{
    uchar *ntp;
    dvb_s_encode_and_modulate( tp, m_dibit );
    tp_file_logger_log( tp, 188 );
    // Encode and queue it for transmission
    if(dvb_tx_time_to_send_null())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        // Log if required
        tp_file_logger_log( ntp, 188 );
        // Encode
        dvb_s_encode_and_modulate( ntp, m_dibit);
        // Encode and queue it for transmission
    }
}
void dvb_tx_encode_and_transmit_tp_dvbs2( uchar *tp )
{
    uchar *ntp;
    dvb_s2_encode_tp( tp );
    tp_file_logger_log( tp, 188 );
    if(dvb_tx_time_to_send_null())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        tp_file_logger_log( ntp, 188 );
        dvb_s2_encode_tp(ntp);
    }
}
void dvb_tx_encode_and_transmit_tp_dvbt( uchar *tp )
{
    uchar *ntp;
    dvb_t_encode_and_modulate( tp, m_dibit );
    tp_file_logger_log( tp, 188 );

    if(dvb_tx_time_to_send_null())
    {
        ntp = get_padding_null_dvb();
        // Update the PCR clock
        pcr_transport_packet_clock_update();
        tp_file_logger_log( ntp, 188 );
        dvb_t_encode_and_modulate(ntp, m_dibit);
        m_null_seperation = 0;
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

    switch( m_info.tx_hardware )
    {
    case HW_EXPRESS_AUTO:
        if( m_info.dvb_mode == MODE_DVBS )  dvb_tx_transmit_tp_dvbs( tp );
        if( m_info.dvb_mode == MODE_DVBS2 ) dvb_tx_encode_and_transmit_tp_dvbs2( tp );
        if( m_info.dvb_mode == MODE_DVBT )  dvb_tx_encode_and_transmit_tp_dvbt( tp );
        break;
    case HW_EXPRESS_16:
    case HW_EXPRESS_8:
        {
            switch( m_info.dvb_mode )
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
        if( m_info.dvb_mode == MODE_DVBS ) dvb_tx_transmit_tp_dvbs( tp );
        break;
    case HW_EXPRESS_UDP:
        if( m_info.dvb_mode == MODE_DVBS ) write_final_tx_queue_udp( tp );
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
