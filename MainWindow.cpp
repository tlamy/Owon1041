#include "MainWindow.h"

#include <iostream>
#include <limits>
#include <QtWidgets/QApplication>
#include <QtWidgets/QLabel>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QGridLayout>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QDoubleSpinBox>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHeaderView>
#include <QResizeEvent>
#include <QTimer>
#include <QMouseEvent>
#include <QDebug>
#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QRegExp>
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

    // Initialize available ranges for each measurement mode
    // DC Voltage ranges - per programming guide: 500E-3, 5, 50, 500, 1000 (V)
    availableRanges["VOLT:DC"] = QStringList() << "AUTO" << "500E-3" << "5" << "50" << "500" << "1000";

    // AC Voltage ranges - per programming guide: 500E-3, 5, 50, 500, 750 (V)
    availableRanges["VOLT:AC"] = QStringList() << "AUTO" << "500E-3" << "5" << "50" << "500" << "750";

    // Resistance ranges - per programming guide: 500, 5E3, 50E3, 500E3, 5E6, 50E6, 500E6 (Ω)
    availableRanges["RES"] = QStringList() << "AUTO" << "500" << "5E3" << "50E3" << "500E3" << "5E6" << "50E6" << "500E6";

    // 4-Wire Resistance ranges - per programming guide: max 50KΩ for FRES
    availableRanges["FRES"] = QStringList() << "AUTO" << "500" << "5E3" << "50E3";

    // Capacitance ranges - based on typical DMM ranges
    availableRanges["CAP"] = QStringList() << "AUTO" << "50E-9" << "500E-9" << "5E-6" << "50E-6" << "500E-6" << "5E-3" << "50E-3";

    // DC Current ranges - per programming guide: likely 500uA, 5mA, 50mA, 500mA, 5A, 10A
    availableRanges["CURR:DC"] = QStringList() << "AUTO" << "500E-6" << "5E-3" << "50E-3" << "500E-3" << "5" << "10";

    // AC Current ranges - same as DC Current
    availableRanges["CURR:AC"] = QStringList() << "AUTO" << "500E-6" << "5E-3" << "50E-3" << "500E-3" << "5" << "10";

    // Initialize current range index for each function to 0 (AUTO)
    currentRangeIndex["VOLT:DC"] = 0;
    currentRangeIndex["VOLT:AC"] = 0;
    currentRangeIndex["RES"] = 0;
    currentRangeIndex["FRES"] = 0;
    currentRangeIndex["CAP"] = 0;
    currentRangeIndex["CURR:DC"] = 0;
    currentRangeIndex["CURR:AC"] = 0;

    setupUi(this);
    setupConnections();

    // Load and apply hotkey settings
    loadHotkeySettings();
    applyHotkeys();

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
    MainWindow->setMinimumSize(QSize(600, 600));
    MainWindow->setWindowTitle("MacWake OWON XDM-1041");

    // Create central widget with a main vertical layout
    const auto centralwidget = new QWidget(MainWindow);
    centralwidget->setObjectName("centralwidget");
    auto mainLayout = new QVBoxLayout(centralwidget);
    mainLayout->setSpacing(10);
    mainLayout->setContentsMargins(10, 10, 10, 10);
    mainLayout->setSizeConstraint(QLayout::SetDefaultConstraint); // Default constraint for better resizing behavior

    // Create a tab widget for the configuration tab only
    tabWidget = new QTabWidget(centralwidget);
    tabWidget->setObjectName("tabWidget");

    // Create tabs
    configTab = new QWidget();
    configTab->setObjectName("configTab");

    // Add tabs to tab widget
    tabWidget->addTab(configTab, "Configuration");

    // ================================================
    // COMPLETELY NEW DIGITAL MULTIMETER DISPLAY DESIGN
    // ================================================

    // Create a digital multimeter display panel
    QFrame* dmPanel = new QFrame(centralwidget);
    dmPanel->setObjectName("dmPanel");
    dmPanel->setFrameStyle(QFrame::Box | QFrame::Raised);
    dmPanel->setLineWidth(2);
    dmPanel->setMidLineWidth(1);
    dmPanel->setStyleSheet("QFrame#dmPanel { background-color: #303030; border: 2px solid #505050; border-radius: 8px; }");

    // Create a grid layout for the digital multimeter display
    QGridLayout* dmLayout = new QGridLayout(dmPanel);
    dmLayout->setSpacing(8);
    dmLayout->setContentsMargins(15, 15, 15, 15);

    // Create a display panel for the readings
    QFrame* displayPanel = new QFrame(dmPanel);
    displayPanel->setObjectName("displayPanel");
    displayPanel->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    displayPanel->setLineWidth(1);
    displayPanel->setStyleSheet("QFrame#displayPanel { background-color: #101010; border: 1px inset #505050; }");

    // ================================================
    // COMPLETELY REFACTORED MEASUREMENT DISPLAY
    // ================================================

    // Create a grid layout for the display panel with improved spacing
    QGridLayout* displayLayout = new QGridLayout(displayPanel);
    displayLayout->setSpacing(10);
    displayLayout->setContentsMargins(10, 15, 10, 15);

    // Create a container for the mode display
    QWidget* modeContainer = new QWidget(displayPanel);
    QVBoxLayout* modeLayout = new QVBoxLayout(modeContainer);
    modeLayout->setContentsMargins(5, 5, 5, 5);
    modeLayout->setSpacing(2);

    // Create a label for the current mode
    modeLabel = new QLabel("DC VOLTAGE", modeContainer);
    modeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    modeLabel->setStyleSheet("QLabel { color: #00ff00; font-size: 14px; font-weight: bold; background-color: transparent; }");
    modeLayout->addWidget(modeLabel);

    // Create a label for the current range
    rangeLabel = new QLabel("AUTO", modeContainer);
    rangeLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    rangeLabel->setStyleSheet("QLabel { color: #00ff00; font-size: 12px; background-color: transparent; }");
    modeLayout->addWidget(rangeLabel);

    // Create a label for additional info (AC/DC, 2/4 wire, etc.)
    infoLabel = new QLabel("", modeContainer);
    infoLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    infoLabel->setStyleSheet("QLabel { color: #00ff00; font-size: 10px; background-color: transparent; }");
    modeLayout->addWidget(infoLabel);

    // Add stretch to push labels to the top
    modeLayout->addStretch();

    // Create the primary measurement display (LCD-style)
    measurement = new QLabel("0.000", displayPanel);
    measurement->setObjectName("measurement");
    QFont lcdFont;
    lcdFont.setFamily("Digital-7");  // Use a digital font if available
    lcdFont.setStyleHint(QFont::TypeWriter);
    lcdFont.setPointSize(48);
    lcdFont.setBold(true);
    measurement->setFont(lcdFont);
    measurement->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    measurement->setStyleSheet("QLabel { color: #00ff00; background-color: transparent; }");  // Green LCD color
    measurement->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    measurement->setMinimumHeight(85); // Increased height for better visibility
    measurement->setToolTip("Primary measurement display");

    // Make measurement label clickable
    measurement->installEventFilter(this);
    measurement->setMouseTracking(true);
    measurement->setAttribute(Qt::WA_Hover, true);
    measurement->setFocusPolicy(Qt::StrongFocus);

    // Create the secondary measurement display (smaller LCD-style)
    secondaryMeasurement = new QLabel("0.000", displayPanel);
    secondaryMeasurement->setObjectName("secondaryMeasurement");
    QFont smallLcdFont;
    smallLcdFont.setFamily("Digital-7");  // Use a digital font if available
    smallLcdFont.setStyleHint(QFont::TypeWriter);
    smallLcdFont.setPointSize(32);
    smallLcdFont.setBold(true);
    secondaryMeasurement->setFont(smallLcdFont);
    secondaryMeasurement->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    secondaryMeasurement->setStyleSheet("QLabel { color: #00ccff; background-color: transparent; }");  // Blue LCD color
    secondaryMeasurement->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    secondaryMeasurement->setMinimumHeight(65); // Increased height for better visibility
    secondaryMeasurement->setVisible(false);  // Initially hidden
    secondaryMeasurement->setToolTip("Secondary measurement display");

    // Create labels for the displays
    QLabel* primaryLabel = new QLabel("PRIMARY", displayPanel);
    primaryLabel->setStyleSheet("QLabel { color: #00ff00; font-size: 12px; font-weight: bold; background-color: transparent; }");
    primaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QLabel* secondaryLabel = new QLabel("SECONDARY", displayPanel);
    secondaryLabel->setStyleSheet("QLabel { color: #00ccff; font-size: 12px; font-weight: bold; background-color: transparent; }");
    secondaryLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    secondaryLabel->setVisible(false);  // Initially hidden

    // Add the displays to the layout in a 2x2 grid
    displayLayout->addWidget(modeContainer, 0, 0, 2, 1); // Mode info spans 2 rows in first column
    displayLayout->addWidget(primaryLabel, 0, 1);        // Primary label in first row, second column
    displayLayout->addWidget(measurement, 0, 2);         // Primary measurement in first row, third column
    displayLayout->addWidget(secondaryLabel, 1, 1);      // Secondary label in second row, second column
    displayLayout->addWidget(secondaryMeasurement, 1, 2); // Secondary measurement in second row, third column

    // Set column stretch factors to ensure proper sizing
    displayLayout->setColumnStretch(0, 1);  // Mode container gets 1 part
    displayLayout->setColumnStretch(1, 0);  // Labels get minimum width
    displayLayout->setColumnStretch(2, 2);  // Measurements get 2 parts

    // ================================================
    // SIMPLIFIED CONTROL PANEL WITH ONLY DUAL DISPLAY BUTTON
    // ================================================

    // Create a control panel for the dual display button
    QFrame* controlPanel = new QFrame(dmPanel);
    controlPanel->setObjectName("controlPanel");
    controlPanel->setFrameStyle(QFrame::Panel | QFrame::Raised);
    controlPanel->setLineWidth(1);
    controlPanel->setStyleSheet(
        "QFrame#controlPanel { background-color: #404040; border: 1px outset #606060; border-radius: 4px; }"
        "QPushButton { color: #ffffff; font-weight: bold; background-color: #505050; "
        "border: 2px outset #707070; border-radius: 4px; padding: 4px; min-width: 80px; min-height: 30px; }"
        "QPushButton:hover { background-color: #606060; }"
        "QPushButton:pressed { border-style: inset; }"
        "QPushButton:checked { background-color: #008000; border-color: #00a000; }"
        "QPushButton:checked:hover { background-color: #009000; }"
    );

    // Create a layout for the control panel
    QHBoxLayout* controlLayout = new QHBoxLayout(controlPanel);
    controlLayout->setSpacing(8);
    controlLayout->setContentsMargins(10, 8, 10, 8);

    // Create the dual display toggle button
    btn_dual_display = new QPushButton("DUAL", controlPanel);
    btn_dual_display->setObjectName("btn_dual_display");
    btn_dual_display->setCheckable(true);
    btn_dual_display->setChecked(false);

    // Create the hold toggle button
    btn_hold = new QPushButton("HOLD", controlPanel);
    btn_hold->setObjectName("btn_hold");
    btn_hold->setCheckable(true);
    btn_hold->setChecked(false);
    btn_hold->setStyleSheet(
        "QPushButton:checked { background-color: #800000; border-color: #a00000; }"
        "QPushButton:checked:hover { background-color: #900000; }"
    );

    // Add the buttons to the layout
    controlLayout->addWidget(btn_dual_display);
    controlLayout->addWidget(btn_hold);
    controlLayout->addStretch();

    // Add the display and control panels to the main layout
    dmLayout->addWidget(displayPanel, 0, 0);
    dmLayout->addWidget(controlPanel, 1, 0);

    // Set the size policy for the digital multimeter panel
    dmPanel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    dmPanel->setMinimumHeight(250); // Increased height for better spacing
    dmPanel->setMaximumHeight(250); // Increased height for better spacing

    // Add digital multimeter panel to main layout
    mainLayout->addWidget(dmPanel, 0); // 0 stretch factor to keep it at fixed size

    // ================================================
    // COMPLETELY REFACTORED GRAPH IMPLEMENTATION
    // ================================================

    // Create chart with improved styling
    chart = new QChart();
    chart->setTitle("Measurement History");
    chart->setTitleFont(QFont("Arial", 12, QFont::Bold));
    chart->setAnimationOptions(QChart::NoAnimation); // Disable animations for better performance
    chart->setTheme(QChart::ChartThemeLight); // Use light theme for better visibility
    chart->setBackgroundVisible(true);
    chart->setBackgroundRoundness(5);
    chart->setMargins(QMargins(10, 10, 10, 10));

    // Create series with improved styling
    series = new QLineSeries();
    series->setName("Measurement");
    series->setPen(QPen(QColor(0, 114, 189), 2.5)); // Thicker blue line for better visibility
    series->setPointsVisible(false); // Hide points for smoother appearance
    chart->addSeries(series);

    // Create X axis (time) with improved properties
    axisX = new QValueAxis();
    axisX->setTitleText("Time (seconds)");
    axisX->setTitleFont(QFont("Arial", 10, QFont::Bold));
    axisX->setLabelFormat("%.1f"); // Show time with one decimal place
    axisX->setTickCount(6);
    axisX->setMinorTickCount(1);
    axisX->setLabelsAngle(0);
    axisX->setGridLineVisible(true);
    axisX->setGridLinePen(QPen(QColor("#e0e0e0"), 1, Qt::DashLine));
    axisX->setLabelsColor(QColor("#404040"));
    axisX->setLinePen(QPen(QColor("#808080"), 1));
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    // Create Y axis (value) with improved properties
    axisY = new QValueAxis();
    axisY->setTitleText("Value");
    axisY->setTitleFont(QFont("Arial", 10, QFont::Bold));
    axisY->setLabelFormat("%.3f");
    axisY->setTickCount(6);
    axisY->setMinorTickCount(1);
    axisY->setGridLineVisible(true);
    axisY->setGridLinePen(QPen(QColor("#e0e0e0"), 1, Qt::DashLine));
    axisY->setLabelsColor(QColor("#404040"));
    axisY->setLinePen(QPen(QColor("#808080"), 1));
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    // Set initial ranges
    axisX->setRange(0, 60); // 60 seconds
    axisY->setRange(-1, 1); // Default range

    // Add a legend with improved styling
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->legend()->setFont(QFont("Arial", 9));

    // Create chart view with improved rendering
    chartView = new QChartView(chart, centralwidget);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumHeight(100); // Minimum height
    chartView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding); // Expanding in both directions

    // Set up the chart to respect the window size
    chart->legend()->setAlignment(Qt::AlignBottom);
    chart->setMargins(QMargins(5, 5, 5, 5)); // Reduce margins to maximize space

    // Create graph scaling controls
    setupGraphScalingControls();

    // Create a grid layout for the buttons
    auto buttonLayout = new QGridLayout();
    buttonLayout->setSpacing(5);

    // ================================================
    // IMPROVED BUTTON CREATION WITH CHECKABLE BUTTONS
    // ================================================

    // Create buttons with improved styling and make them checkable
    QString buttonStyle =
        "QPushButton { color: #ffffff; font-weight: bold; background-color: #505050; "
        "border: 2px outset #707070; border-radius: 4px; padding: 4px; }"
        "QPushButton:hover { background-color: #606060; }"
        "QPushButton:pressed { border-style: inset; }"
        "QPushButton:checked { background-color: #008000; border-color: #00a000; }"
        "QPushButton:checked:hover { background-color: #009000; }";

    // Create measurement mode buttons with updated labels
    btn_50_v = new QPushButton("DC V", centralwidget);
    btn_50_v->setObjectName("btn_50_v");
    btn_50_v->setMinimumHeight(32);
    btn_50_v->setCheckable(true);
    btn_50_v->setStyleSheet(buttonStyle);

    btn_auto_v = new QPushButton("AC V", centralwidget);
    btn_auto_v->setObjectName("btn_auto_v");
    btn_auto_v->setMinimumHeight(32);
    btn_auto_v->setCheckable(true);
    btn_auto_v->setStyleSheet(buttonStyle);

    btn_short = new QPushButton("Temp", centralwidget);
    btn_short->setObjectName("btn_short");
    btn_short->setMinimumHeight(32);
    btn_short->setCheckable(true);
    btn_short->setStyleSheet(buttonStyle);

    btn_diode = new QPushButton("Current", centralwidget);
    btn_diode->setObjectName("btn_diode");
    btn_diode->setMinimumHeight(32);
    btn_diode->setCheckable(true);
    btn_diode->setStyleSheet(buttonStyle);

    btn_50_kr = new QPushButton("Ω", centralwidget);
    btn_50_kr->setObjectName("btn_50_kr");
    btn_50_kr->setMinimumHeight(32);
    btn_50_kr->setCheckable(true);
    btn_50_kr->setStyleSheet(buttonStyle);

    btn_auto_r = new QPushButton("Cont", centralwidget);
    btn_auto_r->setObjectName("btn_auto_r");
    btn_auto_r->setMinimumHeight(32);
    btn_auto_r->setCheckable(true);
    btn_auto_r->setStyleSheet(buttonStyle);

    btn_50_f = new QPushButton("Cap", centralwidget);
    btn_50_f->setObjectName("btn_50_f");
    btn_50_f->setMinimumHeight(32);
    btn_50_f->setCheckable(true);
    btn_50_f->setStyleSheet(buttonStyle);

    btn_auto_f = new QPushButton("Diode", centralwidget);
    btn_auto_f->setObjectName("btn_auto_f");
    btn_auto_f->setMinimumHeight(32);
    btn_auto_f->setCheckable(true);
    btn_auto_f->setStyleSheet(buttonStyle);

    btn_freq = new QPushButton("Freq", centralwidget);
    btn_freq->setObjectName("btn_freq");
    btn_freq->setMinimumHeight(32);
    btn_freq->setCheckable(true);
    btn_freq->setStyleSheet(buttonStyle);

    btn_period = new QPushButton("Period", centralwidget);
    btn_period->setObjectName("btn_period");
    btn_period->setMinimumHeight(32);
    btn_period->setCheckable(true);
    btn_period->setStyleSheet(buttonStyle);

    // Add buttons to the grid layout in a logical order
    // Voltage measurement buttons
    buttonLayout->addWidget(btn_50_v, 0, 0);
    buttonLayout->addWidget(btn_auto_v, 1, 0);

    // Resistance measurement buttons
    buttonLayout->addWidget(btn_50_kr, 0, 1);
    buttonLayout->addWidget(btn_auto_r, 1, 1);

    // Capacitance measurement buttons
    buttonLayout->addWidget(btn_50_f, 0, 2);
    buttonLayout->addWidget(btn_auto_f, 1, 2);

    // Frequency measurement buttons
    buttonLayout->addWidget(btn_freq, 0, 3);
    buttonLayout->addWidget(btn_period, 1, 3);

    // Special measurement buttons
    buttonLayout->addWidget(btn_short, 0, 4);
    buttonLayout->addWidget(btn_diode, 1, 4);

    // Dual display toggle button is now in the measurement area

    // Create a widget to contain the button layout with fixed height
    QWidget* buttonContainer = new QWidget(centralwidget);
    buttonContainer->setLayout(buttonLayout);
    buttonContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Fixed vertical size policy

    // Add button container to main layout
    mainLayout->addWidget(buttonContainer, 0); // 0 stretch factor to keep it at minimum size

    // Now add the chart view to the main layout with a stretch factor of 1
    mainLayout->addWidget(chartView, 1); // The stretch factor of 1 allows the chart to take available space

    // Setup Configuration Tab
    auto configLayout = new QVBoxLayout(configTab);
    configLayout->setSpacing(10);
    configLayout->setContentsMargins(10, 10, 10, 10);

    // Create System Configuration group
    auto systemGroup = new QGroupBox("System Configuration", configTab);
    auto systemLayout = new QGridLayout(systemGroup);

    // Create Beeper checkbox
    auto beeperCheckBox = new QCheckBox("Beeper", systemGroup);
    beeperCheckBox->setChecked(beeperEnabled);
    connect(beeperCheckBox, &QCheckBox::toggled, this, &MainWindow::onBeeperToggle);
    systemLayout->addWidget(beeperCheckBox, 0, 0);

    // Create Auto-Zero checkbox
    auto autoZeroCheckBox = new QCheckBox("Auto-Zero", systemGroup);
    autoZeroCheckBox->setChecked(autoZeroEnabled);
    connect(autoZeroCheckBox, &QCheckBox::toggled, this, &MainWindow::onAutoZeroToggle);
    systemLayout->addWidget(autoZeroCheckBox, 1, 0);

    // Create Remote/Local checkbox
    auto remoteLocalCheckBox = new QCheckBox("Remote Control", systemGroup);
    remoteLocalCheckBox->setChecked(remoteEnabled);
    connect(remoteLocalCheckBox, &QCheckBox::toggled, this, &MainWindow::onRemoteLocalToggle);
    systemLayout->addWidget(remoteLocalCheckBox, 2, 0);

    // Create Brightness control
    auto brightnessLabel = new QLabel("Display Brightness:", systemGroup);
    systemLayout->addWidget(brightnessLabel, 3, 0);

    brightnessSpinBox = new QSpinBox(systemGroup);
    brightnessSpinBox->setRange(0, 100);
    brightnessSpinBox->setValue(brightness);
    brightnessSpinBox->setSuffix("%");
    connect(brightnessSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onBrightnessChanged);
    systemLayout->addWidget(brightnessSpinBox, 3, 1);

    // Create Trigger Configuration group
    auto triggerGroup = new QGroupBox("Trigger Configuration", configTab);
    auto triggerLayout = new QGridLayout(triggerGroup);

    // Create Trigger Mode combo box
    auto triggerModeLabel = new QLabel("Trigger Mode:", triggerGroup);
    triggerLayout->addWidget(triggerModeLabel, 0, 0);

    triggerModeComboBox = new QComboBox(triggerGroup);
    triggerModeComboBox->addItem("Auto");
    triggerModeComboBox->addItem("Single");
    triggerModeComboBox->addItem("External");
    triggerModeComboBox->setCurrentIndex(triggerMode);
    connect(triggerModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTriggerModeChanged);
    triggerLayout->addWidget(triggerModeComboBox, 0, 1);

    // Create Trigger Delay spin box
    auto triggerDelayLabel = new QLabel("Trigger Delay (ms):", triggerGroup);
    triggerLayout->addWidget(triggerDelayLabel, 1, 0);

    triggerDelaySpinBox = new QSpinBox(triggerGroup);
    triggerDelaySpinBox->setRange(0, 3600000); // 0 to 1 hour in ms
    triggerDelaySpinBox->setValue(triggerDelay);
    triggerDelaySpinBox->setSuffix(" ms");
    connect(triggerDelaySpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTriggerDelayChanged);
    triggerLayout->addWidget(triggerDelaySpinBox, 1, 1);

    // Create Sample Count spin box
    auto sampleCountLabel = new QLabel("Sample Count:", triggerGroup);
    triggerLayout->addWidget(sampleCountLabel, 2, 0);

    sampleCountSpinBox = new QSpinBox(triggerGroup);
    sampleCountSpinBox->setRange(1, 50000);
    sampleCountSpinBox->setValue(sampleCount);
    connect(sampleCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onSampleCountChanged);
    triggerLayout->addWidget(sampleCountSpinBox, 2, 1);

    // Add groups to config layout
    configLayout->addWidget(systemGroup);
    configLayout->addWidget(triggerGroup);
    configLayout->addStretch();

    // Set size policy for tab widget
    tabWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed); // Fixed vertical size policy

    // Add tab widget to main layout
    mainLayout->addWidget(tabWidget, 0); // 0 stretch factor to keep it at minimum size

    // Hide the tab widget by default (can be shown from the menu)
    tabWidget->setVisible(false);

    MainWindow->setCentralWidget(centralwidget);

    // Setup menus
    setupMenus();

    m_connect_dialog = new ConnectDialog(this);

    // Initialize data logging
    isLogging = false;
    logFile = nullptr;

    QMetaObject::connectSlotsByName(MainWindow);
}

void MainWindow::setupMenus() {
    // Create Session menu
    sessionMenu = menuBar()->addMenu("Session");

    // Create Save Session action
    saveSessionAction = new QAction("Save Session", this);
    connect(saveSessionAction, &QAction::triggered, this, &MainWindow::onSaveSession);
    sessionMenu->addAction(saveSessionAction);

    // Create Load Session action
    loadSessionAction = new QAction("Load Session", this);
    connect(loadSessionAction, &QAction::triggered, this, &MainWindow::onLoadSession);
    sessionMenu->addAction(loadSessionAction);

    // Create Rate menu
    rateMenu = menuBar()->addMenu("Rate");

    // Create Rate actions
    rateFastAction = new QAction("Fast", this);
    rateMediumAction = new QAction("Medium", this);
    rateSlowAction = new QAction("Slow", this);

    // Make Rate actions checkable and add to menu
    rateFastAction->setCheckable(true);
    rateMediumAction->setCheckable(true);
    rateSlowAction->setCheckable(true);

    // Set Medium as default
    rateMediumAction->setChecked(true);

    // Connect Rate actions
    connect(rateFastAction, &QAction::triggered, this, &MainWindow::onRateFast);
    connect(rateMediumAction, &QAction::triggered, this, &MainWindow::onRateMedium);
    connect(rateSlowAction, &QAction::triggered, this, &MainWindow::onRateSlow);

    // Add Rate actions to menu
    rateMenu->addAction(rateFastAction);
    rateMenu->addAction(rateMediumAction);
    rateMenu->addAction(rateSlowAction);

    // Create Math menu
    mathMenu = menuBar()->addMenu("Math");

    // Create Math actions
    nullAction = new QAction("NULL (Relative)", this);
    dbAction = new QAction("dB", this);
    dbmAction = new QAction("dBm", this);
    minMaxAction = new QAction("Min/Max/Avg", this);
    limitTestingAction = new QAction("Limit Testing", this);

    // Make Math actions checkable
    nullAction->setCheckable(true);
    dbAction->setCheckable(true);
    dbmAction->setCheckable(true);
    minMaxAction->setCheckable(true);
    limitTestingAction->setCheckable(true);

    // Connect Math actions
    connect(nullAction, &QAction::triggered, this, &MainWindow::onNullToggle);
    connect(dbAction, &QAction::triggered, this, &MainWindow::onDbToggle);
    connect(dbmAction, &QAction::triggered, this, &MainWindow::onDbmToggle);
    connect(minMaxAction, &QAction::triggered, this, &MainWindow::onMinMaxToggle);
    connect(limitTestingAction, &QAction::triggered, this, &MainWindow::onLimitTestingToggle);

    // Add Math actions to menu
    mathMenu->addAction(nullAction);
    mathMenu->addAction(dbAction);
    mathMenu->addAction(dbmAction);
    mathMenu->addAction(minMaxAction);
    mathMenu->addAction(limitTestingAction);

    // Create Logging menu
    loggingMenu = menuBar()->addMenu("Logging");

    // Create Logging actions
    startLoggingAction = new QAction("Start Logging", this);
    stopLoggingAction = new QAction("Stop Logging", this);
    clearGraphAction = new QAction("Clear Graph", this);
    exportDataAction = new QAction("Export Data...", this);

    // Connect Logging actions
    connect(startLoggingAction, &QAction::triggered, this, &MainWindow::onStartLogging);
    connect(stopLoggingAction, &QAction::triggered, this, &MainWindow::onStopLogging);
    connect(clearGraphAction, &QAction::triggered, this, &MainWindow::onClearGraph);
    connect(exportDataAction, &QAction::triggered, this, &MainWindow::onExportData);

    // Add Logging actions to menu
    loggingMenu->addAction(startLoggingAction);
    loggingMenu->addAction(stopLoggingAction);
    loggingMenu->addSeparator();
    loggingMenu->addAction(clearGraphAction);
    loggingMenu->addAction(exportDataAction);

    // Initially disable stop logging action
    stopLoggingAction->setEnabled(false);

    // Create Advanced menu
    advancedMenu = menuBar()->addMenu("Advanced");

    // Create Advanced actions
    acDcAction = new QAction("AC+DC", this);
    temperatureAction = new QAction("Temperature", this);
    currentAction = new QAction("Current", this);
    wire2_4Action = new QAction("4-Wire Resistance", this);

    // Make Advanced actions checkable
    acDcAction->setCheckable(true);
    wire2_4Action->setCheckable(true);

    // Connect Advanced actions
    connect(acDcAction, &QAction::triggered, this, &MainWindow::onAcDcToggle);
    connect(temperatureAction, &QAction::triggered, this, &MainWindow::onTemperatureMode);
    connect(currentAction, &QAction::triggered, this, &MainWindow::onCurrentMode);
    connect(wire2_4Action, &QAction::triggered, this, &MainWindow::on2Wire4WireToggle);

    // Add Advanced actions to menu
    advancedMenu->addAction(acDcAction);
    advancedMenu->addAction(temperatureAction);
    advancedMenu->addAction(currentAction);
    advancedMenu->addAction(wire2_4Action);

    // Create System menu
    systemMenu = menuBar()->addMenu("System");

    // Create System actions
    beeperAction = new QAction("Beeper", this);
    autoZeroAction = new QAction("Auto-Zero", this);
    remoteLocalAction = new QAction("Remote Control", this);
    QAction *configTabAction = new QAction("Show Configuration Tab", this);
    QAction *configHotkeysAction = new QAction("Configure Hotkeys...", this);

    // Make System actions checkable
    beeperAction->setCheckable(true);
    autoZeroAction->setCheckable(true);
    remoteLocalAction->setCheckable(true);
    configTabAction->setCheckable(true);

    // Set default values
    beeperAction->setChecked(beeperEnabled);
    autoZeroAction->setChecked(autoZeroEnabled);
    remoteLocalAction->setChecked(remoteEnabled);
    configTabAction->setChecked(false);

    // Connect System actions
    connect(beeperAction, &QAction::triggered, this, &MainWindow::onBeeperToggle);
    connect(autoZeroAction, &QAction::triggered, this, &MainWindow::onAutoZeroToggle);
    connect(remoteLocalAction, &QAction::triggered, this, &MainWindow::onRemoteLocalToggle);
    connect(configTabAction, &QAction::triggered, this, &MainWindow::onToggleConfigTab);
    connect(configHotkeysAction, &QAction::triggered, this, &MainWindow::onConfigureHotkeys);

    // Add System actions to menu
    systemMenu->addAction(beeperAction);
    systemMenu->addAction(autoZeroAction);
    systemMenu->addAction(remoteLocalAction);
    systemMenu->addSeparator();
    systemMenu->addAction(configTabAction);
    systemMenu->addAction(configHotkeysAction);

    // Create Trigger menu
    triggerMenu = menuBar()->addMenu("Trigger");

    // Add trigger mode submenu
    QMenu *triggerModeMenu = triggerMenu->addMenu("Trigger Mode");

    // Create trigger mode actions
    QAction *autoTriggerAction = new QAction("Auto", this);
    QAction *singleTriggerAction = new QAction("Single", this);
    QAction *externalTriggerAction = new QAction("External", this);

    // Make trigger mode actions checkable and add to action group
    QActionGroup *triggerModeGroup = new QActionGroup(this);
    autoTriggerAction->setCheckable(true);
    singleTriggerAction->setCheckable(true);
    externalTriggerAction->setCheckable(true);
    triggerModeGroup->addAction(autoTriggerAction);
    triggerModeGroup->addAction(singleTriggerAction);
    triggerModeGroup->addAction(externalTriggerAction);

    // Set default trigger mode
    autoTriggerAction->setChecked(true);

    // Connect trigger mode actions
    connect(autoTriggerAction, &QAction::triggered, [this]() { onTriggerModeChanged(0); });
    connect(singleTriggerAction, &QAction::triggered, [this]() { onTriggerModeChanged(1); });
    connect(externalTriggerAction, &QAction::triggered, [this]() { onTriggerModeChanged(2); });

    // Add trigger mode actions to submenu
    triggerModeMenu->addAction(autoTriggerAction);
    triggerModeMenu->addAction(singleTriggerAction);
    triggerModeMenu->addAction(externalTriggerAction);

    // Add trigger delay action
    QAction *triggerDelayAction = new QAction("Set Trigger Delay...", this);
    connect(triggerDelayAction, &QAction::triggered, [this]() {
        // Create a dialog for trigger delay
        QDialog dialog(this);
        dialog.setWindowTitle("Trigger Delay");
        dialog.setFixedSize(300, 150);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QLabel *label = new QLabel("Enter trigger delay (ms):", &dialog);
        layout->addWidget(label);

        QSpinBox *spinBox = new QSpinBox(&dialog);
        spinBox->setRange(0, 10000);
        spinBox->setValue(triggerDelay);
        layout->addWidget(spinBox);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttonBox);

        if (dialog.exec() == QDialog::Accepted) {
            onTriggerDelayChanged(spinBox->value());
        }
    });
    triggerMenu->addAction(triggerDelayAction);

    // Add sample count action
    QAction *sampleCountAction = new QAction("Set Sample Count...", this);
    connect(sampleCountAction, &QAction::triggered, [this]() {
        // Create a dialog for sample count
        QDialog dialog(this);
        dialog.setWindowTitle("Sample Count");
        dialog.setFixedSize(300, 150);

        QVBoxLayout *layout = new QVBoxLayout(&dialog);

        QLabel *label = new QLabel("Enter sample count:", &dialog);
        layout->addWidget(label);

        QSpinBox *spinBox = new QSpinBox(&dialog);
        spinBox->setRange(1, 10000);
        spinBox->setValue(sampleCount);
        layout->addWidget(spinBox);

        QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        layout->addWidget(buttonBox);

        if (dialog.exec() == QDialog::Accepted) {
            onSampleCountChanged(spinBox->value());
        }
    });
    triggerMenu->addAction(sampleCountAction);
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
    connect(btn_dual_display, &QPushButton::toggled, this, &MainWindow::onToggleDualDisplay);
    connect(btn_hold, &QPushButton::toggled, this, &MainWindow::onToggleHold);
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    QMainWindow::resizeEvent(event); // Call base class implementation

    // Store the new window size in settings
    settings->setWindowWidth(event->size().width());
    settings->setWindowHeight(event->size().height());

    // Ensure the chart respects the window size
    if (chartView) {
        // Update chart to fit the new window size
        chartView->resize(chartView->parentWidget()->size());
        chartView->repaint();
    }
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
    // ================================================
    // COMPLETELY REWRITTEN MEASUREMENT UPDATE FUNCTION
    // ================================================

    // If hold is enabled, don't update the measurement display
    if (holdEnabled) {
        // In hold mode, we still need to read from the device to keep it responsive
        // but we don't update the display
        if (this->m_port) {
            this->m_port->write("MEAS?\n");
            if (this->m_port->waitForReadyRead(5000)) {
                char buffer[128];
                this->m_port->readLine(buffer, sizeof(buffer));
                // Just log the value but don't update the display
                std::cerr << "Hold mode active, received but not displaying: " << buffer << std::endl;
            }
        }
        return;
    }

    if (!this->m_port) {
        std::cerr << "Port is NULL, stopping timer" << std::endl;
        this->m_timer->stop();
        this->m_timer->deleteLater();
        this->m_timer = nullptr;
        return;
    }

    // Always use MEAS? to get measurement values
    // When dual display is enabled, this will return both values
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

    QString response = QString::fromLocal8Bit(buffer);
    QString primaryValue;
    QString secondaryValue;

    // Process the response based on dual display mode
    if (dualDisplayEnabled && response.contains(',')) {
        // In dual mode with comma-separated values
        QStringList values = response.split(',');
        if (values.size() >= 2) {
            // Extract both values
            primaryValue = values[0].trimmed();
            secondaryValue = values[1].trimmed();

            // Log the parsed values
            std::cerr << "Dual mode - Primary: " << primaryValue.toStdString()
                      << ", Secondary: " << secondaryValue.toStdString() << std::endl;

            // Update the displays
            measurement->setText(primaryValue);
            secondaryMeasurement->setText(secondaryValue);

            // Make sure secondary display and its label are visible
            secondaryMeasurement->setVisible(true);
            QWidget* parent = secondaryMeasurement->parentWidget();
            QList<QLabel*> labels = parent->findChildren<QLabel*>();
            for (QLabel* label : labels) {
                if (label->text() == "SECONDARY") {
                    label->setVisible(true);
                }
            }
        } else {
            // Only one value despite dual mode
            primaryValue = response.trimmed();
            std::cerr << "Dual mode but only one value received: " << primaryValue.toStdString() << std::endl;

            // Update primary display
            measurement->setText(primaryValue);

            // Set a placeholder for secondary display
            secondaryMeasurement->setText("---");
            secondaryMeasurement->setVisible(true);

            // Make sure secondary label is visible
            QWidget* parent = secondaryMeasurement->parentWidget();
            QList<QLabel*> labels = parent->findChildren<QLabel*>();
            for (QLabel* label : labels) {
                if (label->text() == "SECONDARY") {
                    label->setVisible(true);
                }
            }
        }
    } else {
        // Single display mode or no comma in the response
        primaryValue = response.trimmed();
        std::cerr << "Single mode - Value: " << primaryValue.toStdString() << std::endl;

        // Update primary display
        measurement->setText(primaryValue);

        // Hide secondary display and its label
        secondaryMeasurement->setVisible(false);
        QWidget* parent = secondaryMeasurement->parentWidget();
        QList<QLabel*> labels = parent->findChildren<QLabel*>();
        for (QLabel* label : labels) {
            if (label->text() == "SECONDARY") {
                label->setVisible(false);
            }
        }
    }

    // Force layout update to ensure proper display
    QWidget* displayPanel = measurement->parentWidget();
    if (displayPanel && displayPanel->layout()) {
        displayPanel->layout()->activate();
        displayPanel->update();
    }

    // Update graph with the primary value
    updateGraph(primaryValue);

    // If logging is enabled, write to log file
    if (isLogging && logFile && logFile->isOpen()) {
        QDateTime now = QDateTime::currentDateTime();
        QString timestamp = now.toString("yyyy-MM-dd hh:mm:ss.zzz");

        // Write to log file
        QTextStream stream(logFile);
        if (dualDisplayEnabled && !secondaryValue.isEmpty()) {
            stream << timestamp << "," << primaryValue << "," << secondaryValue << "\n";
        } else {
            stream << timestamp << "," << primaryValue << "\n";
        }

        // Flush to ensure data is written immediately
        stream.flush();
    }
}

// ================================================
// COMPLETELY REFACTORED BUTTON HANDLERS
// ================================================

// Helper function to uncheck all mode buttons except the active one
void MainWindow::uncheckAllModeButtons(QPushButton* activeButton) {
    // List of all mode buttons
    QList<QPushButton*> modeButtons = {
        btn_50_v, btn_auto_v, btn_50_kr, btn_auto_r,
        btn_50_f, btn_auto_f, btn_freq, btn_period,
        btn_short, btn_diode
    };

    // Uncheck all buttons except the active one
    for (QPushButton* button : modeButtons) {
        if (button != activeButton) {
            button->setChecked(false);
        }
    }

    // Make sure the active button is checked
    if (activeButton) {
        activeButton->setChecked(true);
    }
}

// Helper function to cycle through available ranges for a measurement mode
QString MainWindow::cycleRange(const QString& function) {
    // Check if this function has available ranges
    if (!availableRanges.contains(function)) {
        return "AUTO"; // Default to AUTO if no ranges defined
    }

    // Get the available ranges for this function
    QStringList ranges = availableRanges[function];

    // Get the current range index
    int currentIndex = currentRangeIndex.value(function, 0);

    // Increment the index, wrapping around if necessary
    currentIndex = (currentIndex + 1) % ranges.size();

    // Store the new index
    currentRangeIndex[function] = currentIndex;

    // Get the new range value
    QString rangeValue = ranges.at(currentIndex);

    // Format the range display text
    QString displayText;
    if (rangeValue == "AUTO") {
        displayText = "AUTO RANGE";
    } else {
        // Apply appropriate units based on the measurement function
        if (function == "VOLT:DC" || function == "VOLT:AC") {
            // Voltage ranges
            if (rangeValue.contains("E-3")) {
                displayText = rangeValue.replace("E-3", " mV");
            } else {
                displayText = rangeValue + " V";
            }
        } else if (function == "CURR:DC" || function == "CURR:AC") {
            // Current ranges
            if (rangeValue.contains("E-6")) {
                displayText = rangeValue.replace("E-6", " µA");
            } else if (rangeValue.contains("E-3")) {
                displayText = rangeValue.replace("E-3", " mA");
            } else {
                displayText = rangeValue + " A";
            }
        } else if (function == "RES" || function == "FRES") {
            // Resistance ranges
            if (rangeValue.contains("E6")) {
                displayText = rangeValue.replace("E6", " MΩ");
            } else if (rangeValue.contains("E3")) {
                displayText = rangeValue.replace("E3", " kΩ");
            } else {
                displayText = rangeValue + " Ω";
            }
        } else if (function == "CAP") {
            // Capacitance ranges
            if (rangeValue.contains("E-9")) {
                displayText = rangeValue.replace("E-9", " nF");
            } else if (rangeValue.contains("E-6")) {
                displayText = rangeValue.replace("E-6", " µF");
            } else if (rangeValue.contains("E-3")) {
                displayText = rangeValue.replace("E-3", " mF");
            } else {
                displayText = rangeValue + " F";
            }
        } else {
            // Default formatting for other functions
            displayText = rangeValue;
        }

        // Add "RANGE" suffix
        displayText += " RANGE";
    }

    // Log the range change
    std::cerr << "Cycling " << function.toStdString() << " range to " << rangeValue.toStdString() << std::endl;

    // Save the current range index
    settings->setValue(function + "_range_index", currentIndex);

    return displayText;
}

void MainWindow::onVoltage50V() {
    // ================================================
    // RANGE CYCLING BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("DC VOLTAGE");
    infoLabel->setText("DC MODE");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_50_v);

    // Set the current function
    currentFunction = "VOLT:DC";

    // Cycle through available ranges
    QString rangeDisplayText = cycleRange(currentFunction);
    rangeLabel->setText(rangeDisplayText);

    // Get the current range value
    QString rangeValue = availableRanges[currentFunction].at(currentRangeIndex[currentFunction]);

    // Set to DC voltage measurement with the selected range
    if (m_port) {
        m_port->write(QString("CONF:VOLT:DC %1\n").arg(rangeValue).toUtf8());

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"VOLT:DC\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to DC Voltage, " << rangeValue.toStdString() << " range" << std::endl;
}

void MainWindow::onVoltageAuto() {
    // ================================================
    // RANGE CYCLING BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("AC VOLTAGE");
    infoLabel->setText("AC MODE");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_auto_v);

    // Set the current function
    currentFunction = "VOLT:AC";

    // Cycle through available ranges
    QString rangeDisplayText = cycleRange(currentFunction);
    rangeLabel->setText(rangeDisplayText);

    // Get the current range value
    QString rangeValue = availableRanges[currentFunction].at(currentRangeIndex[currentFunction]);

    // Set to AC voltage measurement with the selected range
    if (m_port) {
        m_port->write(QString("CONF:VOLT:AC %1\n").arg(rangeValue).toUtf8());

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"VOLT:AC\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to AC Voltage, " << rangeValue.toStdString() << " range" << std::endl;
}

void MainWindow::onShort() {
    // ================================================
    // COMPLETELY REFACTORED BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("TEMPERATURE");
    rangeLabel->setText("AUTO RANGE");
    infoLabel->setText("CELSIUS");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_short);

    // Set to temperature measurement
    if (m_port) {
        m_port->write("CONF:TEMP\n");
        currentFunction = "TEMP";

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"TEMP\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to Temperature" << std::endl;
}

void MainWindow::onDiode() {
    // ================================================
    // RANGE CYCLING BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("CURRENT");
    infoLabel->setText("DC MODE");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_diode);

    // Set the current function
    currentFunction = "CURR:DC";

    // Cycle through available ranges
    QString rangeDisplayText = cycleRange(currentFunction);
    rangeLabel->setText(rangeDisplayText);

    // Get the current range value
    QString rangeValue = availableRanges[currentFunction].at(currentRangeIndex[currentFunction]);

    // Set to current measurement with the selected range
    if (m_port) {
        m_port->write(QString("CONF:CURR:DC %1\n").arg(rangeValue).toUtf8());

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"CURR:DC\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to DC Current, " << rangeValue.toStdString() << " range" << std::endl;
}

void MainWindow::onResistance50K() {
    // ================================================
    // RANGE CYCLING BUTTON HANDLER
    // ================================================

    // Determine which resistance function to use based on 2/4-wire setting
    QString function = wire4Enabled ? "FRES" : "RES";

    // Update the mode display
    modeLabel->setText("RESISTANCE");
    infoLabel->setText(wire4Enabled ? "4-WIRE" : "2-WIRE");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_50_kr);

    // Set the current function
    currentFunction = function;

    // Cycle through available ranges
    QString rangeDisplayText = cycleRange(function);
    rangeLabel->setText(rangeDisplayText);

    // Get the current range value
    QString rangeValue = availableRanges[function].at(currentRangeIndex[function]);

    // Set to resistance measurement with the selected range
    if (m_port) {
        m_port->write(QString("CONF:%1 %2\n").arg(function, rangeValue).toUtf8());

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write(QString("SENS:FUNC1 \"%1\"\n").arg(function).toUtf8());
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to " << (wire4Enabled ? "4-Wire " : "2-Wire ")
              << "Resistance, " << rangeValue.toStdString() << " range" << std::endl;
}

void MainWindow::onResistanceAuto() {
    // ================================================
    // COMPLETELY REFACTORED BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("CONTINUITY");
    rangeLabel->setText("FIXED RANGE");
    infoLabel->setText("BEEPER ON");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_auto_r);

    // Set to continuity test
    if (m_port) {
        m_port->write("CONF:CONT\n");
        currentFunction = "CONT";

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"CONT\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to Continuity" << std::endl;
}

void MainWindow::onCapacitance50uF() {
    // ================================================
    // RANGE CYCLING BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("CAPACITANCE");
    infoLabel->setText("");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_50_f);

    // Set the current function
    currentFunction = "CAP";

    // Cycle through available ranges
    QString rangeDisplayText = cycleRange(currentFunction);
    rangeLabel->setText(rangeDisplayText);

    // Get the current range value
    QString rangeValue = availableRanges[currentFunction].at(currentRangeIndex[currentFunction]);

    // Set to capacitance measurement with the selected range
    if (m_port) {
        m_port->write(QString("CONF:CAP %1\n").arg(rangeValue).toUtf8());

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"CAP\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to Capacitance, " << rangeValue.toStdString() << " range" << std::endl;
}

void MainWindow::onCapacitanceAuto() {
    // ================================================
    // COMPLETELY REFACTORED BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("DIODE");
    rangeLabel->setText("FIXED RANGE");
    infoLabel->setText("");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_auto_f);

    // Set to diode test
    if (m_port) {
        m_port->write("CONF:DIOD\n");
        currentFunction = "DIOD";

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"DIOD\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to Diode Test" << std::endl;
}

void MainWindow::onFrequency() {
    // ================================================
    // COMPLETELY REFACTORED BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("FREQUENCY");
    rangeLabel->setText("AUTO RANGE");
    infoLabel->setText("");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_freq);

    // Set to frequency measurement
    if (m_port) {
        m_port->write("CONF:FREQ\n");
        currentFunction = "FREQ";

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"FREQ\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to Frequency" << std::endl;
}

void MainWindow::onPeriod() {
    // ================================================
    // COMPLETELY REFACTORED BUTTON HANDLER
    // ================================================

    // Update the mode display
    modeLabel->setText("PERIOD");
    rangeLabel->setText("AUTO RANGE");
    infoLabel->setText("");

    // Uncheck all other mode buttons
    uncheckAllModeButtons(btn_period);

    // Set to period measurement
    if (m_port) {
        m_port->write("CONF:PER\n");
        currentFunction = "PER";

        // If dual display is enabled, set the primary function
        if (dualDisplayEnabled) {
            m_port->write("SENS:FUNC1 \"PER\"\n");
        }
    }

    // Log the mode change
    std::cerr << "Mode changed to Period" << std::endl;
}

void MainWindow::onToggleDualDisplay() {
    // ================================================
    // COMPLETELY REWRITTEN DUAL DISPLAY TOGGLE FUNCTION
    // ================================================

    // Debug output
    std::cerr << "Dual display toggled function called" << std::endl;

    // Update the dual display state based on the button's checked state
    dualDisplayEnabled = btn_dual_display->isChecked();
    std::cerr << "Dual display state: " << (dualDisplayEnabled ? "ON" : "OFF") << std::endl;

    // Find the secondary label
    QWidget* parent = secondaryMeasurement->parentWidget();
    QList<QLabel*> labels = parent->findChildren<QLabel*>();
    QLabel* secondaryLabel = nullptr;
    for (QLabel* label : labels) {
        if (label->text() == "SECONDARY") {
            secondaryLabel = label;
            break;
        }
    }

    if (dualDisplayEnabled) {
        // Enable dual display mode

        // Update button appearance
        btn_dual_display->setText("DUAL ON");
        btn_dual_display->setStyleSheet(
            "QPushButton { color: #ffffff; font-weight: bold; background-color: #008000; "
            "border: 2px outset #00a000; border-radius: 4px; padding: 4px; }"
            "QPushButton:hover { background-color: #009000; }"
            "QPushButton:pressed { border-style: inset; }"
        );

        // Show secondary display and label
        secondaryMeasurement->setVisible(true);
        if (secondaryLabel) {
            secondaryLabel->setVisible(true);
        }

        // Ensure the secondary measurement has a default value if empty
        if (secondaryMeasurement->text().isEmpty()) {
            secondaryMeasurement->setText("0.000");
        }

        // Enable dual display with frequency as secondary function if connected
        if (m_port) {
            m_port->write("DISP:DUAL ON\n"); // Turn on dual display mode
            m_port->write("SENS:FUNC1 \"" + currentFunction.toUtf8() + "\"\n"); // Set primary function
            m_port->write("SENS:FUNC2 \"FREQ\"\n"); // Set secondary function to frequency
            secondaryFunction = "FREQ";
            std::cerr << "Dual display enabled with Frequency as secondary function" << std::endl;
        } else {
            std::cerr << "Device not connected, but UI updated for dual display" << std::endl;
        }
    } else {
        // Disable dual display mode

        // Update button appearance
        btn_dual_display->setText("DUAL");
        btn_dual_display->setStyleSheet(
            "QPushButton { color: #ffffff; font-weight: bold; background-color: #505050; "
            "border: 2px outset #707070; border-radius: 4px; padding: 4px; }"
            "QPushButton:hover { background-color: #606060; }"
            "QPushButton:pressed { border-style: inset; }"
        );

        // Hide secondary display and label
        secondaryMeasurement->setVisible(false);
        if (secondaryLabel) {
            secondaryLabel->setVisible(false);
        }

        // Disable dual display if connected
        if (m_port) {
            m_port->write("DISP:DUAL OFF\n"); // Turn off dual display mode
            m_port->write("SENS:FUNC1 \"" + currentFunction.toUtf8() + "\"\n"); // Set primary function
            secondaryFunction = "NONE";
            std::cerr << "Dual display disabled on device" << std::endl;
        } else {
            std::cerr << "Device not connected, but UI updated for single display" << std::endl;
        }
    }

    // Debug the visibility state
    std::cerr << "Secondary measurement visibility: " << secondaryMeasurement->isVisible() << std::endl;
    std::cerr << "Button checked state: " << btn_dual_display->isChecked() << std::endl;
    std::cerr << "dualDisplayEnabled value: " << dualDisplayEnabled << std::endl;

    // Force layout update to ensure proper display
    QWidget* displayPanel = secondaryMeasurement->parentWidget();
    if (displayPanel && displayPanel->layout()) {
        displayPanel->layout()->activate();
        displayPanel->update();
    }
}

void MainWindow::onRateFast() {
    if (!m_port) return;

    m_port->write("RATE F\n");
    std::cerr << "Setting measurement rate to Fast" << std::endl;

    // Update menu checkboxes
    rateFastAction->setChecked(true);
    rateMediumAction->setChecked(false);
    rateSlowAction->setChecked(false);
}

void MainWindow::onRateMedium() {
    if (!m_port) return;

    m_port->write("RATE M\n");
    std::cerr << "Setting measurement rate to Medium" << std::endl;

    // Update menu checkboxes
    rateFastAction->setChecked(false);
    rateMediumAction->setChecked(true);
    rateSlowAction->setChecked(false);
}

void MainWindow::onRateSlow() {
    if (!m_port) return;

    m_port->write("RATE L\n");
    std::cerr << "Setting measurement rate to Slow" << std::endl;

    // Update menu checkboxes
    rateFastAction->setChecked(false);
    rateMediumAction->setChecked(false);
    rateSlowAction->setChecked(true);
}

void MainWindow::saveState() {
    // Save current settings to the Settings object
    settings->setValue("dualDisplayEnabled", dualDisplayEnabled);
    settings->setValue("holdEnabled", holdEnabled);
    settings->setValue("currentFunction", currentFunction);
    settings->setValue("secondaryFunction", secondaryFunction);

    // Save rate setting
    QString rate = "M"; // Default to Medium
    if (rateFastAction->isChecked()) rate = "F";
    if (rateSlowAction->isChecked()) rate = "L";
    settings->setValue("rate", rate);

    // Save math function settings
    settings->setValue("nullEnabled", nullEnabled);
    settings->setValue("dbEnabled", dbEnabled);
    settings->setValue("dbmEnabled", dbmEnabled);
    settings->setValue("minMaxEnabled", minMaxEnabled);
    settings->setValue("limitTestingEnabled", limitTestingEnabled);

    // Save advanced measurement settings
    settings->setValue("acDcEnabled", acDcEnabled);
    settings->setValue("wire4Enabled", wire4Enabled);

    // Save system configuration settings
    settings->setValue("beeperEnabled", beeperEnabled);
    settings->setValue("autoZeroEnabled", autoZeroEnabled);
    settings->setValue("remoteEnabled", remoteEnabled);
    settings->setValue("brightness", brightness);

    // Save trigger system settings
    settings->setValue("triggerMode", triggerMode);
    settings->setValue("triggerDelay", triggerDelay);
    settings->setValue("sampleCount", sampleCount);

    // Save logging settings
    settings->setValue("logFilePath", logFilePath);

    // Save graph scaling settings
    settings->setValue("autoScaleY", autoScaleY);
    settings->setValue("autoScaleX", autoScaleX);
    settings->setValue("manualMinY", manualMinY);
    settings->setValue("manualMaxY", manualMaxY);
    settings->setValue("manualTimeSpan", manualTimeSpan);
}

void MainWindow::loadState() {
    // Load settings from the Settings object
    dualDisplayEnabled = settings->value("dualDisplayEnabled", false).toBool();
    holdEnabled = settings->value("holdEnabled", false).toBool();
    currentFunction = settings->value("currentFunction", "VOLT:DC").toString();
    secondaryFunction = settings->value("secondaryFunction", "NONE").toString();

    // Load math function settings
    nullEnabled = settings->value("nullEnabled", false).toBool();
    dbEnabled = settings->value("dbEnabled", false).toBool();
    dbmEnabled = settings->value("dbmEnabled", false).toBool();
    minMaxEnabled = settings->value("minMaxEnabled", false).toBool();
    limitTestingEnabled = settings->value("limitTestingEnabled", false).toBool();

    // Load advanced measurement settings
    acDcEnabled = settings->value("acDcEnabled", false).toBool();
    wire4Enabled = settings->value("wire4Enabled", false).toBool();

    // Load system configuration settings
    beeperEnabled = settings->value("beeperEnabled", true).toBool();
    autoZeroEnabled = settings->value("autoZeroEnabled", true).toBool();
    remoteEnabled = settings->value("remoteEnabled", true).toBool();
    brightness = settings->value("brightness", 50).toInt();

    // Load trigger system settings
    triggerMode = settings->value("triggerMode", 0).toInt();
    triggerDelay = settings->value("triggerDelay", 0).toInt();
    sampleCount = settings->value("sampleCount", 1).toInt();

    // Load logging settings
    logFilePath = settings->value("logFilePath", "").toString();

    // Load graph scaling settings
    autoScaleY = settings->value("autoScaleY", true).toBool();
    autoScaleX = settings->value("autoScaleX", true).toBool();
    manualMinY = settings->value("manualMinY", -1.0).toDouble();
    manualMaxY = settings->value("manualMaxY", 1.0).toDouble();
    manualTimeSpan = settings->value("manualTimeSpan", 60000).toLongLong();

    // Load range indices
    currentRangeIndex["VOLT:DC"] = settings->value("VOLT:DC_range_index", 0).toInt();
    currentRangeIndex["VOLT:AC"] = settings->value("VOLT:AC_range_index", 0).toInt();
    currentRangeIndex["RES"] = settings->value("RES_range_index", 0).toInt();
    currentRangeIndex["CAP"] = settings->value("CAP_range_index", 0).toInt();
    currentRangeIndex["CURR:DC"] = settings->value("CURR:DC_range_index", 0).toInt();
    currentRangeIndex["CURR:AC"] = settings->value("CURR:AC_range_index", 0).toInt();

    // Update graph scaling controls if they exist
    if (autoScaleYCheckBox) {
        autoScaleYCheckBox->setChecked(autoScaleY);
    }
    if (autoScaleXCheckBox) {
        autoScaleXCheckBox->setChecked(autoScaleX);
    }
    if (minYSpinBox) {
        minYSpinBox->setValue(manualMinY);
        minYSpinBox->setEnabled(!autoScaleY);
    }
    if (maxYSpinBox) {
        maxYSpinBox->setValue(manualMaxY);
        maxYSpinBox->setEnabled(!autoScaleY);
    }
    if (timeSpanComboBox) {
        int index = timeSpanComboBox->findData(manualTimeSpan);
        if (index != -1) {
            timeSpanComboBox->setCurrentIndex(index);
        }
        timeSpanComboBox->setEnabled(!autoScaleX);
    }

    // Update UI to match loaded settings
    btn_dual_display->setChecked(dualDisplayEnabled);
    secondaryMeasurement->setVisible(dualDisplayEnabled);

    // Update hold button state
    btn_hold->setChecked(holdEnabled);
    if (holdEnabled) {
        btn_hold->setText("HOLD ON");
        // Add hold indicator to mode label
        modeLabel->setText(modeLabel->text() + " - HOLD");
    }

    // Update math function checkboxes
    nullAction->setChecked(nullEnabled);
    dbAction->setChecked(dbEnabled);
    dbmAction->setChecked(dbmEnabled);
    minMaxAction->setChecked(minMaxEnabled);
    limitTestingAction->setChecked(limitTestingEnabled);

    // Update advanced measurement checkboxes
    acDcAction->setChecked(acDcEnabled);
    wire2_4Action->setChecked(wire4Enabled);

    // Update system configuration checkboxes
    beeperAction->setChecked(beeperEnabled);
    autoZeroAction->setChecked(autoZeroEnabled);
    remoteLocalAction->setChecked(remoteEnabled);

    // Update configuration controls
    brightnessSpinBox->setValue(brightness);
    triggerModeComboBox->setCurrentIndex(triggerMode);
    triggerDelaySpinBox->setValue(triggerDelay);
    sampleCountSpinBox->setValue(sampleCount);

    // Apply settings to the device if connected
    if (m_port) {
        // Set the measurement function
        m_port->write("SENS:FUNC1 \"" + currentFunction.toUtf8() + "\"\n");

        // Set the dual display mode and secondary function if enabled
        if (dualDisplayEnabled) {
            m_port->write("DISP:DUAL ON\n"); // Turn on dual display mode
            m_port->write("SENS:FUNC2 \"" + secondaryFunction.toUtf8() + "\"\n");
        } else {
            m_port->write("DISP:DUAL OFF\n"); // Turn off dual display mode
            m_port->write("SENS:FUNC2 \"NONE\"\n");
        }

        // Set the rate
        QString rate = settings->value("rate", "M").toString();
        m_port->write("RATE " + rate.toUtf8() + "\n");

        // Update rate menu checkboxes
        rateFastAction->setChecked(rate == "F");
        rateMediumAction->setChecked(rate == "M");
        rateSlowAction->setChecked(rate == "L");

        // Apply math function settings
        if (nullEnabled) {
            m_port->write("CALC:NULL:STAT ON\n");
        } else {
            m_port->write("CALC:NULL:STAT OFF\n");
        }

        if (dbEnabled) {
            m_port->write("CALC:DB:STAT ON\n");
        } else {
            m_port->write("CALC:DB:STAT OFF\n");
        }

        if (dbmEnabled) {
            m_port->write("CALC:DBM:STAT ON\n");
        } else {
            m_port->write("CALC:DBM:STAT OFF\n");
        }

        if (minMaxEnabled) {
            m_port->write("CALC:STAT:STAT ON\n");
        } else {
            m_port->write("CALC:STAT:STAT OFF\n");
        }

        if (limitTestingEnabled) {
            m_port->write("CALC:LIM:STAT ON\n");
        } else {
            m_port->write("CALC:LIM:STAT OFF\n");
        }

        // Apply advanced measurement settings
        if (acDcEnabled) {
            m_port->write("SENS:FUNC:ACDC ON\n");
        } else {
            m_port->write("SENS:FUNC:ACDC OFF\n");
        }

        // Apply system configuration settings
        if (beeperEnabled) {
            m_port->write("SYST:BEEP:STAT ON\n");
        } else {
            m_port->write("SYST:BEEP:STAT OFF\n");
        }

        if (autoZeroEnabled) {
            m_port->write("SENS:ZERO:AUTO ON\n");
        } else {
            m_port->write("SENS:ZERO:AUTO OFF\n");
        }

        if (remoteEnabled) {
            m_port->write("SYST:REM\n");
        } else {
            m_port->write("SYST:LOC\n");
        }

        // Set display brightness
        m_port->write(QString("DISP:BRIGHT %1\n").arg(brightness).toUtf8());

        // Set trigger settings
        switch (triggerMode) {
            case 0: // Auto
                m_port->write("TRIG:SOUR IMM\n");
                break;
            case 1: // Single
                m_port->write("TRIG:SOUR BUS\n");
                break;
            case 2: // External
                m_port->write("TRIG:SOUR EXT\n");
                break;
        }

        // Set trigger delay
        double seconds = triggerDelay / 1000.0;
        m_port->write(QString("TRIG:DEL %1\n").arg(seconds, 0, 'f', 3).toUtf8());

        // Set sample count
        m_port->write(QString("SAMP:COUN %1\n").arg(sampleCount).toUtf8());
    }
}

void MainWindow::onSaveSession() {
    // Ask user for session file location
    QString fileName = QFileDialog::getSaveFileName(this, "Save Session", "", "Session Files (*.session);;All Files (*)");
    if (fileName.isEmpty()) return; // User canceled

    // Ensure file has .session extension
    if (!fileName.endsWith(".session", Qt::CaseInsensitive)) {
        fileName += ".session";
    }

    // Create a QSettings object for the session file
    QSettings sessionFile(fileName, QSettings::IniFormat);

    // Save all current settings to the session file
    sessionFile.setValue("dualDisplayEnabled", dualDisplayEnabled);
    sessionFile.setValue("holdEnabled", holdEnabled);
    sessionFile.setValue("currentFunction", currentFunction);
    sessionFile.setValue("secondaryFunction", secondaryFunction);

    // Save math function states
    sessionFile.setValue("nullEnabled", nullEnabled);
    sessionFile.setValue("dbEnabled", dbEnabled);
    sessionFile.setValue("dbmEnabled", dbmEnabled);
    sessionFile.setValue("minMaxEnabled", minMaxEnabled);
    sessionFile.setValue("limitTestingEnabled", limitTestingEnabled);

    // Save advanced measurement states
    sessionFile.setValue("acDcEnabled", acDcEnabled);
    sessionFile.setValue("wire4Enabled", wire4Enabled);

    // Save system configuration states
    sessionFile.setValue("beeperEnabled", beeperEnabled);
    sessionFile.setValue("autoZeroEnabled", autoZeroEnabled);
    sessionFile.setValue("remoteEnabled", remoteEnabled);
    sessionFile.setValue("brightness", brightness);

    // Save trigger system states
    sessionFile.setValue("triggerMode", triggerMode);
    sessionFile.setValue("triggerDelay", triggerDelay);
    sessionFile.setValue("sampleCount", sampleCount);

    // Save graph scaling states
    sessionFile.setValue("autoScaleY", autoScaleY);
    sessionFile.setValue("autoScaleX", autoScaleX);
    sessionFile.setValue("manualMinY", manualMinY);
    sessionFile.setValue("manualMaxY", manualMaxY);
    sessionFile.setValue("manualTimeSpan", manualTimeSpan);

    // Save range cycling states
    QStringList functions = availableRanges.keys();
    for (const QString& function : functions) {
        sessionFile.setValue(function + "_range_index", currentRangeIndex.value(function, 0));
    }

    // Notify the user
    QMessageBox::information(this, "Session Saved", "Session has been saved to:\n" + fileName);
}

void MainWindow::onLoadSession() {
    // Ask user for session file location
    QString fileName = QFileDialog::getOpenFileName(this, "Load Session", "", "Session Files (*.session);;All Files (*)");
    if (fileName.isEmpty()) return; // User canceled

    // Create a QSettings object for the session file
    QSettings sessionFile(fileName, QSettings::IniFormat);

    // Load settings from the session file
    dualDisplayEnabled = sessionFile.value("dualDisplayEnabled", false).toBool();
    holdEnabled = sessionFile.value("holdEnabled", false).toBool();
    currentFunction = sessionFile.value("currentFunction", "VOLT:DC").toString();
    secondaryFunction = sessionFile.value("secondaryFunction", "NONE").toString();

    // Load math function states
    nullEnabled = sessionFile.value("nullEnabled", false).toBool();
    dbEnabled = sessionFile.value("dbEnabled", false).toBool();
    dbmEnabled = sessionFile.value("dbmEnabled", false).toBool();
    minMaxEnabled = sessionFile.value("minMaxEnabled", false).toBool();
    limitTestingEnabled = sessionFile.value("limitTestingEnabled", false).toBool();

    // Load advanced measurement states
    acDcEnabled = sessionFile.value("acDcEnabled", false).toBool();
    wire4Enabled = sessionFile.value("wire4Enabled", false).toBool();

    // Load system configuration states
    beeperEnabled = sessionFile.value("beeperEnabled", true).toBool();
    autoZeroEnabled = sessionFile.value("autoZeroEnabled", true).toBool();
    remoteEnabled = sessionFile.value("remoteEnabled", true).toBool();
    brightness = sessionFile.value("brightness", 50).toInt();

    // Load trigger system states
    triggerMode = sessionFile.value("triggerMode", 0).toInt();
    triggerDelay = sessionFile.value("triggerDelay", 0).toInt();
    sampleCount = sessionFile.value("sampleCount", 1).toInt();

    // Load graph scaling states
    autoScaleY = sessionFile.value("autoScaleY", true).toBool();
    autoScaleX = sessionFile.value("autoScaleX", true).toBool();
    manualMinY = sessionFile.value("manualMinY", -1.0).toDouble();
    manualMaxY = sessionFile.value("manualMaxY", 1.0).toDouble();
    manualTimeSpan = sessionFile.value("manualTimeSpan", 60000).toLongLong();

    // Load range cycling states
    QStringList functions = availableRanges.keys();
    for (const QString& function : functions) {
        currentRangeIndex[function] = sessionFile.value(function + "_range_index", 0).toInt();
    }

    // Update UI to match loaded settings
    btn_dual_display->setChecked(dualDisplayEnabled);
    secondaryMeasurement->setVisible(dualDisplayEnabled);
    if (secondaryLabel) {
        secondaryLabel->setVisible(dualDisplayEnabled);
    }

    // Update hold button state
    btn_hold->setChecked(holdEnabled);
    if (holdEnabled) {
        btn_hold->setText("HOLD ON");
        // Add hold indicator to mode label
        modeLabel->setText(modeLabel->text() + " - HOLD");
    }

    // Update menu actions
    nullAction->setChecked(nullEnabled);
    dbAction->setChecked(dbEnabled);
    dbmAction->setChecked(dbmEnabled);
    minMaxAction->setChecked(minMaxEnabled);
    limitTestingAction->setChecked(limitTestingEnabled);
    acDcAction->setChecked(acDcEnabled);
    wire2_4Action->setChecked(wire4Enabled);
    beeperAction->setChecked(beeperEnabled);
    autoZeroAction->setChecked(autoZeroEnabled);
    remoteLocalAction->setChecked(remoteEnabled);

    // Update graph scaling controls
    if (autoScaleYCheckBox) autoScaleYCheckBox->setChecked(autoScaleY);
    if (autoScaleXCheckBox) autoScaleXCheckBox->setChecked(autoScaleX);
    if (minYSpinBox) {
        minYSpinBox->setValue(manualMinY);
        minYSpinBox->setEnabled(!autoScaleY);
    }
    if (maxYSpinBox) {
        maxYSpinBox->setValue(manualMaxY);
        maxYSpinBox->setEnabled(!autoScaleY);
    }
    if (timeSpanComboBox) {
        int index = timeSpanComboBox->findData(manualTimeSpan);
        if (index != -1) {
            timeSpanComboBox->setCurrentIndex(index);
        }
        timeSpanComboBox->setEnabled(!autoScaleX);
    }

    // Update trigger controls
    if (triggerModeComboBox) triggerModeComboBox->setCurrentIndex(triggerMode);
    if (triggerDelaySpinBox) triggerDelaySpinBox->setValue(triggerDelay);
    if (sampleCountSpinBox) sampleCountSpinBox->setValue(sampleCount);

    // Apply settings to the device if connected
    if (m_port) {
        // Apply dual display setting
        if (dualDisplayEnabled) {
            m_port->write("DISP:DUAL ON\n");
            m_port->write(QString("SENS:FUNC2 \"%1\"\n").arg(secondaryFunction).toUtf8());
        } else {
            m_port->write("DISP:DUAL OFF\n");
            m_port->write("SENS:FUNC2 \"NONE\"\n");
        }

        // Apply current function
        m_port->write(QString("CONF:%1\n").arg(currentFunction).toUtf8());

        // Apply math function settings
        m_port->write(QString("CALC:NULL:STAT %1\n").arg(nullEnabled ? "ON" : "OFF").toUtf8());
        m_port->write(QString("CALC:DB:STAT %1\n").arg(dbEnabled ? "ON" : "OFF").toUtf8());
        m_port->write(QString("CALC:DBM:STAT %1\n").arg(dbmEnabled ? "ON" : "OFF").toUtf8());
        m_port->write(QString("CALC:STAT:STAT %1\n").arg(minMaxEnabled ? "ON" : "OFF").toUtf8());
        m_port->write(QString("CALC:LIM:STAT %1\n").arg(limitTestingEnabled ? "ON" : "OFF").toUtf8());

        // Apply advanced measurement settings
        m_port->write(QString("SENS:FUNC:ACDC %1\n").arg(acDcEnabled ? "ON" : "OFF").toUtf8());

        // Apply system configuration settings
        m_port->write(QString("SYST:BEEP:STAT %1\n").arg(beeperEnabled ? "ON" : "OFF").toUtf8());
        m_port->write(QString("SENS:ZERO:AUTO %1\n").arg(autoZeroEnabled ? "ON" : "OFF").toUtf8());
        m_port->write(QString("SYST:%1\n").arg(remoteEnabled ? "REM" : "LOC").toUtf8());
        m_port->write(QString("DISP:BRIGHT %1\n").arg(brightness).toUtf8());

        // Apply trigger settings
        switch (triggerMode) {
            case 0: // Auto
                m_port->write("TRIG:SOUR IMM\n");
                break;
            case 1: // Single
                m_port->write("TRIG:SOUR BUS\n");
                break;
            case 2: // External
                m_port->write("TRIG:SOUR EXT\n");
                break;
        }
        m_port->write(QString("TRIG:DEL %1\n").arg(triggerDelay / 1000.0).toUtf8());
        m_port->write(QString("SAMP:COUN %1\n").arg(sampleCount).toUtf8());
    }

    // Notify the user
    QMessageBox::information(this, "Session Loaded", "Session has been loaded from:\n" + fileName);
}

void MainWindow::onNullToggle() {
    if (!m_port) return;

    nullEnabled = nullAction->isChecked();

    if (nullEnabled) {
        // Enable NULL (relative) measurement
        m_port->write("CALC:NULL:STAT ON\n");
        m_port->write("CALC:NULL:ACQ\n"); // Acquire current reading as reference
        std::cerr << "NULL (relative) measurement enabled" << std::endl;
    } else {
        // Disable NULL measurement
        m_port->write("CALC:NULL:STAT OFF\n");
        std::cerr << "NULL (relative) measurement disabled" << std::endl;
    }
}

void MainWindow::onDbToggle() {
    if (!m_port) return;

    dbEnabled = dbAction->isChecked();

    // Ensure dBm is turned off if dB is turned on
    if (dbEnabled && dbmEnabled) {
        dbmEnabled = false;
        dbmAction->setChecked(false);
        m_port->write("CALC:DBM:STAT OFF\n");
    }

    if (dbEnabled) {
        // Enable dB calculation
        m_port->write("CALC:DB:STAT ON\n");
        std::cerr << "dB calculation enabled" << std::endl;
    } else {
        // Disable dB calculation
        m_port->write("CALC:DB:STAT OFF\n");
        std::cerr << "dB calculation disabled" << std::endl;
    }
}

void MainWindow::onDbmToggle() {
    if (!m_port) return;

    dbmEnabled = dbmAction->isChecked();

    // Ensure dB is turned off if dBm is turned on
    if (dbmEnabled && dbEnabled) {
        dbEnabled = false;
        dbAction->setChecked(false);
        m_port->write("CALC:DB:STAT OFF\n");
    }

    if (dbmEnabled) {
        // Enable dBm calculation
        m_port->write("CALC:DBM:STAT ON\n");
        std::cerr << "dBm calculation enabled" << std::endl;
    } else {
        // Disable dBm calculation
        m_port->write("CALC:DBM:STAT OFF\n");
        std::cerr << "dBm calculation disabled" << std::endl;
    }
}

void MainWindow::onMinMaxToggle() {
    if (!m_port) return;

    minMaxEnabled = minMaxAction->isChecked();

    if (minMaxEnabled) {
        // Enable Min/Max/Avg statistics
        m_port->write("CALC:STAT:STAT ON\n");
        std::cerr << "Min/Max/Avg statistics enabled" << std::endl;
    } else {
        // Disable Min/Max/Avg statistics
        m_port->write("CALC:STAT:STAT OFF\n");
        std::cerr << "Min/Max/Avg statistics disabled" << std::endl;
    }
}

void MainWindow::onLimitTestingToggle() {
    if (!m_port) return;

    limitTestingEnabled = limitTestingAction->isChecked();

    if (limitTestingEnabled) {
        // Enable limit testing
        m_port->write("CALC:LIM:STAT ON\n");
        std::cerr << "Limit testing enabled" << std::endl;
    } else {
        // Disable limit testing
        m_port->write("CALC:LIM:STAT OFF\n");
        std::cerr << "Limit testing disabled" << std::endl;
    }
}

// Data Logging and Visualization
void MainWindow::updateGraph(const QString &value) {
    // Try to convert the value to a number
    bool ok;
    double numericValue = value.toDouble(&ok);

    // If conversion failed, try to extract a number from the string
    if (!ok) {
        // Extract numeric part using a regular expression
        QRegExp rx("([\\-\\+]?\\d+\\.?\\d*)");
        if (rx.indexIn(value) != -1) {
            QString numericString = rx.cap(1);
            numericValue = numericString.toDouble(&ok);
        }
    }

    // If we have a valid numeric value, add it to the graph
    if (ok) {
        // Get current time
        QDateTime now = QDateTime::currentDateTime();
        qint64 msecsSinceEpoch = now.toMSecsSinceEpoch();

        // Initialize start time if this is the first point
        static qint64 startTime = 0;
        if (series->count() == 0 || dataPoints.isEmpty()) {
            startTime = msecsSinceEpoch;
        }

        // Calculate relative time in seconds
        double relativeTimeSeconds = (msecsSinceEpoch - startTime) / 1000.0;

        // Add point to series with relative time
        series->append(relativeTimeSeconds, numericValue);

        // Add to data points list for export (still store absolute time for export)
        dataPoints.append(QPointF(msecsSinceEpoch, numericValue));

        // Determine how many points to keep based on measurement intensity
        // For high-frequency measurements, keep more points
        int maxPoints = 200; // Default max points

        // Keep only the last maxPoints to avoid performance issues
        while (series->count() > maxPoints) {
            series->remove(0);
            // Also remove from dataPoints to keep them in sync
            if (!dataPoints.isEmpty()) {
                dataPoints.removeFirst();
            }
        }

        // Handle X-axis scaling based on auto/manual setting
        double timeSpanSeconds;

        if (autoScaleX) {
            // Auto-scale X-axis based on data
            timeSpanSeconds = 60.0; // Default 60 seconds

            // Calculate rate of change
            if (series->count() >= 2) {
                double totalChange = 0;
                int sampleSize = std::min(10, series->count() - 1);
                for (int i = series->count() - 1; i > series->count() - 1 - sampleSize; i--) {
                    if (i > 0) {
                        double change = std::abs(series->at(i).y() - series->at(i-1).y());
                        totalChange += change;
                    }
                }
                double avgChange = totalChange / sampleSize;

                // Adjust time span based on rate of change
                // Higher rate of change = shorter time span for better detail
                if (avgChange > 1.0) {
                    timeSpanSeconds = 30.0; // 30 seconds for rapidly changing values
                } else if (avgChange < 0.1) {
                    timeSpanSeconds = 120.0; // 2 minutes for slowly changing values
                }
            }
        } else {
            // Use manual time span (convert from ms to seconds)
            timeSpanSeconds = manualTimeSpan / 1000.0;
        }

        // Calculate the visible time range based on the current time span
        double visibleStartTime;
        double visibleEndTime;

        if (relativeTimeSeconds <= timeSpanSeconds) {
            // If we haven't collected enough data to fill the time span yet,
            // show from 0 to the time span
            visibleStartTime = 0;
            visibleEndTime = timeSpanSeconds;
        } else {
            // Otherwise, show the most recent time span
            visibleStartTime = relativeTimeSeconds - timeSpanSeconds;
            visibleEndTime = relativeTimeSeconds;
        }

        // Update X axis range - using relative time in seconds
        axisX->setRange(visibleStartTime, visibleEndTime);

        // Update X-axis segments based on time span
        updateXAxisSegments(timeSpanSeconds);

        // Handle Y-axis scaling based on auto/manual setting
        if (autoScaleY) {
            // Auto-scale Y-axis based on data
            if (series->count() > 0) {
                // Find min and max Y values in the visible range
                double minY = std::numeric_limits<double>::max();
                double maxY = std::numeric_limits<double>::lowest();

                for (int i = 0; i < series->count(); ++i) {
                    QPointF point = series->at(i);
                    if (point.x() >= visibleStartTime) {
                        minY = std::min(minY, point.y());
                        maxY = std::max(maxY, point.y());
                    }
                }

                // Handle case where min and max are the same (flat line)
                if (qFuzzyCompare(minY, maxY)) {
                    // Add some range around the single value
                    double value = minY;
                    double range = std::max(0.1, std::abs(value * 0.1));
                    minY = value - range;
                    maxY = value + range;
                }

                // Calculate padding based on the range and intensity
                double range = maxY - minY;
                double padding;

                if (range < 0.001) {
                    // Very small range, use fixed padding
                    padding = 0.001;
                } else if (range < 0.1) {
                    // Small range, use 20% padding
                    padding = range * 0.2;
                } else if (range < 1.0) {
                    // Medium range, use 15% padding
                    padding = range * 0.15;
                } else {
                    // Large range, use 10% padding
                    padding = range * 0.1;
                }

                // Set Y axis range with padding
                axisY->setRange(minY - padding, maxY + padding);

                // Update the manual min/max values to match the current auto-scaled range
                // This makes switching between auto and manual smoother
                manualMinY = axisY->min();
                manualMaxY = axisY->max();

                // Update the spin boxes without triggering their signals
                minYSpinBox->blockSignals(true);
                maxYSpinBox->blockSignals(true);
                minYSpinBox->setValue(manualMinY);
                maxYSpinBox->setValue(manualMaxY);
                minYSpinBox->blockSignals(false);
                maxYSpinBox->blockSignals(false);

                // Update Y-axis segments based on range
                updateYAxisSegments(manualMinY, manualMaxY);
            }
        } else {
            // Use manual Y-axis range
            axisY->setRange(manualMinY, manualMaxY);

            // Update Y-axis segments based on range
            updateYAxisSegments(manualMinY, manualMaxY);
        }

        // Force chart update
        chart->update();
    }
}

void MainWindow::onStartLogging() {
    if (isLogging) return; // Already logging

    // Ask user for log file location
    QString fileName = QFileDialog::getSaveFileName(this, "Save Log File", "", "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return; // User canceled

    // Ensure file has .csv extension
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }

    // Open log file
    logFile = new QFile(fileName);
    if (!logFile->open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not open log file for writing.");
        delete logFile;
        logFile = nullptr;
        return;
    }

    // Write header
    QTextStream stream(logFile);
    if (dualDisplayEnabled) {
        stream << "Timestamp,Primary Value,Secondary Value\n";
    } else {
        stream << "Timestamp,Value\n";
    }

    // Update state
    isLogging = true;
    logFilePath = fileName;

    // Update UI
    startLoggingAction->setEnabled(false);
    stopLoggingAction->setEnabled(true);

    // Notify user
    QMessageBox::information(this, "Logging Started", "Data logging has been started to:\n" + fileName);
}

void MainWindow::onStopLogging() {
    if (!isLogging) return; // Not logging

    // Close log file
    if (logFile) {
        logFile->close();
        delete logFile;
        logFile = nullptr;
    }

    // Update state
    isLogging = false;

    // Update UI
    startLoggingAction->setEnabled(true);
    stopLoggingAction->setEnabled(false);

    // Notify user
    QMessageBox::information(this, "Logging Stopped", "Data logging has been stopped.");
}

void MainWindow::onClearGraph() {
    // Clear series
    series->clear();

    // Clear data points
    dataPoints.clear();

    // Reset axes for relative time (0 to 60 seconds)
    axisX->setRange(0, 60); // Show 0 to 60 seconds
    axisY->setRange(-1, 1); // Default Y range

    // Reset axis format
    axisX->setLabelFormat("%.1f"); // One decimal place
    axisX->setTickCount(5);

    // Force the chart to update
    chart->update();
}

void MainWindow::onExportData() {
    if (dataPoints.isEmpty()) {
        QMessageBox::information(this, "No Data", "There is no data to export.");
        return;
    }

    // Ask user for export file location
    QString fileName = QFileDialog::getSaveFileName(this, "Export Data", "", "CSV Files (*.csv);;All Files (*)");
    if (fileName.isEmpty()) return; // User canceled

    // Ensure file has .csv extension
    if (!fileName.endsWith(".csv", Qt::CaseInsensitive)) {
        fileName += ".csv";
    }

    // Open export file
    QFile exportFile(fileName);
    if (!exportFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, "Error", "Could not open export file for writing.");
        return;
    }

    // Write data
    QTextStream stream(&exportFile);
    stream << "Timestamp,Value\n";

    for (const QPointF &point : dataPoints) {
        QDateTime timestamp = QDateTime::fromMSecsSinceEpoch(point.x());
        stream << timestamp.toString("yyyy-MM-dd hh:mm:ss.zzz") << "," << point.y() << "\n";
    }

    // Close file
    exportFile.close();

    // Notify user
    QMessageBox::information(this, "Export Complete", "Data has been exported to:\n" + fileName);
}

// Advanced Measurement Options
void MainWindow::onAcDcToggle() {
    if (!m_port) return;

    acDcEnabled = acDcAction->isChecked();

    if (acDcEnabled) {
        // Enable AC+DC measurement
        m_port->write("SENS:FUNC:ACDC ON\n");
        std::cerr << "AC+DC measurement enabled" << std::endl;
    } else {
        // Disable AC+DC measurement
        m_port->write("SENS:FUNC:ACDC OFF\n");
        std::cerr << "AC+DC measurement disabled" << std::endl;
    }
}

void MainWindow::onTemperatureMode() {
    if (!m_port) return;

    // Switch to temperature measurement mode
    m_port->write("CONF:TEMP\n");
    std::cerr << "Setting to Temperature measurement" << std::endl;
    currentFunction = "TEMP";
}

void MainWindow::onCurrentMode() {
    if (!m_port) return;

    // Switch to current measurement mode
    m_port->write("CONF:CURR:DC AUTO\n");
    std::cerr << "Setting to Current measurement" << std::endl;
    currentFunction = "CURR:DC";
}

void MainWindow::on2Wire4WireToggle() {
    if (!m_port) return;

    wire4Enabled = wire2_4Action->isChecked();

    if (wire4Enabled) {
        // Enable 4-wire resistance measurement
        m_port->write("CONF:FRES AUTO\n");
        std::cerr << "4-wire resistance measurement enabled" << std::endl;
        currentFunction = "FRES";

        // Update the mode display
        modeLabel->setText("RESISTANCE");
        rangeLabel->setText("AUTO RANGE");
        infoLabel->setText("4-WIRE");

        // Reset range index for 4-wire resistance
        currentRangeIndex["FRES"] = 0;
    } else {
        // Enable 2-wire resistance measurement
        m_port->write("CONF:RES AUTO\n");
        std::cerr << "2-wire resistance measurement enabled" << std::endl;
        currentFunction = "RES";

        // Update the mode display
        modeLabel->setText("RESISTANCE");
        rangeLabel->setText("AUTO RANGE");
        infoLabel->setText("2-WIRE");

        // Reset range index for 2-wire resistance
        currentRangeIndex["RES"] = 0;
    }
}

// System Configuration
void MainWindow::onBeeperToggle() {
    if (!m_port) return;

    beeperEnabled = beeperAction->isChecked();

    if (beeperEnabled) {
        // Enable beeper
        m_port->write("SYST:BEEP:STAT ON\n");
        std::cerr << "Beeper enabled" << std::endl;
    } else {
        // Disable beeper
        m_port->write("SYST:BEEP:STAT OFF\n");
        std::cerr << "Beeper disabled" << std::endl;
    }
}

void MainWindow::onBrightnessChanged(int value) {
    if (!m_port) return;

    brightness = value;

    // Set display brightness (0-100%)
    m_port->write(QString("DISP:BRIGHT %1\n").arg(value).toUtf8());
    std::cerr << "Display brightness set to " << value << "%" << std::endl;
}

void MainWindow::onAutoZeroToggle() {
    if (!m_port) return;

    autoZeroEnabled = autoZeroAction->isChecked();

    if (autoZeroEnabled) {
        // Enable auto-zero
        m_port->write("SENS:ZERO:AUTO ON\n");
        std::cerr << "Auto-zero enabled" << std::endl;
    } else {
        // Disable auto-zero
        m_port->write("SENS:ZERO:AUTO OFF\n");
        std::cerr << "Auto-zero disabled" << std::endl;
    }
}

void MainWindow::onRemoteLocalToggle() {
    if (!m_port) return;

    remoteEnabled = remoteLocalAction->isChecked();

    if (remoteEnabled) {
        // Enable remote control
        m_port->write("SYST:REM\n");
        std::cerr << "Remote control enabled" << std::endl;
    } else {
        // Enable local control
        m_port->write("SYST:LOC\n");
        std::cerr << "Local control enabled" << std::endl;
    }
}

// Trigger System
void MainWindow::onTriggerModeChanged(int index) {
    if (!m_port) return;

    triggerMode = index;

    // Update trigger mode combo box if it exists
    if (triggerModeComboBox) {
        triggerModeComboBox->setCurrentIndex(index);
    }

    // Set trigger mode
    switch (index) {
        case 0: // Auto
            m_port->write("TRIG:SOUR IMM\n");
            std::cerr << "Trigger mode set to Auto" << std::endl;
            break;
        case 1: // Single
            m_port->write("TRIG:SOUR BUS\n");
            std::cerr << "Trigger mode set to Single" << std::endl;
            break;
        case 2: // External
            m_port->write("TRIG:SOUR EXT\n");
            std::cerr << "Trigger mode set to External" << std::endl;
            break;
    }

    // Save the trigger mode
    settings->setValue("triggerMode", triggerMode);
}

void MainWindow::onTriggerDelayChanged(int delay) {
    if (!m_port) return;

    triggerDelay = delay;

    // Set trigger delay
    m_port->write(QString("TRIG:DEL %1\n").arg(delay / 1000.0).toUtf8());
    std::cerr << "Trigger delay set to " << delay << " ms" << std::endl;

    // Save the trigger delay
    settings->setValue("triggerDelay", triggerDelay);
}

void MainWindow::onSampleCountChanged(int count) {
    if (!m_port) return;

    sampleCount = count;

    // Set sample count
    m_port->write(QString("SAMP:COUN %1\n").arg(count).toUtf8());
    std::cerr << "Sample count set to " << count << std::endl;

    // Save the sample count
    settings->setValue("sampleCount", sampleCount);
}



void MainWindow::onToggleConfigTab() {
    // Toggle the visibility of the configuration tab
    tabWidget->setVisible(!tabWidget->isVisible());

    // Update the action text based on the new state
    QAction* action = qobject_cast<QAction*>(sender());
    if (action) {
        action->setChecked(tabWidget->isVisible());
        if (tabWidget->isVisible()) {
            action->setText("Hide Configuration Tab");
        } else {
            action->setText("Show Configuration Tab");
        }
    }
}

void MainWindow::onConfigureHotkeys() {
    showHotkeyDialog();
}

void MainWindow::showHotkeyDialog() {
    // Create a dialog for hotkey configuration
    QDialog dialog(this);
    dialog.setWindowTitle("Configure Hotkeys");
    dialog.setMinimumSize(500, 400);

    // Create a layout for the dialog
    QVBoxLayout* layout = new QVBoxLayout(&dialog);

    // Create a table for displaying and editing hotkeys
    QTableWidget* table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels(QStringList() << "Action" << "Hotkey");
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->verticalHeader()->setVisible(false);

    // Populate the table with actions and their current hotkeys
    QMap<QString, QPushButton*> actionButtons;
    actionButtons["50 V"] = btn_50_v;
    actionButtons["Auto V"] = btn_auto_v;
    actionButtons["Short"] = btn_short;
    actionButtons["Diode"] = btn_diode;
    actionButtons["50 kΩ"] = btn_50_kr;
    actionButtons["Auto Ω"] = btn_auto_r;
    actionButtons["50 µF"] = btn_50_f;
    actionButtons["Auto F"] = btn_auto_f;
    actionButtons["Hz"] = btn_freq;
    actionButtons["Period"] = btn_period;
    actionButtons["Dual Display"] = btn_dual_display;
    actionButtons["Hold"] = btn_hold;

    // Add menu actions
    QMap<QString, QAction*> actionMenuItems;
    actionMenuItems["Save Session"] = saveSessionAction;
    actionMenuItems["Load Session"] = loadSessionAction;
    actionMenuItems["Rate Fast"] = rateFastAction;
    actionMenuItems["Rate Medium"] = rateMediumAction;
    actionMenuItems["Rate Slow"] = rateSlowAction;
    actionMenuItems["NULL (Relative)"] = nullAction;
    actionMenuItems["dB"] = dbAction;
    actionMenuItems["dBm"] = dbmAction;
    actionMenuItems["Min/Max/Avg"] = minMaxAction;
    actionMenuItems["Limit Testing"] = limitTestingAction;
    actionMenuItems["Start Logging"] = startLoggingAction;
    actionMenuItems["Stop Logging"] = stopLoggingAction;
    actionMenuItems["Clear Graph"] = clearGraphAction;
    actionMenuItems["Export Data"] = exportDataAction;

    // Set the table row count
    int totalActions = actionButtons.size() + actionMenuItems.size();
    table->setRowCount(totalActions);

    // Fill the table with actions and their hotkeys
    int row = 0;

    // Add button actions
    for (auto it = actionButtons.begin(); it != actionButtons.end(); ++it) {
        QString actionName = it.key();

        // Create action name item
        QTableWidgetItem* nameItem = new QTableWidgetItem(actionName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable); // Make read-only
        table->setItem(row, 0, nameItem);

        // Create hotkey editor
        QKeySequenceEdit* keyEdit = new QKeySequenceEdit(&dialog);
        if (hotkeySettings.contains(actionName)) {
            keyEdit->setKeySequence(hotkeySettings[actionName]);
        }

        // Add a clear button
        QPushButton* clearButton = new QPushButton("Clear", &dialog);
        clearButton->setFixedWidth(60);
        connect(clearButton, &QPushButton::clicked, [keyEdit]() {
            keyEdit->clear();
        });

        // Create a layout for the key edit and clear button
        QHBoxLayout* keyLayout = new QHBoxLayout();
        keyLayout->addWidget(keyEdit);
        keyLayout->addWidget(clearButton);

        // Create a widget to hold the layout
        QWidget* keyWidget = new QWidget(&dialog);
        keyWidget->setLayout(keyLayout);

        // Add the widget to the table
        table->setCellWidget(row, 1, keyWidget);

        row++;
    }

    // Add menu actions
    for (auto it = actionMenuItems.begin(); it != actionMenuItems.end(); ++it) {
        QString actionName = it.key();

        // Create action name item
        QTableWidgetItem* nameItem = new QTableWidgetItem(actionName);
        nameItem->setFlags(nameItem->flags() & ~Qt::ItemIsEditable); // Make read-only
        table->setItem(row, 0, nameItem);

        // Create hotkey editor
        QKeySequenceEdit* keyEdit = new QKeySequenceEdit(&dialog);
        if (hotkeySettings.contains(actionName)) {
            keyEdit->setKeySequence(hotkeySettings[actionName]);
        }

        // Add a clear button
        QPushButton* clearButton = new QPushButton("Clear", &dialog);
        clearButton->setFixedWidth(60);
        connect(clearButton, &QPushButton::clicked, [keyEdit]() {
            keyEdit->clear();
        });

        // Create a layout for the key edit and clear button
        QHBoxLayout* keyLayout = new QHBoxLayout();
        keyLayout->addWidget(keyEdit);
        keyLayout->addWidget(clearButton);

        // Create a widget to hold the layout
        QWidget* keyWidget = new QWidget(&dialog);
        keyWidget->setLayout(keyLayout);

        // Add the widget to the table
        table->setCellWidget(row, 1, keyWidget);

        row++;
    }

    // Add the table to the dialog layout
    layout->addWidget(table);

    // Add buttons for OK and Cancel
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    QPushButton* okButton = new QPushButton("OK", &dialog);
    QPushButton* cancelButton = new QPushButton("Cancel", &dialog);
    QPushButton* resetButton = new QPushButton("Reset to Defaults", &dialog);

    buttonLayout->addWidget(resetButton);
    buttonLayout->addStretch();
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(cancelButton);

    layout->addLayout(buttonLayout);

    // Connect the buttons
    connect(okButton, &QPushButton::clicked, [&]() {
        // Save the hotkey settings
        hotkeySettings.clear();

        // Process button actions
        int row = 0;
        for (auto it = actionButtons.begin(); it != actionButtons.end(); ++it) {
            QString actionName = it.key();
            QWidget* cellWidget = table->cellWidget(row, 1);
            QKeySequenceEdit* keyEdit = cellWidget->findChild<QKeySequenceEdit*>();
            if (keyEdit && !keyEdit->keySequence().isEmpty()) {
                hotkeySettings[actionName] = keyEdit->keySequence();
            }
            row++;
        }

        // Process menu actions
        for (auto it = actionMenuItems.begin(); it != actionMenuItems.end(); ++it) {
            QString actionName = it.key();
            QWidget* cellWidget = table->cellWidget(row, 1);
            QKeySequenceEdit* keyEdit = cellWidget->findChild<QKeySequenceEdit*>();
            if (keyEdit && !keyEdit->keySequence().isEmpty()) {
                hotkeySettings[actionName] = keyEdit->keySequence();
            }
            row++;
        }

        // Save and apply the settings
        saveHotkeySettings();
        applyHotkeys();

        dialog.accept();
    });

    connect(cancelButton, &QPushButton::clicked, [&]() {
        dialog.reject();
    });

    connect(resetButton, &QPushButton::clicked, [&]() {
        // Clear all hotkey settings
        for (int row = 0; row < table->rowCount(); ++row) {
            QWidget* cellWidget = table->cellWidget(row, 1);
            QKeySequenceEdit* keyEdit = cellWidget->findChild<QKeySequenceEdit*>();
            if (keyEdit) {
                keyEdit->clear();
            }
        }
    });

    // Show the dialog
    dialog.exec();
}

void MainWindow::saveHotkeySettings() {
    // Save hotkey settings to the application settings
    for (auto it = hotkeySettings.begin(); it != hotkeySettings.end(); ++it) {
        settings->setValue("Hotkeys/" + it.key(), it.value().toString());
    }
}

void MainWindow::loadHotkeySettings() {
    // Load hotkey settings from the application settings
    hotkeySettings.clear();

    // Get all keys in the Hotkeys group
    settings->beginGroup("Hotkeys");
    QStringList keys = settings->childKeys();
    for (const QString& key : keys) {
        QString keySequenceStr = settings->value(key).toString();
        if (!keySequenceStr.isEmpty()) {
            hotkeySettings[key] = QKeySequence(keySequenceStr);
        }
    }
    settings->endGroup();
}

void MainWindow::applyHotkeys() {
    // Clear existing shortcuts
    for (QShortcut* shortcut : shortcuts) {
        delete shortcut;
    }
    shortcuts.clear();

    // Create shortcuts for buttons
    QMap<QString, QPushButton*> actionButtons;
    actionButtons["50 V"] = btn_50_v;
    actionButtons["Auto V"] = btn_auto_v;
    actionButtons["Short"] = btn_short;
    actionButtons["Diode"] = btn_diode;
    actionButtons["50 kΩ"] = btn_50_kr;
    actionButtons["Auto Ω"] = btn_auto_r;
    actionButtons["50 µF"] = btn_50_f;
    actionButtons["Auto F"] = btn_auto_f;
    actionButtons["Hz"] = btn_freq;
    actionButtons["Period"] = btn_period;
    actionButtons["Dual Display"] = btn_dual_display;
    actionButtons["Hold"] = btn_hold;

    for (auto it = actionButtons.begin(); it != actionButtons.end(); ++it) {
        QString actionName = it.key();
        QPushButton* button = it.value();

        if (hotkeySettings.contains(actionName)) {
            QShortcut* shortcut = new QShortcut(hotkeySettings[actionName], this);

            // Connect to a lambda that animates the button press and then clicks it
            connect(shortcut, &QShortcut::activated, [this, button]() {
                // Animate the button press
                button->setDown(true);

                // Use a timer to release the button after a short delay
                QTimer::singleShot(100, [button]() {
                    button->setDown(false);

                    // Actually click the button
                    button->click();
                });
            });

            shortcuts.append(shortcut);
        }
    }

    // Create shortcuts for menu actions
    QMap<QString, QAction*> actionMenuItems;
    actionMenuItems["Save Session"] = saveSessionAction;
    actionMenuItems["Load Session"] = loadSessionAction;
    actionMenuItems["Rate Fast"] = rateFastAction;
    actionMenuItems["Rate Medium"] = rateMediumAction;
    actionMenuItems["Rate Slow"] = rateSlowAction;
    actionMenuItems["NULL (Relative)"] = nullAction;
    actionMenuItems["dB"] = dbAction;
    actionMenuItems["dBm"] = dbmAction;
    actionMenuItems["Min/Max/Avg"] = minMaxAction;
    actionMenuItems["Limit Testing"] = limitTestingAction;
    actionMenuItems["Start Logging"] = startLoggingAction;
    actionMenuItems["Stop Logging"] = stopLoggingAction;
    actionMenuItems["Clear Graph"] = clearGraphAction;
    actionMenuItems["Export Data"] = exportDataAction;

    for (auto it = actionMenuItems.begin(); it != actionMenuItems.end(); ++it) {
        QString actionName = it.key();
        QAction* action = it.value();

        if (hotkeySettings.contains(actionName)) {
            // Create a custom shortcut instead of setting it directly on the action
            // This allows us to add visual feedback
            QShortcut* shortcut = new QShortcut(hotkeySettings[actionName], this);

            // Connect to a lambda that highlights the menu action and then triggers it
            connect(shortcut, &QShortcut::activated, [this, action]() {
                // Flash the menu item by temporarily changing its font
                QFont originalFont = action->font();
                QFont boldFont = originalFont;
                boldFont.setBold(true);

                action->setFont(boldFont);

                // Use a timer to restore the original font after a short delay
                QTimer::singleShot(200, [action, originalFont]() {
                    action->setFont(originalFont);

                    // Trigger the action
                    action->trigger();
                });
            });

            shortcuts.append(shortcut);

            // Also set the shortcut on the action for display purposes
            action->setShortcut(hotkeySettings[actionName]);
        } else {
            action->setShortcut(QKeySequence());
        }
    }
}

void MainWindow::setupGraphScalingControls() {
    // ================================================
    // COMPLETELY REVAMPED GRAPH SCALING CONTROLS
    // ================================================

    // Create a collapsible panel for graph controls
    QWidget* controlsPanel = new QWidget();
    controlsPanel->setObjectName("graphControlsPanel");
    controlsPanel->setStyleSheet(
        "QWidget#graphControlsPanel { background-color: rgba(240, 240, 240, 220); border: 1px solid #aaa; border-radius: 4px; }"
        "QPushButton { min-height: 24px; }"
        "QLabel { font-weight: bold; }"
        "QGroupBox { font-weight: bold; }"
    );

    // Create a vertical layout for the panel
    QVBoxLayout* panelLayout = new QVBoxLayout(controlsPanel);
    panelLayout->setSpacing(4);
    panelLayout->setContentsMargins(6, 6, 6, 6);

    // Create a header with toggle button
    QWidget* headerWidget = new QWidget();
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(0, 0, 0, 0);
    headerLayout->setSpacing(4);

    // Create toggle button with icon
    QPushButton* toggleButton = new QPushButton("Graph Controls");
    toggleButton->setCheckable(true);
    toggleButton->setChecked(false); // Start collapsed
    toggleButton->setStyleSheet(
        "QPushButton { font-weight: bold; background-color: #404040; color: white; border-radius: 2px; }"
        "QPushButton:checked { background-color: #008000; }"
    );
    headerLayout->addWidget(toggleButton);
    headerLayout->addStretch();

    // Create content widget that will be shown/hidden
    QWidget* contentWidget = new QWidget();
    QVBoxLayout* contentLayout = new QVBoxLayout(contentWidget);
    contentLayout->setSpacing(6);
    contentLayout->setContentsMargins(0, 0, 0, 0);

    // Initially hide the content
    contentWidget->setVisible(false);

    // Connect toggle button to show/hide content
    connect(toggleButton, &QPushButton::toggled, [contentWidget](bool checked) {
        contentWidget->setVisible(checked);
    });

    // Create a horizontal layout for Y and X axis controls
    QHBoxLayout* axisControlsLayout = new QHBoxLayout();
    axisControlsLayout->setSpacing(10);

    // Create Y-axis controls
    QGroupBox* yScalingGroup = new QGroupBox("Y-Axis");
    QVBoxLayout* yScalingLayout = new QVBoxLayout(yScalingGroup);
    yScalingLayout->setSpacing(4);

    // Auto-scale Y checkbox
    autoScaleYCheckBox = new QCheckBox("Auto-scale");
    autoScaleYCheckBox->setChecked(autoScaleY);
    connect(autoScaleYCheckBox, &QCheckBox::toggled, this, &MainWindow::onAutoScaleYToggled);
    yScalingLayout->addWidget(autoScaleYCheckBox);

    // Create a grid for min/max controls
    QGridLayout* yRangeLayout = new QGridLayout();
    yRangeLayout->setSpacing(4);

    // Min Y control
    QLabel* minYLabel = new QLabel("Min:");
    yRangeLayout->addWidget(minYLabel, 0, 0);

    minYSpinBox = new QDoubleSpinBox();
    minYSpinBox->setRange(-1000000, 1000000);
    minYSpinBox->setDecimals(5);
    minYSpinBox->setValue(manualMinY);
    minYSpinBox->setEnabled(!autoScaleY);
    minYSpinBox->setFixedWidth(100);
    connect(minYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onManualMinYChanged);
    yRangeLayout->addWidget(minYSpinBox, 0, 1);

    // Max Y control
    QLabel* maxYLabel = new QLabel("Max:");
    yRangeLayout->addWidget(maxYLabel, 1, 0);

    maxYSpinBox = new QDoubleSpinBox();
    maxYSpinBox->setRange(-1000000, 1000000);
    maxYSpinBox->setDecimals(5);
    maxYSpinBox->setValue(manualMaxY);
    maxYSpinBox->setEnabled(!autoScaleY);
    maxYSpinBox->setFixedWidth(100);
    connect(maxYSpinBox, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &MainWindow::onManualMaxYChanged);
    yRangeLayout->addWidget(maxYSpinBox, 1, 1);

    // Add the range layout to the Y scaling layout
    yScalingLayout->addLayout(yRangeLayout);

    // Create X-axis controls
    QGroupBox* xScalingGroup = new QGroupBox("X-Axis");
    QVBoxLayout* xScalingLayout = new QVBoxLayout(xScalingGroup);
    xScalingLayout->setSpacing(4);

    // Auto-scale X checkbox
    autoScaleXCheckBox = new QCheckBox("Auto-scale");
    autoScaleXCheckBox->setChecked(autoScaleX);
    connect(autoScaleXCheckBox, &QCheckBox::toggled, this, &MainWindow::onAutoScaleXToggled);
    xScalingLayout->addWidget(autoScaleXCheckBox);

    // Time span selection
    QHBoxLayout* timeSpanLayout = new QHBoxLayout();
    timeSpanLayout->setSpacing(4);

    QLabel* timeSpanLabel = new QLabel("Time Span:");
    timeSpanLayout->addWidget(timeSpanLabel);

    timeSpanComboBox = new QComboBox();
    timeSpanComboBox->addItem("30 seconds", 30000);
    timeSpanComboBox->addItem("1 minute", 60000);
    timeSpanComboBox->addItem("2 minutes", 120000);
    timeSpanComboBox->addItem("5 minutes", 300000);
    timeSpanComboBox->addItem("10 minutes", 600000);
    timeSpanComboBox->addItem("30 minutes", 1800000);
    timeSpanComboBox->addItem("1 hour", 3600000);

    // Set the default selection based on manualTimeSpan
    int index = timeSpanComboBox->findData(manualTimeSpan);
    if (index != -1) {
        timeSpanComboBox->setCurrentIndex(index);
    } else {
        // Default to 1 minute if the current value is not in the list
        index = timeSpanComboBox->findData(60000);
        if (index != -1) {
            timeSpanComboBox->setCurrentIndex(index);
        }
    }

    timeSpanComboBox->setEnabled(!autoScaleX);
    timeSpanComboBox->setFixedWidth(100);
    connect(timeSpanComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onTimeSpanChanged);
    timeSpanLayout->addWidget(timeSpanComboBox);

    // Add the time span layout to the X scaling layout
    xScalingLayout->addLayout(timeSpanLayout);

    // Add the Y and X axis controls to the horizontal layout
    axisControlsLayout->addWidget(yScalingGroup);
    axisControlsLayout->addWidget(xScalingGroup);

    // Add the axis controls to the content layout
    contentLayout->addLayout(axisControlsLayout);

    // Create action buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(8);

    // Apply button
    QPushButton* applyButton = new QPushButton("Apply");
    applyButton->setStyleSheet("QPushButton { background-color: #4a86e8; color: white; }");
    connect(applyButton, &QPushButton::clicked, [this]() {
        // Force update of the graph with current settings
        if (!autoScaleY) {
            axisY->setRange(manualMinY, manualMaxY);
            updateYAxisSegments(manualMinY, manualMaxY);
        }

        if (!autoScaleX && series->count() > 0) {
            // Apply manual time span using relative time
            double timeSpanSeconds = manualTimeSpan / 1000.0;

            // Get the latest time value from the series
            double latestTime = series->at(series->count() - 1).x();

            // Calculate the visible time range based on the current time span
            double visibleStartTime;
            double visibleEndTime;

            if (latestTime <= timeSpanSeconds) {
                // If we haven't collected enough data to fill the time span yet,
                // show from 0 to the time span
                visibleStartTime = 0;
                visibleEndTime = timeSpanSeconds;
            } else {
                // Otherwise, show the most recent time span
                visibleStartTime = latestTime - timeSpanSeconds;
                visibleEndTime = latestTime;
            }

            // Update X axis range - using relative time in seconds
            axisX->setRange(visibleStartTime, visibleEndTime);

            // Update X-axis segments based on time span
            updateXAxisSegments(timeSpanSeconds);
        }

        chart->update();
    });
    buttonLayout->addWidget(applyButton);

    // Reset button
    QPushButton* resetButton = new QPushButton("Reset");
    resetButton->setStyleSheet("QPushButton { background-color: #e06666; color: white; }");
    connect(resetButton, &QPushButton::clicked, [this]() {
        // Reset to default settings
        autoScaleY = true;
        autoScaleX = true;
        manualMinY = -1.0;
        manualMaxY = 1.0;
        manualTimeSpan = 60000; // 60 seconds

        // Update UI controls
        autoScaleYCheckBox->setChecked(true);
        autoScaleXCheckBox->setChecked(true);
        minYSpinBox->setValue(manualMinY);
        maxYSpinBox->setValue(manualMaxY);

        // Find and select the 60-second option in the combo box
        int index = timeSpanComboBox->findData(60000);
        if (index != -1) {
            timeSpanComboBox->setCurrentIndex(index);
        }

        // Update control states
        minYSpinBox->setEnabled(false);
        maxYSpinBox->setEnabled(false);
        timeSpanComboBox->setEnabled(false);

        // Clear the graph data
        series->clear();
        dataPoints.clear();

        // Reset axes to default for relative time
        axisX->setRange(0, 60); // Show 0 to 60 seconds
        axisY->setRange(-1.0, 1.0);

        // Reset axis segments and format
        axisX->setTickCount(5);
        axisX->setLabelFormat("%.1f"); // One decimal place for seconds
        axisY->setTickCount(5);
        axisY->setLabelFormat("%.3f");

        // Force the chart to update
        chart->update();
    });
    buttonLayout->addWidget(resetButton);

    // Add the button layout to the content layout
    contentLayout->addLayout(buttonLayout);

    // Add the header and content to the panel layout
    panelLayout->addWidget(headerWidget);
    panelLayout->addWidget(contentWidget);

    // Create a layout for the chart view
    QVBoxLayout* chartLayout = new QVBoxLayout(chartView);
    chartLayout->setContentsMargins(0, 0, 0, 0);

    // Add the controls panel to the top-right corner of the chart view
    chartLayout->setAlignment(Qt::AlignTop | Qt::AlignRight);
    chartLayout->addWidget(controlsPanel);

    // Set a fixed width for the controls panel to prevent it from expanding
    controlsPanel->setFixedWidth(300);
}

void MainWindow::onAutoScaleYToggled(bool checked) {
    autoScaleY = checked;
    minYSpinBox->setEnabled(!checked);
    maxYSpinBox->setEnabled(!checked);

    if (!checked) {
        // Apply manual Y range immediately
        axisY->setRange(manualMinY, manualMaxY);
        chart->update();
    }
}

void MainWindow::onAutoScaleXToggled(bool checked) {
    // ================================================
    // COMPLETELY REFACTORED AUTO SCALE X TOGGLE HANDLER
    // ================================================

    // Update the auto scale flag
    autoScaleX = checked;

    // Enable/disable the time span combo box based on auto scale setting
    timeSpanComboBox->setEnabled(!checked);

    // Log the change for debugging
    std::cerr << "Auto scale X toggled: " << (checked ? "ON" : "OFF") << std::endl;

    if (!checked) {
        // Apply manual time span immediately
        double timeSpanSeconds = manualTimeSpan / 1000.0;

        // Calculate the visible time range based on the current time span
        double visibleStartTime = 0;
        double visibleEndTime = timeSpanSeconds;

        if (series->count() > 0) {
            // Get the latest time value from the series
            double latestTime = series->at(series->count() - 1).x();

            if (latestTime > timeSpanSeconds) {
                // If we have enough data to fill the time span,
                // show the most recent time span
                visibleStartTime = latestTime - timeSpanSeconds;
                visibleEndTime = latestTime;
            }

            std::cerr << "Setting X range to " << visibleStartTime << " - "
                      << visibleEndTime << " seconds" << std::endl;
        } else {
            std::cerr << "No data points yet, setting X range to 0 - "
                      << timeSpanSeconds << " seconds" << std::endl;
        }

        // Update X axis range - using relative time in seconds
        axisX->setRange(visibleStartTime, visibleEndTime);

        // Update X-axis segments based on time span
        updateXAxisSegments(timeSpanSeconds);
    } else {
        // Auto scale is enabled, use default time span (60 seconds)
        double timeSpanSeconds = 60.0;

        if (series->count() > 0) {
            // Get the latest time value from the series
            double latestTime = series->at(series->count() - 1).x();

            // Calculate the visible time range
            double visibleStartTime = std::max(0.0, latestTime - timeSpanSeconds);
            double visibleEndTime = std::max(timeSpanSeconds, latestTime);

            // Update X axis range
            axisX->setRange(visibleStartTime, visibleEndTime);

            std::cerr << "Auto scale X enabled, setting range to " << visibleStartTime
                      << " - " << visibleEndTime << " seconds" << std::endl;
        } else {
            // No data points yet, use default range
            axisX->setRange(0, timeSpanSeconds);

            std::cerr << "Auto scale X enabled, no data points, setting range to 0 - "
                      << timeSpanSeconds << " seconds" << std::endl;
        }

        // Update X-axis segments based on time span
        updateXAxisSegments(timeSpanSeconds);
    }

    // Force the chart to update
    chart->update();

    // Save the setting
    settings->setValue("autoScaleX", autoScaleX);
}

void MainWindow::onManualMinYChanged(double value) {
    manualMinY = value;

    // Ensure min is less than max
    if (manualMinY >= manualMaxY) {
        manualMaxY = manualMinY + 0.1;
        maxYSpinBox->setValue(manualMaxY);
    }

    if (!autoScaleY) {
        // Apply manual Y range immediately
        axisY->setRange(manualMinY, manualMaxY);

        // Update Y-axis segments based on range
        updateYAxisSegments(manualMinY, manualMaxY);

        chart->update();
    }
}

void MainWindow::onManualMaxYChanged(double value) {
    manualMaxY = value;

    // Ensure max is greater than min
    if (manualMaxY <= manualMinY) {
        manualMinY = manualMaxY - 0.1;
        minYSpinBox->setValue(manualMinY);
    }

    if (!autoScaleY) {
        // Apply manual Y range immediately
        axisY->setRange(manualMinY, manualMaxY);

        // Update Y-axis segments based on range
        updateYAxisSegments(manualMinY, manualMaxY);

        chart->update();
    }
}

void MainWindow::onTimeSpanChanged(int index) {
    // ================================================
    // COMPLETELY REFACTORED TIME SPAN CHANGE HANDLER
    // ================================================

    // Get the time span value from the combo box
    qint64 oldTimeSpan = manualTimeSpan;
    manualTimeSpan = timeSpanComboBox->itemData(index).toLongLong();

    // Convert to seconds for easier handling
    double timeSpanSeconds = manualTimeSpan / 1000.0;

    // Log the change for debugging
    std::cerr << "Time span changed from " << (oldTimeSpan / 1000.0) << " to "
              << timeSpanSeconds << " seconds" << std::endl;

    if (!autoScaleX) {
        // Calculate the visible time range based on the current time span
        double visibleStartTime = 0;
        double visibleEndTime = timeSpanSeconds;

        if (series->count() > 0) {
            // Get the latest time value from the series
            double latestTime = series->at(series->count() - 1).x();

            if (latestTime > timeSpanSeconds) {
                // If we have enough data to fill the time span,
                // show the most recent time span
                visibleStartTime = latestTime - timeSpanSeconds;
                visibleEndTime = latestTime;
            }

            std::cerr << "Setting X range to " << visibleStartTime << " - "
                      << visibleEndTime << " seconds" << std::endl;
        } else {
            std::cerr << "No data points yet, setting X range to 0 - "
                      << timeSpanSeconds << " seconds" << std::endl;
        }

        // Update X axis range - using relative time in seconds
        axisX->setRange(visibleStartTime, visibleEndTime);

        // Update X-axis segments based on time span
        updateXAxisSegments(timeSpanSeconds);

        // Force the chart to update
        chart->update();
    }

    // Save the setting
    settings->setValue("manualTimeSpan", manualTimeSpan);
}

void MainWindow::updateYAxisSegments(double min, double max) {
    // Calculate the range
    double range = max - min;

    // Adjust tick count based on the range
    if (range <= 0.01) {
        axisY->setTickCount(3); // Very small range
        axisY->setMinorTickCount(1);
        axisY->setLabelFormat("%.5f");
    } else if (range <= 0.1) {
        axisY->setTickCount(4); // Small range
        axisY->setMinorTickCount(1);
        axisY->setLabelFormat("%.4f");
    } else if (range <= 1.0) {
        axisY->setTickCount(5); // Medium range
        axisY->setMinorTickCount(1);
        axisY->setLabelFormat("%.3f");
    } else if (range <= 10.0) {
        axisY->setTickCount(6); // Large range
        axisY->setMinorTickCount(1);
        axisY->setLabelFormat("%.2f");
    } else if (range <= 100.0) {
        axisY->setTickCount(6); // Very large range
        axisY->setMinorTickCount(4);
        axisY->setLabelFormat("%.1f");
    } else {
        axisY->setTickCount(8); // Extremely large range
        axisY->setMinorTickCount(4);
        axisY->setLabelFormat("%.0f");
    }
}

void MainWindow::updateXAxisSegments(double timeSpanSeconds) {
    // Adjust tick count and format based on the time span in seconds
    if (timeSpanSeconds <= 30.0) { // 30 seconds
        axisX->setTickCount(6);
        axisX->setLabelFormat("%.1f"); // One decimal place for short time spans
    } else if (timeSpanSeconds <= 60.0) { // 1 minute
        axisX->setTickCount(7);
        axisX->setLabelFormat("%.1f"); // One decimal place
    } else if (timeSpanSeconds <= 300.0) { // 5 minutes
        axisX->setTickCount(6);
        axisX->setLabelFormat("%.0f"); // No decimal places for longer spans
    } else if (timeSpanSeconds <= 600.0) { // 10 minutes
        axisX->setTickCount(6);
        axisX->setLabelFormat("%.0f"); // No decimal places
    } else if (timeSpanSeconds <= 1800.0) { // 30 minutes
        axisX->setTickCount(7);
        axisX->setLabelFormat("%.0f"); // No decimal places
    } else { // 1 hour or more
        axisX->setTickCount(8);
        axisX->setLabelFormat("%.0f"); // No decimal places
    }

    // Force the chart to update
    chart->update();
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

// Slot to handle hold button toggle
void MainWindow::onToggleHold() {
    // ================================================
    // HOLD TOGGLE FUNCTION
    // ================================================

    holdEnabled = btn_hold->isChecked();

    if (holdEnabled) {
        // Enable hold mode

        // Update button appearance
        btn_hold->setText("HOLD ON");

        // Store current values
        lastPrimaryValue = measurement->text();
        if (dualDisplayEnabled) {
            lastSecondaryValue = secondaryMeasurement->text();
        }

        // Add a visual indicator to show hold is active
        modeLabel->setText(modeLabel->text() + " - HOLD");

        std::cerr << "Hold mode enabled" << std::endl;
    } else {
        // Disable hold mode

        // Update button appearance
        btn_hold->setText("HOLD");

        // Clear the hold indicator from the mode label
        QString currentMode = modeLabel->text();
        currentMode.replace(" - HOLD", "");
        modeLabel->setText(currentMode);

        std::cerr << "Hold mode disabled" << std::endl;
    }

    // Save the hold state
    settings->setValue("holdEnabled", holdEnabled);
}
