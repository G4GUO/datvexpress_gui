#-------------------------------------------------
#
# Project created by QtCreator 2013-03-20T12:04:31
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = DATV-Express
TEMPLATE = app

HEADERS  += mainwindow.h \
    DVB-C/dvb_c.h \
    Sources/an_capture.h \
    Sources/si570.h \
    Sources/dvb_buffer.h \
    Sources/mp_desc.h \
    Sources/mp_config.h \
    DVB-T/dvb_t_sym.h \
    capturedialog.h \
    Sources/dvb_options.h

FORMS    += mainwindow.ui \
            Capturedialog.ui

unix{
    CONFIG    += link_pkgconfig
    PKGCONFIG += libusb-1.0
    PKGCONFIG += libavutil
}

LIBS += -lfftw -lpthread
LIBS += -lavdevice -lavresample -lavformat -lavcodec -lavutil -lswscale -lmp3lame -lavfilter -lz -lrt -lbz2
LIBS += -lasound

INCLUDEPATH += Sources

SOURCES += main.cpp \
    mainwindow.cpp \
    Sources/dvb_gui_if.cpp \
    Sources/dvb.cpp \
    Sources/dvb_capture_ctl.cpp \
    Sources/dvb_capture.cpp \
    Sources/dvb_gen.cpp \
    Sources/mp_pat.cpp \
    Sources/mp_pmt.cpp \
    Sources/mp_eit.cpp \
    Sources/mp_nit.cpp \
    Sources/mp_sdt.cpp \
    Sources/mp_tdt.cpp \
    Sources/mp_desc.cpp \
    Sources/mp_f_tp_desc.cpp \
    Sources/dvb_crc32.cpp \
    Sources/mp_si_desc.cpp \
    Sources/mp_f_tp.cpp \
    Sources/dvb_encode.cpp \
    Sources/dvb_tx_frame.cpp \
    Sources/dvb_conv.cpp \
    Sources/dvb_timer.cpp \
    Sources/dvb_config.cpp \
    Sources/dvb_time.cpp \
    Sources/dvb_teletext.cpp \
    Sources/dvb_ts_if.cpp \
    DVB-S2/dvb2_bbheader.cpp \
    DVB-S2/DVBS2.cpp \
    DVB-S2/dvbs2_tables.cpp \
    DVB-S2/dvbs2_scrambler.cpp \
    DVB-S2/dvbs2_physical.cpp \
    DVB-S2/dvbs2_modulator.cpp \
    DVB-S2/dvbs2_interleave.cpp \
    DVB-S2/DVB2.cpp \
    DVB-S2/dvb2_scrambler.cpp \
    DVB-S2/dvb2_ldpc_tables.cpp \
    DVB-S2/dvb2_ldpc_encode.cpp \
    DVB-S2/dvb2_bch.cpp \
    Sources/dvb_s2_if.cpp \
    Sources/dvb_final_tx_queue.cpp \
    Sources/equipment_control.cpp \
    Sources/mp_elem.cpp \
    Sources/express.cpp \
    Sources/tx_hardware.cpp \
    DVB-T/dvb_t_tp.cpp \
    DVB-T/dvb_t_sym.cpp \
    DVB-T/dvb_t_stab.cpp \
    DVB-T/dvb_t_qam_tab.cpp \
    DVB-T/dvb_t_mod.cpp \
    DVB-T/dvb_t_linux_fft.cpp \
    DVB-T/dvb_t_i.cpp \
    DVB-T/dvb_t_enc.cpp \
    DVB-T/dvb_t_bits.cpp \
    DVB-T/dvb_t.cpp \
    DVB-S/dvbs_receive.cpp \
    DVB-S/dvbs_modulator.cpp \
    DVB-S/dvb_rs_encoder.cpp \
    DVB-S/dvb_interleave.cpp \
    Sources/dvb_log.cpp \
    Sources/dvb_udp.cpp \
    DVB-C/dvb_c_tables.cpp \
    Sources/mp_pcr.cpp \
    Sources/tp_logger.cpp \
    Sources/mp_scr_parse.cpp \
    Sources/dvb_tx_transport_flow.cpp \
    Sources/dvb_pes_process.cpp \
    DVB-C/dvb_c_modulate.cpp \
    Sources/an_capture.cpp \
    Sources/mp_null.cpp \
    Sources/si570.cpp \
    DVB-T/dvb_t_lpf.cpp \
    Sources/dvb_buffer.cpp \
    capturedialog.cpp
HEADERS += mainwindow.h \
    Sources/dvb_gui_if.h \
    Sources/dvb_config.h \
    Sources/dvb_capture_ctl.h \
    Sources/dvb_gen.h \
    Sources/dvb.h \
    Sources/mp_ts_ids.h \
    Sources/mp_tp.h \
    Sources/dvb_gen.h \
    Sources/dvb_uhd.h \
    Sources/dvb_types.h \
    Sources/dvb_si.h \
    Sources/mp_si_desc.h \
    DVB-S2/DVB2.h \
    DVB-S2/DVBS2.h \
    Sources/dvb_s2_if.h \
    Sources/dvb_s2_if.h \
    Sources/express.h \
    Sources/tx_hardware.h \
    DVB-T/dvb_t.h
OTHER_FILES  +=  dvb_t_sym.h
