#include <stdio.h>
#include <memory.h>
#include "mp_ts_ids.h"
#include "dvb_gen.h"
#include "mp_tp.h"
#include "mp_si_desc.h"

int si_service_desc_fmt( uchar *b, si_service_descriptor *sd )
{
	int len = 0;
	b[len++] = SI_DESC_SERVICE;
	len++;
	b[len++] = sd->type;

	b[len++] = sd->provider_length;
	memcpy(&b[len],sd->provider, sd->provider_length );
	len += sd->provider_length;

	b[len++] = sd->name_length;
	memcpy(&b[len],sd->name, sd->name_length );
	len += sd->name_length;
	b[1] = len-2;

	return len;
}
int si_network_name_desc_fmt( uchar *b, si_net_name_desc *nd )
{
	int len = 0;
	b[len++] = SI_DESC_NET_NAME;
	b[len++] = nd->name_length;
	memcpy(&b[len],nd->name, nd->name_length );
	len += nd->name_length;
	return len;
}

int si_svc_lst_desc_fmt( uchar *b, si_svc_list_desc *ld )
{
	int i;
	int len = 0;
	b[len++] = SI_DESC_SVC_LST;
	len++;
	for( i = 0; i < ld->table_length; i++ )
	{
		b[len++] = ld->entry[i].service_id>>8;
		b[len++] = ld->entry[i].service_id&0xFF;
		b[len++] = ld->entry[i].service_type;
	}
	b[1] = (len-2);
	return len;
}
//
// Description of the event
//
int si_extended_event_desc_fmt( uchar *b, si_ee_descriptor *ed )
{
	int i,n;
	int len = 0;
	b[len++] = SI_DESC_EXT_EVNT;
	b[len++] = ed->length;
	b[len++] = ed->number;
	b[len++] = ed->last_number;
	b[len++] = (ed->iso_639_language_code>>16)&0xFF;
	b[len++] = (ed->iso_639_language_code>>8)&0xFF;
	b[len++] = (ed->iso_639_language_code)&0xFF;
	b[len++] = ed->length_of_items;
	for( i = 0; i < ed->length_of_items; i++ )
	{
		b[len++] = ed->items[i].item_description_length;
		for( n = 0; n < ed->items[i].item_description_length; n++ )
		{
			b[len++] = ed->items[i].item_desc_text[n];
		}
		b[len++] = ed->items[i].item_description_length;
		for( n = 0; n < ed->items[i].item_length; n++ )
		{
			b[len++] = ed->items[i].item_text[n];
		}
	}
	b[len++] = ed->text_length;
	for( n = 0; n < ed->text_length; n++ )
	{
		b[len++] = ed->text[n];
	}
	return len;
}
//
// Description of the event
//
int si_short_event_desc_fmt( uchar *b, si_se_descriptor *ed )
{
    int i,n;
    int len = 0;
    b[len++] = SI_DESC_SE;
    len++;
    b[len++] = (ed->iso_639_language_code>>16)&0xFF;
    b[len++] = (ed->iso_639_language_code>>8)&0xFF;
    b[len++] = (ed->iso_639_language_code)&0xFF;
    // Get the length of the event name
    n = strlen(ed->event_name);
    b[len++] = n;
    // Add the name
    for( i = 0; i < n; i++ )
    {
        b[len++] = ed->event_name[i];
    }
    n = strlen(ed->event_text);
    b[len++] = n;
    for( i = 0; i < n; i++ )
    {
       b[len++] = ed->event_text[i];
    }
    // Set up the length field
    b[1] = len - 2;
    return len;
}

int si_content_event_desc_fmt( uchar *b, si_ct_descriptor *ct )
{
    int len = 0;
    b[len++] = SI_DESC_CONTENT;
    b[len++] = ct->nr_items*2;// length
    for( int i = 0; i < ct->nr_items; i++ )
    {
        b[len++] = (ct->item[i].l1_nibble<<4)|(ct->item[i].l2_nibble);
        b[len++] = (ct->item[i].u1_nibble<<4)|(ct->item[i].u2_nibble);
    }
    return len;
}

int si_teletext_desc_fmt( uchar *b, si_tt_descriptor *tt )
{
    int len = 0;
    b[len++] = SI_DESC_TELETEXT;
    b[len++] = tt->nr_items*5;// length
    for( int i = 0; i < tt->nr_items; i++ )
    {
        b[len++] = tt->item[i].iso_language_code[0];
        b[len++] = tt->item[i].iso_language_code[1];
        b[len++] = tt->item[i].iso_language_code[2];
        b[len]   = tt->item[i].teletext_type<<3;
        b[len++] |= tt->item[i].teletext_magazine_number&0x07;
        b[len++] = tt->item[i].teletext_page_number;
    }
    return len;
}

int add_si_descriptor( uchar *b, si_desc *d )
{
    int len = 0;

    switch( d->tag )
    {
        case SI_DESC_SERVICE:
            d->sd.tag = d->tag;
            len = si_service_desc_fmt( b, &d->sd );
            break;
        case SI_DESC_NET_NAME:
            d->nnd.tag = d->tag;
            len = si_network_name_desc_fmt( b, &d->nnd );
            break;
        case SI_DESC_SVC_LST:
            d->sld.tag = d->tag;
            len = si_svc_lst_desc_fmt( b, &d->sld );
            break;
        case SI_DESC_EXT_EVNT:
            d->eed.tag = d->tag;
            len = si_extended_event_desc_fmt( b, &d->eed );
            break;
        case SI_DESC_SE:
            d->sed.tag = d->tag;
            len = si_short_event_desc_fmt( b, &d->sed );
            break;
        case SI_DESC_CONTENT:
            d->sctd.tag = d->tag;
            len = si_content_event_desc_fmt( b, &d->sctd );
            break;
        case SI_DESC_TELETEXT:
            d->ttd.tag = d->tag;
            len = si_teletext_desc_fmt( b, &d->ttd );
            break;
        default:
            break;
    }
    return len;
}

