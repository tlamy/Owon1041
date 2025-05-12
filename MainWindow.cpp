#include "MainWindow.h"

#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QResizeEvent>
#include <QTimer>
#include <QMouseEvent>
#include <QDebug>
#include "ConnectDialog.h"

// In your constructor or initialization method
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    // Initialize settings with your app/organization name
    settings = new Settings("MacWake", "Owon1041", this);
    settings->load(); // Load stored settings

    // Apply window position/size from settings
    if (settings->windowWidth() > 0 && settings->windowHeight() > 0) {
        setGeometry(settings->windowX(), settings->windowY(),
                    settings->windowWidth(), settings->windowHeight());
    }

    setupUi(this);

    QTimer::singleShot(2000, this, &MainWindow::connectSerial);
}

MainWindow::~MainWindow() {
    // No need to delete UI elements as they are deleted when parent is deleted
}

void MainWindow::setupUi(QMainWindow *MainWindow) {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName("MainWindow");
    setGeometry(settings->windowX(), settings->windowY(),
                settings->windowWidth(), settings->windowHeight());
    MainWindow->setMinimumSize(QSize(450, 150));
    MainWindow->setWindowTitle("MacWake OWON XDM-1041");

    // ReSharper disable once CppDFAMemoryLeak
    const auto centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName("centralwidget");

    // Create measurement label with text in constructor
    measurement = new QLabel("0.1235 µF", centralwidget);
    measurement->setObjectName("measurement");
    QFont font;
    //font.setFamily("Menlo");
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(80);
    measurement->setFont(font);
    measurement->setFrameShape(QFrame::StyledPanel);
    measurement->setFrameShadow(QFrame::Raised);
    measurement->setTextFormat(Qt::PlainText);
    measurement->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    measurement->setMargin(0);
    measurement->setContentsMargins(0, 0, 0, 0);
    measurement->setStyleSheet("QLabel { padding: 0px; margin: 0px; background-color: #f0f0f0; }");
    measurement->setToolTip("Click to open connection dialog"); // Add this line

    // Make measurement label clickable by installing event filter
    measurement->installEventFilter(this);

    // In setupUi function after creating the measurement label
    measurement->setMouseTracking(true);
    measurement->setAttribute(Qt::WA_Hover, true);
    measurement->setFocusPolicy(Qt::StrongFocus);

    // Create buttons with text in constructors
    btn_50_v = new QPushButton("50 V", centralwidget);
    btn_50_v->setObjectName("btn_50_v");

    btn_auto_v = new QPushButton("Auto V", centralwidget);
    btn_auto_v->setObjectName("btn_auto_v");

    btn_short = new QPushButton("Short", centralwidget);
    btn_short->setObjectName("btn_short");

    btn_diode = new QPushButton("Diode", centralwidget);
    btn_diode->setObjectName("btn_diode");

    btn_50_kr = new QPushButton("50 kΩ", centralwidget);
    btn_50_kr->setObjectName("btn_50_kr");

    btn_auto_r = new QPushButton("Auto Ω", centralwidget);
    btn_auto_r->setObjectName("btn_auto_r");

    btn_50_f = new QPushButton("50 µF", centralwidget);
    btn_50_f->setObjectName("btn_50_f");

    btn_auto_f = new QPushButton("Auto F", centralwidget);
    btn_auto_f->setObjectName("btn_auto_f");

    btn_freq = new QPushButton("Hz", centralwidget);
    btn_freq->setObjectName("btn_freq");

    btn_period = new QPushButton("Period", centralwidget);
    btn_period->setObjectName("btn_period");

    setupPositions(MainWindow->width(), MainWindow->height());
    MainWindow->setCentralWidget(centralwidget);

    m_connect_dialog = new ConnectDialog(this);

    QMetaObject::connectSlotsByName(MainWindow);
}

void MainWindow::setupConnections()
{
    connect(btn_50_v, &QPushButton::clicked, this, &MainWindow::onVoltage50V);
    connect(btn_auto_v, &QPushButton::clicked, this, &MainWindow::onVoltageAuto);
    connect(btn_short, &QPushButton::clicked, this, &MainWindow::onShort);
    connect(btn_diode, &QPushButton::clicked, this, &MainWindow::onDiode);
    connect(btn_50_kr, &QPushButton::clicked, this, &MainWindow::onResistance50K);
    connect(btn_auto_r, &QPushButton::clicked, this, &MainWindow::onResistanceAuto);
    connect(btn_50_f, &QPushButton::clicked, this, &MainWindow::onCapacitance50uF);
    connect(btn_auto_f, &QPushButton::clicked, this, &MainWindow::onCapacitanceAuto);
    connect(btn_freq, &QPushButton::clicked, this, &MainWindow::onFrequency);
    connect(btn_period, &QPushButton::clicked, this, &MainWindow::onPeriod);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event); // Call base class implementation

    setupPositions(event->size().width(), event->size().height());
}

void MainWindow::setupPositions(const int width, const int height) const {
    const int btn_width = 70;
    const int btn_height = 32;
    const int btnbar_w = 350;
    const int btn_x = (width - btnbar_w) / 2;

    // max width
    measurement->setGeometry(QRect(2, 0, width - 4, 80));
    const int btngroup_y1 = measurement->y() + measurement->height() + 2;
    const int btngroup_y2 = btngroup_y1 + btn_height + 1;

    btn_50_v->setGeometry(QRect(btn_x, btngroup_y1, btn_width, btn_height));
    btn_auto_v->setGeometry(QRect(btn_x, btngroup_y2, btn_width, btn_height));
    btn_short->setGeometry(QRect(btn_x + 70, btngroup_y1, btn_width, btn_height));
    btn_diode->setGeometry(QRect(btn_x + 70, btngroup_y2, btn_width, btn_height));
    btn_50_kr->setGeometry(QRect(btn_x + 140, btngroup_y1, btn_width, btn_height));
    btn_auto_r->setGeometry(QRect(btn_x + 140, btngroup_y2, btn_width, btn_height));
    btn_50_f->setGeometry(QRect(btn_x + 210, btngroup_y1, btn_width, btn_height));
    btn_auto_f->setGeometry(QRect(btn_x + 210, btngroup_y2, btn_width, btn_height));
    btn_freq->setGeometry(QRect(btn_x + 280, btngroup_y1, btn_width, btn_height));
    btn_period->setGeometry(QRect(btn_x + 280, btngroup_y2, btn_width, btn_height));
}

void MainWindow::connectSerial() {
    std::cerr << "Connecting to serial port (auto)" << std::endl;
    if (this->settings->device().isEmpty()) {
        this->openConnectDialog();
    } else {
        if (!this->m_connect_dialog->tryPortByName(this->settings->device())) {
            std::cerr << "Could not connect to serial port " << this->settings->device().toStdString() << std::endl;
        } else {
            this->m_port = m_connect_dialog->getConfiguredSerialPort();
            this->onConnect();
        }
    }
}

void MainWindow::onConnect() {
    std::cerr << "Connected; creating timer" << std::endl;
    this->m_timer = new QTimer(this);
    this->m_timer->setInterval(100);
    this->m_timer->setSingleShot(false);
    connect(this->m_timer, &QTimer::timeout, this, &MainWindow::updateMeasurement);
    this->m_timer->start();
}

bool MainWindow::openConnectDialog() {
    if (m_connect_dialog->exec() == QDialog::Accepted) {
        // Get the configured serial port
        const auto serialPort = m_connect_dialog->getConfiguredSerialPort();
        if (serialPort) {
            // Store the selected port in settings
            settings->setDevice(serialPort->portName());
            return true;
        }
        this->m_port = serialPort;
    }
    return false;
}

void MainWindow::updateMeasurement() {
    if (!this->m_port) {
        std::cerr << "Port is NULL, stopping timer" << std::endl;
        this->m_timer->stop();
        this->m_timer->deleteLater();
        this->m_timer = nullptr;
        return;
    }
    this->m_port->write("MEAS?\n");
    std::cerr << "MEAS? sent, waiting for response" << std::endl;
    if (!this->m_port->waitForReadyRead(5000)) {
        std::cerr << "Read timeout, stopping timer" << std::endl;
        this->m_timer->stop();
        this->m_timer->deleteLater();
        this->m_timer = nullptr;
        if (this->m_port->isOpen())this->m_port->close();
        this->m_port = nullptr;
        return;
    }
    char buffer[128];
    this->m_port->readLine(buffer, sizeof(buffer));
    std::cerr << "Received " << buffer << std::endl;
    this->measurement->setText(QString::fromLocal8Bit(buffer));
}

// Slot implementations
void MainWindow::onVoltage50V() {
    // Implementation
}

void MainWindow::onVoltageAuto() {
    // Implementation
}

void MainWindow::onShort() {
    // Implementation
}

void MainWindow::onDiode() {
    // Implementation
}

void MainWindow::onResistance50K() {
    // Implementation
}

void MainWindow::onResistanceAuto() {
    // Implementation
}

void MainWindow::onCapacitance50uF() {
    // Implementation
}

void MainWindow::onCapacitanceAuto() {
    // Implementation
}

void MainWindow::onFrequency() {
    // Implementation
}

void MainWindow::onPeriod() {
    // Implementation
}

// Event filter to handle mouse events on the measurement label
bool MainWindow::eventFilter(QObject *obj, QEvent *event) {
    if (obj == measurement) {
        if (event->type() == QEvent::MouseButtonRelease) {
            if (const QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
                mouseEvent->button() == Qt::LeftButton) {
                onMeasurementClicked();
                return true;
            }
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

// Slot to handle measurement label clicks
void MainWindow::onMeasurementClicked() {
    openConnectDialog();
}
