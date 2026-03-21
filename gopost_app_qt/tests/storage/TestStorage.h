#pragma once

#include <QTest>

class TestStorage : public QObject {
    Q_OBJECT
private slots:
    void storageProfileDetection();
    void sequentialReadBenchmark();
    void readAheadDetection();
    void priorityOrdering();
    void bandwidthMonitor();
    void blockAlignment();
};
