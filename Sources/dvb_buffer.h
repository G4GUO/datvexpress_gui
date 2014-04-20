#ifndef TX_BUFFER_H
#define TX_BUFFER_H

typedef enum{BUF_SCMPLX,BUF_TS,BUF_UDP}dvb_buffer_type;

typedef struct{
    int len;
    dvb_buffer_type type;
    void *b;
}dvb_buffer;

dvb_buffer *dvb_buffer_alloc( int len, dvb_buffer_type type );
void dvb_buffer_free( dvb_buffer *b );
void dvb_buffer_write(dvb_buffer *b, void *m);

#endif // TX_BUFFER_H
