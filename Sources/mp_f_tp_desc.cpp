#include "dvb_gen.h"
#include "mp_tp.h"

//
// Format transport descriptors
//

int tp_video_desc_fmt( uchar *b, tp_v_desc *p )
{
	int len = 0;

	b[0] = 0x02;
	b[1] = 0;
	b[2] = 0;
	if( p->multiple_frame_rate_flag) b[2] |= 0x80;
	b[2] |= (p->frame_rate_code<<3);
	if( p->mpeg_1_only_flag ) b[2] |= 0x04;
	if( p->constrained_parameter_flag ) b[2] |= 0x02;
	if( p->still_picture_flag ) b[2] |= 0x01;
	len = 3;
	if( p->mpeg_1_only_flag )
	{
		b[len++] = p->profile_and_level_indication;
		b[len]   = (p->chroma_format<<6);
		b[len++] |= (p->frame_rate_extension_flag<<5);
	}
	b[1] = len-2;
	return( len );
}
int tp_audio_desc_fmt( uchar *b, tp_a_desc *p )
{
	b[0] = 0x03;
	b[2] = 0;
	if( p->free_format_flag ) b[2] |= 0x80;
	if( p->id ) b[2] |= 0x40;
	b[2] |= (p->layer<<4);
	if(p->variable_rate_audio_indicator) b[2] |= 0x04;
	b[1] = 1;
	return 3;
}
int tp_hierarchy_desc_fmt( uchar *b, tp_h_desc *p )
{

	b[0] = 0x04;
	b[2] = 0;
	b[3] = p->type;
	b[4] = p->layer_index;
	b[5] = p->embedded_layer_index;
	b[6] = p->channel;
	b[1] = 5;
	return 7;
}
int tp_registration_desc_fmt( uchar *b, tp_r_desc *p )
{
	int i;
	int len = 0;

	b[0] = 0x05;
	b[2] = p->format_indentifier;
	len = 3;
	for( i = 0; i < p->info_length; i++ )
	{
		b[len++] = p->info[i];
	}
	b[1] = len-2;
	return( len ); 
}
