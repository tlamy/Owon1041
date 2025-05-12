#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>

#include "ConnectDialog.h"
#include "Settings.h"

class MainWindow final : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    void setupUi(QMainWindow *MainWindow);

    Settings *settings;

protected:
    void resizeEvent(QResizeEvent *event) override;
    void setupPositions(int width, int height) const;
private slots:
    void onVoltage50V();
    void onVoltageAuto();
    void onShort();
    void onDiode();
    void onResistance50K();
    void onResistanceAuto();
    void onCapacitance50uF();
    void onCapacitanceAuto();
    void onFrequency();
    void onPeriod();

    bool eventFilter(QObject *obj, QEvent *event) override;

    void onMeasurementClicked();
    void updateMeasurement();  // Make sure this exists and is declared as a slot


private:
    // UI elements as member variables (excluding centralwidget)
    QLabel *measurement;
    QPushButton *btn_50_v;
    QPushButton *btn_auto_v;
    QPushButton *btn_short;
    QPushButton *btn_diode;
    QPushButton *btn_50_kr;
    QPushButton *btn_auto_r;
    QPushButton *btn_50_f;
    QPushButton *btn_auto_f;
    QPushButton *btn_freq;
    QPushButton *btn_period;

    ConnectDialog *m_connect_dialog;

    void connectSerial();

    void onConnect();

    bool openConnectDialog();


    QTimer * m_timer = nullptr;
    QSerialPort * m_port = nullptr;
};

#endif // MAINWINDOW_H
