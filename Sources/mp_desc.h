#include "dvb_gen.h"
#include "mp_si_desc.h"
#include "dvb_si.h"

// Program map section
// Descriptors
// Video
typedef struct{
	uchar multiple_frame_rate_flag;
	uchar frame_rate_code;
	uchar mpeg_1_only_flag;
	uchar constrained_parameter_flag;
	uchar still_picture_flag;
	uchar profile_and_level_indication;
	uchar chroma_format;
	uchar frame_rate_extension_flag;
}tp_v_desc;

// Audio
typedef struct{
	uchar free_format_flag;
	uchar id;
	uchar layer;
	uchar variable_rate_audio_indicator;
}tp_a_desc;

// Hierarchy descriptor
typedef struct{
	uchar tag;
	uchar length;
	uchar type;
	uchar layer_index;
	uchar embedded_layer_index;
	uchar channel;
}tp_h_desc;

// Registration descriptor
typedef struct{
	uchar tag;
	uchar length;
	uchar format_indentifier;
	uchar info_length;
	uchar info[1024];
}tp_r_desc;

// Descriptor
typedef union{
	uchar table_id;
	tp_v_desc video;
	tp_a_desc audio;
	tp_h_desc hier;
	tp_r_desc reg;
}td_descriptor;
//
// SDT definition
//

typedef struct{
	int service_id;
	uchar eit_schedule_flag;
	uchar eit_present_follow_flag;
	uchar running_status;
	uchar free_ca_mode;
	int nr_descriptors;
	si_desc descriptor[10];
}sdt_section;

typedef struct{
	uchar table_id;
	uchar section_syntax_indicator;
	int section_length;
	int transport_stream_id;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	int original_network_id;
	sdt_section section[10];
}service_description_section;

// Network Information section{

typedef struct{
	int transport_stream_id;
	int original_network_id;
	int transport_desriptors_length;
	int nr_descriptors;
	si_desc desc[10];
}transport_stream_info;

typedef struct{
	uchar table_id;
	uchar section_syntax_indicator;
	int section_length;
	int network_id;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	int nr_network_descriptors;
	si_desc n_desc[10];
	int nr_transport_streams;
	transport_stream_info ts_info[10];
}network_information_section;

// EIS

typedef struct{
	int event_id;
        dvb_si_time     start_time;
        dvb_si_duration duration;
	uchar running_status;
	uchar free_ca_mode;
	int nr_descriptors;
	si_desc descriptors[10];
}eis_info;

typedef struct{
	uchar table_id;
	uchar section_syntax_indicator;
	int section_length;
	int service_id;
	uchar version_number;
	uchar current_next_indicator;
	uchar section_number;
	uchar last_section_number;
	int transport_stream_id;
	int original_network_id;
	uchar segment_last_section_number;
	uchar last_table_id;
	eis_info section[10];
}event_information_section;


int tp_video_desc_fmt( uchar *b, tp_v_desc *p );
int tp_audio_desc_fmt( uchar *b, tp_a_desc *p );
int tp_hierarchy_desc_fmt( uchar *b, tp_h_desc *p );
int tp_registration_desc_fmt( uchar *b, tp_r_desc *p );

// Adds a descriptor to the buffer
int add_descriptor( uchar *b, td_descriptor *d );

