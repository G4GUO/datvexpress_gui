#include "mainwindow.h"
#include <QApplication>
#include "dvb_gui_if.h"
#include "bm_mod_interface.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dvb_start();
//    blackmagic_main(0, NULL);

    MainWindow w;
    w.show();
    w.NextUpdateDisplayedDVBParams();
    a.exec();
    dvb_stop();
    return 0;
}
