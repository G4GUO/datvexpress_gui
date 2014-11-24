//
// Configuration
//
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "dvb.h"
#include "dvb_gen.h"
#include "dvb_capture_ctl.h"
#include "dvb_config.h"
#include "DVB-T/dvb_t.h"
#include "dvb_uhd.h"
#include "mp_ts_ids.h"
#include "mp_tp.h"

sys_config m_sysc;
static char config_file[1024];


// See whether the config directory exists, if not create it
void dvb_config_create( void )
{
    char *p = getenv("HOME");
    // Create the pathname
    sprintf(config_file,"%s%s",p,"/.datvexpress/");
    mkdir(config_file, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

const char *dvb_config_get_path( const char *filename )
{
    char *p = getenv("HOME");
    sprintf(config_file,"%s%s%s",p,"/.datvexpress/",filename);
    return config_file;
}

void dvb_config_save_to_disk(sys_config *cfg)
{
    FILE *fp;
    SysConfigRecord record;

    dvb_config_create();

    if((fp=fopen(dvb_config_get_path("datvexpress.cfg"),"w"))>0)
    {
        // Create a CRC of the data and sve it along with the data
        record.crc.crc  = dvb_crc32_calc((unsigned char*)cfg, sizeof(sys_config));
        // Copy the cfg into the disk record structure
        memcpy( &record.cfg, cfg, sizeof(sys_config));
        // Save
        fwrite(&record,sizeof(SysConfigRecord), 1, fp );
        fclose(fp);
    }
    else
    {
        loggerf("Cannot save configuration file (check owner)");
    }
}
//
// Set a structure to its default configuration
//
void dvb_default_configuration( sys_config *cfg)
{

    memset( cfg, 0, sizeof(sys_config));
    strcpy(cfg->version, S_VERSION );
    cfg->dvb_mode         = MODE_DVBS;
    cfg->sr_mem[0]        = 4000000;
    cfg->sr_mem[1]        = 2000000;
    cfg->sr_mem[2]        = 1000000;
    cfg->sr_mem[3]        = 4000000;
    cfg->sr_mem[4]        = 4000000;
    cfg->sr_mem[5]        = 4000000;
    cfg->sr_mem[6]        = 4000000;
    cfg->sr_mem[7]        = 4000000;
    cfg->sr_mem[8]        = 4000000;
    cfg->sr_mem[9]        = 4000000;
    cfg->sr_mem[10]       = 4000000;
    cfg->sr_mem[11]       = 4000000;
    cfg->sr_mem_nr        = 0;

    cfg->dvbs_fmt.fec     = FEC_RATE_12;
    cfg->dvbt_fmt.co      = CO_QPSK;
    cfg->dvbt_fmt.sf      = SF_NH;
    cfg->dvbt_fmt.fec     = CR_12;
    cfg->dvbt_fmt.gi      = GI_14;
    cfg->dvbt_fmt.tm      = TM_2K;
    cfg->dvbt_fmt.chan    = CH_7M;
    cfg->tx_frequency     = 1280000000;
    cfg->tx_level         = 10;
    cfg->video_pid        = P1_VID_PID;
    cfg->audio_pid        = P1_AUD_PID;
    cfg->pcr_pid          = P1_VID_PID;
    cfg->pmt_pid          = P1_MAP_PID;
    cfg->nit_pid          = NIT_PID;
    cfg->network_id       = DEFAULT_NETWORK_ID;
    cfg->stream_id        = DEFAULT_STREAM_ID;
    cfg->service_id       = DEFAULT_SERVICE_ID;
    cfg->program_nr       = DEFAULT_PROGRAM_NR;
    strcpy(cfg->video_capture_device_name," ");
    strcpy(cfg->server_ip_address,"192.168.1.64 ");
    cfg->cap_dev_type               = CAP_DEV_TYPE_NONE;
    cfg->server_socket_number       = 1958;
    cfg->video_capture_device_class = DVB_V4L;
    cfg->video_capture_device_input = 1;
    cfg->cap_format.video_format    = CAP_AUTO;
    cfg->video_codec_class          = CODEC_MPEG2;
    cfg->tx_hardware                = HW_EXPRESS_AUTO;

    strcpy(cfg->service_provider_name,"HamTV");
    strcpy(cfg->service_name,"Channel 1");
    // EPG event
    cfg->event.event_duration = 60;
    strcpy( cfg->event.event_title,"Amateur Radio");
    strcpy( cfg->event.event_text, "Amateur Radio themed program");
    //
    // DVB-S2
    //
    cfg->dvbs2_fmt.frame_type    = FRAME_NORMAL;
    cfg->dvbs2_fmt.code_rate     = CR_1_2;
    cfg->dvbs2_fmt.constellation = M_QPSK;
    cfg->dvbs2_fmt.roll_off      = RO_0_35;
    cfg->dvbs2_fmt.pilots        = 0;
    cfg->dvbs2_fmt.dummy_frame   = 0;
    cfg->dvbs2_fmt.null_deletion = 0;
    //
    // Calibration
    //
    cfg->i_chan_dc_offset = 0;
    cfg->q_chan_dc_offset = 0;
    cfg->i_chan_gain      = 1.0;
    cfg->q_chan_gain      = 1.0;
}
//
// Retrieve a recors from disk and check it's CRC
//
void dvb_config_retrieve_from_disk( sys_config *cfg )
{
    FILE *fp;
    SysConfigRecord rd;
    int success = -1;

    dvb_config_create();

    if((fp=fopen(dvb_config_get_path("datvexpress.cfg"),"r")) > 0)
    {
        size_t size = fread( &rd, 1, sizeof(SysConfigRecord), fp );
        if( size == sizeof(SysConfigRecord))
        {
            // Check the CRC to make sure it is valid
            unsigned long crc = dvb_crc32_calc((unsigned char*)&rd.cfg, sizeof(sys_config));
            if( crc == rd.crc.crc )
            {
                memcpy( cfg, &rd.cfg, sizeof(sys_config));
                success = 0;
            }
        }
        //cfg->dvbs2_fmt.constellation = M_QPSK;
        //cfg->dvb_mode         = MODE_DVBS;
        fclose(fp);
    }
    if( success != 0 )
    {
        loggerf("Using default Settings");
        dvb_default_configuration( cfg );
        dvb_config_save_to_disk( cfg );
    }
}
void dvb_config_retrieve_from_disk( void )
{
    dvb_config_retrieve_from_disk( &m_sysc );
}
//
// Copy the configuration from the running db
//
void dvb_config_get( sys_config *cfg )
{
    memcpy( cfg, &m_sysc, sizeof(sys_config));
}
const sys_config *dvb_config_get( void )
{
    return &m_sysc;
}
void dvb_config_get_disk( sys_config *cfg )
{
    dvb_config_retrieve_from_disk( cfg );
}
//
// Save the parameters to disk, do not action them
//
void dvb_config_save( sys_config *cfg )
{
    // Save to disk
    dvb_config_save_to_disk( cfg );
}
//
// Update the current active config, do not save
//
void dvb_config_save_and_update( sys_config *cfg )
{
    // Save to memory
    memcpy( &m_sysc, cfg, sizeof(sys_config));
    dvb_config_save_to_disk( cfg );
}
void dvb_config_save_to_local_memory( sys_config *cfg )
{
    // Save to memory
    memcpy( &m_sysc, cfg, sizeof(sys_config));
}
