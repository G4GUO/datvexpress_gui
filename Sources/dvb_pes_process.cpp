//
// This module takes a pes packet received from the codec
// it inspects it and creates a modified version (expanded)
// It also provides a routine for the transport packetiser to read in
// blocks of data.
//
#include <memory.h>
#include "dvb.h"
#include "mp_tp.h"
#include "dvb_capture_ctl.h"

static uchar m_nb[MB_BUFFER_SIZE*100];
static int   m_rbp,m_wbp;
//
// Read from the capture device into memory
//
void pes_write_from_capture( int len )
{
    cap_rd_bytes( &m_nb[m_wbp], len );
    m_wbp += len;
}
//
// Read from buffer into memory
//
void pes_write_from_memory( uchar *b, int len )
{
    memcpy( &m_nb[m_wbp], b, len);
    m_wbp += len;
}
// Read the buffer in for processing
void pes_read( uchar *b, int len )
{
    memcpy( b, &m_nb[m_rbp], len);
    m_rbp += len;
}
int pes_get_length( void )
{
    return m_wbp;
}
uchar *pes_get_packet(void)
{
    return m_nb;
}

int pes_process( void )
{
    int len = m_wbp;
    // Return the new length
    return len;
}
void pes_reset( void )
{
    m_wbp   = 0;// reset the read position
    m_rbp   = 0;// reset the read position
}


