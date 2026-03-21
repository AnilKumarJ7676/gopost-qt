#pragma once

#include <QObject>
#include <QQmlApplicationEngine>
#include <QString>

namespace gopost::diagnostics {

class BootInfoProvider : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString tier READ tier CONSTANT)
    Q_PROPERTY(QString cpuInfo READ cpuInfo CONSTANT)
    Q_PROPERTY(int ramGb READ ramGb CONSTANT)
    Q_PROPERTY(QString storageType READ storageType CONSTANT)
    Q_PROPERTY(QString gpuName READ gpuName CONSTANT)
    Q_PROPERTY(double bootTimeMs READ bootTimeMs NOTIFY bootTimeMsChanged)

public:
    explicit BootInfoProvider(QObject* parent = nullptr);

    QString tier() const { return tier_; }
    QString cpuInfo() const { return cpuInfo_; }
    int ramGb() const { return ramGb_; }
    QString storageType() const { return storageType_; }
    QString gpuName() const { return gpuName_; }
    double bootTimeMs() const { return bootTimeMs_; }

    void setTier(const QString& t) { tier_ = t; }
    void setCpuInfo(const QString& c) { cpuInfo_ = c; }
    void setRamGb(int r) { ramGb_ = r; }
    void setStorageType(const QString& s) { storageType_ = s; }
    void setGpuName(const QString& g) { gpuName_ = g; }
    void setBootTimeMs(double ms) { bootTimeMs_ = ms; emit bootTimeMsChanged(); }

    void registerWithQml(QQmlApplicationEngine* engine);

signals:
    void bootTimeMsChanged();

private:
    QString tier_;
    QString cpuInfo_;
    int ramGb_ = 0;
    QString storageType_;
    QString gpuName_;
    double bootTimeMs_ = 0.0;
};

} // namespace gopost::diagnostics
