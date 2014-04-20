//
// SI descriptors
//
#include "dvb_gen.h"
#include "dvb_si.h"

#ifndef SI_DESC_H
#define SI_DESC_H

#define SVC_DIGITAL_TV   0x01

#define SI_DESC_SERVICE  0x48
#define SI_DESC_NET_NAME 0x40
#define SI_DESC_SVC_LST  0x41
#define SI_DESC_EXT_EVNT 0x43
#define SI_DESC_SE       0x4D
#define SI_DESC_CONTENT  0x54
#define SI_DESC_TELETEXT 0x56

typedef struct{
	int service_id;
	uchar service_type;
}si_svc_tbl_entry;

typedef struct{
	uchar tag;
	int table_length;
	si_svc_tbl_entry entry[10];
}si_svc_list_desc;

typedef struct{
	uchar tag;
	uchar name_length;
	uchar name[1024];
}si_net_name_desc;

typedef struct{
	uchar tag;
	uchar type;
	uchar provider_length;
	uchar provider[1024];
	uchar name_length;
	uchar name[1024];
}si_service_descriptor;

typedef struct{
	uchar item_description_length;
	uchar item_desc_text[300];
	uchar item_length;
	uchar item_text[300];
}ee_item;

typedef struct{
	uchar tag;
	uchar length;
	uchar number;
	uchar last_number;
	unsigned long iso_639_language_code;
	uchar length_of_items;
	ee_item items[10];
	uchar text_length;
	uchar text[300];
}si_ee_descriptor;

typedef struct{
    uchar tag;
    int iso_639_language_code;
    const char *event_name;
    const char *event_text;
}si_se_descriptor;

typedef struct{
    uchar l1_nibble;
    uchar l2_nibble;
    uchar u1_nibble;
    uchar u2_nibble;
}content_nibbles;

typedef struct{
    uchar tag;
    int nr_items;
    content_nibbles item[5];
}si_ct_descriptor;

typedef struct{
    uchar iso_language_code[3];
    uchar teletext_type;
    uchar teletext_magazine_number;
    uchar teletext_page_number;
}si_teletext_item;

typedef struct{
    uchar tag;
    int nr_items;
    si_teletext_item item[5];
}si_tt_descriptor;

typedef union{
	uchar tag;
	si_service_descriptor sd;
	si_net_name_desc      nnd;
	si_svc_list_desc      sld;
	si_ee_descriptor      eed;
        si_se_descriptor      sed;
        si_ct_descriptor      sctd;
        si_tt_descriptor      ttd;
}si_desc;


int add_si_descriptor( uchar *b, si_desc *d );


#endif
