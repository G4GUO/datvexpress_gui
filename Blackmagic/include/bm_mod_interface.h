#ifndef BM_MOD_INTERFACE_H
#define BM_MOD_INTERFACE_H
#include "dvb_config.h"

int blackmagic_main(int argc, char *argv[]);
int dl_list_devices( CaptureCardList *cards );
int dl_init(void);
void dl_close(void);

#endif // BM_MOD_INTERFACE_H
