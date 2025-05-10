#include "ConnectDialog.h"
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QGridLayout>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent), serialPort(nullptr) {
    setWindowTitle("Serial Port Connection");
    setupUi();
    populatePortsList();
    populateComboBoxes();
}

ConnectDialog::~ConnectDialog() {
    // Port will be owned by the caller or deleted when connection failed
}

void ConnectDialog::setupUi() {
    // Main layout
    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    // Port selection group
    QGroupBox *portGroupBox = new QGroupBox("Port Selection");
    QHBoxLayout *portLayout = new QHBoxLayout(portGroupBox);

    portComboBox = new QComboBox();
    refreshButton = new QPushButton("Refresh");

    portLayout->addWidget(portComboBox);
    portLayout->addWidget(refreshButton);

    // Serial configuration group
    QGroupBox *configGroupBox = new QGroupBox("Port Configuration");


    // Status label
    statusLabel = new QLabel();
    statusLabel->setStyleSheet("QLabel { color: gray; }");

    // Buttons
    QDialogButtonBox *buttonBox = new QDialogButtonBox();
    connectButton = new QPushButton("Connect");
    auto tryButton = new QPushButton("Test");
    cancelButton = new QPushButton("Cancel");
    buttonBox->addButton(connectButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tryButton);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    // Add all widgets to main layout
    mainLayout->addWidget(portGroupBox);
    mainLayout->addWidget(configGroupBox);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(buttonBox);

    // Connect signals and slots
    connect(refreshButton, &QPushButton::clicked, this, &ConnectDialog::refreshPorts);
    connect(connectButton, &QPushButton::clicked, this, &ConnectDialog::connectToPort);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ConnectDialog::populatePortsList() {
    portComboBox->clear();

    // Get available ports
    const auto serialPortInfos = QSerialPortInfo::availablePorts();

    // Check if there are any available ports
    if (serialPortInfos.isEmpty()) {
        statusLabel->setText("No serial ports found");
        connectButton->setEnabled(false);
        return;
    }

    // Add ports to the combo box
    for (const QSerialPortInfo &portInfo: serialPortInfos) {
        QString portDescription = portInfo.portName();

        // Add description if available
        if (!portInfo.description().isEmpty()) {
            portDescription += " - " + portInfo.description();
        }

        // Add manufacturer if available
        if (!portInfo.manufacturer().isEmpty()) {
            portDescription += " (" + portInfo.manufacturer() + ")";
        }

        portComboBox->addItem(portDescription, portInfo.portName());
    }

    statusLabel->setText("Select a port and click Connect");
    connectButton->setEnabled(true);
}


void ConnectDialog::refreshPorts() {
    populatePortsList();
}

bool ConnectDialog::configureSerialPort() {
    // Create a new serial port if not already created
    if (!serialPort) {
        serialPort = new QSerialPort(this);
    }

    // Configure with current settings
    serialPort->setPortName(portComboBox->currentData().toString());
    serialPort->setBaudRate(115200);
    serialPort->setDataBits(QSerialPort::DataBits::Data8);
    serialPort->setParity(QSerialPort::Parity::NoParity);
    serialPort->setStopBits(QSerialPort::StopBits::OneStop);
    serialPort->setFlowControl(QSerialPort::FlowControl::UnknownFlowControl);

    return true;
}

QString ConnectDialog::tryPort() {
    if (portComboBox->currentIndex() == -1) {
        return "No serial port selected.";
    }
    // Try to open the port
    if (!serialPort->open(QIODevice::ReadWrite)) {
        auto result = "Could not open serial port: " + serialPort->errorString();
        delete serialPort;
        serialPort = nullptr;
        return result;
    }
    this->configureSerialPort();
}

void ConnectDialog::connectToPort() {
    if (portComboBox->currentIndex() == -1) {
        QMessageBox::warning(this, "Error", "No serial port selected.");
        return;
    }

    // Configure the serial port
    if (!configureSerialPort()) {
        QMessageBox::critical(this, "Error", "Failed to configure serial port.");
        return;
    }

    // Try to open the port
    if (!serialPort->open(QIODevice::ReadWrite)) {
        QMessageBox::critical(this, "Connection Failed",
                              "Could not open serial port: " + serialPort->errorString());
        delete serialPort;
        serialPort = nullptr;
        return;
    }

    // Port opened successfully
    statusLabel->setText("Connected to " + portComboBox->currentText());
    statusLabel->setStyleSheet("QLabel { color: green; }");

    // Accept the dialog
    accept();
}

QString ConnectDialog::getSelectedPort() const {
    if (portComboBox->currentIndex() == -1) {
        return QString();
    }

    return portComboBox->currentData().toString();
}

QSerialPort *ConnectDialog::getConfiguredSerialPort() {
    return serialPort;
}
