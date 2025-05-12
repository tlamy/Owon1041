#ifndef SETTINGS_H
#define SETTINGS_H

#include <QSettings>
#include <QString>

class Settings final : public QSettings {
    Q_OBJECT

public:
    // Constructor uses default QSettings format and location
    explicit Settings(QObject *parent = nullptr);
    
    // Optional: Custom constructor with specific organization/application
    Settings(const QString &organization, const QString &application, QObject *parent = nullptr);
    
    // Initialize default values
    void init();
    
    // Load settings from storage
    void load();
    
    // Save settings to storage
    void save();

    // Properties with getters and setters
    int windowHeight() const { return m_windowHeight; }
    void setWindowHeight(int height);
    
    int windowWidth() const { return m_windowWidth; }
    void setWindowWidth(int width);
    
    int windowX() const { return m_windowX; }
    void setWindowX(int x);
    
    int windowY() const { return m_windowY; }
    void setWindowY(int y);
    
    QString device() const { return m_device; }
    void setDevice(const QString &device);

private:
    // Private member variables
    int m_windowHeight = 150;
    int m_windowWidth = 450;
    int m_windowX = 100;
    int m_windowY = 100;
    QString m_device = "";
};

#endif // SETTINGS_H