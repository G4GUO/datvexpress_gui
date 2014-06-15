#ifndef CAPTUREDIALOG_H
#define CAPTUREDIALOG_H

#include <QDialog>
#include "dvb_config.h"

namespace Ui {
    class CaptureDialog;
}

class CaptureDialog : public QDialog
{
    Q_OBJECT
public:
    explicit CaptureDialog(QWidget *parent = 0);

private:
    sys_config m_cfg;
    Ui::CaptureDialog *ui;

    void initial_parameter_display(void);

signals:

public slots:

private slots:
    void on_buttonBox_accepted();
};

#endif // CAPTUREDIALOG_H
