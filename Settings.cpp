#include "Settings.h"

Settings::Settings(QObject *parent)
    : QSettings(parent) {
}

Settings::Settings(const QString &organization, const QString &application, QObject *parent)
    : QSettings(organization, application, parent) {
}

void Settings::init() {
    load();
}

void Settings::load() {
    // Load settings from storage using QSettings methods
    m_windowHeight = value("window/height", m_windowHeight).toInt();
    m_windowWidth = value("window/width", m_windowWidth).toInt();
    m_windowX = value("window/x", m_windowX).toInt();
    m_windowY = value("window/y", m_windowY).toInt();
    m_device = value("hardware/device", m_device).toString();
}

void Settings::save() {
    // Save settings to storage using QSettings methods
    setValue("window/height", m_windowHeight);
    setValue("window/width", m_windowWidth);
    setValue("window/x", m_windowX);
    setValue("window/y", m_windowY);
    setValue("hardware/device", m_device);

    // Ensure settings are written to disk
    sync();
}

// Setter methods with automatic value updates
void Settings::setWindowHeight(int height) {
    m_windowHeight = height;
    setValue("window/height", height);
}

void Settings::setWindowWidth(int width) {
    m_windowWidth = width;
    setValue("window/width", width);
}

void Settings::setWindowX(int x) {
    m_windowX = x;
    setValue("window/x", x);
}

void Settings::setWindowY(int y) {
    m_windowY = y;
    setValue("window/y", y);
}

void Settings::setDevice(const QString &device) {
    m_device = device;
    setValue("hardware/device", device);
}
