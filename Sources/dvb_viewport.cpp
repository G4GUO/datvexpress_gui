#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <memory.h>
#include <fcntl.h>
#include <sys/types.h>
#include <queue>
#include <deque>
#include <list>
#include "dvb.h"
#include "dvb_gen.h"

using namespace std;

extern int m_dvb_running;

sem_t f_vtsem;
pthread_t b_vt_thread;
int m_vt_fd;

queue <Buffer *> m_vtpool_q;
queue <Buffer *> m_vtpend_q;

Buffer m_vtbuff[N_TX_BUFFS];

//
// This thread reads from the queue of tp frames that
// are waiting to be transmitted.
//
void *vt_thread( void *arg )
{
    Buffer *p;
    //
    // This needs to be called within the thread otherwise it blocks
    // if Xine is not running.
    //
    arg = 0;
    return NULL;

    if((m_vt_fd = open( "/tmp/dvb.ts", O_WRONLY )) <= 0 )
    {
        loggerf("Viewport /tmp/dvb.ts open failed\n");
    }

    while( m_dvb_running )
    {
        // Wait signal semaphore
        sem_wait( &f_vtsem );
        while( m_vtpend_q.size() > 0 )
        {
            p = m_vtpend_q.front();
            m_vtpend_q.pop();
//            write( m_vt_fd, p->b, p->length );
            m_vtpool_q.push( p );
        }
    }
    arg = 0;
    return NULL;
}

int vt_q_percentage(void)
{
    return ((m_vtpool_q.size()*100)/N_TX_BUFFS);
}

void vt_init( void )
{
    int res;
    // Populate the pool queue
    for( int i = 0; i < N_TX_BUFFS; i++ )
    {
        m_vtbuff[i].b = (uchar*)malloc(MP_T_FRAME_LEN);
        m_vtpool_q.push( &m_vtbuff[i] );
    }
    sem_init( &f_vtsem, 0, 0 );

    res = pthread_create(&b_vt_thread, NULL, vt_thread, NULL );
    if( res!= 0 ) loggerf("DVB Viewport Thread creation failed\n");
}
//
// Queue to the view port
//
void vt_queue_tp( uchar *b  )
{
    Buffer *p;

    if( m_vtpool_q.size() > 0 )
    {
        p = m_vtpool_q.front();
        m_vtpool_q.pop();
        if( p != NULL )
        {
            p->length = MP_T_FRAME_LEN;
            memcpy( p->b, b, MP_T_FRAME_LEN );
            m_vtpend_q.push( p );
        }
    }
}

