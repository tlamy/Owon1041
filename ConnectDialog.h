#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSerialPort>
#include <QStringList>
#include <QString>

class ConnectDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ConnectDialog(QWidget *parent = nullptr);

    ~ConnectDialog() override;

    // Get the selected serial port
    QString getSelectedPort() const;

    // Get configured serial port (with settings applied)
    QSerialPort *getConfiguredSerialPort() const;

    bool tryPortByName(const QString &portName);

private slots:
    void refreshPorts() const;
    void connectToPort();
    void tryPort(); // Add this method declaration

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
    QSerialPort *serialPort = nullptr;
    const QString *status;
    bool isError = false;

    // Methods
    void setupUi();

    void populatePortsList() const;

    bool configureSerialPort(const QString &device);
    void tryConfiguredPort();

private:
    bool m_check_ok = false;
};

#endif // CONNECTDIALOG_H
