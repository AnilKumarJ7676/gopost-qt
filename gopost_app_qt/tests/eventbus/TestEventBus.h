#pragma once

#include <QTest>

class TestEventBus : public QObject {
    Q_OBJECT
private slots:
    void immediateDelivery();
    void queuedDelivery();
    void unsubscribeTest();
    void performance();
};
