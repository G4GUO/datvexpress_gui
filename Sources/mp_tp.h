//
// Transport header structures
//
#ifndef MP_TP_H
#define MP_TP_H
#include <sys/types.h>
#include "dvb_gen.h"
#include "mp_desc.h"
#include "dvb_types.h"
#define TP_SYNC  0x47
#define TP_CHANS 10
#define TP_LEN 188
#define TP_STREAM_ID 0x001
#define VIDEO_CHAN 0
#define AUDIO_CHAN 1
#define SYSTM_CHAN 2
#define PGM_ID     1
#define TDT_FLAG   0x33
#define PES_PAYLOAD_LENGTH 184
#define SI_PAYLOAD_LENGTH  184

// Transport Error 
#define TRANSPORT_ERROR_FALSE 0
#define TRANSPORT_ERROR_TRUE  1

// Transport priority
#define TRANSPORT_PRIORITY_LOW  0
#define TRANSPORT_PRIORITY_HIGH 1

// Payload start
#define PAYLOAD_START_FALSE 0
#define PAYLOAD_START_TRUE  1


// Adaption field values
#define ADAPTION_RESERVED     0
#define ADAPTION_PAYLOAD_ONLY 1
#define ADAPTION_FIELD_ONLY   2
#define ADAPTION_BOTH         3

// Scrambling field
#define SCRAMBLING_OFF    0
#define SCRAMBLING_USER_1 1
#define SCRAMBLING_USER_2 2
#define SCRAMBLING_USER_3 3

// Transport stream packet format

typedef struct{
    uchar transport_error_indicator;
	uchar payload_unit_start_indicator;
	uchar transport_priority;
    uint  pid;
	uchar transport_scrambling_control;
	uchar adaption_field_control;
	uchar continuity_counter;
}tp_hdr;

typedef struct{
	uchar b[20];
	uchar seq;
	uchar len;
}tp_chan;

// Program associated section
typedef struct{
	int program_number;
	int pid;
}tp_pa_sections;
/*
// Program associated section
typedef struct{
	uchar table_id;
	uchar section_syntax_indicator;
	int section_length;
	int transport_stream_id;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	tp_pa_sections sections(10);
}tp_pass;
*/

// PAT table
typedef struct{
	uchar section_syntax_indicator;
	int transport_stream_id;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	int   nr_table_entries;
	tp_pa_sections entry[10];
}tp_pat;


typedef struct{
	uchar stream_type;
	int elementary_pid;
        int nr_descriptors;
        si_desc descriptor[10];
}tp_pmt_section;

typedef struct{
	uchar section_syntax_indicator;
	int program_number;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	int pcr_pid;
	td_descriptor desc[2];
    int nr_elementary_streams;
    tp_pmt_section stream[5];
}tp_pmt;

//
// Program Clock Reference
//
typedef struct{
    uchar pgm_clk_ref_base[5];//33 bits
    uchar pgm_clk_ref_extn[2];//9 bits
}tp_pcr;

typedef struct{
    uchar pts_val[5];//33 bits
}tp_pts;

typedef struct{
    uchar dts_val[5];//33 bits
}tp_dts;

typedef struct{
    uchar  discontinuity_ind;
    uchar  random_access_ind;
    uchar  elem_stream_pr_ind;
    uchar  PCR_flag;
    uchar  OPCR_flag;
    uchar  splicing_point_flag;
    uchar  trans_priv_data_flag;
    uchar  adapt_field_extn_flag;
}tp_adaption;

// Transport stream descriptor table

typedef struct{
	uchar table_id;
	uchar section_syntax_indicator;
	int section_length;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	uchar descriptor[10];
}tp_sdt;

typedef struct{
    uchar *inbuff;
    int ibuff_len;
    int seq_count;
    int nr_frames;
    int pid;
    uchar frames[100][MP_T_FRAME_LEN];
}tp_si_seq;

// Teletext
typedef struct{
    uchar data_unit_id;
    uchar field_parity;
    uchar line_offset;
    uchar framing_code;
    int magazine_address;
    uchar data_block[40];
}ebu_teletext_data_field;

typedef struct{
    uchar data_identifier;
    int nr_fields;
    ebu_teletext_data_field field[40];
}teletext_pes_data_field;

// Used when extracting program info from a transport stream

typedef struct{
    uint pat_detected   : 1;
    uint pmt_detected   : 1;
    uint pmt_parsed     : 1;
    uint video_detected : 1;
    uint video_type;
    uint audio_detected : 1;
    uint audio_type;
    uint pmt_id;
    uint pcr_id;
    uint required_pcr_id;
    uint video_id;
    uint required_video_id;
    uint audio_id;
    uint required_audio_id;
}ts_info;

typedef struct{
    uint pgm_nr;
    uint pgm_id;
}pat_info;

//

int  tp_fmt( uchar *b, tp_hdr *hdr );
void f_tp_init( void );
int  tp_pat_fmt( uchar *b, tp_pat *p );
void set_cont_counter( uchar *b, uchar c );

// Send PES as a series of transport packets
int f_send_pes_first_tp( uchar *b, int pid, uchar c, bool pcr );
int f_send_pes_next_tp( uchar *b,  int pid, uchar c, bool pcr );
int f_send_pes_last_tp( uchar *b, int bytes, int pid, uchar c, bool pcr );
//void f_send_tp_with_adaption_no_payload( int pid, int c );
//int  send_pcr_tp(void);

void f_send_pes_seq( uchar *b, int length, int pid, uchar &count );

void f_create_si_first( uchar *pkt, uchar *b, int pid, int len );
void f_create_si_next( uchar *pkt, uchar *b, int pid);
void f_create_si_last( uchar *pkt, uchar *b, int bytes, int pid );
void f_create_si_seq( tp_si_seq *cblk );

//
// PCR fields
//
int pcr_fmt( uchar *b, int stuff );
void pcr_timestamp( uchar *b );


// eit.c
void eit_init(void);
void eit_dvb( void );

// pat.c
void pat_fmt( void );
void pat_init( void );
void pat_dvb( void );

// pmt.c
void pmt_fmt( void );
void pmt_init( void );
void pmt_dvb( void );
void pmt_fmt( int video_stream_type, int audio_stream_type );

// null.c
void null_fmt( void );
void null_init( void );
void padding_null_dvb( void );
uchar *get_padding_null_dvb( void );

// sdt.c
void sdt_fmt( void );
void sdt_init( void );
void sdt_dvb( void );

// nit.c
void nit_fmt( void );
void nit_init( void );
void nit_dvb( void);

// eit.c
void eit_fmt( void );
void eit_init( void );
void eit_dvb( void);

// tdt.c
void tdt_init( void );
void tdt_dvb( void );
void tdt_fmt( uchar *b );

// f_tp.h
void tp_send( uchar *b );

// Update the continuits counter in a packet
void update_cont_counter( uchar *b );

int tp_pmt_fmt( uchar *b, tp_pmt *p );

// Adaption and clock recovery dts pts escr file
void post_pes_actions( uchar *b );
int  extract_scr_from_pack_header( uchar *b, int len );
void post_scr_actions(void);
void post_ts(int64_t pts);
void force_pcr( int64_t ts);
void scr_pcr_sync_clocks( uchar *b, int len );
void pcr_transport_packet_clock_update(void);
bool is_pcr_less_than_scr( void );
double pcr_value(void);
double scr_value(void);
double pcr_scr_difference(bool print);
bool is_pcr_update(void);
void pcr_scr_init(void);
void pcr_scr_equate_clocks(void);
int  add_pcr_field( uchar *b );
void scr_clock_add( unsigned int len );
void extract_pts( uchar *b );
void haup_pvr_audio_packet(void);
void haup_pvr_video_packet(void);

#endif
