#include "mainwindow.h"
#include <QApplication>
#include "dvb_gui_if.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    dvb_start();
    MainWindow w;
    w.show();
    w.NextUpdateDisplayedDVBParams();
    a.exec();
    dvb_stop();
    return 0;
}
