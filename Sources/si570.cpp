//
// Controll Si570 clock generator
//
#include "si570.h"

HsDiv m_hsddiv[N_HSDIV]={{0,4},{1,5},{2,6},{3,7},{5,9},{7,11}};

static int m_hs_div;
static int m_n1;
static int64_t m_rfreq;
static long double m_fxtal;// Crystal freq

int decode_hs_div( int v )
{
    for( int i = 0; i < N_HSDIV; i++)
    {
        if(m_hsddiv[i].r == v ) return m_hsddiv[i].n;
    }
    return 0;
}

long double ldv( long double n, long double d )
{
    long double res = (n/d);
    return res;
}

void si570_calc_float_to_int_38_bit_muliplier( long double m, int64_t &r38 )
{
    long double fracpart;
    int intpart;
    intpart  = m;
    fracpart = m - intpart;
    r38      = intpart&0x3FF;
    r38      = r38<<28;
    fracpart = fracpart * 0x10000000;
    int64_t ifract = fracpart;
    r38 = r38 | (ifract&0xFFFFFFF);
}

void si570_calc_int_to_float_38_bit_muliplier( int64_t r38, long double &m )
{
    m  = (r38>>28)+ldv((r38&0xFFFFFFF),0x10000000);
}

void si570_rx( uchar reg, uchar val )
{
    // Message received fron device
    long double rfreq;
    long double fxtal;
//    printf("Received %d %d\n",reg,val);
    switch(reg)
    {
    case 7:
        m_hs_div = val>>5;
        m_n1     = val&0x1F;
        m_n1     = m_n1<<2;
        break;
    case 8:
        m_n1    |= val>>6;
        m_rfreq = val&0x3F;
        break;
    case 9:
        m_rfreq = (m_rfreq<<8) | val;
        break;
    case 10:
        m_rfreq = (m_rfreq<<8) | val;
        break;
    case 11:
        m_rfreq = (m_rfreq<<8) | val;
        break;
    case 12:
        m_rfreq = (m_rfreq<<8) | val;
        m_n1    = m_n1 +1;
        m_hs_div = decode_hs_div( m_hs_div );
        si570_calc_int_to_float_38_bit_muliplier( m_rfreq, rfreq );
        if(rfreq > 0 )
        {
            fxtal = (100000000.0 * m_n1 * m_hs_div)/rfreq;
            // Only set the xtal frequency if it is within range
            // Ideally we need to power cycle the board for this to happen
            m_fxtal = fxtal;
            express_si570_fitted();
            //            printf("HS_DIV %d N1 %d RFREQ %Lf Xtal %Lf\n",m_hs_div,m_n1,rfreq,fxtal);
        }
        break;
    default:
        break;
    }
}

void si570_query( uchar reg )
{
    // Query the contents of a register
    // Format up into an I2C message for the SI570
    unsigned char msg[4];
    msg[0]  = SI570_ADD | I2C_RD;//address
    msg[1]  = 1;//1 byte to read
    msg[2]  = reg;//Register to read
    msg[3]  = 0;//Response will go here
    // Set reg address to read from
    express_i2c_bulk_transfer( EP1OUT, msg, 3 );
    // Read the actual register
    express_i2c_bulk_transfer( EP1IN, msg, 4 );

    express_handle_events( 2 );
}

int si570_find_best_clock_params( long double fclk, int &hv, int &nv, int64_t &r38v )
{
    long double e_min = 10000.0;

    for( int h = 0; h < N_HSDIV; h++ )
    {
        for( int n = 0; n <= 128; n+=2 )
        {
            int N;
            if(n==0)
                N = 1;
            else
                N = n;

            long double fosc = N * m_hsddiv[h].n * fclk;
            // Check it is within range
            if((fosc >= MIN_DCO) && (fosc <= MAX_DCO))
            {
                // Calculate the fractional divider value
                long double fd = ldv( fosc, m_fxtal );
                // See if it is within range
                if((fd > 0)&&(fd < 1024))
                {
                    // Calculate frequency error
                    int64_t r38;
                    long double fm,error;
                    si570_calc_float_to_int_38_bit_muliplier( fd, r38 );
                    si570_calc_int_to_float_38_bit_muliplier( r38, fm );
                    error = fd - fm;
                    if((error*error) <= e_min )
                    {
                        // found better value
                        e_min = error*error;
                        hv = m_hsddiv[h].r;
                        nv = N-1;
                        r38v = r38;
                    }
                }
            }
        }
    }
    return 1;
}
void si570_set_clock( long double fclk )
{
    int rh,rn;
    int64_t r38;
    uchar msg[3];

    if( si570_find_best_clock_params( fclk, rh, rn, r38 ))
    {
        // Now set the actual register values in the Si570
        msg[0] = SI570_ADD;
        // Freeze the DCO
        msg[1] = 137;
        msg[2] = 0x10;
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        // Send the HS_DIV, N1, RFREQ
        msg[1] = 7;
        msg[2] = (rh<<5)|(rn>>2);
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        msg[1] = 8;
        msg[2] = ((rn&0x03)<<6) | ((r38>>32)&0x3F);
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        msg[1] = 9;
        msg[2] = ((r38>>24)&0xFF);
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        msg[1] = 10;
        msg[2] = ((r38>>16)&0xFF);
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        msg[1] = 11;
        msg[2] = ((r38>>8)&0xFF);
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        msg[1] = 12;
        msg[2] = (r38&0xFF);
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        // Un Freeze the DCO
        msg[1] = 137;
        msg[2] = 0x00;
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        // Invoke new frequency
        msg[1] = 135;
        msg[2] = 0x40;
        express_i2c_bulk_transfer( EP1OUT, msg, 3 );

        express_handle_events( 9 );

//        printf("Best HDIV=%d N1=%d REG=%x%x\n",rh,rn,(r38>>28),(r38&0xFFFFFFF));
    }
}
void si570_initialise( void )
{
    uchar msg[3];
    // Recall the initial conditions
    msg[0] = SI570_ADD;
    msg[1] = 135;
    msg[2] = 0x01;
    express_i2c_bulk_transfer( EP1OUT, msg, 3 );
    express_handle_events( 1 );

    // Read all the registers
    si570_query( 7 );
    si570_query( 8 );
    si570_query( 9 );
    si570_query( 10 );
    si570_query( 11 );
    si570_query( 12 );
}
