#pragma once

#include <QTest>

class TestLogger : public QObject {
    Q_OBJECT
private slots:
    void coloredConsoleOutput();
    void fileLogging();
    void categoryFiltering();
    void fileRotation();
};
