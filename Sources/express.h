#ifndef EXPRESS_H
#define EXPRESS_H
#include "dvb.h"
#include "dvb_types.h"
#include "dvb_gen.h"
#include "dvb_buffer.h"

#define USB_VENDOR     0x4B4
#define USB_PROD       0x8613
// Both are out for in add 0x80
#define EP1OUT         0x01
#define EP1IN          0x81
#define EP2OUT         0x02

#define USB_TIMEOUT    1000

#ifdef __LP64__
#define N_USB_TX_BUFFS 20
#else
#define N_USB_TX_BUFFS 200
#endif


//
// Addresses of various I2C devices on the DATVExpress board
// Everything has an address including the FPGA
// If the LSB is zero it is a write operation if it is a one
// it is a read operation.
//
#define ADRF6755_ADD 0x80
#define AD7992_ADD   0x40
#define FPGA_ADD     0x8C
#define SI570_ADD    0xAA
#define FLASH_ADD    0xAE
#define FX2_ADD      0x42
#define I2C_WR       0x00
#define I2C_RD       0x01

// Commands to FX2
#define RST_CMD      0x00
#define BK_CMD       0x01
#define TX_CMD       0x02
#define RX_CMD       0x03

// Comparison frequency in the PLL
#define PFD40        40000000.0

// FPGA registers
// Configuration register
#define FPGA_CONFIG_REG   0
#define FPGA_8BIT_MODE    0x00
#define FPGA_16BIT_MODE   0x01
#define FPGA_USE_SI570    0x02

// Filter and interpolator register
#define FPGA_FIL_REG  1
#define FPGA_INT_REG  2
#define FPGA_FEC_REG  3

#define IRATE2        0
#define IRATE4        1
#define IRATE8        2

// Ancillary values
#define FPGA_CARRIER 0x01
#define FPGA_CALIBRA 0x02

// DC offset registers
#define FPGA_I_DC_MSB_REG 4
#define FPGA_I_DC_LSB_REG 5
#define FPGA_Q_DC_MSB_REG 6
#define FPGA_Q_DC_LSB_REG 7

// Ancillary register
#define FPGA_ANC_REG      8

// Symbol rate registers
#define FPGA_SR_REG       10

// Iteration rates
#define SR_THRESHOLD_HZ       100000000
#define SR_THRESHOLD_SI570_HZ 100000000

// Threshold we need to switch to 8 bit mode
//#define BIT_MODE_THRESHOLD 320000000

#define EXP_OK    1
#define EXP_FAIL -1
#define EXP_MISS -2
#define EXP_IHX  -3
#define EXP_RBF  -4
#define EXP_CONF -5

// Write data to USB
int express_send_dvb_buffer( dvb_buffer *b );
//
void express_insert_bytes(int n);
void express_deinit(void);
int  express_init( const char *fx2_filename, const char *fpga_filename);
void express_set_freq( double freq );
void express_set_level( int level );
void express_enable( void );
void express_disable( void );
void express_fpga_reset(void);
// Program the symbol rate
int express_set_sr( long double sr );
// Select the channel interpolation rate and filter (of 4)
void express_set_interp( int interp );
void express_set_filter( int filter );
void express_set_fec( int fec );
// Read the Version number of the ADRF6755 chip
void express_read_adrf6755_version(void);
// Read the two ADC channels
void express_read_ad7992_chans(void);
// Start the FX2 code running
void express_run(void);
// Load calibration values
void express_load_calibration( void );
// Number of outstanding samples left to send on the hardware
double express_outstanding_queue_size(void);
// Can we queue another buffer
bool express_tx_below_threshold(void);
// Release any outstanding transfrt buffers
void express_release_transfer_buffers(void);
// Used to send a bulk transfer message to the Express board for I2C transmittal
int express_i2c_bulk_transfer(int ep, unsigned char *b, int l );
// Polls the event handler, this is needed for I2C messages as they
// are not handled by the sample transfer routines in Express
void express_handle_events(int n);
// Signal that a Si570 has been detected and configured
void express_si570_fitted(void);
// Enables the DAC and modulator
void express_transmit(void);
// Disables the DAC and modulator
void express_receive(void);
// Sets whether Si570 is in use
void express_set_config_byte(void);
// Set/clear carrier output
void express_set_carrier( bool b);
// Set/clear calibration output
void express_set_calibrate( bool b);

#endif // FX2USB_H
