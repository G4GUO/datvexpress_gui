//
// This module logs to a disk file
//
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dvb.h"

static FILE *m_fp;
static bool m_logger_active;
static sem_t m_logger_sem;

void tp_file_logger_wait_sem(void)
{
    if(dvb_is_system_running()) sem_wait( &m_logger_sem );
}
void tp_file_logger_post_sem(void)
{
    if(dvb_is_system_running()) sem_post( &m_logger_sem );
}

void tp_file_logger_log( uchar *b, int length)
{
    tp_file_logger_wait_sem();
    if( m_logger_active )
    {
        fwrite( b, length, 1, m_fp );
    }
    tp_file_logger_post_sem();
}

void tp_file_logger_start(void)
{
    char *p = getenv("HOME");
    char filename[1024];
    // Create the pathname
    sprintf(filename,"%s%s",p,"/datvexpress.ts");
    tp_file_logger_wait_sem();
    if((m_fp=fopen(filename,"w"))>0)
    {
        m_logger_active = true;
    }
    else
    {
        m_logger_active = false;
    }
    tp_file_logger_post_sem();
}
void tp_file_logger_stop(void)
{
    tp_file_logger_wait_sem();
    if( m_logger_active )
    {
        fclose( m_fp );
        m_logger_active = false;
    }
    tp_file_logger_post_sem();
}
void tp_file_logger_init( void )
{
    m_logger_active = false;
    sem_init( &m_logger_sem, 0, 0 );
    sem_post( &m_logger_sem );
}

