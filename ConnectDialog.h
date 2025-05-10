#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSerialPort>
#include <QStringList>
#include <QString>

class ConnectDialog : public QDialog {
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = nullptr);

    ~ConnectDialog();

    // Get the selected serial port
    QString getSelectedPort() const;

    // Get configured serial port (with settings applied)
    QSerialPort *getConfiguredSerialPort();

private slots:
    void refreshPorts();

    void connectToPort();

private:
    // UI Elements
    QComboBox *portComboBox;
    QComboBox *baudRateComboBox;
    QComboBox *dataBitsComboBox;
    QComboBox *parityComboBox;
    QComboBox *stopBitsComboBox;
    QComboBox *flowControlComboBox;
    QPushButton *refreshButton;
    QPushButton *connectButton;
    QPushButton *cancelButton;
    QLabel *statusLabel;

    // Active serial port (set when connecting successfully)
    QSerialPort *serialPort;

    // Methods
    void setupUi();

    void populatePortsList();

    bool configureSerialPort();

    QString tryPort();
};

#endif // CONNECTDIALOG_H
