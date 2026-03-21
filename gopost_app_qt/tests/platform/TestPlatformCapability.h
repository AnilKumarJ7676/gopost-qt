#pragma once

#include <QTest>

class TestPlatformCapability : public QObject {
    Q_OBJECT
private slots:
    void detectCpu();
    void detectGpu();
    void detectRam();
    void detectStorage();
    void detectHwDecode();
    void classifyTier();
};
