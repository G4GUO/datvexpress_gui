//
// This module controls the associated equipment line
// PA select, antenna switch over etc
//
// C.H Brain March 2012
//
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>

#include "dvb.h"

static int m_eq_id;

int eq_initialise( void )
{
    if((m_eq_id = open("/dev/ttyUSB0",O_RDWR | O_NOCTTY | O_NDELAY ))> 0 )
    {
        struct termios options;

        /*
         * Get the current options for the port...
         */
        tcgetattr(m_eq_id, &options);

        // 7 bit even parity
        options.c_cflag |= PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS7;
        // Disable flow control
        //options.c_cflag &= ~CNEW_RTSCTS;
        // Raw data
        options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        options.c_oflag &= ~OPOST;
        // Disable software flow control
        options.c_iflag &= ~(IXON | IXOFF | IXANY);

        /*
         * Set the baud rates to 9600...
         */
        cfsetispeed(&options, B9600);
        cfsetospeed(&options, B9600);
        /*
         * Enable the receiver and set local mode...
         */
        options.c_cflag |= (CLOCAL | CREAD);
        /*
         * Set the new options for the port...
         */
        tcsetattr(m_eq_id, TCSANOW, &options);
        sleep(1);
        char msg[12];
        msg[0] = 'R';
        msg[1] = '9';
        msg[2] = '9';
        msg[3] = '0';
        msg[4] =  13;
        write(m_eq_id, msg, 5);
        return 0;
    }
    else
    {
//        loggerf("Serial interface disabled\n");
        m_eq_id = -1;
        return -1;
    }
}
void eq_change_frequency( double freq )
{
    freq = freq;
/*
    char msg[5];
    // Deselect all outputs
    msg[0] = 0x1B;
    msg[1] = '[';
    msg[2] = '0';
    msg[3] = 'a';
    write(m_eq_id, msg, 4);
    msg[3] = 'b';
    write(m_eq_id, msg, 4);
    msg[3] = 'c';
    write(m_eq_id, msg, 4);
    msg[3] = 'd';
    write(m_eq_id, msg, 4);
    msg[3] = 'e';
    write(m_eq_id, msg, 4);
    msg[3] = 'f';
    write(m_eq_id, msg, 4);
    msg[3] = 'g';
    write(m_eq_id, msg, 4);
    msg[3] = 'h';
    write(m_eq_id, msg, 4);

    // Select correct output
    msg[2] = '1';

    if((freq > 430000000.0) && (freq < 440000000.0))
    {
        // 70 cms
        msg[3] = 'a';
        write(m_eq_id, msg, 4);
        return;
    }
    if((freq > 1240000000.0) && (freq < 1325000000.0))
    {
        // 23 cms
        msg[3] = 'b';
        write(m_eq_id, msg, 4);
        return;
    }
    if((freq > 2310000000.0) && (freq < 2450000000.0))
    {
        // 13 cms
        msg[3] = 'c';
        write(m_eq_id, msg, 4);
        return;
    }
    if((freq > 3400000000.0) && (freq < 3475000000.0))
    {
        // 9 cms
        msg[3] = 'd';
        write(m_eq_id, msg, 4);
        return;
    }
    // If it gets here a frequency that is not in an amateur band has been requested
*/
}
void eq_transmit( void )
{
    char msg[5];
    if(m_eq_id < 0) return;
    msg[0] = 'R';
    msg[1] = '0';
    msg[2] = '0';
    msg[3] = '1';
    msg[4] = 13;
    write(m_eq_id, msg, 5);
}
void eq_receive( void )
{
    char msg[5];
    if(m_eq_id < 0) return;
    msg[0] = 'R';
    msg[1] = '0';
    msg[2] = '0';
    msg[3] = '0';
    msg[4] = 13;
    write(m_eq_id, msg, 5);
}
