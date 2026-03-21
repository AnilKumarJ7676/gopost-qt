#pragma once

#include <QTest>

class TestMmap : public QObject {
    Q_OBJECT
private slots:
    void smallFileRejection();
    void basicMmap10MB();
    void offsetMapping();
    void advisoryHints();
    void sequentialThroughput();
    void slidingWindow();
    void cleanupUnmap();
};
