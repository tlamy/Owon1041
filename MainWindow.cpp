#include "MainWindow.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QResizeEvent>
#include <QTimer>

#include "ConnectDialog.h"

// In your constructor or initialization method
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    // Initialize settings with your app/organization name
    settings = new Settings("MacWake", "Owon1041", this);
    settings->load(); // Load stored settings

    // Apply window position/size from settings
    if (settings->windowWidth() > 0 && settings->windowHeight() > 0) {
        setGeometry(settings->windowX(), settings->windowY(),
                    settings->windowWidth(), settings->windowHeight());
    }

    setupUi(this);

    QTimer::singleShot(50, this, &MainWindow::connectSerial);
}

MainWindow::~MainWindow()
{
    // No need to delete UI elements as they are deleted when parent is deleted
}

void MainWindow::setupUi(QMainWindow *MainWindow)
{
    QWidget *centralwidget;
    if (MainWindow->objectName().isEmpty())
        MainWindow->setObjectName("MainWindow");
    setGeometry(settings->windowX(), settings->windowY(),
                settings->windowWidth(), settings->windowHeight());
    MainWindow->setMinimumSize(QSize(450, 150));
    MainWindow->setWindowTitle("MacWake OWON XDM-1041");

    centralwidget = new QWidget(MainWindow);
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
    measurement->setStyleSheet("QLabel { padding: 0px; margin: 0px; }");

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

    // Get new window dimensions
    int width = event->size().width();
    int height = event->size().height();
    setupPositions(width, height);
}

void MainWindow::setupPositions(int width, int height) {
    int btn_width = 70;
    int btn_height = 32;
    int btnbar_w = 350;
    int btn_x = (width-btnbar_w)/2;

    // max width
    measurement->setGeometry(QRect(2, 0, width-4, 80));
    int btngroup_y1 = measurement->y() + measurement->height() + 2;
    int btngroup_y2 = btngroup_y1 + btn_height + 1;

    btn_50_v->setGeometry(QRect(btn_x, btngroup_y1, btn_width, btn_height));
    btn_auto_v->setGeometry(QRect(btn_x, btngroup_y2, btn_width, btn_height));
    btn_short->setGeometry(QRect(btn_x+70, btngroup_y1, btn_width, btn_height));
    btn_diode->setGeometry(QRect(btn_x+70, btngroup_y2, btn_width, btn_height));
    btn_50_kr->setGeometry(QRect(btn_x+140, btngroup_y1, btn_width, btn_height));
    btn_auto_r->setGeometry(QRect(btn_x+140, btngroup_y2, btn_width, btn_height));
    btn_50_f->setGeometry(QRect(btn_x+210, btngroup_y1, btn_width, btn_height));
    btn_auto_f->setGeometry(QRect(btn_x+210, btngroup_y2, btn_width, btn_height));
    btn_freq->setGeometry(QRect(btn_x+280, btngroup_y1, btn_width, btn_height));
    btn_period->setGeometry(QRect(btn_x+280, btngroup_y2, btn_width, btn_height));
}

void MainWindow::connectSerial() {
    if (this->settings->device().isEmpty()) {
        this->openConnectDialog();
    }
}

bool MainWindow::openConnectDialog() {
    ConnectDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Get the configured serial port
        auto serialPort = dialog.getConfiguredSerialPort();
        if (serialPort) {
            // Store the selected port in settings
            settings->setDevice(serialPort->portName());
            return true;
        }
    }
    return false;
}

// Slot implementations
void MainWindow::onVoltage50V()
{
    // Implementation
}

void MainWindow::onVoltageAuto()
{
    // Implementation
}

void MainWindow::onShort()
{
    // Implementation
}

void MainWindow::onDiode()
{
    // Implementation
}

void MainWindow::onResistance50K()
{
    // Implementation
}

void MainWindow::onResistanceAuto()
{
    // Implementation
}

void MainWindow::onCapacitance50uF()
{
    // Implementation
}

void MainWindow::onCapacitanceAuto()
{
    // Implementation
}

void MainWindow::onFrequency()
{
    // Implementation
}

void MainWindow::onPeriod()
{
    // Implementation
}