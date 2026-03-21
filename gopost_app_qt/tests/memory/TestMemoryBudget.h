#pragma once

#include <QTest>

class TestMemoryEngine : public QObject {
    Q_OBJECT
private slots:
    void printBudgets();
    void enforcement();
    void usageTracking();
};
