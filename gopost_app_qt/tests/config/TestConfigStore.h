#pragma once

#include <QTest>

class TestConfigStore : public QObject {
    Q_OBJECT
private slots:
    void defaults();
    void setAndGet();
    void persistence();
    void scopeOverride();
    void migration();
};
