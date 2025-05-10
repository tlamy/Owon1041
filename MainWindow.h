#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    void setupUi(QMainWindow *MainWindow);
protected:
    void resizeEvent(QResizeEvent *event) override;
    void setupPositions(int width, int height);
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

    void setupConnections();
};

#endif // MAINWINDOW_H