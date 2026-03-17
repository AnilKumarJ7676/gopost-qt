#include "core/navigation/router.h"

#include <QUrl>
#include <QUrlQuery>
#include <QRegularExpression>

namespace gopost::core {

Router::Router(QObject* parent)
    : QObject(parent) {
    m_routeStack.append(QStringLiteral("/auth/login"));
    m_currentPath = QStringLiteral("/auth/login");
}

const QList<Router::RouteInfo>& Router::routes() {
    static const QList<RouteInfo> r = {
        {QStringLiteral("/auth/login"), false},
        {QStringLiteral("/auth/register"), false},
        {QStringLiteral("/editor/video"), true},
        {QStringLiteral("/editor/video2"), true},
        {QStringLiteral("/editor/image"), true},
        {QStringLiteral("/editor/customize/:id"), true},
        {QStringLiteral("/editor/video/customize/:id"), true},
        {QStringLiteral("/projects/video"), true},
        {QStringLiteral("/"), true, true},
        {QStringLiteral("/templates"), true, true},
        {QStringLiteral("/templates/:id"), true, true},
        {QStringLiteral("/create"), true, true},
        {QStringLiteral("/profile"), true, true},
    };
    return r;
}

QString Router::currentRoute() const {
    return m_currentPath;
}

QString Router::currentLocation() const {
    return m_routeStack.isEmpty() ? QString() : m_routeStack.last();
}

bool Router::canGoBack() const {
    return m_routeStack.size() > 1;
}

void Router::go(const QString& path) {
    m_routeStack.clear();
    m_routeStack.append(path);
    m_currentPath = extractPath(path);
    m_queryParams = extractQueryParams(path);
    applyRedirect();
    emit routeChanged();
    emit navigationRequested(m_currentPath);
}

void Router::push(const QString& path) {
    m_routeStack.append(path);
    m_currentPath = extractPath(path);
    m_queryParams = extractQueryParams(path);
    applyRedirect();
    emit routeChanged();
    emit navigationRequested(m_currentPath);
}

void Router::pop() {
    if (m_routeStack.size() > 1) {
        m_routeStack.removeLast();
        const QString& top = m_routeStack.last();
        m_currentPath = extractPath(top);
        m_queryParams = extractQueryParams(top);
        emit routeChanged();
        emit navigationRequested(m_currentPath);
    }
}

void Router::replace(const QString& path) {
    if (!m_routeStack.isEmpty())
        m_routeStack.removeLast();
    m_routeStack.append(path);
    m_currentPath = extractPath(path);
    m_queryParams = extractQueryParams(path);
    applyRedirect();
    emit routeChanged();
    emit navigationRequested(m_currentPath);
}

QString Router::pathParameter(const QString& name) const {
    return m_pathParams.value(name).toString();
}

QString Router::queryParameter(const QString& name) const {
    return m_queryParams.value(name).toString();
}

void Router::setCanAccessApp(bool canAccess) {
    if (m_canAccessApp == canAccess) return;
    m_canAccessApp = canAccess;
    applyRedirect();
    emit routeChanged();
    emit navigationRequested(m_currentPath);
}

bool Router::isAuthRoute() const {
    return m_currentPath.startsWith(QStringLiteral("/auth"));
}

void Router::applyRedirect() {
    const bool isAuth = isAuthRoute();

    if (!m_canAccessApp && !isAuth) {
        m_routeStack.clear();
        m_routeStack.append(QStringLiteral("/auth/login"));
        m_currentPath = QStringLiteral("/auth/login");
        m_queryParams.clear();
    } else if (m_canAccessApp && isAuth) {
        m_routeStack.clear();
        m_routeStack.append(QStringLiteral("/"));
        m_currentPath = QStringLiteral("/");
        m_queryParams.clear();
    }
}

QString Router::extractPath(const QString& fullPath) const {
    const int qmark = fullPath.indexOf(QLatin1Char('?'));
    return qmark >= 0 ? fullPath.left(qmark) : fullPath;
}

QVariantMap Router::extractQueryParams(const QString& fullPath) const {
    QVariantMap params;
    const int qmark = fullPath.indexOf(QLatin1Char('?'));
    if (qmark < 0) return params;

    const QUrlQuery query(fullPath.mid(qmark + 1));
    for (const auto& item : query.queryItems()) {
        params[item.first] = item.second;
    }
    return params;
}

QVariantMap Router::extractPathParams(const QString& pattern, const QString& path) const {
    QVariantMap params;
    const auto patternParts = pattern.split(QLatin1Char('/'));
    const auto pathParts = path.split(QLatin1Char('/'));

    if (patternParts.size() != pathParts.size()) return params;

    for (int i = 0; i < patternParts.size(); ++i) {
        if (patternParts[i].startsWith(QLatin1Char(':'))) {
            params[patternParts[i].mid(1)] = pathParts[i];
        }
    }
    return params;
}

} // namespace gopost::core
