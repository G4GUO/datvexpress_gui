//
// used to extract and insert elementary streams
//
#include <memory.h>
#include "dvb.h"
//
// See Table 2-17 ISO 13818-1
//
//
// Add TS field
//
void pes_add_pts_dts( uchar *b, int64_t ts )
{
    b[0] &= 0xF0;
    // 4 bits 0010 or 0011
    if(ts & 0x100000000) b[0] |= 0x08;
    if(ts & 0x080000000) b[0] |= 0x04;
    if(ts & 0x040000000) b[0] |= 0x02;
    b[0] |= 0x01; // Marker bit
    b[1] = 0;
    if(ts & 0x020000000) b[1] |= 0x80;
    if(ts & 0x010000000) b[1] |= 0x40;
    if(ts & 0x008000000) b[1] |= 0x20;
    if(ts & 0x004000000) b[1] |= 0x10;
    if(ts & 0x002000000) b[1] |= 0x08;
    if(ts & 0x001000000) b[1] |= 0x04;
    if(ts & 0x000800000) b[1] |= 0x02;
    if(ts & 0x000400000) b[1] |= 0x01;
    b[2] = 0;
    if(ts & 0x000200000) b[2] |= 0x80;
    if(ts & 0x000100000) b[2] |= 0x40;
    if(ts & 0x000080000) b[2] |= 0x20;
    if(ts & 0x080040000) b[2] |= 0x10;
    if(ts & 0x000020000) b[2] |= 0x08;
    if(ts & 0x000010000) b[2] |= 0x04;
    if(ts & 0x000008000) b[2] |= 0x02;
    b[2] |= 0x01;// Marker bit
    b[3] = 0;
    if(ts & 0x000004000) b[3] |= 0x80;
    if(ts & 0x000002000) b[3] |= 0x40;
    if(ts & 0x000001000) b[3] |= 0x20;
    if(ts & 0x000000800) b[3] |= 0x10;
    if(ts & 0x000000400) b[3] |= 0x08;
    if(ts & 0x000000200) b[3] |= 0x04;
    if(ts & 0x000000100) b[3] |= 0x02;
    if(ts & 0x000000080) b[3] |= 0x01;
    b[4] = 0;
    if(ts & 0x000000040) b[4] |= 0x80;
    if(ts & 0x000000020) b[4] |= 0x40;
    if(ts & 0x000000010) b[4] |= 0x20;
    if(ts & 0x000000008) b[4] |= 0x10;
    if(ts & 0x000000004) b[4] |= 0x08;
    if(ts & 0x000000002) b[4] |= 0x04;
    if(ts & 0x000000001) b[4] |= 0x02;
    b[4] |= 0x01;// Marker bit
}
//
// Add PTS and DTS fields
//
int pes_add_pts_dts( uchar *b, int64_t pts, int64_t dts )
{
    if((pts >= 0)&&(dts>=0))
    {
        // Both PTS and DTS required
        b[7] = 0xC0;
        b[8] = 10; // Extra header
        b[9] = 0x30;
        pes_add_pts_dts( &b[9],  pts );
        b[14] = 0x10;
        pes_add_pts_dts( &b[14], dts );
        return 20;
    }
    else
    {
        if( pts >= 0 )
        {
            // PTS only required
            b[7] = 0x80;
            b[8] = 5; // extra header length
            b[9] = 0x20;
            pes_add_pts_dts( &b[9], pts );
            return 15;
        }
        else
        {
            // Neither required
            b[7] = 0x00;
            b[8] = 0; // payload starts straight away, no extra header
            return 9;
        }
    }
}

//
// form a pes packet from the video elementary stream
//
int pes_video_el_to_pes( uchar *b, int length, int64_t pts, int64_t dts )
{
    int len,start;
    uchar d[40];

    d[0] = 0x00;
    d[1] = 0x00;
    d[2] = 0x01;
    d[3] = 0xE0;

    d[6] = 0x84;// Data aligned
    start = pes_add_pts_dts( d, pts, dts );
    len = length + start;
    // Set the length of the packet
    d[4] = (len-6)>>8;
    d[5] = (len-6)&0xFF;
    pes_write_from_memory( d, start );
    pes_write_from_memory( b, length );
    return len;
}
//
// form a pes packet from the video elementary stream
//
int pes_audio_el_to_pes( uchar *b, int length, int64_t pts, int64_t dts )
{
    int len,start;
    uchar d[40];

    d[0] = 0x00;
    d[1] = 0x00;
    d[2] = 0x01;
    d[3] = 0xC0;

    d[6] = 0x84;// Data aligned
    start = pes_add_pts_dts( d, pts, dts );
    len = length + start;
    // Set the length of the packet
    d[4] = (len-6)>>8;
    d[5] = (len-6)&0xFF;
    pes_write_from_memory( d, start );
    pes_write_from_memory( b, length );
    return len;
}
