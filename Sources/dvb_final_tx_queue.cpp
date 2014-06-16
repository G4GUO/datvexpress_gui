#include <queue>
#include <deque>
#include <list>
#include <queue>
#include <semaphore.h>
#include <pthread.h>
#include <memory.h>
#include <string.h>
#include <stdlib.h>
#include "dvb_uhd.h"
#include <queue>
#include <deque>
#include <list>
#include "dvb.h"
#include "dvb_gen.h"
#include "dvb_config.h"
#include "tx_hardware.h"
#include "dvb_buffer.h"
#include "dvb_capture_ctl.h"
#include "../DVB-T/dvb_t.h"

using namespace std;
unsigned int m_final_txq_len;
extern const sys_config m_sysc;

static sem_t work_sem;
static pthread_mutex_t mutex;
static queue <dvb_buffer *> m_tx_q;

//
// This is the time delay through the system
// It can be used to calculate the delay and
// time offsets.
//
double final_txq_time_delay( void )
{
    double rate,fdelay;
    // Get the un interpolated sample clock rate
    rate = hw_uniterpolated_sample_rate();
    // Get the size of the unsent USB data in IQ samples
    double qout = hw_outstanding_queue_size();
    // Get the number of outstanding samples on the transmit queue
    double fq_size = m_tx_q.size();
    // Calculate the total number of outstanding samples
    fq_size        = fq_size * TX_BUFFER_LENGTH;
    fq_size       += qout;
    // Calculate the resultant time delay
    fdelay = fq_size / rate;
    return fdelay;
}
//
// Read percentage full of final queue, no semaphore protection
//
int final_tx_queue_percentage_unprotected( void )
{
    // return as a percentage
    int ival = (m_tx_q.size()*100)/m_final_txq_len;
    return (ival);
}
//
// Read the fullness of the final queue with semaphore protection
//
int final_tx_queue_size( void )
{
    // return as a percentage
    // Get exclusive access
    pthread_mutex_lock( &mutex );
    int ival = m_tx_q.size();
    // Get exclusive access
    pthread_mutex_unlock( &mutex );
    return (ival);
}
//
// Write samples to the final TX queue
//
void write_final_tx_queue( scmplx* samples, int length )
{
    int i;

    // New work available
    sem_post( &work_sem );

    // Get exclusive access
    // Get exclusive access
    pthread_mutex_lock( &mutex );

    if( m_tx_q.size() < m_final_txq_len)
    {
        for( i = 0; i < (length - TX_BUFFER_LENGTH); i += TX_BUFFER_LENGTH )
        {
            dvb_buffer *b = dvb_buffer_alloc( TX_BUFFER_LENGTH, BUF_SCMPLX );
            dvb_buffer_write( b, &samples[i] );
            m_tx_q.push( b );
        }
        length = length - i;
        if(length > 0 )
        {
            dvb_buffer *b = dvb_buffer_alloc( length, BUF_SCMPLX );
            dvb_buffer_write( b, &samples[i] );
            m_tx_q.push( b );
        }

    }
    pthread_mutex_unlock( &mutex );
}
void write_final_tx_queue_ts( uchar* tp )
{
    // New work available
    sem_post( &work_sem );

    // Get exclusive access
    pthread_mutex_lock( &mutex );

    if( m_tx_q.size() < m_final_txq_len)
    {
        dvb_buffer *b = dvb_buffer_alloc( 188, BUF_TS );
        dvb_buffer_write( b, tp );
        m_tx_q.push( b );
    }
    pthread_mutex_unlock( &mutex );
}
void write_final_tx_queue_udp( uchar* tp )
{
    // New work available
    sem_post( &work_sem );

    // Get exclusive access
    pthread_mutex_lock( &mutex );

    if( m_tx_q.size() < m_final_txq_len)
    {
        dvb_buffer *b = dvb_buffer_alloc( 188, BUF_UDP );
        dvb_buffer_write( b, tp );
        m_tx_q.push( b );
    }
    pthread_mutex_unlock( &mutex );
}
//
// Read from the final TX queue
//
dvb_buffer *read_final_tx_queue(void)
{
    dvb_buffer *b;

    // Wait until there is new work
    sem_wait( &work_sem );

    // Get exclusive access
    pthread_mutex_lock( &mutex );

    if( m_tx_q.size() > 0 )
    {
        b = m_tx_q.front();
        m_tx_q.pop();
    }
    else
    {
        b      = NULL;
        dvb_block_rx_check();
    }
    pthread_mutex_unlock( &mutex );
    return b;
}
void create_final_tx_queue( void )
{
    calculate_video_bitrate();

    if((m_sysc.tx_hardware == HW_EXPRESS_AUTO)||(m_sysc.tx_hardware == HW_EXPRESS_TS))
    {
        if(m_sysc.dvb_mode == MODE_DVBS )
        {
            m_final_txq_len = get_raw_bitrate()/(204*8);
        }
    }
    else
    {
        if(m_sysc.dvb_mode == MODE_DVBS )
        {
             m_final_txq_len = (double)m_sysc.sr_mem[m_sysc.sr_mem_nr]/TX_BUFFER_LENGTH;
        }
    }
    if(m_sysc.tx_hardware == HW_EXPRESS_TS)
    {
        m_final_txq_len = get_raw_bitrate()/(204*8);
    }
    if(m_sysc.dvb_mode == MODE_DVBS2 )
    {
         m_final_txq_len = (double)m_sysc.sr_mem[m_sysc.sr_mem_nr]/TX_BUFFER_LENGTH;
    }
    if(m_sysc.dvb_mode == MODE_DVBT )
    {
         m_final_txq_len = dvb_t_get_sample_rate()/TX_BUFFER_LENGTH;
    }
//    printf("Queue length %d\n",m_final_txq_len);

    // Create the work semaphore
    sem_init( &work_sem, 0, 0 );

    // Create the mutex
    pthread_mutex_init( &mutex, NULL );
}
