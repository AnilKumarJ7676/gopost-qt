#pragma once

#include <QTest>

class TestImport : public QObject {
    Q_OBJECT
private slots:
    void importStub();
    void statusStub();
};
