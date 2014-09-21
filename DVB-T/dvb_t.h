#ifndef __DVB_T_H__
#define __DVB_T_H__

#include "dvb_options.h"

#ifdef USE_AVFFT

#define __STDC_CONSTANT_MACROS

extern "C" {
#include <libavutil/avutil.h>
#include <libavcodec/avfft.h>
#include <libavutil/mem.h>
}
#else
#include "fftw.h"
#endif

#include "Sources/dvb_types.h"
#include "Sources/dvb_gen.h"

#define SYMS_IN_FRAME 68

#ifdef USE_AVFFT
#define fft_complex FFTComplex
#define FLOAT float
#else
#define fft_complex fftw_complex
#define FLOAT double
#endif

// Minimum cell nr
#define K2MIN 0
#define K8MIN 0

// Max cell nr
#define K2MAX 1704
#define K8MAX 6816
#define SF_NR 4

// Size of FFT
#define M2KS  2048
#define M4KS  4096
#define M8KS  8192
#define M16KS 16384

// Number of data cells
#define M2SI 1512
#define M8SI 6048

// FFT bin number to start at
#define M8KSTART 688
#define M2KSTART 172

#define BIL 126
#define M_QPSK  0
#define M_QAM16 1
#define M_QAM64 2

#define BCH_POLY  0x4377
//#define BCH_RPOLY 0x7761
#define BCH_RPOLY 0x3761

// Definition of mode
#define FN_1_SP 0
#define FN_2_SP 1
#define FN_3_SP 2
#define FN_4_SP 3

#define CO_QPSK  0
#define CO_16QAM 1
#define CO_64QAM 2

#define SF_NH    0
#define SF_A1    1
#define SF_A2    2
#define SF_A4    3

#define CR_12    0
#define CR_23    1
#define CR_34    2
#define CR_56    3
#define CR_78    4

#define GI_132   0
#define GI_116   1
#define GI_18    2
#define GI_14    3

#define TM_2K    0
#define TM_8K    1

#define CH_8     0
#define CH_7     1
#define CH_6     2
#define CH_4     3
#define CH_3     4
#define CH_2     5
#define CH_1     6

typedef struct{
	uchar co;
	uchar sf;
	uchar gi;
	uchar tm;
    uchar chan;
    int   fec;
}DVBTFormat;

#define AVG_E8 (1.0/600.0)
//#define AVG_E2 (1.0/1704.0)
#define AVG_E2 (1.0/150.0)


// Prototypes

// dvb_t_tp.c
void build_tp_block( void );

// dvb_t_ta.c
void dvb_t_configure( DVBTFormat *p );

// dvb_t_sym.c
void init_reference_frames( void );
void reference_symbol_reset( void );
int reference_symbol_seq_get( void );
int reference_symbol_seq_update( void );

//dvb_t_i.c
void dvb_t_build_p_tables( void );

// dvb_t_enc.c
void init_dvb_t_enc( void );
void dvb_t_enc_dibit( uchar *in, int length );
void dvb_t_encode_and_modulate( uchar *in, uchar *dibit );

// dvb_t_mod.c
void dvb_t_select_constellation_table( void );
void dvb_t_calculate_guard_period( void );
void dvb_t_modulate( uchar *syms );
void dvb_t_write_samples( short *s, int len );
void dvb_t_modulate_init( void );

// dvb_t_linux_fft.c
void init_dvb_t_fft( void );
void deinit_dvb_t_fft( void );
void fft_2k_test( fft_complex *out );
void dvbt_fft_modulate( fft_complex *in, int guard );

// dvb_t.cpp
void   dvb_t_init( void );
void   dvb_t_deinit( void );
void   dvb_t_re_init( void );
long double dvb_t_get_sample_rate( void );
double dvb_t_get_symbol_rate( void );

// dvb_t_qam_tab.cpp
void build_tx_sym_tabs( void );

// Video bitrate
int dvb_t_raw_bitrate(void);

// dvb_t_lpf.cpp
int dvbt_filter( fft_complex *in, int length, fft_complex *out );

#endif
