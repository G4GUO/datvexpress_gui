#include <memory.h>
#include <stdio.h>
#include "dvb_t.h"
#include "dvb_config.h"

#define M_K 53
#define M_N 67

// Externally visible
extern DVBTFormat m_format;
int m_bit_modulo;

// TP information 8 versions
uchar sstd[8][SYMS_IN_FRAME];
// 
// The Poly has been bit reversed
//
int bch( int bit, int en, long *shift )
{
    if(en)
    {
        if( bit^(*shift&1))
        {
            *shift ^= BCH_RPOLY;
            *shift |= (1<<(M_N-M_K));
        }
    }
    else
    {
        bit = *shift&1;
    }
    *shift>>=1;

    return bit;
}

void dvb_t_bch_encode( uchar *b )
{
    int i;
    long shift = 0;
    //
    // Information bits.
    //
    for( i = 0; i < M_K; i++ )
    {
        if(b[i])
        {
            bch( 1, 1, &shift );
        }
        else
        {
            bch( 0, 1, &shift );
        }
    }
    //
    // Now the parity bits
    //
    for( i = 0; i < M_N-M_K; i++ )
    {
        b[i+M_K] = bch( 0, 0, &shift );
    }
}
//
// Differentially encode the tables
// The correct magnitudes will be set during the 
// frame building process
//
void build_diff_tp_tables( uchar *in, uchar *out )
{
    int i;

    out[0] = in[0];

    for( i = 1; i < SYMS_IN_FRAME; i++ )
    {
        if( in[i] == 0 )
            out[i] = out[i-1];
        else
            out[i] = out[i-1]^1;
    }
}
void build_tp_block( void )
{
	int i;

    uchar s[4][SYMS_IN_FRAME];


	// Length
        s[0][17] = 0;
        s[0][18] = 1;
        s[0][19] = 0;
        s[0][20] = 1;
        s[0][21] = 1;
        s[0][22] = 1;


	// Constellation patterns

        if( m_format.co == CO_QPSK )
	{
                s[0][25] = 0;
                s[0][26] = 0;
		m_bit_modulo = 2;
	}
        if( m_format.co == CO_16QAM )
	{
                s[0][25] = 0;
                s[0][26] = 1;
		m_bit_modulo = 4;
	}
        if( m_format.co == CO_64QAM )
	{
                s[0][25] = 1;
                s[0][26] = 0;
		m_bit_modulo = 6;
	}

	// Signalling format for a values
        if( m_format.sf == SF_NH )
	{
                s[0][27] = 0;
                s[0][28] = 0;
                s[0][29] = 0;
	}
        if( m_format.sf == SF_A1 )
	{
                s[0][27] = 0;
                s[0][28] = 0;
                s[0][29] = 1;
	}
        if( m_format.sf == SF_A2 )
	{
                s[0][27] = 0;
                s[0][28] = 1;
                s[0][29] = 0;
	}
        if( m_format.sf == SF_A4 )
	{
                s[0][27] = 0;
                s[0][28] = 1;
                s[0][29] = 1;
	}

	// Code rates

        if( m_format.fec == CR_12 )
	{
                s[0][30] = 0;
                s[0][31] = 0;
                s[0][32] = 0;
	}
        if( m_format.fec == CR_23 )
	{
                s[0][30] = 0;
                s[0][31] = 0;
                s[0][32] = 1;
	}
        if( m_format.fec == CR_34 )
	{
                s[0][30] = 0;
                s[0][31] = 1;
                s[0][32] = 0;
	}
        if( m_format.fec == CR_56 )
	{
                s[0][30] = 0;
                s[0][31] = 1;
                s[0][32] = 1;
	}
        if( m_format.fec == CR_78 )
	{
                s[0][30] = 1;
                s[0][31] = 0;
                s[0][32] = 0;
	}

        if( m_format.fec == CR_12 )
	{
                s[0][33] = 0;
                s[0][34] = 0;
                s[0][35] = 0;
	}
        if( m_format.fec == CR_23 )
	{
                s[0][33] = 0;
                s[0][34] = 0;
                s[0][35] = 1;
	}
        if( m_format.fec == CR_34 )
	{
                s[0][33] = 0;
                s[0][34] = 1;
                s[0][35] = 0;
	}
        if( m_format.fec == CR_56 )
	{
                s[0][33] = 0;
                s[0][34] = 1;
                s[0][35] = 1;
	}
        if( m_format.fec == CR_78 )
	{
                s[0][33] = 1;
                s[0][34] = 0;
                s[0][35] = 0;
	}

	// Guard intervals

        if( m_format.gi == GI_132 )
	{
                s[0][36] = 0;
                s[0][37] = 0;
	}
        if( m_format.gi == GI_116 )
	{
                s[0][36] = 0;
                s[0][37] = 1;
	}
        if( m_format.gi == GI_18 )
	{
                s[0][36] = 1;
                s[0][37] = 0;
	}
        if( m_format.gi == GI_14 )
	{
                s[0][36] = 1;
                s[0][37] = 1;
	}

	// Transmission mode 

        if( m_format.tm == TM_2K )
	{
                s[0][38] = 0;
                s[0][39] = 0;
	}
        if( m_format.tm == TM_8K  )
	{
                s[0][38] = 0;
                s[0][39] = 1;
	}

	// Stuff with zero

	for( i = 40; i < 54; i++ )
	{
                s[0][i] = 0;
	}
        // Duplicate the frames
        memcpy(s[1],s[0],sizeof(uchar)*SYMS_IN_FRAME);
        memcpy(s[2],s[0],sizeof(uchar)*SYMS_IN_FRAME);
        memcpy(s[3],s[0],sizeof(uchar)*SYMS_IN_FRAME);
        // Sync sequences
        s[0][1]  = 0; s[1][1]  = 1;
        s[0][2]  = 0; s[1][2]  = 1;
        s[0][3]  = 1; s[1][3]  = 0;
        s[0][4]  = 1; s[1][4]  = 0;
        s[0][5]  = 0; s[1][5]  = 1;
        s[0][6]  = 1; s[1][6]  = 0;
        s[0][7]  = 0; s[1][7]  = 1;
        s[0][8]  = 1; s[1][8]  = 0;
        s[0][9]  = 1; s[1][9]  = 0;
        s[0][10] = 1; s[1][10] = 0;
        s[0][11] = 1; s[1][11] = 0;
        s[0][12] = 0; s[1][12] = 1;
        s[0][13] = 1; s[1][13] = 0;
        s[0][14] = 1; s[1][14] = 0;
        s[0][15] = 1; s[1][15] = 0;
        s[0][16] = 0; s[1][16] = 1;
        memcpy(&s[2][1],&s[0][1],sizeof(uchar)*16);
        memcpy(&s[3][1],&s[1][1],sizeof(uchar)*16);
        //
        // Signalling format for each frame
        //
        // Frame 1
        s[0][23] = 0;
        s[0][24] = 0;
        // Frame 2
        s[1][23] = 0;
        s[1][24] = 1;
        // Frame 3
        s[2][23] = 1;
        s[2][24] = 0;
        // Frame 4
        s[3][23] = 1;
        s[3][24] = 1;

	// Add BCH code
	dvb_t_bch_encode( &s[0][1] );
	dvb_t_bch_encode( &s[1][1] );
    dvb_t_bch_encode( &s[2][1] );
    dvb_t_bch_encode( &s[3][1] );

	// Generate differentially encoded tables
	s[0][0] = 0;
    build_diff_tp_tables( s[0], sstd[0] );
	s[0][0] = 1;
    build_diff_tp_tables( s[0], sstd[1] );

	s[1][0] = 0;
    build_diff_tp_tables( s[1], sstd[2] );
	s[1][0] = 1;
    build_diff_tp_tables( s[1], sstd[3] );

    s[2][0] = 0;
    build_diff_tp_tables( s[2], sstd[4] );
    s[2][0] = 1;
    build_diff_tp_tables( s[2], sstd[5] );

    s[3][0] = 0;
    build_diff_tp_tables( s[3], sstd[6] );
    s[3][0] = 1;
    build_diff_tp_tables( s[3], sstd[7] );

}
