//
// This module inserts NULL packets into the tx flow
// this is done to provide automatic control of the 
// tx buffering.
// It also multiplexes in the necessary NIS table packets.
//
#include <stdio.h>
#include <time.h>
#include <memory.h>
#include <stdlib.h>
#include <semaphore.h>
#include <queue>
#include <deque>
#include <list>
#include "dvb.h"
#include "mp_tp.h"
#include "dvb_s2_if.h"

using namespace std;
extern int m_final_txq_len;

static sem_t tp_sem;

extern bool m_pat_flag;
extern bool m_tdt_flag;
extern bool m_pmt_flag;
extern bool m_eit_flag;
extern bool m_sdt_flag;
extern bool m_nit_flag;
extern bool m_tdt_flag;

static int m_null_count;
static int m_null_items;

//
// Used to tell how many nulls have been sent.
//
void increment_null_count( void )
{
    m_null_count++;
}
//
// read the number of NULLS sent and refresh them
//
int flow_read_null_count( void )
{
    int ival     = m_null_count;
    m_null_count = 0;
    return ival;
}
int tx_queue_percentage(void)
{
    return ((final_tx_queue_size()*100)/m_final_txq_len);
}
//
// This transmits the SI tables necessary for a single ts.
//
int ts_single_stream( void )
{
    //
    // Send the appropriate SI packets at the appropriate time
    // refer to ETSI TR 101 211 page 36
    //
    // Only send one at a time to prevent timing issues
    //
    // We are either not at the transmitter
    // or we are combined station so send SI tables

    if( m_pmt_flag == true )
    {
        pmt_dvb();
        m_pmt_flag = false;
        return 1;
    }

    if( m_eit_flag == true )
    {
        eit_dvb();
        m_eit_flag = false;
        return 1;
    }

    if( m_sdt_flag == true )
    {
        sdt_dvb();
        m_sdt_flag = false;
        return 1;
    }

    if( m_nit_flag == true )
    {
        nit_dvb();
        m_nit_flag = false;
        return 1;
    }

    if( m_tdt_flag == true )
    {
        tdt_dvb();
        m_tdt_flag = false;
        return 1;
    }
    return 0;
}
//
// This transmits the SI tables common to all channels and does any padding.
//
int ts_multi_stream( void )
{
    //
    // Send the appropriate SI packets at the appropriate time
    // refer to ETSI TR 101 211 page 36
    //

    // We are either not at the transmitter
    // or we are combined station so send SI tables
    if( m_pat_flag == true )
    {
        pat_dvb();
        m_pat_flag = false;
        return 1;
    }

    if( m_tdt_flag == true )
    {
        tdt_dvb();
        m_tdt_flag = false;
        return 1;
    }
    return 0;
}
//
// Called multistream to do the parequired padding
//
int ts_multi_pad( void )
{
    if( tx_queue_percentage() < 10 )
    {
        // Emergency measures to stop dropouts
        padding_null_dvb();
        return 1;
    }
    return 0;
}

//
// Write to the semaphore protected transport queue
// All transmitting tasks applications write to this queue
// 188 byte transport packets are queued on each evocation.
//
// This is the big daddy.
//
void tx_write_transport_queue( uchar *tp )
{
    sem_wait( &tp_sem );
    // Update the PCR clock
    pcr_transport_packet_clock_update();
    // Log it to file if required
    tp_file_logger_log( tp, 188 );
    // Encode and queue it for transmission
    dvb_tx_encode_and_transmit_tp( tp );
    // Done
    sem_post( &tp_sem );
}
//
// Don't update the PCR clock
//
void tx_write_transport_queue_silent( uchar *tp )
{
    sem_wait( &tp_sem );
    // Log it to file if required
    tp_file_logger_log( tp, 188 );
    // Encode and queue it for transmission
    dvb_tx_encode_and_transmit_tp( tp );
    // Done
    sem_post( &tp_sem );
}
//
// Elementary streams call this to prevent re-entrancy
//
void tx_write_transport_queue_elementary( uchar *tp )
{
    tx_write_transport_queue( tp );
    // Send SI if required
    ts_single_stream();
}
//
// Initialise this module
//
void tx_init_transport_flow( void )
{
    sem_init( &tp_sem, 0, 0 );
    sem_post( &tp_sem );
    m_null_items = 0;
}
