#include "mainwindow.h"
#include "stdio.h"
#include "ui_mainwindow.h"
#include "dvb.h"

using namespace std;

#define MAX_LOGS 20
#define MAX_LOG_LEN 79

static char m_log_text[MAX_LOGS][80];
static int m_write_log_index;
static int m_display_log_index;
static char m_display_box_text[200];
static int  m_log_updated;

int logger_updated( void )
{
    return m_log_updated;
}
void logger_released( void )
{
//    m_log_text[0] = 0;
}

const char *logger_get_text( void )
{
    char *msg[2];
    msg[0] = m_log_text[(m_write_log_index+MAX_LOGS-1)%MAX_LOGS];
    msg[1] = m_log_text[(m_display_log_index+MAX_LOGS-1)%MAX_LOGS];
    sprintf(m_display_box_text,"%s\n%s",msg[0],msg[1]);
    m_log_updated = 0;
    return m_display_box_text;
}

void logger( char const *text )
{
    char buff[100];
    time_t now = time (0);
    strftime (buff, 100, "%Y-%m-%d %H:%M:%S ", localtime (&now));
    strcat( buff, text );
    strncpy( m_log_text[m_write_log_index],buff,MAX_LOG_LEN );
    m_display_log_index = m_write_log_index;
    m_write_log_index = (m_write_log_index + 1)%MAX_LOGS;
    m_log_updated = 1;
}
void loggerf( char *fmt, ... )
{
    va_list ap;
    char temp[10000];// Long enough I hope!
    va_start(ap,fmt);
    vsprintf(temp,fmt,ap );
    va_end(ap);
    temp[79] = 0;
    logger(temp);
}
void loggerf( const char *fmt, ... )
{
    va_list ap;
    char temp[10000];

    va_start(ap,fmt);
    vsprintf(temp,fmt,ap );
    va_end(ap);
    temp[79] = 0;
    logger(temp);
}
void increment_display_log_index(void)
{
    for( int i = 0; i < MAX_LOGS; i++ )
    {
        m_display_log_index = (m_display_log_index + 1)%MAX_LOGS;
        if(strlen(m_log_text[m_display_log_index]) > 0 ) break;
    }
    m_log_updated = 1;
}
void display_logger_init( void )
{
    for( int i = 0; i < MAX_LOGS; i++ )
    {
       // m_log_text[i][0] = '\0';
    }
}
