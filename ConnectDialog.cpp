// ReSharper disable CppDFAMemoryLeak
#include "MainWindow.h" // For MainWindow::settings
#include "ConnectDialog.h"
#include "Settings.h"     // For Settings::Rate

#include <iostream>
#include <QDebug>
#include <QSerialPortInfo>
#include <QDialogButtonBox>
#include <QVBoxLayout>
#include <QHBoxLayout> // Added for radio button layout
#include <QFormLayout>
#include <QGroupBox>
#include <QComboBox>
#include <QCheckBox>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QRadioButton>   // Ensure this is included (already in .h)
#include <QButtonGroup>   // Ensure this is included (already in .h)

ConnectDialog::ConnectDialog(QWidget *parent)
    : QDialog(parent), serialPort(nullptr) {
    setWindowTitle("Serial Port Connection");
    setupUi();
    populatePortsList();
    loadSettings(); // Load settings when dialog is created
}

ConnectDialog::~ConnectDialog() {
    // Port will be owned by the caller or deleted when connection failed
}

void ConnectDialog::setupUi() {
    // Main layout
    auto mainLayout = new QVBoxLayout(this);

    // Defaults group
    auto defaultsGroupBox = new QGroupBox("Defaults");
    auto defaultsLayout = new QFormLayout(defaultsGroupBox);

    m_beep_short = new QCheckBox("Beep in SHORT mode");
    // Connect this checkbox to settings load/save if needed
    defaultsLayout->addWidget(m_beep_short);

    m_short_threshold = new QLineEdit(this);
    m_short_threshold->setFixedWidth(50); // Increased width a bit
    // Connect this line edit to settings load/save if needed
    defaultsLayout->addRow("Threshold (Ω):", m_short_threshold);

    m_beep_diode = new QCheckBox("Beep in DIODE mode");
    // Connect this checkbox to settings load/save if needed
    defaultsLayout->addWidget(m_beep_diode);

    // Rate selection group
    rateGroupBox = new QGroupBox("Measurement Rate");
    auto rateLayout = new QHBoxLayout(); // Use QHBoxLayout for horizontal radio buttons
    slowRateButton = new QRadioButton("Slow");
    mediumRateButton = new QRadioButton("Medium");
    fastRateButton = new QRadioButton("Fast");

    rateButtonGroup = new QButtonGroup(this);
    rateButtonGroup->addButton(slowRateButton, static_cast<int>(Settings::Rate::SLOW));
    rateButtonGroup->addButton(mediumRateButton, static_cast<int>(Settings::Rate::MEDIUM));
    rateButtonGroup->addButton(fastRateButton, static_cast<int>(Settings::Rate::FAST));

    rateLayout->addWidget(slowRateButton);
    rateLayout->addWidget(mediumRateButton);
    rateLayout->addWidget(fastRateButton);
    rateGroupBox->setLayout(rateLayout);
    defaultsLayout->addRow(rateGroupBox); // Add rate group to defaultsLayout


    // Port selection group
    auto portGroupBox = new QGroupBox("Port Selection");
    auto portLayout = new QHBoxLayout(portGroupBox);

    portComboBox = new QComboBox();
    refreshButton = new QPushButton("Refresh");

    portLayout->addWidget(portComboBox);
    portLayout->addWidget(refreshButton);

    // Serial configuration group (currently empty, but kept for structure)
    // auto configGroupBox = new QGroupBox("Port Configuration");


    // Status label
    statusLabel = new QLabel("Select a port and connect."); // Initial message
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
    mainLayout->addWidget(defaultsGroupBox);
    mainLayout->addWidget(portGroupBox);
    // mainLayout->addWidget(configGroupBox); // configGroupBox is currently empty
    mainLayout->addWidget(statusLabel);
    mainLayout->addWidget(buttonBox);

    // Connect signals and slots
    connect(refreshButton, &QPushButton::clicked, this, &ConnectDialog::refreshPorts);
    connect(tryButton, &QPushButton::clicked, this, &ConnectDialog::tryPort);
    connect(connectButton, &QPushButton::clicked, this, &ConnectDialog::connectToPort);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(rateButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, &ConnectDialog::onRateChanged);
}

void ConnectDialog::onRateChanged(int id) {
    // This slot is called when a radio button is clicked.
    // We will save all settings when "Connect" or "Test" is clicked, or on accept.
    // For immediate saving, you could call saveSettings here.
    // MainWindow::settings->setRate(static_cast<Settings::Rate>(id));
    // MainWindow::settings->save(); // Optional: save immediately
}

void ConnectDialog::loadSettings() {
    if (MainWindow::settings) {
        // Load Beep settings
        // Assuming QCheckBox* beepShort and beepDiode are members now, or find them
        this->m_beep_short->setChecked(MainWindow::settings->getBeepShort());
        this->m_beep_diode->setChecked(MainWindow::settings->getBeepDiode());
        this->m_short_threshold->setText(QString::number(MainWindow::settings->getBeepResistance()));

        // Load Rate
        Settings::Rate currentRate = MainWindow::settings->getRate();
        if (currentRate == Settings::Rate::SLOW) {
            slowRateButton->setChecked(true);
        } else if (currentRate == Settings::Rate::MEDIUM) {
            mediumRateButton->setChecked(true);
        } else {
            // Default to FAST
            fastRateButton->setChecked(true);
        }
    }
}

void ConnectDialog::saveSettings() {
    if (MainWindow::settings) {
        MainWindow::settings->setBeepShort(m_beep_short->isChecked());
        MainWindow::settings->setBeepDiode(m_beep_diode->isChecked());
        MainWindow::settings->setBeepResistance(m_short_threshold->text().toInt());

        // Save Rate
        int selectedRateId = rateButtonGroup->checkedId();
        if (selectedRateId != -1) {
            // -1 if no button is checked
            MainWindow::settings->setRate(static_cast<Settings::Rate>(selectedRateId));
        }
        MainWindow::settings->save();
    }
}

void ConnectDialog::populatePortsList() const {
    portComboBox->clear();
    const auto serialPortInfos = QSerialPortInfo::availablePorts();
    if (serialPortInfos.isEmpty()) {
        statusLabel->setText("No serial ports found");
        statusLabel->setStyleSheet("QLabel { color: red; }"); // More visible
        connectButton->setEnabled(false);
        return;
    }
    for (const QSerialPortInfo &portInfo: serialPortInfos) {
        QString portDescription = portInfo.portName();
        if (!portInfo.description().isEmpty()) {
            portDescription += " - " + portInfo.description();
        }
        if (!portInfo.manufacturer().isEmpty()) {
            portDescription += " (" + portInfo.manufacturer() + ")";
        }
        portComboBox->addItem(portDescription, portInfo.portName());
    }
    statusLabel->setText("Select a port and click Connect/Test.");
    statusLabel->setStyleSheet("QLabel { color: gray; }");
    connectButton->setEnabled(true);
}


void ConnectDialog::refreshPorts() const {
    populatePortsList();
}

bool ConnectDialog::configureSerialPort(const QString &device) {
    if (serialPort) {
        delete serialPort;
    }
    serialPort = new QSerialPort(this);
    
    serialPort->setPortName(device);
    serialPort->setBaudRate(QSerialPort::Baud115200);  // Typical for SCPI devices
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::OneStop);
    serialPort->setFlowControl(QSerialPort::NoFlowControl);

    // Add more verbose error handling
    if (!serialPort->open(QIODevice::ReadWrite)) {
        qDebug() << "Failed to open serial port:" << serialPort->errorString();
        return false;
    }

    // Configure timeouts
    serialPort->setReadBufferSize(1024);  // Increase read buffer size
    
    return true;
}

bool ConnectDialog::tryPortByName(const QString &portName) {
    configureSerialPort(portName);
    saveSettings(); // Save settings before trying the port
    tryConfiguredPort();
    return m_check_ok;
}

void ConnectDialog::tryPort() {
    if (portComboBox->currentIndex() == -1) {
        statusLabel->setText("No serial port selected.");
        statusLabel->setStyleSheet("QLabel { color: red; }");
        m_check_ok = false;
        return;
    }
    configureSerialPort(portComboBox->currentData().toString());
    saveSettings(); // Save settings before trying the port
    tryConfiguredPort();
}

void ConnectDialog::tryConfiguredPort() {
    if (!serialPort) {
        // Should not happen if configureSerialPort was called
        statusLabel->setText("Serial port not initialized.");
        statusLabel->setStyleSheet("QLabel { color: red; }");
        m_check_ok = false;
        return;
    }
    if (serialPort->isOpen()) { serialPort->close(); }
    if (!serialPort->open(QIODevice::ReadWrite)) {
        statusLabel->setText("Could not open: " + serialPort->errorString());
        statusLabel->setStyleSheet("QLabel { color: red; }");
        // No need to delete serialPort here, configureSerialPort handles it or it's parented
        m_check_ok = false;
        return;
    }

    statusLabel->setText("Testing communication...");
    statusLabel->setStyleSheet("QLabel { color: blue; }"); // Indicate activity

    serialPort->write("*IDN?\n");
    if (!serialPort->waitForBytesWritten(1000)) {
        statusLabel->setText("Write timeout to " + serialPort->portName());
        statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        m_check_ok = false;
        return;
    }
    if (!serialPort->waitForReadyRead(2000)) {
        // Increased timeout slightly
        statusLabel->setText("Read timeout from " + serialPort->portName());
        statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        m_check_ok = false;
        return;
    }
    char buffer[1024] = {0}; // Initialize buffer
    qint64 bytesRead = serialPort->readLine(buffer, sizeof(buffer) - 1);

    if (bytesRead <= 0) {
        statusLabel->setText("No response from " + serialPort->portName());
        statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        m_check_ok = false;
        return;
    }

    const QString input(buffer);
    QStringList parts = input.trimmed().split(','); // Trim whitespace

    if (parts.size() < 2) {
        // Check for at least model (often 4 parts: Manufacturer,Model,SN,Firmware)
        statusLabel->setText("Invalid response: " + input.left(50)); // Show part of response
        statusLabel->setStyleSheet("QLabel { color: red; }");
        serialPort->close();
        m_check_ok = false;
        return;
    }
    const QString model = parts.value(1, "Unknown Model").trimmed();
    const QString version = parts.value(3, "Unknown Fw").trimmed();

    statusLabel->setText("Connected: " + model + " (FW: " + version + ")");
    statusLabel->setStyleSheet("QLabel { color: green; }");
    m_check_ok = true;
    // Do not close the port here if the test is successful and it's intended to be used by "Connect"
}

void ConnectDialog::connectToPort() {
    tryPort();
    if (m_check_ok) {
        accept();
    } else {
        if (serialPort) {
            if (serialPort->isOpen()) {
                serialPort->close();
            }
            delete serialPort;
            serialPort = nullptr;
        }
    }
}

QString ConnectDialog::getSelectedPort() const {
    if (portComboBox->currentIndex() == -1) {
        return QString();
    }
    return portComboBox->currentData().toString();
}

QSerialPort *ConnectDialog::getConfiguredSerialPort() const {
    // The serialPort member is configured in tryPort/tryPortByName
    // It's up to the caller (MainWindow) to manage this port after the dialog is accepted.
    // If m_check_ok is true, serialPort should be valid and potentially open.
    return m_check_ok ? serialPort : nullptr;
}