#pragma once

#include <QTest>

class TestDiagnostics : public QObject {
    Q_OBJECT
private slots:
    void frameTracking();
    void scopedTiming();
    void overlayToggle();
};
