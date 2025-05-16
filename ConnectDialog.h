#ifndef CONNECTDIALOG_H
#define CONNECTDIALOG_H

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <QSerialPort>
#include <QString>
#include <QRadioButton>
#include <QGroupBox>
#include <QCheckBox>

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

    void tryPort();

    void onRateChanged(int id); // Slot to handle rate changes

private:
    // UI Elements
    QComboBox *portComboBox;
    QCheckBox *m_beep_short;
    QLineEdit *m_short_threshold;
    QCheckBox *m_beep_diode;

    // Removed baudRateComboBox, dataBitsComboBox, etc. as they were not in use in setupUi
    QPushButton *refreshButton;
    QPushButton *connectButton;
    QPushButton *cancelButton;
    QLabel *statusLabel;

    // Rate selection UI
    QGroupBox *rateGroupBox; // Added
    QRadioButton *slowRateButton; // Added
    QRadioButton *mediumRateButton; // Added
    QRadioButton *fastRateButton; // Added
    QButtonGroup *rateButtonGroup; // Added


    // Active serial port (set when connecting successfully)
    QSerialPort *serialPort = nullptr;
    // const QString *status; // This was unused
    // bool isError = false; // This was unused

    // Methods
    void setupUi();

    void populatePortsList() const;

    bool configureSerialPort(const QString &device);

    void tryConfiguredPort();

    void loadSettings(); // Added
    void saveSettings(); // Added


private:
    bool m_check_ok = false;
};

#endif // CONNECTDIALOG_H
