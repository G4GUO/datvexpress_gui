//
// This file contains the UDP handlers
//
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "dvb.h"

extern int m_dvb_running;
int        m_tx_sock;
int        m_rx_sock;
struct     sockaddr_in m_udp_client;
struct     sockaddr_in m_udp_server;
socklen_t  m_udp_server_len;

//
// This process reads UDP socket
//
/*
// First it initialises both receive and transmit sides
// Then it sits waiting to receive transport packets.
// It assumes the transport packets are correctly set up
// for the DVB transmit stream.
//
void *udp_proc( void * args)
{
    sys_config cfg;
    dvb_config_get( &cfg );

    args = args;

    if( cfg.tx_hardware == HW_UDP )
    {
        // Create the UDP socket
        if ((m_tx_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            loggerf("Failed to create UDP TX socket\n");
            return 0;
        }

        // Construct the client sockaddr_in structure
        memset(&m_udp_client, 0, sizeof(m_udp_client));// Clear struct
        m_udp_client.sin_family = AF_INET;             // Internet/IP
        m_udp_client.sin_addr.s_addr = inet_addr(cfg.server_ip_address);  // IP address
        m_udp_client.sin_port = htons(cfg.server_socket_number);          // server port
    }

    if( cfg.capture_device_input == DVB_UDP )
    {
        struct sockaddr_in udp_server;
        int rx_sock;
        uchar buffer[MP_T_FRAME_LEN];
        socklen_t  udp_server_len = sizeof(udp_server);

        // Create the UDP socket
        if ((rx_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        {
            loggerf("Failed to create UDP RX socket\n");
            return 0;
        }

        // Construct the server sockaddr_in structure
        memset(&udp_server, 0, sizeof(udp_server));      // Clear struct
        udp_server.sin_family = AF_INET;                 // Internet/IP
        udp_server.sin_addr.s_addr = INADDR_ANY;         // IP address
        udp_server.sin_port = htons(cfg.server_socket_number); // server port

        bind(rx_sock, (struct sockaddr *)&udp_server, sizeof(udp_server));

        while( m_dvb_running )
        {
            int res = recvfrom( rx_sock, buffer, MP_T_FRAME_LEN, 0,
                         (struct sockaddr *) &udp_server, &udp_server_len);

            if( res == MP_T_FRAME_LEN )
            {
                // We have received a packet, check it is aligned
                if( buffer[0] == MP_T_SYNC)
                {
                    // Aligned packet queue for transmission
                    tx_write_transport_queue( buffer );
                }
                else
                {
                    // slip a byte, try to regain alignment
                    recvfrom( rx_sock, buffer, 1, 0,(struct sockaddr *) &udp_server, &udp_server_len);
                }
            }
        }
        loggerf("UDP process terminated\n");
    }
    return 0;
}

*/
//
// Send a transport packet over UDP to the client.
//
int udp_send_tp( dvb_buffer *b  )
{
    int res;
    if( dvb_get_major_txrx_status() == DVB_TRANSMITTING )
    {
        res = sendto(m_tx_sock, b->b, b->len, 0,(struct sockaddr *) &m_udp_client, sizeof(m_udp_client));
    }
    else
    {
        res = 0;
    }
    dvb_buffer_free(b);
//printf("res = %d\n",res);
    return res;
}

//
// Due to the way the UDP does it's buffering and the way Express pulls bytes
// of data from the network / file there needs to be extra buffering here unfortunately.
//
#define MAX_RB_LEN MB_BUFFER_SIZE
int m_rb_offset;
int m_rb_length;
uchar m_rb[MAX_RB_LEN];

int udp_read( uchar *b, int length )
{
    int offset = 0;
    int bytes;

    while( offset < length )
    {
        if( m_rb_offset < m_rb_length)
        {
            b[offset++] = m_rb[m_rb_offset++];
        }
        else
        {
            if((bytes = recvfrom( m_rx_sock, m_rb, MAX_RB_LEN, 0,(struct sockaddr *) &m_udp_server, &m_udp_server_len)) >= 0 )
            {
                m_rb_length = bytes;
                m_rb_offset = 0;
            }
            else
            {
                return 0;
            }
        }
    }
    return offset;
}
//
// Called by process thread, will return a transport packet.
// It will automatically align to the TP boundary.
//
uchar *udp_get_transport_packet(void)
{
    int bytes = 0;

    if((bytes = recvfrom( m_rx_sock, m_rb, MP_T_FRAME_LEN, 0,(struct sockaddr *) &m_udp_server, &m_udp_server_len)) >= 0 )
    {
        if( m_rb[0] == 0x47 )
        {
            return m_rb;
        }
        else
        {
            for( int i = 0; i < 187; i++)
            {
                // Try to synchronise
                if((bytes = recvfrom( m_rx_sock, m_rb, 1, 0,(struct sockaddr *) &m_udp_server, &m_udp_server_len)) >= 0 )
                {
                    if(m_rb[0] == 0x47)
                    {
                        if((bytes = recvfrom( m_rx_sock, &m_rb[1], MP_T_FRAME_LEN - 1, 0,(struct sockaddr *) &m_udp_server, &m_udp_server_len)) >= 0 )
                        {
                            if( bytes == 187 )
                                return m_rb;
                            else
                                return NULL;
                        }
                    }
                }

            }
        }
    }
    return NULL;
}
//
// Initialise the UDP handler
//
int udp_tx_init( void )
{
    sys_config cfg;
    dvb_config_get( &cfg );

    // Create a socket for transmitting UDP TS packets
    if ((m_tx_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        loggerf("Failed to create UDP TX socket");
        return -1;
    }
    // Construct the client sockaddr_in structure
    memset(&m_udp_client, 0, sizeof(m_udp_client));// Clear struct
    m_udp_client.sin_family = AF_INET;             // Internet/IP
    m_udp_client.sin_addr.s_addr = inet_addr(cfg.server_ip_address);  // IP address
    m_udp_client.sin_port = htons(cfg.server_socket_number);          // server port

    return 0;
}
int udp_rx_init( void )
{
    sys_config cfg;
    dvb_config_get( &cfg );

    //socklen_t  m_udp_server_len = sizeof(m_udp_server);

    // Create the UDP receive socket
    if ((m_rx_sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
    {
        loggerf("Failed to create UDP RX socket");
        return -1;
    }

    // Construct the server sockaddr_in structure
    memset(&m_udp_server, 0, sizeof(m_udp_server));      // Clear struct
    m_udp_server.sin_family = AF_INET;                 // Internet/IP
    m_udp_server.sin_addr.s_addr = INADDR_ANY;         // IP address
    m_udp_server.sin_port = htons(cfg.server_socket_number); // server port
//    loggerf("socket number %d\n",cfg.server_socket_number);

    if( bind(m_rx_sock, (struct sockaddr *)&m_udp_server, sizeof(m_udp_server))< 0)
    {
        loggerf("Failed to bind to RX socket");
        return -1;
    }
    return 0;
}
