#include <memory.h>
#include "dvb_config.h"
#include "dvb.h"
#include "DVB-T/dvb_t.h"
#include "dvb_s2_if.h"
#include "mp_tp.h"

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
void dvb_tx_encode_and_transmit_tp_dvbs( uchar *tp )
{
    dvb_s_encode_and_modulate( tp, m_dibit );
    if(below_null_threshold())
    {
        dvb_s_encode_and_modulate(get_padding_null_dvb(), m_dibit);
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            dvb_s_encode_and_modulate(get_padding_null_dvb(), m_dibit);
            m_null_seperation = 0;
        }
    }
}
void dvb_tx_encode_and_transmit_tp_dvbs2( uchar *tp )
{
    dvb_s2_encode_tp( tp );
    if(below_null_threshold())
    {
        dvb_s2_encode_tp(get_padding_null_dvb());
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            dvb_s2_encode_tp(get_padding_null_dvb());
            m_null_seperation = 0;
        }
    }
}
void dvb_tx_encode_and_transmit_tp_dvbt( uchar *tp )
{
    dvb_t_encode_and_modulate( tp, m_dibit );
    if(below_null_threshold())
    {
        dvb_t_encode_and_modulate(get_padding_null_dvb(), m_dibit);
        m_null_seperation = 0;
    }
    else
    {
        m_null_seperation++;
        if( m_null_seperation >= MAX_NULL_SEPERATION)
        {
            dvb_t_encode_and_modulate(get_padding_null_dvb(), m_dibit);
            m_null_seperation = 0;
        }
    }
}

//
// Encode and transmit a transport packet
// This accepts a 188 byte packet and is semaphore protected
//
void dvb_tx_encode_and_transmit_tp( uchar *tp )
{
    //
    // Send to monitor port
    //
    //vt_queue_tp( tp );

    //
    // Modulate
    //
    switch( m_tx_hardware )
    {
    case HW_EXPRESS_AUTO:
        if( m_dvb_mode == MODE_DVBS )
        {
            write_final_tx_queue_ts( tp );
            if(below_null_threshold())
            {
                write_final_tx_queue_ts(get_padding_null_dvb());
                m_null_seperation = 0;
            }
            else
            {
                m_null_seperation++;
                if( m_null_seperation >= MAX_NULL_SEPERATION)
                {
                    write_final_tx_queue_ts(get_padding_null_dvb());
                    m_null_seperation = 0;
                }
            }
        }
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
        {
            if( m_dvb_mode == MODE_DVBS )
            {
                write_final_tx_queue_ts( tp );
                if(below_null_threshold())
                {
                    write_final_tx_queue_ts(get_padding_null_dvb());
                    m_null_seperation = 0;
                }
                else
                {
                    m_null_seperation++;
                    if( m_null_seperation >= MAX_NULL_SEPERATION)
                    {
                        write_final_tx_queue_ts(get_padding_null_dvb());
                        m_null_seperation = 0;
                    }
                }
            }
        }
        break;
    case HW_EXPRESS_UDP:
        {
            if( m_dvb_mode == MODE_DVBS ) write_final_tx_queue_udp( tp );
        }
        break;
    default:
        break;
        }
    }
}
