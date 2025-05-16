#include "MainWindow.h"

#include <iostream>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QTimer>
#include <QMouseEvent>
#include <QDebug>
#include <QThread>
#include "ConnectDialog.h"

// Define the static member variable
Settings *MainWindow::settings = nullptr;

// In your constructor or initialization method
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    if (!MainWindow::settings) {
        MainWindow::settings = new Settings("MacWake", "Owon1041", this);
        MainWindow::settings->load(); // Load stored settings
    }


    // Apply window position/size from settings
    if (MainWindow::settings->windowWidth() > 0 && MainWindow::settings->windowHeight() > 0) {
        setGeometry(MainWindow::settings->windowX(), MainWindow::settings->windowY(),
                    MainWindow::settings->windowWidth(), MainWindow::settings->windowHeight());
    }

    setupUi(this);

    QTimer::singleShot(2000, this, &MainWindow::connectSerial);
}

MainWindow::~MainWindow() {
    // No need to delete UI elements as they are deleted when parent is deleted
    if (m_port) {
        if (m_port->isOpen()) {
            m_port->close();
        }
        delete m_port;
        m_port = nullptr;
    }
    
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}

void MainWindow::setupUi(QMainWindow *MainWindow) {
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName("MainWindow");
    setGeometry(this->settings->windowX(), this->settings->windowY(),
                this->settings->windowWidth(), this->settings->windowHeight());
    MainWindow->setMinimumSize(QSize(450, 150));
    MainWindow->setWindowTitle("MacWake OWON XDM-1041");

    // ReSharper disable once CppDFAMemoryLeak
    const auto centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName("centralwidget");

    measurement = new QLabel("0.1235 µF", centralwidget);
    measurement->setObjectName("measurement");
    QFont font;
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

    measurement->installEventFilter(this);

    measurement->setMouseTracking(true);
    measurement->setAttribute(Qt::WA_Hover, true);
    measurement->setFocusPolicy(Qt::StrongFocus);

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

    this->m_timer = new QTimer(this);
    this->m_timer->setInterval(100);
    this->m_timer->setSingleShot(false);
    connect(this->m_timer, &QTimer::timeout, this, &MainWindow::updateMeasurement);

}

void MainWindow::setupConnections() {
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

    this->settings->setWindowWidth(event->size().width());
    this->settings->setWindowHeight(event->size().height());
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
    if (MainWindow::settings->device().isEmpty()) {
        this->openConnectDialog();
    } else {
        if (!this->m_connect_dialog->tryPortByName(MainWindow::settings->device())) {
            std::cerr << "Could not connect to serial port " << MainWindow::settings->device().toStdString() <<
                    std::endl;
        } else {
            this->m_port = m_connect_dialog->getConfiguredSerialPort();
            this->onConnect();
        }
    }
}

QString MainWindow::rateToSerial(Settings::Rate rate) {
    switch (rate) {
        case Settings::Rate::SLOW:
            return "S";
        case Settings::Rate::MEDIUM:
            return "M";
        case Settings::Rate::FAST:
            return "F";
    }
    return "F";
}

void MainWindow::onConnect() {
    // Initialize meter from settings

    this->writeSCPI(QString("RATE " + this->rateToSerial(this->settings->getRate())), false);

    this->writeSCPI("SYST:BEEP:STAT OFF", false);
    this->onVoltage50V();

    this->m_timer->start();
}

bool MainWindow::openConnectDialog() {
    if (m_connect_dialog->exec() == QDialog::Accepted) {
        // Get the configured serial port
        const auto serialPort = m_connect_dialog->getConfiguredSerialPort();
        if (serialPort) {
            // Store the selected port in settings
            MainWindow::settings->setDevice(serialPort->portName());
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
    auto reading = this->writeSCPI("MEAS1?", true);
    QString display;
    bool isOk;
    auto value = reading.toFloat(&isOk);
    qDebug() << "Received " << reading << " as " << value;
    if (isOk) {
        if (false && value < 60E-9) {
            display = QString::asprintf("%+0.5f n%s", value * 1E9, this->m_unit.toStdString().c_str());
        } else if (false && value < 60E-6) {
            display = QString::asprintf("%+0.5f µ%s", value * 1E6, this->m_unit.toStdString().c_str());
        } else if (false && value < 60E-3) {
            display = QString::asprintf("%+0.5f m%s", value * 1E3, this->m_unit.toStdString().c_str());
        } else if (value < 60E-0) {
            display = QString::asprintf("%+0.5f %s", value, this->m_unit.toStdString().c_str());
        } else if (value < 60E+3) {
            display = QString::asprintf("%+0.5f m%s", value * 1E3, this->m_unit.toStdString().c_str());
        }
    }

    std::cerr << "Received " << reading.toStdString() << std::endl;
    this->measurement->setText(display);
}

// Slot implementations
void MainWindow::onVoltage50V() {
    this->m_unit = "V";
    this->writeSCPI("CONF:VOLT:DC 50", false);
}

void MainWindow::onVoltageAuto() {
    this->m_unit = "V";
    this->writeSCPI("CONF:VOLT:DC AUTO", false);
}

void MainWindow::onShort() {
    this->m_unit = "Ω";
    this->writeSCPI("CONF:CONT", false);
    if (this->settings->getBeepShort()) {
        this->writeSCPI("CONT:THRE " + QString(this->settings->getBeepResistance()), false);
        this->writeSCPI("BEEP:STAT ON", false);
    } else {
        this->writeSCPI("BEEP:STAT OFF", false);
    }
}

void MainWindow::onDiode() {
    this->m_unit = "V";
    this->writeSCPI("CONF:DIOD", false);
    if (this->settings->getBeepDiode()) {
        this->writeSCPI("BEEP:STAT ON", false);
    } else {
        this->writeSCPI("BEEP:STAT OFF", false);
    }
}

void MainWindow::onResistance50K() {
    this->m_unit = "Ω";
    this->writeSCPI("CONF:RES 50E3", false);
}

void MainWindow::onResistanceAuto() {
    this->m_unit = "Ω";
    this->writeSCPI("CONF:RES AUTO", false);
}

void MainWindow::onCapacitance50uF() {
    this->m_unit = "F";
    this->writeSCPI("CONF:CAP 50E-6", false);
}

void MainWindow::onCapacitanceAuto() {
    this->m_unit = "F";
    this->writeSCPI("CONF:CAP AUTO", false);
}

void MainWindow::onFrequency() {
    this->m_unit = "Hz";
    this->writeSCPI("CONF:FREQ", false);
}

void MainWindow::onPeriod() {
    this->m_unit = "%";
    this->writeSCPI("CONF:PER", false);
}

void MainWindow::onSerialError(const QString &message) {
    if (this->m_timer) {
        this->m_timer->stop();
        qDebug() << "Stopping timer";
    }
    qDebug() << "Serial port error: " << message;
    std::cerr << "Serial port error, closing\n";
    if (this->m_port) {
        if (this->m_port->isOpen()) {
            this->m_port->close();
        }
        delete this->m_port;
        this->m_port = nullptr; // This is good, but ensure it's consistently used
    }
}

QString MainWindow::readSCPI() {
    if (!m_port || !m_port->isOpen()) {
        qDebug() << "Serial port not open";
        return QString();
    }

    // Increase timeout for reading
    m_port->waitForReadyRead(500);  // Wait up to 500 ms for data

    QByteArray responseData;
    QElapsedTimer timer;
    timer.start();

    while (!responseData.contains('\n')) {
        if (timer.elapsed() > 1000) {  // 1-second total timeout
            qDebug() << "Read timeout occurred";
            emit onSerialError("readSCPI timeout");
            return QString();
        }

        if (m_port->bytesAvailable() > 0) {
            responseData.append(m_port->readAll());
        } else {
            QThread::msleep(10);  // Small sleep to prevent busy waiting
        }
    }

    QString response = QString::fromUtf8(responseData.trimmed());
    qDebug() << "Received response:" << response;
    return response;
}

QString MainWindow::writeSCPI(const QString &command, bool readResponse) {
    if (!this->m_port) {
        std::cerr << "No port open, refusing writeSCPI\n";
        return nullptr;
    }
    qDebug() << "Writing " << command;
    this->m_port->write(QString(command + "\n").toLocal8Bit());
    this->m_port->flush();
    this->m_port->waitForBytesWritten(1000);
    return readResponse ? readSCPI() : "";
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
    this->settings->save();
    openConnectDialog();
}