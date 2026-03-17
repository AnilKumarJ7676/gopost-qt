#pragma once

#include <QObject>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <memory>

#include "core/di/service_locator.h"

namespace gopost {

class GopostApp : public QObject {
    Q_OBJECT
public:
    explicit GopostApp(QGuiApplication* app, QObject* parent = nullptr);
    ~GopostApp() override;

    int run();

private:
    void initializeEngines();
    void setupQml();

    QGuiApplication* m_app = nullptr;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
    std::unique_ptr<core::ServiceLocator> m_serviceLocator;
};

} // namespace gopost
