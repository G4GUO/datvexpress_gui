#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

protected:
    void changeEvent(QEvent *e);

private:
    Ui::MainWindow *ui;
    QTimer *m_tx_timer;
    QString m_log;
    QAction *captureAction;
    QAction *aboutAction;
    QAction *exitAction;
    QMenu *fileMenu;
    QMenu *helpMenu;
    void InitialUpdateDisplayedDVBParams(void);
    void SettingsUpdateMessageBox(void);
    void createActions();
    void createMenus();
public:
    void NextUpdateDisplayedDVBParams(void);

private slots:
    void about();
    void capture_config();
    void on_checkBoxCarrier_clicked();
    void on_pushButtonPTT_clicked();
    void on_pushButtonApplyDVBS2_clicked();
    void on_pushButtonApplyEPG_clicked();
    void on_pushButtonApplyTPInfo_clicked();
    void on_tabWidget_currentChanged(int index);
    void on_pushButtonApplyDVBMode_clicked();
    void on_pushButtonApplyDVBS_clicked();
    void on_pushButtonApplyDVBT_clicked();
    void onTimerUpdate();
    void on_pushButtonApplyVideoCapture_clicked();
    void on_pushButtonApplyService_clicked();
    void on_pushButtonApplyTx_clicked();
    void on_checkBoxCalibrationEnable_clicked( bool checked );
    void on_spinBoxDACdcOffsetIChan_valueChanged(int arg1);
    void on_spinBoxDACdcOffsetQChan_valueChanged(int arg1);
    void on_radioButtonDACdc_clicked(bool checked);
    void on_checkBoxLogging_clicked(bool checked);
    void on_pushButtonLogText_clicked();
    void on_pushButtonApplySR_clicked();
};

#endif // MAINWINDOW_H
