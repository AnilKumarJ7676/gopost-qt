#pragma once

#include <QTest>

class TestBridge : public QObject {
    Q_OBJECT
private slots:
    void factoryCreates();
    void directoryResolution();
    void storageInfo();
    void deviceInfo();
    void fileRead();
    void fileMetadata();
    void filePickerManual();
    void pathSanitizer();
};
