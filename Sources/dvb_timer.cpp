//
// Timer module.
//
#include <syscall.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <memory.h>
#include "dvb_gen.h"
#include "dvb.h"
#include "mp_tp.h"

bool m_pat_flag;
bool m_tdt_flag;
bool m_pmt_flag;
bool m_eit_flag;
bool m_sdt_flag;
bool m_nit_flag;

#define TMR_30_MS    3
#define TMR_50_MS    5
#define TMR_100_MS   10
#define TMR_1000_MS  100
#define TMR_2000_MS  200
#define TMR_9000_MS  900
#define TMR_30000_MS 3000

void *timer_proc( void *arg )
{
    struct timespec tim;
    int m_50ms_clock_ticks   = TMR_50_MS;
    int m_100ms_clock_ticks  = TMR_100_MS;
    int m_1s_clock_ticks     = TMR_1000_MS;
    int m_2s_clock_ticks     = TMR_2000_MS;
    int m_9s_clock_ticks     = TMR_9000_MS;
    int m_30s_clock_ticks    = TMR_30000_MS;

    tim.tv_sec  = 0;
    tim.tv_nsec = 10000000; // 10 ms

    while(dvb_is_system_running())
    {
        nanosleep( &tim, NULL);

        m_50ms_clock_ticks--;
        if( m_50ms_clock_ticks <= 0 )
        {
            m_50ms_clock_ticks = TMR_50_MS;
        }

        m_100ms_clock_ticks--;
        if( m_100ms_clock_ticks <= 0 )
        {
            m_pat_flag = true;
            m_pmt_flag = true;
            m_100ms_clock_ticks = TMR_100_MS;
        }

        m_1s_clock_ticks--;
        if( m_1s_clock_ticks <= 0 )
        {
            m_sdt_flag = true;
            m_eit_flag = true;
            m_nit_flag = true;
            m_1s_clock_ticks = TMR_1000_MS;
        }

        m_2s_clock_ticks--;
        if( m_2s_clock_ticks <= 0 )
        {
            m_2s_clock_ticks = TMR_2000_MS;
        }

        m_9s_clock_ticks--;
        if( m_9s_clock_ticks <= 0 )
        {
            m_tdt_flag        = true;
            m_9s_clock_ticks = TMR_9000_MS;
        }

        m_30s_clock_ticks--;
        if( m_30s_clock_ticks <= 0 )
        {
            m_30s_clock_ticks = TMR_30000_MS;
        }
    }
    arg = 0;
    return arg;
}


