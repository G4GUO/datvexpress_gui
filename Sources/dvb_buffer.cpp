#include "memory.h"
#include "dvb.h"
#include "dvb_buffer.h"

dvb_buffer *dvb_buffer_alloc( int len, dvb_buffer_type type )
{
    dvb_buffer *b;

    b = (dvb_buffer *)malloc(sizeof(dvb_buffer));
    if( b != NULL )
    {
        b->len = len;
        b->type = type;
        if(type == BUF_SCMPLX)
        {
            b->b = malloc(sizeof(scmplx)*len);
            if( b->b != NULL ) return b;
            free(b);
        }
        if(type == BUF_TS)
        {
            b->b = malloc(len);
            if( b->b != NULL ) return b;
            free(b);
        }
        if(type == BUF_UDP)
        {
            b->b = malloc(len);
            if( b->b != NULL ) return b;
            free(b);
        }
    }
    return NULL;
}
void dvb_buffer_free(dvb_buffer *b)
{
    if(b != NULL )
    {
        free(b->b);
        free(b);
    }
}
void dvb_buffer_write(dvb_buffer *b, void *m)
{
    if(b->type == BUF_SCMPLX)
    {
        memcpy(b->b,m,sizeof(scmplx)*b->len);
    }
    if(b->type == BUF_TS)
    {
        memcpy(b->b,m,b->len);
    }
    if(b->type == BUF_UDP)
    {
        memcpy(b->b,m,b->len);
    }
}
