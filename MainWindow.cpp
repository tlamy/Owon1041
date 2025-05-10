#include "MainWindow.h"
#include <QtCore/QVariant>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    setupUi(this);
    setupConnections();
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
    MainWindow->resize(450, 150);
    MainWindow->setMinimumSize(QSize(450, 150));
    MainWindow->setWindowTitle("MainWindow");
    
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

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event); // Call base class implementation

    // Get new window dimensions
    int width = event->size().width();
    int height = event->size().height();
    setupPositions(width, height);
}

void MainWindow::setupPositions(int width, int height) {
    int btngroup_y1 = 85;
    int btngroup_y2 = 110;
    measurement->setGeometry(QRect(2, 0, width-4, 80));
    btn_50_v->setGeometry(QRect(40, btngroup_y1, 71, 32));
    btn_auto_v->setGeometry(QRect(40, btngroup_y2, 71, 32));
    btn_short->setGeometry(QRect(110, btngroup_y1, 71, 31));
    btn_diode->setGeometry(QRect(110, btngroup_y2, 71, 31));
    btn_50_kr->setGeometry(QRect(180, btngroup_y1, 71, 31));
    btn_auto_r->setGeometry(QRect(180, btngroup_y2, 71, 31));
    btn_50_f->setGeometry(QRect(250, btngroup_y1, 71, 31));
    btn_auto_f->setGeometry(QRect(250, btngroup_y2, 71, 31));
    btn_freq->setGeometry(QRect(320, btngroup_y1, 71, 31));
    btn_period->setGeometry(QRect(320, btngroup_y2, 71, 31));
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