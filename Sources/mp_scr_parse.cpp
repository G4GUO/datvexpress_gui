#include <math.h>
#include "dvb.h"
#include "mp_tp.h"
#include "dvb_config.h"
#include "dvb_capture_ctl.h"

#define MPEG_CLK     27000000
#define PCR_DELAY    ((int)(PVR_PCR_DELAY*27000000))
#define PCR_ERROR    270000000
#define PCR_INTERVAL 945000

//
// Use 2 32 bit fields to represent the clock
//
static uint m_mux_rate;
static int64_t m_pcr_clk;
static int64_t m_scr_clk;
static int64_t m_scr_clk_last;
static int64_t m_pcr_scr_offset;
static int64_t m_pcr_clk_last;
static int    m_packets_sent;
static int    m_bitrate;
static int    m_error_ms;
static int    m_pcr_sync;

void pcr_transport_packet_clock_update(void)
{
    // Calculate the time in seconds
    double v = get_bits_in_transport_packet();
    v = ( v*MPEG_CLK / get_raw_bitrate());
    m_pcr_clk   += (int)v;
    m_packets_sent++;
}
//
// Dummy to calculate period
//
int pcr_increment(void)
{
    double v = get_bits_in_transport_packet();
    v = ( v*MPEG_CLK / get_raw_bitrate());
    return (int)v;
}
void pcr_scr_equate_clocks(void)
{
    m_pcr_clk = m_scr_clk;
}

double pcr_value( void )
{
    // calculate value
    return m_pcr_clk;
}
double scr_value( void )
{
    // calculate value
    return m_scr_clk;
}
double pcr_scr_difference( bool print )
{
    // calculate the difference
    double dif = m_pcr_clk - m_scr_clk;
//    double dif = pcr_value() - scr_value();
//    printf("Bitrate %f\n",(get_raw_bitrate() + m_bitrate_correction) );
    if(print) printf("(p-s) %f ms\n", dif*1000 );

    return dif;
}
//
// Determines when to add a PCR adaption field
//
bool is_pcr_update(void)
{
    // PCR updated every PCR_INTERVAL
    if((m_pcr_clk - m_pcr_clk_last) >= PCR_INTERVAL)
    {
        m_pcr_clk_last = m_pcr_clk;
        return true;
    }
    return false;
}

//
// Spec 13818-1 page 22
//
int add_pcr_field( uchar *b )
{
    int64_t m,l,pcr_clk;
    pcr_clk =  m_pcr_clk - (PCR_DELAY);
    m = pcr_clk/300;
    l = pcr_clk%300;
    b[0] = (uchar)((m>>25)&0xFF);
    b[1] = (uchar)((m>>17)&0xFF);
    b[2] = (uchar)((m>>9)&0xFF);
    b[3] = (uchar)((m>>1)&0xFF);
    if(m&1)
        b[4] = 0x80 | 0x7E;
    else
        b[4] = 0x00 | 0x7E;
    if(l&0x100) b[4] |= 1;// MSB of extension
    b[5] = (uchar)(l&0xFF);
    return 6;
}
//
// Restamp the PTS and DTS if they exists
//
int pts_dts_stamp( uchar *b, int64_t &clk )
{
    b[0] &= 0xF0;
    // 4 bits 0010 or 0011
    if(clk & 0x100000000) b[0] |= 0x08;
    if(clk & 0x080000000) b[0] |= 0x04;
    if(clk & 0x040000000) b[0] |= 0x02;
    b[0] |= 0x01; // Marker bit
    b[1] = 0;
    if(clk & 0x020000000) b[1] |= 0x80;
    if(clk & 0x010000000) b[1] |= 0x40;
    if(clk & 0x008000000) b[1] |= 0x20;
    if(clk & 0x004000000) b[1] |= 0x10;
    if(clk & 0x002000000) b[1] |= 0x08;
    if(clk & 0x001000000) b[1] |= 0x04;
    if(clk & 0x000800000) b[1] |= 0x02;
    if(clk & 0x000400000) b[1] |= 0x01;
    b[2] = 0;
    if(clk & 0x000200000) b[2] |= 0x80;
    if(clk & 0x000100000) b[2] |= 0x40;
    if(clk & 0x000080000) b[2] |= 0x20;
    if(clk & 0x000040000) b[2] |= 0x10;
    if(clk & 0x000020000) b[2] |= 0x08;
    if(clk & 0x000010000) b[2] |= 0x04;
    if(clk & 0x000008000) b[2] |= 0x02;
    b[2] |= 0x01;// Marker bit
    b[3] = 0;
    if(clk & 0x000004000) b[3] |= 0x80;
    if(clk & 0x000002000) b[3] |= 0x40;
    if(clk & 0x000001000) b[3] |= 0x20;
    if(clk & 0x000000800) b[3] |= 0x10;
    if(clk & 0x000000400) b[3] |= 0x08;
    if(clk & 0x000000200) b[3] |= 0x04;
    if(clk & 0x000000100) b[3] |= 0x02;
    if(clk & 0x000000080) b[3] |= 0x01;
    b[4] = 0;
    if(clk & 0x000000040) b[4] |= 0x80;
    if(clk & 0x000000020) b[4] |= 0x40;
    if(clk & 0x000000010) b[4] |= 0x20;
    if(clk & 0x000000008) b[4] |= 0x10;
    if(clk & 0x000000004) b[4] |= 0x08;
    if(clk & 0x000000002) b[4] |= 0x04;
    if(clk & 0x000000201) b[4] |= 0x02;
    b[4] |= 0x01;// Marker bit
    return 5;
}

void restamp_pts_dts( uchar *b, int64_t pts, int64_t dts )
{
    int offset;
    offset = 9;

    if((b[7]&0xc0) == 0x80 )
    {
        if((b[offset]&0xF0) == 0x20)
        {
            // PTS only
            pts_dts_stamp( &b[offset], pts );
        }
    }

    if((b[7]&0xc0) == 0xC0 )
    {
        if((b[offset]&0xF0) == 0x30)
        {
            // PTS and DTS
            offset += pts_dts_stamp( &b[offset], pts );
            offset += pts_dts_stamp( &b[offset], dts );
        }
    }
}

//
// This module extracts information from a PES packet
//
int extract_ts33( uchar *b, int64_t &ts )
{
    ts = 0;
    // 4 bits 0010 or 0011
    if(b[0]&0x08) ts |= 0x100000000;
    if(b[0]&0x04) ts |= 0x080000000;
    if(b[0]&0x02) ts |= 0x040000000;
    // Marker bit
    if(b[1]&0x80) ts |= 0x020000000;
    if(b[1]&0x40) ts |= 0x010000000;
    if(b[1]&0x20) ts |= 0x008000000;
    if(b[1]&0x10) ts |= 0x004000000;
    if(b[1]&0x08) ts |= 0x002000000;
    if(b[1]&0x04) ts |= 0x001000000;
    if(b[1]&0x02) ts |= 0x000800000;
    if(b[1]&0x01) ts |= 0x000400000;
    if(b[2]&0x80) ts |= 0x000200000;
    if(b[2]&0x40) ts |= 0x000100000;
    if(b[2]&0x20) ts |= 0x000080000;
    if(b[2]&0x10) ts |= 0x000040000;
    if(b[2]&0x08) ts |= 0x000020000;
    if(b[2]&0x04) ts |= 0x000010000;
    if(b[2]&0x02) ts |= 0x000008000;
    // Marker bit
    if(b[3]&0x80) ts |= 0x000004000;
    if(b[3]&0x40) ts |= 0x000002000;
    if(b[3]&0x20) ts |= 0x000001000;
    if(b[3]&0x10) ts |= 0x000000800;
    if(b[3]&0x08) ts |= 0x000000400;
    if(b[3]&0x04) ts |= 0x000000200;
    if(b[3]&0x02) ts |= 0x000000100;
    if(b[3]&0x01) ts |= 0x000000080;
    if(b[4]&0x80) ts |= 0x000000040;
    if(b[4]&0x40) ts |= 0x000000020;
    if(b[4]&0x20) ts |= 0x000000010;
    if(b[4]&0x10) ts |= 0x000000008;
    if(b[4]&0x08) ts |= 0x000000004;
    if(b[4]&0x04) ts |= 0x000000002;
    if(b[4]&0x02) ts |= 0x000000001;
    // Marker bit
    return 5;
}
int extract_escr( uchar *b, uchar *escr, uchar *extn )
{
    escr[0] = 0;
    if(b[0]&0x20) escr[0] |= 0x80;
    if(b[0]&0x10) escr[0] |= 0x40;
    if(b[0]&0x08) escr[0] |= 0x20;
    // marker bit
    if(b[0]&0x02) escr[0] |= 0x10;
    if(b[0]&0x01) escr[0] |= 0x08;
    if(b[1]&0x80) escr[0] |= 0x04;
    if(b[1]&0x40) escr[0] |= 0x02;
    if(b[1]&0x20) escr[0] |= 0x01;
    escr[1] = 0;
    if(b[1]&0x10) escr[1] |= 0x80;
    if(b[1]&0x08) escr[1] |= 0x40;
    if(b[1]&0x04) escr[1] |= 0x20;
    if(b[1]&0x02) escr[1] |= 0x10;
    if(b[1]&0x01) escr[1] |= 0x08;
    if(b[2]&0x80) escr[1] |= 0x04;
    if(b[2]&0x40) escr[1] |= 0x02;
    if(b[2]&0x20) escr[1] |= 0x01;
    escr[2] = 0;
    if(b[2]&0x10) escr[2] |= 0x80;
    if(b[2]&0x08) escr[2] |= 0x40;
    // Marker bit
    if(b[3]&0x02) escr[2] |= 0x20;
    if(b[3]&0x01) escr[2] |= 0x10;
    if(b[4]&0x80) escr[2] |= 0x08;
    if(b[4]&0x40) escr[2] |= 0x04;
    if(b[4]&0x20) escr[2] |= 0x02;
    if(b[4]&0x10) escr[2] |= 0x01;
    escr[3] = 0;
    if(b[4]&0x08) escr[3] |= 0x80;
    if(b[4]&0x04) escr[3] |= 0x40;
    if(b[4]&0x02) escr[3] |= 0x20;
    if(b[4]&0x01) escr[3] |= 0x10;
    if(b[5]&0x20) escr[3] |= 0x08;
    if(b[5]&0x10) escr[3] |= 0x04;
    if(b[5]&0x08) escr[3] |= 0x02;
    if(b[5]&0x04) escr[3] |= 0x01;
    escr[4] = 0;
    if(b[4]&0x02) escr[4] |= 0x80;
    // Marker bit
    extn[0] = 0;
    if(b[5]&0x80) extn[3] |= 0x80;
    if(b[5]&0x40) extn[3] |= 0x40;
    if(b[5]&0x20) extn[3] |= 0x20;
    if(b[5]&0x08) extn[3] |= 0x10;
    if(b[5]&0x04) extn[3] |= 0x08;
    if(b[5]&0x02) extn[3] |= 0x04;
    if(b[5]&0x01) extn[3] |= 0x02;
    if(b[6]&0x80) extn[3] |= 0x01;
    extn[1] = 0;
    if(b[6]&0x40) extn[1] |= 0x80;

    return 7;
}
//
// Points to start of PES packet
//
void extract_pts_dts( uchar *b, int64_t &pts, int64_t &dts )
{
    int offset;
    offset = 9;

    pts = dts = 0;

    if((b[7]&0xc0) == 0x80 )
    {
        // PTS present
        if((b[offset]&0xF0) == 0x20)
        {
            // PTS only
            extract_ts33(&b[offset], pts );
        }
    }

    if((b[7]&0xc0) == 0xC0 )
    {
        // PTS and DTS present
        if((b[offset]&0xF0) == 0x30)
        {
            offset += extract_ts33( &b[offset], pts );
            offset += extract_ts33( &b[offset], dts );
        }
    }
}
void pad_stream( void )
{
    // Send SI if required
    if(ts_single_stream() == 0 )
    {
        // Send SI if required
        if(ts_multi_stream() == 0)
        {
            // Else pad out the stream
            padding_null_dvb();
        }
    }
}
//
// Calculate dts pts offsets from SCR and re-timestamp the fields with
// the new values.
//
void post_pes_actions( uchar *b )
{
    int64_t pts;
    int64_t dts;
    // Extract the DTS and PTS values from the packet
    extract_pts_dts( b, pts, dts );
    if(b[3] == 0xC0 )
    {
        // Audio
        if( pts > 0 )
        {
            post_ts( pts );
//            printf("Audio difference %f\n", (m_pcr_clk - PCR_DELAY - pts)*1000.0/MPEG_CLK);
        }
    }
    if(b[3] == 0xE0 )
    {
        // Video
        if( dts > 0 )
        {
            //post_ts( dts );
        }
        if((pts>0)&&(dts==0))
        {
//            printf("Video difference %f\n", (m_dpcr_clk - PCR_DELAY - dpts)*1000/MPEG_CLK);
        }
    }
    return;

    if((pts > 0) && (dts> 0))
    {
//printf("Post PES called PTS %f DTS %f\n",dpts,ddts);
        // Appy the correction
        pts += m_pcr_scr_offset;
        dts += m_pcr_scr_offset;
        // Restamp the packet
        restamp_pts_dts( b, pts, dts );
//printf("Post PES called PTS %f DTS %f\n\n",dpts,ddts);
    }
    else
    {
        if(pts > 0)
        {
//printf("Post PES called PTS %f\n",dpts);
            // Appy the correction
            pts += m_pcr_scr_offset;
            // Restamp the packet
            restamp_pts_dts( b, pts, dts );
//printf("Post PES called PTS %f\n\n",dpts);
        }
    }
}
//
// Extract the SCR field (27 MHz System clock)
//
int extract_scr_from_pack_header( uchar *b, int len )
{
//    printf("old %.8x%.8x\n", m_scr_clk[0],m_scr_clk[1]);
    int64_t scr_clk;
    m_scr_clk  = 0;
    if(b[4]&0x20) scr_clk |= 0x100000000;
    if(b[4]&0x10) scr_clk |= 0x080000000;
    if(b[4]&0x08) scr_clk |= 0x040000000;
    // Marker bit
    if(b[4]&0x02) scr_clk |= 0x020000000;
    if(b[4]&0x01) scr_clk |= 0x010000000;
    if(b[5]&0x80) scr_clk |= 0x008000000;
    if(b[5]&0x40) scr_clk |= 0x004000000;
    if(b[5]&0x20) scr_clk |= 0x002000000;
    if(b[5]&0x10) scr_clk |= 0x001000000;
    if(b[5]&0x08) scr_clk |= 0x000800000;
    if(b[5]&0x04) scr_clk |= 0x000400000;
    if(b[5]&0x02) scr_clk |= 0x000200000;
    if(b[5]&0x01) scr_clk |= 0x000100000;
    if(b[6]&0x80) scr_clk |= 0x000080000;
    if(b[6]&0x40) scr_clk |= 0x000040000;
    if(b[6]&0x20) scr_clk |= 0x000020000;
    if(b[6]&0x10) scr_clk |= 0x000010000;
    if(b[6]&0x08) scr_clk |= 0x000008000;
    // Marker bit
    if(b[6]&0x02) scr_clk |= 0x000004000;
    if(b[6]&0x01) scr_clk |= 0x000002000;
    if(b[7]&0x80) scr_clk |= 0x000001000;
    if(b[7]&0x40) scr_clk |= 0x000000800;
    if(b[7]&0x20) scr_clk |= 0x000000400;
    if(b[7]&0x10) scr_clk |= 0x000000200;
    if(b[7]&0x08) scr_clk |= 0x000000100;
    if(b[7]&0x04) scr_clk |= 0x000000080;
    if(b[7]&0x02) scr_clk |= 0x000000040;
    if(b[7]&0x01) scr_clk |= 0x000000020;
    if(b[8]&0x80) scr_clk |= 0x000000010;
    if(b[8]&0x40) scr_clk |= 0x000000008;
    if(b[8]&0x20) scr_clk |= 0x000000004;
    if(b[8]&0x10) scr_clk |= 0x000000002;
    if(b[8]&0x08) scr_clk |= 0x000000001;// LSB of upper word
    // Marker bit
    if(b[8]&0x02) scr_clk |= 0x000000100;// MSB of extension
    if(b[8]&0x01) scr_clk |= 0x000000080;
    if(b[9]&0x80) scr_clk |= 0x000000040;
    if(b[9]&0x40) scr_clk |= 0x000000020;
    if(b[9]&0x20) scr_clk |= 0x000000010;
    if(b[9]&0x10) scr_clk |= 0x000000008;
    if(b[9]&0x08) scr_clk |= 0x000000004;
    if(b[9]&0x04) scr_clk |= 0x000000002;
    if(b[9]&0x02) scr_clk |= 0x000000001;
    // Marker bit
    m_mux_rate = b[10];// mux rate
    m_mux_rate = (m_mux_rate<<8) | b[11];
    m_mux_rate = (m_mux_rate<<8) | b[12];
    m_mux_rate =  m_mux_rate>>2;
    // Other stuff that might be interesting (todo)
    return len;
}
//
// Called after an SCR has been received
// Pads out the stream so the PCR and SCR clocks remain in sync
//
void post_scr_actions(void)
{
    static int count;
    if( count++ > 1000 )
    {
        m_bitrate =  ((m_packets_sent * 204.0 * 8.0)*MPEG_CLK)/(m_scr_clk - m_scr_clk_last);
        m_error_ms = (m_pcr_clk - m_scr_clk)*1000.0/MPEG_CLK;
//        printf("Nr %d Err %d ms Sent %d Bitrate %d\n",m_pcr_clk,m_error_ms,m_packets_sent,m_bitrate);
        // If the difference in the clocks gest too great reset the PCR
//        if(fabs(m_dpcr_clk - m_dscr_clk) > (0.08 * MPEG_CLK)) m_dpcr_clk = m_dscr_clk;
        m_scr_clk_last = m_scr_clk;
        m_packets_sent  = 0;
        count           = 0;
    }
}
//
// Only called with Hauppauge decoders
//
void post_ts( int64_t ts )
{
    if(m_pcr_sync)
    {
        m_pcr_sync = 0;
        m_pcr_clk = ts*300;
        return;
    }
    // Let the PCR catch up if needed by inserting either SI tables or NULLs
    while(((m_pcr_clk + pcr_increment()) < (ts*300)) &&
          (tx_queue_percentage() < 30) &&
          ((m_pcr_clk - m_pcr_clk_last) < PCR_INTERVAL))
    {
        pad_stream();
    }
    // 300 converts from DTS/PTS to PCR
    if((m_pcr_clk - PCR_DELAY) > (ts*300)) m_pcr_sync = 1;
//    if((ts*300)  - m_pcr_clk > PCR_ERROR) m_pcr_clk = ts*300;
}
//
// Only called when using Software encoders
//
void force_pcr(int64_t ts)
{
    // Set the PCR to the value ts
    while(((m_pcr_clk + pcr_increment()) < (ts)) &&
          (tx_queue_percentage() < 30) &&
          ((m_pcr_clk - m_pcr_clk_last) < PCR_INTERVAL))
    {
        pad_stream();
    }
}
//
// We use the audio packet pts values to retain sync
//
void haup_pvr_audio_packet(void)
{
    int64_t pts;
    int64_t dts;
    // Extract the DTS and PTS values from the packet
    extract_pts_dts( pes_get_packet(), pts, dts );
    if(pts > 0 )
    {
        post_ts( pts );
    }
}
//
// We must not let the pcr clock advance beyond the dts or pts of the
// video or audio packets
//
void haup_pvr_video_packet(void)
{
    int64_t pts;
    int64_t dts;
    // Extract the DTS and PTS values from the packet
    extract_pts_dts( pes_get_packet(), pts, dts );
    if( pts > 0 )
    {
//        if(m_pcr_clk > (pts*300)) m_pcr_sync = 1;
    }
    if( dts > 0 )
    {
        if( (dts*300) > (m_pcr_clk + PCR_DELAY*10)) m_pcr_sync = 1;
    }
}

void pcr_scr_init(void)
{
    m_pcr_clk_last = 0;
    m_pcr_clk      = 0;
    m_pcr_sync     = 1;
}
