#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QMenuBar>
#include "dvb.h"
#include "dvb_gui_if.h"
#include "DVB-T/dvb_t.h"
#include "dvb_capture_ctl.h"
#include "tx_hardware.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    InitialUpdateDisplayedDVBParams();
    // Transmit timer
    QTimer *timer = new QTimer(this);
    connect( timer, SIGNAL(timeout()), this, SLOT(onTimerUpdate()));
    timer->start(100);
    createActions();
    createMenus();

    this->update();
}

void MainWindow::createActions()
{
    // Exit action
    exitAction = new QAction(tr("E&xit"), this);
    connect(exitAction, SIGNAL(triggered()), this, SLOT(close()));
    // About action
    aboutAction = new QAction(tr("&About DATV-Express"), this);
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

}
void MainWindow::createMenus()
{
    // File Menu
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(exitAction);

    // Help Menu
    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAction);
}
void MainWindow::about()
{
    QString str = tr("Controller program for the DATV-Express transmitter board\n\nVersion ");
    str += S_VERSION;
    str += "\n\nwww.DATV-Express.com";
    QMessageBox::information(this, tr("DATV-Express"),
                 str,
                 QMessageBox::Ok);
}

void MainWindow::InitialUpdateDisplayedDVBParams(void)
{

    QString st;

    // DVB-S

    ui->lineEditSR0->setText("1000000");
    ui->lineEditSR1->setText("2000000");
    ui->lineEditSR2->setText("4000000");
    ui->lineEditSR3->setText("0000000");
    ui->lineEditSR4->setText("0000000");
    ui->lineEditSR5->setText("0000000");
    ui->lineEditSR6->setText("0000000");
    ui->lineEditSR7->setText("0000000");
    ui->lineEditSR8->setText("0000000");
    ui->lineEditSR9->setText("0000000");
    ui->lineEditSR10->setText("0000000");
    ui->lineEditSR11->setText("0000000");

    ui->comboBoxDVBSFECRate->addItem(S_FEC_1_2);
    ui->comboBoxDVBSFECRate->addItem(S_FEC_2_3);
    ui->comboBoxDVBSFECRate->addItem(S_FEC_3_4);
    ui->comboBoxDVBSFECRate->addItem(S_FEC_5_6);
    ui->comboBoxDVBSFECRate->addItem(S_FEC_7_8);

    // DVB-T
    ui->comboBoxDVBTFFTMode->addItem(S_FFT_2K);
    ui->comboBoxDVBTFFTMode->addItem(S_FFT_8K);

    ui->comboBoxDVBTModulation->addItem(S_M_QPSK);
    ui->comboBoxDVBTModulation->addItem(S_M_16QAM);
    ui->comboBoxDVBTModulation->addItem(S_M_64QAM);

    ui->comboBoxDVBTGuardPeriod->addItem(S_GP_1_4);
    ui->comboBoxDVBTGuardPeriod->addItem(S_GP_1_8);
    ui->comboBoxDVBTGuardPeriod->addItem(S_GP_1_16);
    ui->comboBoxDVBTGuardPeriod->addItem(S_GP_1_32);

    ui->comboBoxDVBTFEC->addItem(S_FEC_1_2);
    ui->comboBoxDVBTFEC->addItem(S_FEC_2_3);
    ui->comboBoxDVBTFEC->addItem(S_FEC_3_4);
    ui->comboBoxDVBTFEC->addItem(S_FEC_5_6);
    ui->comboBoxDVBTFEC->addItem(S_FEC_7_8);

    ui->comboBoxDVBTChannel->addItem(S_CH_8MHZ);
    ui->comboBoxDVBTChannel->addItem(S_CH_7MHZ);
    ui->comboBoxDVBTChannel->addItem(S_CH_6MHZ);
    ui->comboBoxDVBTChannel->addItem(S_CH_4MHZ);
    ui->comboBoxDVBTChannel->addItem(S_CH_3MHZ);
    ui->comboBoxDVBTChannel->addItem(S_CH_2MHZ);

    // DVB-S2
    ui->comboBoxDVBS2Modulation->addItem(S_M_QPSK);
    ui->comboBoxDVBS2Modulation->addItem(S_M_8PSK);
    ui->comboBoxDVBS2Modulation->addItem(S_M_16APSK);
    ui->comboBoxDVBS2Modulation->addItem(S_M_32APSK);

    ui->comboBoxDVBS2FEC->addItem(S_FEC_1_4);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_1_3);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_2_5);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_1_2);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_3_5);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_2_3);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_3_4);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_4_5);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_5_6);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_8_9);
    ui->comboBoxDVBS2FEC->addItem(S_FEC_9_10);

    ui->comboBoxDVBS2RollOff->addItem(S_RO_0_35);
    ui->comboBoxDVBS2RollOff->addItem(S_RO_0_25);
    ui->comboBoxDVBS2RollOff->addItem(S_RO_0_20);

    ui->comboBoxDVBS2FrameType->addItem(S_FM_NORMAL);
   // ui->comboBoxDVBS2FrameType->addItem(S_FM_SHORT);

   // ui->comboBoxDVBS2NullPacketDeletion->addItem(S_YES);
    ui->comboBoxDVBS2NullPacketDeletion->addItem(S_NO);

   // ui->comboBoxDVBS2DummyFrameInsertion->addItem(S_YES);
    ui->comboBoxDVBS2DummyFrameInsertion->addItem(S_NO);

    ui->comboBoxDVBS2PilotInsertion->addItem(S_YES);
    ui->comboBoxDVBS2PilotInsertion->addItem(S_NO);

    // TP info

    // Program info

    // Video Capture device

    // List available video capture devices
    CaptureList list;
    populate_video_capture_list( &list );
    for( int i = 0; i < list.items; i++ )
    {
        ui->comboBoxVideoCaptureDevice->addItem(list.item[i]);
    }

#ifdef _USE_SW_CODECS
    populate_audio_capture_list( &list );
    for( int i = 0; i < list.items; i++ )
    {
        ui->comboBoxAudioCaptureDevice->addItem(list.item[i]);
    }
#else
    ui->comboBoxAudioCaptureDevice->hide();
    ui->labelAudioCaptureDevice->hide();
#endif

    // Transmitter Hardware
    ui->comboBoxTransmitterHWType->addItem(S_EXPRESS_16);
    ui->comboBoxTransmitterHWType->addItem(S_EXPRESS_8);
    ui->comboBoxTransmitterHWType->addItem(S_EXPRESS_TS);
    ui->comboBoxTransmitterHWType->addItem(S_EXPRESS_UDP);


    // DVB Mode
    ui->comboBoxDVBMode->addItem(S_DVB_S);
    ui->comboBoxDVBMode->addItem(S_DVB_S2);
    ui->comboBoxDVBMode->addItem(S_DVB_T);
//    ui->comboBoxDVBMode->addItem(S_DVB_T2);

    // Calibration
    ui->spinBoxDACdcOffsetIChan->setDisabled(true);
    ui->spinBoxDACdcOffsetQChan->setDisabled(true);
    ui->radioButtonDACdc->setEnabled(false);

    NextUpdateDisplayedDVBParams();

}
void MainWindow::NextUpdateDisplayedDVBParams(void)
{
    sys_config cfg;
    dvb_config_get_disk( &cfg );
    int idx = -1;
    QString st;

    // DVB-S
    st.setNum( cfg.sr_mem[0], 10 );
    ui->lineEditSR0->setText(st);
    st.setNum( cfg.sr_mem[1], 10 );
    ui->lineEditSR1->setText(st);
    st.setNum( cfg.sr_mem[2], 10 );
    ui->lineEditSR2->setText(st);
    st.setNum( cfg.sr_mem[3], 10 );
    ui->lineEditSR3->setText(st);
    st.setNum( cfg.sr_mem[4], 10 );
    ui->lineEditSR4->setText(st);
    st.setNum( cfg.sr_mem[5], 10 );
    ui->lineEditSR5->setText(st);
    st.setNum( cfg.sr_mem[6], 10 );
    ui->lineEditSR6->setText(st);
    st.setNum( cfg.sr_mem[7], 10 );
    ui->lineEditSR7->setText(st);
    st.setNum( cfg.sr_mem[8], 10 );
    ui->lineEditSR8->setText(st);
    st.setNum( cfg.sr_mem[9], 10 );
    ui->lineEditSR9->setText(st);
    st.setNum( cfg.sr_mem[10], 10 );
    ui->lineEditSR10->setText(st);
    st.setNum( cfg.sr_mem[11], 10 );
    ui->lineEditSR11->setText(st);

    switch(cfg.sr_mem_nr)
    {
    case 0:
        ui->radioButtonSR0->setChecked(true);
        break;
    case 1:
        ui->radioButtonSR1->setChecked(true);
        break;
    case 2:
        ui->radioButtonSR2->setChecked(true);
        break;
    case 3:
        ui->radioButtonSR3->setChecked(true);
        break;
    case 4:
        ui->radioButtonSR4->setChecked(true);
        break;
    case 5:
        ui->radioButtonSR5->setChecked(true);
        break;
    case 6:
        ui->radioButtonSR6->setChecked(true);
        break;
    case 7:
        ui->radioButtonSR7->setChecked(true);
        break;
    case 8:
        ui->radioButtonSR8->setChecked(true);
        break;
    case 9:
        ui->radioButtonSR9->setChecked(true);
        break;
    case 10:
        ui->radioButtonSR10->setChecked(true);
        break;
    case 11:
        ui->radioButtonSR11->setChecked(true);
        break;
    default:
        break;
    }

    st.setNum( cfg.sr_mem[cfg.sr_mem_nr], 10 );
    ui->labelMainSymbolRate->setText(st);

    if( cfg.dvbs_fmt.fec == FEC_RATE_12 ) idx = ui->comboBoxDVBSFECRate->findText(S_FEC_1_2);
    if( cfg.dvbs_fmt.fec == FEC_RATE_23 ) idx = ui->comboBoxDVBSFECRate->findText(S_FEC_2_3);
    if( cfg.dvbs_fmt.fec == FEC_RATE_34 ) idx = ui->comboBoxDVBSFECRate->findText(S_FEC_3_4);
    if( cfg.dvbs_fmt.fec == FEC_RATE_56 ) idx = ui->comboBoxDVBSFECRate->findText(S_FEC_5_6);
    if( cfg.dvbs_fmt.fec == FEC_RATE_78 ) idx = ui->comboBoxDVBSFECRate->findText(S_FEC_7_8);
    ui->comboBoxDVBSFECRate->setCurrentIndex(idx);

    // DVB-T

    if( cfg.dvbt_fmt.co == M_QPSK )  idx = ui->comboBoxDVBTModulation->findText(S_M_QPSK);
    if( cfg.dvbt_fmt.co == M_QAM16 ) idx = ui->comboBoxDVBTModulation->findText(S_M_16QAM);
    if( cfg.dvbt_fmt.co == M_QAM64 ) idx = ui->comboBoxDVBTModulation->findText(S_M_64QAM);
    ui->comboBoxDVBTModulation->setCurrentIndex(idx);

    if( cfg.dvbt_fmt.gi == GI_14 )  idx = ui->comboBoxDVBTGuardPeriod->findText(S_GP_1_4);
    if( cfg.dvbt_fmt.gi == GI_18 )  idx = ui->comboBoxDVBTGuardPeriod->findText(S_GP_1_8);
    if( cfg.dvbt_fmt.gi == GI_116 ) idx = ui->comboBoxDVBTGuardPeriod->findText(S_GP_1_16);
    if( cfg.dvbt_fmt.gi == GI_132 ) idx = ui->comboBoxDVBTGuardPeriod->findText(S_GP_1_32);
    ui->comboBoxDVBTGuardPeriod->setCurrentIndex(idx);

    if( cfg.dvbt_fmt.fec == CR_12 ) idx = ui->comboBoxDVBTFEC->findText(S_FEC_1_2);
    if( cfg.dvbt_fmt.fec == CR_23 ) idx = ui->comboBoxDVBTFEC->findText(S_FEC_2_3);
    if( cfg.dvbt_fmt.fec == CR_34 ) idx = ui->comboBoxDVBTFEC->findText(S_FEC_3_4);
    if( cfg.dvbt_fmt.fec == CR_56 ) idx = ui->comboBoxDVBTFEC->findText(S_FEC_5_6);
    if( cfg.dvbt_fmt.fec == CR_78 ) idx = ui->comboBoxDVBTFEC->findText(S_FEC_7_8);
    ui->comboBoxDVBTFEC->setCurrentIndex(idx);

    if( cfg.dvbt_fmt.tm == TM_2K ) idx = ui->comboBoxDVBTFFTMode->findText(S_FFT_2K);
    if( cfg.dvbt_fmt.tm == TM_8K ) idx = ui->comboBoxDVBTFFTMode->findText(S_FFT_8K);
    ui->comboBoxDVBTFFTMode->setCurrentIndex(idx);

    if( cfg.dvbt_fmt.chan == CH_8 ) idx = ui->comboBoxDVBTChannel->findText(S_CH_8MHZ);
    if( cfg.dvbt_fmt.chan == CH_7 ) idx = ui->comboBoxDVBTChannel->findText(S_CH_7MHZ);
    if( cfg.dvbt_fmt.chan == CH_6 ) idx = ui->comboBoxDVBTChannel->findText(S_CH_6MHZ);
    if( cfg.dvbt_fmt.chan == CH_4 ) idx = ui->comboBoxDVBTChannel->findText(S_CH_4MHZ);
    if( cfg.dvbt_fmt.chan == CH_3 ) idx = ui->comboBoxDVBTChannel->findText(S_CH_3MHZ);
    if( cfg.dvbt_fmt.chan == CH_2 ) idx = ui->comboBoxDVBTChannel->findText(S_CH_2MHZ);
    ui->comboBoxDVBTChannel->setCurrentIndex(idx);

    // DVB-S2

    if( cfg.dvbs2_fmt.constellation == M_QPSK )   idx = ui->comboBoxDVBS2Modulation->findText(S_M_QPSK);
    if( cfg.dvbs2_fmt.constellation == M_8PSK )   idx = ui->comboBoxDVBS2Modulation->findText(S_M_8PSK);
    if( cfg.dvbs2_fmt.constellation == M_16APSK ) idx = ui->comboBoxDVBS2Modulation->findText(S_M_16APSK);
    if( cfg.dvbs2_fmt.constellation == M_32APSK ) idx = ui->comboBoxDVBS2Modulation->findText(S_M_32APSK);
    ui->comboBoxDVBS2Modulation->setCurrentIndex(idx);

    if( cfg.dvbs2_fmt.code_rate == CR_1_4 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_1_4);
    if( cfg.dvbs2_fmt.code_rate == CR_1_3 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_1_3);
    if( cfg.dvbs2_fmt.code_rate == CR_2_5 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_2_5);
    if( cfg.dvbs2_fmt.code_rate == CR_1_2 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_1_2);
    if( cfg.dvbs2_fmt.code_rate == CR_3_5 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_3_5);
    if( cfg.dvbs2_fmt.code_rate == CR_2_3 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_2_3);
    if( cfg.dvbs2_fmt.code_rate == CR_3_4 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_3_4);
    if( cfg.dvbs2_fmt.code_rate == CR_4_5 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_4_5);
    if( cfg.dvbs2_fmt.code_rate == CR_5_6 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_5_6);
    if( cfg.dvbs2_fmt.code_rate == CR_8_9 )  idx = ui->comboBoxDVBS2FEC->findText(S_FEC_8_9);
    if( cfg.dvbs2_fmt.code_rate == CR_9_10 ) idx = ui->comboBoxDVBS2FEC->findText(S_FEC_9_10);
    ui->comboBoxDVBS2FEC->setCurrentIndex(idx);

    if( cfg.dvbs2_fmt.roll_off == RO_0_35 ) idx = ui->comboBoxDVBS2RollOff->findText(S_RO_0_35);
    if( cfg.dvbs2_fmt.roll_off == RO_0_25 ) idx = ui->comboBoxDVBS2RollOff->findText(S_RO_0_25);
    if( cfg.dvbs2_fmt.roll_off == RO_0_20 ) idx = ui->comboBoxDVBS2RollOff->findText(S_RO_0_20);
    ui->comboBoxDVBS2RollOff->setCurrentIndex(idx);

    if( cfg.dvbs2_fmt.frame_type == FRAME_NORMAL )
        idx = ui->comboBoxDVBS2FrameType->findText(S_FM_NORMAL);
    else
        idx = ui->comboBoxDVBS2FrameType->findText(S_FM_SHORT);
    ui->comboBoxDVBS2FrameType->setCurrentIndex(idx);

    if( cfg.dvbs2_fmt.null_deletion != 0)
        idx = ui->comboBoxDVBS2NullPacketDeletion->findText(S_YES);
    else
        idx = ui->comboBoxDVBS2NullPacketDeletion->findText(S_NO);
    ui->comboBoxDVBS2NullPacketDeletion->setCurrentIndex(idx);

    if( cfg.dvbs2_fmt.dummy_frame != 0 )
        idx = ui->comboBoxDVBS2DummyFrameInsertion->findText(S_YES);
    else
        idx = ui->comboBoxDVBS2DummyFrameInsertion->findText(S_NO);
    ui->comboBoxDVBS2DummyFrameInsertion->setCurrentIndex(idx);

    if( cfg.dvbs2_fmt.pilots != 0 )
        idx = ui->comboBoxDVBS2PilotInsertion->findText(S_YES);
    else
        idx = ui->comboBoxDVBS2PilotInsertion->findText(S_NO);
    ui->comboBoxDVBS2PilotInsertion->setCurrentIndex(idx);

    // Transmitter

    st.setNum( cfg.tx_frequency,'f',0 );
    ui->lineEditFrequency->setText(st);
    ui->labelMainFrequency->setText(st);

    st.setNum( cfg.tx_level, 'f' ,0 );
    ui->lineEditPower->setText(st);
    ui->labelMailLevel->setText(st);

    // TP info
    st.setNum(cfg.pmt_pid);
    ui->lineEditPmtPID->setText(st);
    st.setNum(cfg.audio_pid);
    ui->lineEditAudioPID->setText(st);
    st.setNum(cfg.video_pid);
    ui->lineEditVideoPID->setText(st);
    st.setNum(cfg.pcr_pid);
    ui->lineEditPCRPID->setText(st);

    // Program info
    ui->lineEditServiceProvider->setText(cfg.service_provider_name);
    ui->lineEditServiceName->setText(cfg.service_name);

    // EPG Event Info
    st.setNum(cfg.event.event_duration);
    ui->lineEditEPGDuration->setText(st);
    ui->plainTextEditEPGTitle->setPlainText(cfg.event.event_title);
    ui->plainTextEditEPGText->setPlainText(cfg.event.event_text);

    // Video Capture device
    int n = ui->comboBoxVideoCaptureDevice->findText(cfg.capture_device_name);
    if( n >= 0 ) ui->comboBoxVideoCaptureDevice->setCurrentIndex(n);
    ui->spinBoxSelectInput->setValue(cfg.capture_device_input);

    ui->comboBoxTransmitterHWType->setCurrentIndex(cfg.tx_hardware);

    // Video Bitrate
    st.sprintf("%.2f Mbit/s",cfg.video_bitrate/1000000.0);
    ui->labelMainVideoBitrate->setText(st);

    // DVB Mode

    if(cfg.dvb_mode == MODE_DVBS )  ui->comboBoxDVBMode->setCurrentIndex(0);
    if(cfg.dvb_mode == MODE_DVBS2 ) ui->comboBoxDVBMode->setCurrentIndex(1);
    if(cfg.dvb_mode == MODE_DVBT )  ui->comboBoxDVBMode->setCurrentIndex(2);
    if(cfg.dvb_mode == MODE_DVBT2 ) ui->comboBoxDVBMode->setCurrentIndex(3);

    if(cfg.dvb_mode == MODE_DVBS )
    {
        QString mode = "DVB-S ";

        mode += "FEC   ";
        if(cfg.dvbs_fmt.fec == FEC_RATE_12) mode += S_FEC_1_2;
        if(cfg.dvbs_fmt.fec == FEC_RATE_23) mode += S_FEC_2_3;
        if(cfg.dvbs_fmt.fec == FEC_RATE_34) mode += S_FEC_3_4;
        if(cfg.dvbs_fmt.fec == FEC_RATE_56) mode += S_FEC_5_6;
        if(cfg.dvbs_fmt.fec == FEC_RATE_78) mode += S_FEC_7_8;

        ui->labelMainMode->setText(mode);
    }
    if(cfg.dvb_mode == MODE_DVBS2 )
    {
        QString mode;
        mode = "DVB-S2 ";
        if(cfg.dvbs2_fmt.constellation == M_QPSK   ) mode += S_M_QPSK;
        if(cfg.dvbs2_fmt.constellation == M_8PSK   ) mode += S_M_8PSK;
        if(cfg.dvbs2_fmt.constellation == M_16APSK ) mode += S_M_16APSK;
        if(cfg.dvbs2_fmt.constellation == M_32APSK ) mode += S_M_32APSK;
        mode += " FEC   ";

        if(cfg.dvbs2_fmt.code_rate == CR_1_4 ) mode += S_FEC_1_4;
        if(cfg.dvbs2_fmt.code_rate == CR_1_3 ) mode += S_FEC_1_3;
        if(cfg.dvbs2_fmt.code_rate == CR_2_5 ) mode += S_FEC_2_5;
        if(cfg.dvbs2_fmt.code_rate == CR_1_2 ) mode += S_FEC_1_2;
        if(cfg.dvbs2_fmt.code_rate == CR_3_5 ) mode += S_FEC_3_5;
        if(cfg.dvbs2_fmt.code_rate == CR_2_3 ) mode += S_FEC_2_3;
        if(cfg.dvbs2_fmt.code_rate == CR_3_4 ) mode += S_FEC_3_4;
        if(cfg.dvbs2_fmt.code_rate == CR_4_5 ) mode += S_FEC_4_5;
        if(cfg.dvbs2_fmt.code_rate == CR_5_6 ) mode += S_FEC_5_6;
        if(cfg.dvbs2_fmt.code_rate == CR_8_9 ) mode += S_FEC_8_9;
        if(cfg.dvbs2_fmt.code_rate == CR_9_10 ) mode += S_FEC_9_10;
        ui->labelMainMode->setText(mode);
    }
    if(cfg.dvb_mode == MODE_DVBT )
    {
        QString mode;
        mode = "DVB-T ";
        if(cfg.dvbt_fmt.chan == CH_8) mode += S_CH_8MHZ;
        if(cfg.dvbt_fmt.chan == CH_7) mode += S_CH_7MHZ;
        if(cfg.dvbt_fmt.chan == CH_6) mode += S_CH_6MHZ;
        if(cfg.dvbt_fmt.chan == CH_4) mode += S_CH_4MHZ;
        if(cfg.dvbt_fmt.chan == CH_3) mode += S_CH_3MHZ;
        if(cfg.dvbt_fmt.chan == CH_2) mode += S_CH_2MHZ;

        mode += " ";

        if(cfg.dvbt_fmt.tm == TM_2K ) mode += S_FFT_2K;
        if(cfg.dvbt_fmt.tm == TM_8K ) mode += S_FFT_8K;

        mode += " ";

        if(cfg.dvbt_fmt.co == CO_QPSK )  mode += S_M_QPSK;
        if(cfg.dvbt_fmt.co == CO_16QAM ) mode += S_M_16QAM;
        if(cfg.dvbt_fmt.co == CO_64QAM ) mode += S_M_64QAM;

        mode += " FEC = ";
        if(cfg.dvbt_fmt.fec == FEC_RATE_12) mode += S_FEC_1_2;
        if(cfg.dvbt_fmt.fec == FEC_RATE_23) mode += S_FEC_2_3;
        if(cfg.dvbt_fmt.fec == FEC_RATE_34) mode += S_FEC_3_4;
        if(cfg.dvbt_fmt.fec == FEC_RATE_56) mode += S_FEC_5_6;
        if(cfg.dvbt_fmt.fec == FEC_RATE_78) mode += S_FEC_7_8;

        ui->labelMainMode->setText(mode);
        // The symbol rate is no longer valid
        ui->labelMainSymbolRate->setText("");
    }
    if(cfg.dvb_mode == MODE_DVBT2 ) ui->labelMainMode->setText("DVB-T2");

    // IP server
    ui->lineEditServerIpAddress->setText(cfg.server_ip_address);
    st.sprintf("%d",cfg.server_socket_number);
    ui->lineEditServerSocketNumber->setText(st);

    // Calibration routine
    ui->spinBoxDACdcOffsetIChan->setValue( cfg.i_chan_dc_offset );
    ui->spinBoxDACdcOffsetQChan->setValue( cfg.q_chan_dc_offset );
}
void MainWindow::SettingsUpdateMessageBox(void)
{
    QMessageBox::information(this, tr("Settings Changed"),
                 tr("Any new settings will only come into effect\n"
                 "after you have restarted the program"),
                 QMessageBox::Ok);
}
MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}
void MainWindow::onTimerUpdate()
{
    ui->progressBarTxQueue->setValue(final_tx_queue_percentage_unprotected());
    QString st;
    //Null count
    st.setNum(flow_read_null_count(),10);
    ui->labelNullCount->setText(st);
    // Transmit delay
    st.sprintf("%.2f Secs",final_txq_time_delay());
    ui->labelDelay->setText(st);
    // Video Bitrate
    sys_config cfg;
    dvb_get_config( &cfg );
    st.sprintf("%.2f Mbit/s",cfg.video_bitrate/1000000.0);
    ui->labelMainVideoBitrate->setText(st);
    if((cfg.dvb_mode == MODE_DVBT)||(cfg.dvb_mode == MODE_DVBT2))
    {
        st.sprintf("%.2f Symbols/s",dvb_t_get_symbol_rate());
        ui->labelMainSymbolRate->setText(st);
    }
    else
    {
        st.sprintf("%.2f MSymbol/s",cfg.sr_mem[cfg.sr_mem_nr]/1000000.0);
        ui->labelMainSymbolRate->setText(st);
    }
    st.sprintf("%.2f Kbit/s",192000/1000.0);
    ui->labelMainAudioBitrate->setText(st);
    st.sprintf("%.2f MHz",cfg.tx_frequency/1000000.0);
    ui->labelMainFrequency->setText(st);


    ui->checkBoxVideoStream->setChecked(cap_check_video_present());
    ui->checkBoxAudioStream->setChecked(cap_check_audio_present());

    if(logger_updated())
    {
        m_log = logger_get_text();
        logger_released();
        ui->label_log_display->setText(m_log);
    }
}

void MainWindow::on_pushButtonApplyTx_clicked()
{
    QString st;
    st = ui->lineEditPower->text();
    dvb_set_tx_lvl( st.toInt() );
    st = ui->lineEditFrequency->text();
    dvb_set_tx_freq( st.toDouble() );

}

void MainWindow::on_pushButtonApplyService_clicked()
{
    QString st;
    st = ui->lineEditServiceProvider->text();
    dvb_set_service_provider_name( st.toLatin1() );
    st = ui->lineEditServiceName->text();
    dvb_set_service_name( st.toLatin1() );
}

void MainWindow::on_pushButtonApplyVideoCapture_clicked()
{
    QString st;

    st = ui->comboBoxVideoCaptureDevice->currentText();
    dvb_set_video_capture_device( st.toLatin1() );

    int input = ui->spinBoxSelectInput->value();
    dvb_set_video_capture_device_input( input );

    QString str = ui->comboBoxTransmitterHWType->currentText();

    // Save the transmitter hardware type
    if( str == S_EXPRESS_16 )
        dvb_set_tx_hardware_type( HW_EXPRESS_16, S_EXPRESS_16 );
    if( str == S_EXPRESS_8 )
        dvb_set_tx_hardware_type( HW_EXPRESS_8, S_EXPRESS_8 );
    if( str == S_EXPRESS_TS )
        dvb_set_tx_hardware_type( HW_EXPRESS_TS, S_EXPRESS_TS );
    if( str == S_EXPRESS_UDP )
        dvb_set_tx_hardware_type( HW_EXPRESS_UDP, S_EXPRESS_UDP );

    // Now get the IP address and socket number for the UDP server
    str = ui->lineEditServerIpAddress->text();
    dvb_set_server_ip_address( str.toLatin1());
    str = ui->lineEditServerSocketNumber->text();
    dvb_set_server_socket_number( str.toLatin1());
    SettingsUpdateMessageBox();
    NextUpdateDisplayedDVBParams();
}

void MainWindow::on_pushButtonApplyDVBT_clicked()
{
    sys_config cfg;
    dvb_get_config( &cfg );
    QString str;

    // New DVB-T parameters have been requested
    str = ui->comboBoxDVBTModulation->currentText();

    if( str == S_M_QPSK  ) cfg.dvbt_fmt.co = M_QPSK;
    if( str == S_M_16QAM ) cfg.dvbt_fmt.co = M_QAM16;
    if( str == S_M_64QAM ) cfg.dvbt_fmt.co = M_QAM64;

    cfg.dvbt_fmt.sf = SF_NH; // Only mode supported

    str = ui->comboBoxDVBTGuardPeriod->currentText();

    if( str == S_GP_1_4  ) cfg.dvbt_fmt.gi = GI_14;
    if( str == S_GP_1_8  ) cfg.dvbt_fmt.gi = GI_18;
    if( str == S_GP_1_16 ) cfg.dvbt_fmt.gi = GI_116;
    if( str == S_GP_1_32 ) cfg.dvbt_fmt.gi = GI_132;

    str = ui->comboBoxDVBTFEC->currentText();
    if( str == S_FEC_1_2 ) cfg.dvbt_fmt.fec = CR_12;
    if( str == S_FEC_2_3 ) cfg.dvbt_fmt.fec = CR_23;
    if( str == S_FEC_3_4 ) cfg.dvbt_fmt.fec = CR_34;
    if( str == S_FEC_5_6 ) cfg.dvbt_fmt.fec = CR_56;
    if( str == S_FEC_7_8 ) cfg.dvbt_fmt.fec = CR_78;

    str = ui->comboBoxDVBTFFTMode->currentText();
    if( str == S_FFT_2K ) cfg.dvbt_fmt.tm = TM_2K;
    if( str == S_FFT_8K ) cfg.dvbt_fmt.tm = TM_8K;

    str = ui->comboBoxDVBTChannel->currentText();
    if( str == S_CH_8MHZ ) cfg.dvbt_fmt.chan = CH_8;
    if( str == S_CH_7MHZ ) cfg.dvbt_fmt.chan = CH_7;
    if( str == S_CH_6MHZ ) cfg.dvbt_fmt.chan = CH_6;
    if( str == S_CH_4MHZ ) cfg.dvbt_fmt.chan = CH_4;
    if( str == S_CH_3MHZ ) cfg.dvbt_fmt.chan = CH_3;
    if( str == S_CH_2MHZ ) cfg.dvbt_fmt.chan = CH_2;

    dvb_set_dvbt_params( &cfg );
    SettingsUpdateMessageBox();
    NextUpdateDisplayedDVBParams();
}

void MainWindow::on_pushButtonApplyDVBS_clicked()
{
    QString str;
    int res = 0;
    // New DVB-S parameters have been selected

    str = ui->comboBoxDVBSFECRate->currentText();

    if( str == S_FEC_1_2 ) res += dvb_set_dvbs_fec_rate( FEC_RATE_12 );
    if( str == S_FEC_2_3 ) res += dvb_set_dvbs_fec_rate( FEC_RATE_23 );
    if( str == S_FEC_3_4 ) res += dvb_set_dvbs_fec_rate( FEC_RATE_34 );
    if( str == S_FEC_5_6 ) res += dvb_set_dvbs_fec_rate( FEC_RATE_56 );
    if( str == S_FEC_7_8 ) res += dvb_set_dvbs_fec_rate( FEC_RATE_78 );


    SettingsUpdateMessageBox();
    NextUpdateDisplayedDVBParams();
}

void MainWindow::on_pushButtonApplyDVBMode_clicked()
{
    if( ui->comboBoxDVBMode->currentText() == S_DVB_S)
        dvb_set_dvb_mode(MODE_DVBS);
    else
        if( ui->comboBoxDVBMode->currentText() == S_DVB_S2)
            dvb_set_dvb_mode(MODE_DVBS2);
        else
            if( ui->comboBoxDVBMode->currentText() == S_DVB_T)
                dvb_set_dvb_mode(MODE_DVBT);
            else
                if( ui->comboBoxDVBMode->currentText() == S_DVB_T2)
                    dvb_set_dvb_mode(MODE_DVBT2);

    SettingsUpdateMessageBox();
    NextUpdateDisplayedDVBParams();
}

void MainWindow::on_tabWidget_currentChanged(int index)
{
    index = index;
    NextUpdateDisplayedDVBParams();
}

void MainWindow::on_pushButtonApplyTPInfo_clicked()
{
    QString st;
    st = ui->lineEditPmtPID->text();
    dvb_set_PmtPid( st.toInt() );
    st = ui->lineEditVideoPID->text();
    dvb_set_VideoPid( st.toInt() );
    st = ui->lineEditAudioPID->text();
    dvb_set_AudioPid( st.toInt() );
    st = ui->lineEditPCRPID->text();
    dvb_set_PcrPid( st.toInt() );
}

void MainWindow::on_pushButtonApplyEPG_clicked()
{
    QString st;
    st = ui->lineEditEPGDuration->text();
    dvb_set_epg_event_duration( st.toInt() );
    st = ui->plainTextEditEPGTitle->toPlainText();
    dvb_set_epg_event_title( st.toLatin1() );
    st = ui->plainTextEditEPGText->toPlainText();
    dvb_set_epg_event_text( st.toLatin1() );
}

//
// Called when the DVB-S2 Apply button id clicked
// This is not an elegant way of reading the GUI but I am lazy
// so this is how it is done.
//
void MainWindow::on_pushButtonApplyDVBS2_clicked()
{
    DVB2FrameFormat f;
    QString str;

    str = ui->comboBoxDVBS2Modulation->currentText();
    if( str == S_M_QPSK )   f.constellation = M_QPSK;
    if( str == S_M_8PSK )   f.constellation = M_8PSK;
    if( str == S_M_16APSK ) f.constellation = M_16APSK;
    if( str == S_M_32APSK ) f.constellation = M_32APSK;

    str = ui->comboBoxDVBS2FEC->currentText();
    if( str == S_FEC_1_4 )  f.code_rate = CR_1_4;
    if( str == S_FEC_1_3 )  f.code_rate = CR_1_3;
    if( str == S_FEC_2_5 )  f.code_rate = CR_2_5;
    if( str == S_FEC_1_2 )  f.code_rate = CR_1_2;
    if( str == S_FEC_3_5 )  f.code_rate = CR_3_5;
    if( str == S_FEC_2_3 )  f.code_rate = CR_2_3;
    if( str == S_FEC_3_4 )  f.code_rate = CR_3_4;
    if( str == S_FEC_4_5 )  f.code_rate = CR_4_5;
    if( str == S_FEC_5_6 )  f.code_rate = CR_5_6;
    if( str == S_FEC_8_9 )  f.code_rate = CR_8_9;
    if( str == S_FEC_9_10 ) f.code_rate = CR_9_10;

    str = ui->comboBoxDVBS2RollOff->currentText();
    if( str == S_RO_0_35 ) f.roll_off = RO_0_35;
    if( str == S_RO_0_25 ) f.roll_off = RO_0_25;
    if( str == S_RO_0_20 ) f.roll_off = RO_0_20;

    str = ui->comboBoxDVBS2FrameType->currentText();
    if( str == S_FM_NORMAL ) f.frame_type = FRAME_NORMAL;
    if( str == S_FM_SHORT  ) f.frame_type = FRAME_SHORT;

    str = ui->comboBoxDVBS2NullPacketDeletion->currentText();
    if( str == S_YES ) f.null_deletion = 1;
    if( str == S_NO )  f.null_deletion = 0;

    str = ui->comboBoxDVBS2DummyFrameInsertion->currentText();
    if( str == S_YES ) f.dummy_frame = 1;
    if( str == S_NO )  f.dummy_frame = 0;

    str = ui->comboBoxDVBS2PilotInsertion->currentText();
    if( str == S_YES ) f.pilots = 1;
    if( str == S_NO  ) f.pilots = 0;

    // We only handle Broadcast format anyway!
    f.broadcasting = 1;

    if( dvb_set_s2_configuration( &f ) >= 0 )
    {
        SettingsUpdateMessageBox();
        NextUpdateDisplayedDVBParams();
    }
    else
    {
        QMessageBox::warning(this, tr("DVB-S2 Settings Error"),
                     tr("Invalid combination of settings selected\n"),
                     QMessageBox::Ok);
    }
}

void MainWindow::on_pushButtonPTT_clicked()
{
    if( dvb_get_major_txrx_status() == DVB_TRANSMITTING )
    {
        dvb_set_major_txrx_status( DVB_RECEIVING );
        ui->label_38_TX_STATUS->setText("Receiving");
    }
    else
    {
        dvb_set_major_txrx_status( DVB_TRANSMITTING );
        ui->label_38_TX_STATUS->setText("Transmitting");
    }
}

void MainWindow::on_checkBoxCarrier_clicked()
{
    if( ui->checkBoxCarrier->isChecked())
    {
        hw_set_carrier( 1 );
    }
    else
    {
        hw_set_carrier( 0 );
    }

}
//
// Hardware Calibration routines
//
void MainWindow::on_checkBoxCalibrationEnable_clicked( bool checked)
{
    if( checked )
    {
        dvb_calibration( true, DVB_CALIBRATING_OFFSET );
        ui->radioButtonDACdc->setChecked(true);
        ui->spinBoxDACdcOffsetIChan->setEnabled(true);
        ui->spinBoxDACdcOffsetQChan->setEnabled(true);
        ui->radioButtonDACdc->setEnabled(true);
    }
    else
    {
        dvb_calibration(false, DVB_CALIBRATING_OFFSET );
        ui->radioButtonDACdc->setChecked(false);
        ui->spinBoxDACdcOffsetIChan->setEnabled(false);
        ui->spinBoxDACdcOffsetQChan->setEnabled(false);
        ui->radioButtonDACdc->setEnabled(false);
    }
}

void MainWindow::on_spinBoxDACdcOffsetIChan_valueChanged(int val)
{
    dvb_i_chan_dac_offset_changed( val );
}

void MainWindow::on_spinBoxDACdcOffsetQChan_valueChanged(int val)
{
    dvb_q_chan_dac_offset_changed( val );
}

void MainWindow::on_radioButtonDACdc_clicked(bool checked)
{
    if( ui->checkBoxCalibrationEnable->isChecked())
    {
        if(checked)
        {
            dvb_dac_offset_selected();
            ui->spinBoxDACdcOffsetIChan->setEnabled(true);
            ui->spinBoxDACdcOffsetQChan->setEnabled(true);
        }
    }
}

void MainWindow::on_checkBoxLogging_clicked(bool checked)
{
    if(checked == true)
        tp_file_logger_start();
    else
        tp_file_logger_stop();
}
//
// Display next line of log output
//
void MainWindow::on_pushButtonLogText_clicked()
{
    increment_display_log_index();
//    express_insert_bytes(255);//For testing
}

void MainWindow::on_pushButtonApplySR_clicked()
{
    QString str;
    int res = 0;

    // Symbol rate
    if( ui->radioButtonSR0->isChecked() == true )
    {
        str = ui->lineEditSR0->text();
        res += dvb_set_dvbs_symbol_rate( 0, str.toInt() );
    }
    if( ui->radioButtonSR1->isChecked() == true )
    {
        str = ui->lineEditSR1->text();
        res += dvb_set_dvbs_symbol_rate( 1, str.toInt() );
    }
    if( ui->radioButtonSR2->isChecked() == true )
    {
        str = ui->lineEditSR2->text();
        res += dvb_set_dvbs_symbol_rate( 2, str.toInt() );
    }
    if( ui->radioButtonSR3->isChecked() == true )
    {
        str = ui->lineEditSR3->text();
        res += dvb_set_dvbs_symbol_rate( 3, str.toInt() );
    }
    if( ui->radioButtonSR4->isChecked() == true )
    {
        str = ui->lineEditSR4->text();
        res += dvb_set_dvbs_symbol_rate( 4, str.toInt() );
    }
    if( ui->radioButtonSR5->isChecked() == true )
    {
        str = ui->lineEditSR5->text();
        res += dvb_set_dvbs_symbol_rate( 5, str.toInt() );
    }
    if( ui->radioButtonSR6->isChecked() == true )
    {
        str = ui->lineEditSR6->text();
        res += dvb_set_dvbs_symbol_rate( 6, str.toInt() );
    }
    if( ui->radioButtonSR7->isChecked() == true )
    {
        str = ui->lineEditSR7->text();
        res += dvb_set_dvbs_symbol_rate( 7, str.toInt() );
    }
    if( ui->radioButtonSR8->isChecked() == true )
    {
        str = ui->lineEditSR8->text();
        res += dvb_set_dvbs_symbol_rate( 8, str.toInt() );
    }
    if( ui->radioButtonSR9->isChecked() == true )
    {
        str = ui->lineEditSR9->text();
        res += dvb_set_dvbs_symbol_rate( 9, str.toInt() );
    }
    if( ui->radioButtonSR10->isChecked() == true )
    {
        str = ui->lineEditSR10->text();
        res += dvb_set_dvbs_symbol_rate( 10, str.toInt() );
    }
    if( ui->radioButtonSR11->isChecked() == true )
    {
        str = ui->lineEditSR11->text();
        res += dvb_set_dvbs_symbol_rate( 11, str.toInt() );
    }
    SettingsUpdateMessageBox();
    NextUpdateDisplayedDVBParams();
}
