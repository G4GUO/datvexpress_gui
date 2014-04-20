//
// Add descriptors
//
#include "dvb_gen.h"
#include "mp_tp.h"

int add_descriptor( uchar *b, td_descriptor *d )
{
	int len = 0;

	switch( d->table_id )
	{
		case 0x02:
			len = tp_video_desc_fmt( b, &d->video );
			break;
		case 0x03:
			len = tp_audio_desc_fmt( b, &d->audio );
			break;
		case 0x04:
			len = tp_hierarchy_desc_fmt( b, &d->hier );
			break;
		case 0x05:
			len = tp_registration_desc_fmt( b, &d->reg );
		default:
			break;
	}
    return len;
}
