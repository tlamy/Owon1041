#include "ConnectDialog.h"

#include <iostream>
#include <QSerialPortInfo>
#include <QMessageBox>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include <__ostream/basic_ostream.h>

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent), serialPort(nullptr) {
    setWindowTitle("Serial Port Connection");
    setupUi();
    populatePortsList();
}

ConnectDialog::~ConnectDialog() {
    // Port will be owned by the caller or deleted when connection failed
}

void ConnectDialog::setupUi() {
    // Main layout
    auto mainLayout = new QVBoxLayout(this);

    // Port selection group
    auto portGroupBox = new QGroupBox("Port Selection");
    auto portLayout = new QHBoxLayout(portGroupBox);

    portComboBox = new QComboBox();
    refreshButton = new QPushButton("Refresh");

    portLayout->addWidget(portComboBox);
    portLayout->addWidget(refreshButton);

    // Serial configuration group
    auto configGroupBox = new QGroupBox("Port Configuration");


    // Status label
    statusLabel = new QLabel();
    statusLabel->setStyleSheet("QLabel { color: gray; }");

    // Buttons
    auto buttonBox = new QDialogButtonBox();
    connectButton = new QPushButton("Connect");
    const auto tryButton = new QPushButton("Test");
    cancelButton = new QPushButton("Cancel");
    buttonBox->addButton(connectButton, QDialogButtonBox::AcceptRole);
    buttonBox->addButton(tryButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(cancelButton, QDialogButtonBox::RejectRole);

    // Add all widgets to main layout
    mainLayout->addWidget(portGroupBox);
    mainLayout->addWidget(configGroupBox);
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(buttonBox);

    // Connect signals and slots
    connect(refreshButton, &QPushButton::clicked, this, &ConnectDialog::refreshPorts);
    connect(tryButton, &QPushButton::clicked, this, &ConnectDialog::tryPort);
    connect(connectButton, &QPushButton::clicked, this, &ConnectDialog::connectToPort);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

void ConnectDialog::populatePortsList() const {
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


void ConnectDialog::refreshPorts() const {
    populatePortsList();
}

bool ConnectDialog::configureSerialPort(const QString &device) {
    // Create a new serial port if not already created
    if (!serialPort) {
        serialPort = new QSerialPort(this);
    }
    //     if (serialPort->isOpen())serialPort->close();
    //     delete serialPort;
    // }

    // Configure with current settings
    serialPort->setPortName(device);
    serialPort->setBaudRate(115200);
    serialPort->setDataBits(QSerialPort::DataBits::Data8);
    serialPort->setParity(QSerialPort::Parity::NoParity);
    serialPort->setStopBits(QSerialPort::StopBits::OneStop);
    serialPort->setFlowControl(QSerialPort::FlowControl::UnknownFlowControl);

    return true;
}

bool ConnectDialog::tryPortByName(const QString &portName) {
    // Configure serial port
    configureSerialPort(portName);
    this->tryConfiguredPort();
    return this->m_check_ok;
}

void ConnectDialog::tryPort() {
    std::cerr << "ConnectDialog::tryPort " << portComboBox->currentData().toString().toStdString() << std::endl;
    if (portComboBox->currentIndex() == -1) {
        this->statusLabel->setText("No serial port selected.");
        this->statusLabel->setStyleSheet("QLabel { color: red; }");
        this->m_check_ok = false;
        std::cerr << "ConnectDialog::tryPort error";
        return;
    }
    this->configureSerialPort(portComboBox->currentData().toString());
    this->tryConfiguredPort();
}

void ConnectDialog::tryConfiguredPort() {
    // Try to open the port
    if (serialPort->isOpen()) { serialPort->close(); }
    if (!serialPort->open(QIODevice::ReadWrite)) {
        this->statusLabel->setText("Could not open serial port: " + serialPort->errorString());
        this->statusLabel->setStyleSheet("QLabel { color: red; }");
        delete serialPort;
        serialPort = nullptr;
        this->m_check_ok = false;
        return;
    }

    this->statusLabel->setText("Trying to communicate...");
    this->statusLabel->setStyleSheet("QLabel { color: grey; }");

    serialPort->write("*IDN?\n");
    if (!serialPort->waitForBytesWritten(1000)) {
        this->statusLabel->setText("Can not write to " + serialPort->portName());
        this->statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        delete serialPort;
        this->m_check_ok = false;
        return;
    };
    if (!serialPort->waitForReadyRead(1000)) {
        this->statusLabel->setText("Can not read from " + serialPort->portName());
        this->statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        delete serialPort;
        this->m_check_ok = false;
        return;
    }
    char buffer[1024];
    serialPort->readLine(buffer, sizeof(buffer));

    const QString input(buffer);
    QStringList parts = input.split(',');

    if (parts.size() < 4) {
        this->statusLabel->setText("Invalid response from " + serialPort->portName());
        this->statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        delete serialPort;
        this->m_check_ok = false;
        return;
    }
    const QString model = parts[1]; // "XDM1041"
    const QString version = parts[3]; // "V3.8.0"

    std::cerr << "Connected to " << model.toStdString() << " " << version.toStdString() << std::endl;
    this->statusLabel->setText("Connected: " + model + "  Firmware " + version);
    this->statusLabel->setStyleSheet("QLabel { color: green; }");

    this->m_check_ok = true;
    return;
}

void ConnectDialog::connectToPort() {
    tryPort();
    if (m_check_ok) {
        accept();
    }
}

QString ConnectDialog::getSelectedPort() const {
    if (portComboBox->currentIndex() == -1) {
        return QString();
    }

    return portComboBox->currentData().toString();
}

QSerialPort *ConnectDialog::getConfiguredSerialPort() const {
    return serialPort;
}
