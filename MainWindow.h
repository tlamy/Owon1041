#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QPushButton>
#include <QMenu>
#include <QAction>
#include <QMenuBar>
#include <QtCharts/QChart>
#include <QtCharts/QChartView>
#include <QtCharts/QLineSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>
#include <QFileDialog>
#include <QSpinBox>
#include <QComboBox>
#include <QCheckBox>
#include <QGroupBox>
#include <QTabWidget>
#include <QDateTime>
#include <QShortcut>
#include <QKeySequenceEdit>
#include <QDialog>
#include <QTableWidget>
#include <QDialogButtonBox>

QT_CHARTS_USE_NAMESPACE

#include "ConnectDialog.h"
#include "Settings.h"

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

    ~MainWindow() override;

    void setupUi(QMainWindow *MainWindow);

    Settings *settings;

protected:
    void resizeEvent(QResizeEvent *event) override;

    void setupConnections();
private slots:
    void onVoltage50V();
    void onVoltageAuto();
    void onShort();
    void onDiode();
    void onResistance50K();
    void onResistanceAuto();
    void onCapacitance50uF();
    void onCapacitanceAuto();
    void onFrequency();
    void onPeriod();
    void onToggleDualDisplay();
    void onToggleHold();
    void onSaveSession();
    void onLoadSession();
    void onRateFast();
    void onRateMedium();
    void onRateSlow();
    void onNullToggle();
    void onDbToggle();
    void onDbmToggle();
    void onMinMaxToggle();
    void onLimitTestingToggle();

    // Data Logging and Visualization
    void onStartLogging();
    void onStopLogging();
    void onClearGraph();
    void onExportData();
    void updateGraph(const QString &value);

    // Graph Scaling Controls
    void onAutoScaleYToggled(bool checked);
    void onAutoScaleXToggled(bool checked);
    void onManualMinYChanged(double value);
    void onManualMaxYChanged(double value);
    void onTimeSpanChanged(int index);
    void updateYAxisSegments(double min, double max);
    void updateXAxisSegments(double timeSpanSeconds);

    // Advanced Measurement Options
    void onAcDcToggle();
    void onTemperatureMode();
    void onCurrentMode();
    void on2Wire4WireToggle();

    // System Configuration
    void onBeeperToggle();
    void onBrightnessChanged(int value);
    void onAutoZeroToggle();
    void onRemoteLocalToggle();
    void onToggleConfigTab();

    // Hotkey Configuration
    void onConfigureHotkeys();
    void showHotkeyDialog();
    void saveHotkeySettings();
    void loadHotkeySettings();
    void applyHotkeys();

    // Trigger System
    void onTriggerModeChanged(int index);
    void onTriggerDelayChanged(int delay);
    void onSampleCountChanged(int count);

    bool eventFilter(QObject *obj, QEvent *event) override;

    void onMeasurementClicked();

    void updateMeasurement(); // Make sure this exists and is declared as a slot


private:
    // UI elements as member variables (excluding centralwidget)
    QLabel *measurement;
    QLabel *secondaryMeasurement;
    QLabel *secondaryLabel;
    QLabel *modeLabel;
    QLabel *rangeLabel;
    QLabel *infoLabel;
    QPushButton *btn_50_v;
    QPushButton *btn_auto_v;
    QPushButton *btn_short;
    QPushButton *btn_diode;
    QPushButton *btn_50_kr;
    QPushButton *btn_auto_r;
    QPushButton *btn_50_f;
    QPushButton *btn_auto_f;
    QPushButton *btn_freq;
    QPushButton *btn_period;
    QPushButton *btn_dual_display;
    QPushButton *btn_hold;
    // Main UI elements
    QTabWidget *tabWidget;
    QWidget *measurementTab;
    QWidget *graphTab;
    QWidget *configTab;

    // Menus
    QMenu *sessionMenu;
    QMenu *rateMenu;
    QMenu *mathMenu;
    QMenu *loggingMenu;
    QMenu *advancedMenu;
    QMenu *systemMenu;
    QMenu *triggerMenu;

    // Session actions
    QAction *saveSessionAction;
    QAction *loadSessionAction;

    // Rate actions
    QAction *rateFastAction;
    QAction *rateMediumAction;
    QAction *rateSlowAction;

    // Math actions
    QAction *nullAction;
    QAction *dbAction;
    QAction *dbmAction;
    QAction *minMaxAction;
    QAction *limitTestingAction;

    // Logging actions
    QAction *startLoggingAction;
    QAction *stopLoggingAction;
    QAction *clearGraphAction;
    QAction *exportDataAction;

    // Advanced measurement actions
    QAction *acDcAction;
    QAction *temperatureAction;
    QAction *currentAction;
    QAction *wire2_4Action;

    // System configuration actions
    QAction *beeperAction;
    QAction *autoZeroAction;
    QAction *remoteLocalAction;

    // Graph elements
    QChartView *chartView;
    QChart *chart;
    QLineSeries *series;
    QValueAxis *axisX; // Using QValueAxis for relative time
    QValueAxis *axisY;

    // Graph scaling controls
    QCheckBox *autoScaleYCheckBox;
    QCheckBox *autoScaleXCheckBox;
    QDoubleSpinBox *minYSpinBox;
    QDoubleSpinBox *maxYSpinBox;
    QComboBox *timeSpanComboBox;

    // Configuration elements
    QSpinBox *brightnessSpinBox;
    QComboBox *triggerModeComboBox;
    QSpinBox *triggerDelaySpinBox;
    QSpinBox *sampleCountSpinBox;

    // Data logging
    QFile *logFile;
    bool isLogging;

    ConnectDialog *m_connect_dialog;

    void connectSerial();

    void onConnect();

    bool openConnectDialog();

    // Helper function to uncheck all mode buttons except the active one
    void uncheckAllModeButtons(QPushButton* activeButton);

    // Helper function to cycle through available ranges for a measurement mode
    QString cycleRange(const QString& function);
    void saveState();
    void loadState();
    void setupMenus();
    void setupGraphScalingControls();


    QTimer *m_timer = nullptr;
    QSerialPort *m_port = nullptr;
    bool dualDisplayEnabled = false;
    QString currentFunction = "VOLT:DC"; // Default function
    QString secondaryFunction = "NONE";  // Default no secondary function
    // Math function states
    bool nullEnabled = false;
    bool dbEnabled = false;
    bool dbmEnabled = false;
    bool minMaxEnabled = false;
    bool limitTestingEnabled = false;

    // Advanced measurement states
    bool acDcEnabled = false;
    bool wire4Enabled = false;

    // System configuration states
    bool beeperEnabled = true;
    bool autoZeroEnabled = true;
    bool remoteEnabled = true;
    int brightness = 50;

    // Trigger system states
    int triggerMode = 0; // 0=Auto, 1=Single, 2=External
    int triggerDelay = 0;
    int sampleCount = 1;

    // Data logging states
    QList<QPointF> dataPoints;
    QString logFilePath;

    // Graph scaling states
    bool autoScaleY = true;
    bool autoScaleX = true;
    double manualMinY = -1.0;
    double manualMaxY = 1.0;
    qint64 manualTimeSpan = 60000; // 60 seconds in milliseconds

    // Hold feature state
    bool holdEnabled = false;
    QString lastPrimaryValue;
    QString lastSecondaryValue;

    // Range cycling state
    QMap<QString, int> currentRangeIndex;
    QMap<QString, QStringList> availableRanges;

    // Hotkey settings
    QMap<QString, QKeySequence> hotkeySettings;
    QList<QShortcut*> shortcuts;
};

#endif // MAINWINDOW_H
