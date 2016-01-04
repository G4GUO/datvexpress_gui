#include "capturedialog.h"
#include "ui_Capturedialog.h"

CaptureDialog::CaptureDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CaptureDialog)
{
    ui->setupUi(this);
    initial_parameter_display();

    this->update();
}

void CaptureDialog::initial_parameter_display(void)
{
    dvb_config_get( &m_cfg );
    switch(m_cfg.cap_format.video_format)
    {
    case CAP_PAL:
        ui->radioButtonPal720x576->setChecked(true);
        break;
    case CAP_NTSC:
        ui->radioButtonNtsc720x480->setChecked(true);
        break;
    case CAP_AUTO:
    default:
        ui->radioButtonAuto->setChecked(true);
        break;
    }
    if(m_cfg.sw_codec.using_sw_codec == false){
        ui->labelVideoCodec->hide();
        ui->radioButtonHEVC->hide();
        ui->radioButtonMPEG2->hide();
        ui->radioButtonMPEG4->hide();
    }else{
        switch(m_cfg.sw_codec.video_encoder_type){
        case CODEC_MPEG2:
            ui->radioButtonMPEG2->setChecked(true);
            break;
        case CODEC_MPEG4:
            ui->radioButtonMPEG4->setChecked(true);
            break;
        case CODEC_HEVC:
            ui->radioButtonHEVC->setChecked(true);
            break;
        }
    }
}

void CaptureDialog::on_buttonBox_accepted()
{
    if(ui->radioButtonPal720x576->isChecked() == true ) m_cfg.cap_format.video_format = CAP_PAL;
    if(ui->radioButtonNtsc720x480->isChecked() == true ) m_cfg.cap_format.video_format = CAP_NTSC;
    if(ui->radioButtonAuto->isChecked()    == true ) m_cfg.cap_format.video_format = CAP_AUTO;

    if(m_cfg.sw_codec.using_sw_codec == true){
        if(ui->radioButtonMPEG2->isChecked() == true) m_cfg.sw_codec.video_encoder_type = CODEC_MPEG2;
        if(ui->radioButtonMPEG4->isChecked() == true) m_cfg.sw_codec.video_encoder_type = CODEC_MPEG4;
        if(ui->radioButtonHEVC->isChecked() == true) m_cfg.sw_codec.video_encoder_type = CODEC_HEVC;
    }
    dvb_config_save( &m_cfg );
}
