#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

namespace gopost::core {

class Router : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString currentRoute READ currentRoute NOTIFY routeChanged)
    Q_PROPERTY(QString currentLocation READ currentLocation NOTIFY routeChanged)
    Q_PROPERTY(bool canGoBack READ canGoBack NOTIFY routeChanged)

public:
    explicit Router(QObject* parent = nullptr);

    QString currentRoute() const;
    QString currentLocation() const;
    bool canGoBack() const;

    Q_INVOKABLE void go(const QString& path);
    Q_INVOKABLE void push(const QString& path);
    Q_INVOKABLE void pop();
    Q_INVOKABLE void replace(const QString& path);

    Q_INVOKABLE QString pathParameter(const QString& name) const;
    Q_INVOKABLE QString queryParameter(const QString& name) const;

    // Auth redirect logic
    void setCanAccessApp(bool canAccess);
    bool isAuthRoute() const;

    // Route definitions matching GoRouter
    struct RouteInfo {
        QString path;
        bool requiresAuth = true;
        bool isShellRoute = false;
    };

    static const QList<RouteInfo>& routes();

signals:
    void routeChanged();
    void navigationRequested(const QString& route);

private:
    void applyRedirect();
    QString extractPath(const QString& fullPath) const;
    QVariantMap extractQueryParams(const QString& fullPath) const;
    QVariantMap extractPathParams(const QString& pattern, const QString& path) const;

    QStringList m_routeStack;
    QString m_currentPath;
    QVariantMap m_pathParams;
    QVariantMap m_queryParams;
    bool m_canAccessApp = false;
};

} // namespace gopost::core
