#include "dvb_t.h"
// This modules is an LPF filter used for filtering the shoulders and aliases
// on the ouput of the DVB-T iFFT
// It operates in place
//
/*

FIR filter designed with
http://t-filter.appspot.com

sampling frequency: 1000000 Hz

* 0 Hz - 108000 Hz
  gain = 1
  desired ripple = 0.1 dB
  actual ripple = 0.3163245849364087 dB

* 125000 Hz - 500000 Hz
  gain = 0
  desired attenuation = -30 dB
  actual attenuation = -17.351411726038933 dB

*/

#define FILTER_TAP_NUM 63

static double filter_taps[FILTER_TAP_NUM] = {
  0.01564319265083877,
  -0.05360372433912335,
  0.008823966495059024,
  0.01946696271674653,
  0.014144626255836222,
  0.004361238484562726,
  -0.005195423651821919,
  -0.011502589504731202,
  -0.012265050466923388,
  -0.007043621874215022,
  0.0020889937904723698,
  0.010969950321719045,
  0.014955015997906912,
  0.011280887944933924,
  0.0008716627692732737,
  -0.011560064444749683,
  -0.019385421771133744,
  -0.017467384086196223,
  -0.005266970802671308,
  0.012249886250828589,
  0.026253392081978875,
  0.027933912094048476,
  0.01345208497392874,
  -0.012893898586358688,
  -0.039351373824861875,
  -0.05065241375456205,
  -0.034240265488211456,
  0.013331179663418256,
  0.0835727661521914,
  0.15836967807309296,
  0.21526028205130704,
  0.2365899530831242,
  0.21526028205130704,
  0.15836967807309296,
  0.0835727661521914,
  0.013331179663418256,
  -0.034240265488211456,
  -0.05065241375456205,
  -0.039351373824861875,
  -0.012893898586358688,
  0.01345208497392874,
  0.027933912094048476,
  0.026253392081978875,
  0.012249886250828589,
  -0.005266970802671308,
  -0.017467384086196223,
  -0.019385421771133744,
  -0.011560064444749683,
  0.0008716627692732737,
  0.011280887944933924,
  0.014955015997906912,
  0.010969950321719045,
  0.0020889937904723698,
  -0.007043621874215022,
  -0.012265050466923388,
  -0.011502589504731202,
  -0.005195423651821919,
  0.004361238484562726,
  0.014144626255836222,
  0.01946696271674653,
  0.008823966495059024,
  -0.05360372433912335,
  0.01564319265083877
};
fft_complex m_mem[FILTER_TAP_NUM];// Circular buffer
fft_complex *m_out = NULL;
int m_ip;
int m_length = 0;

//
// This return a pointer to the filtered data
//
fft_complex *dvbt_filter( fft_complex *in, int length )
{
    int op = 0;
    // Allocate memory if needed
    if( length > m_length )
    {
        if( m_out != NULL ) free(m_out);
        m_length = sizeof(fft_complex)*length;
        m_out = (fft_complex*)malloc(m_length);

    }

    for( int i = 0; i < length; i++ )
    {
        m_mem[m_ip] = in[i];
        m_ip = (m_ip+1)%FILTER_TAP_NUM;

        m_out[op].re = m_out[op].im = 0;
        int k = m_ip;
        for( int n = 0; n < FILTER_TAP_NUM; n++ )
        {
            m_out[op].re += filter_taps[n]*m_mem[k].re;
            m_out[op].im += filter_taps[n]*m_mem[k].im;
            k = (k+1)%FILTER_TAP_NUM;
        }
        op++;
    }
    return m_out;
}
