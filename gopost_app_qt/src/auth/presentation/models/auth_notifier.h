#pragma once

#include <QObject>
#include <QString>
#include <memory>

#include "auth/presentation/models/auth_state.h"
#include "auth/domain/entities/user_entity.h"
#include "auth/domain/usecases/login_usecase.h"
#include "auth/domain/usecases/register_usecase.h"
#include "auth/domain/usecases/logout_usecase.h"
#include "auth/domain/repositories/auth_repository.h"

namespace gopost::core {
class SecureStorageService;
class AuthInterceptor;
} // namespace gopost::core

namespace gopost::auth {

/// QObject-based state notifier for authentication, exposed to QML.
class AuthNotifier : public QObject {
    Q_OBJECT

    Q_PROPERTY(int status READ statusInt NOTIFY stateChanged)
    Q_PROPERTY(QString userName READ userName NOTIFY stateChanged)
    Q_PROPERTY(QString userEmail READ userEmail NOTIFY stateChanged)
    Q_PROPERTY(QString userAvatarUrl READ userAvatarUrl NOTIFY stateChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY stateChanged)
    Q_PROPERTY(bool isAuthenticated READ isAuthenticated NOTIFY stateChanged)
    Q_PROPERTY(bool isGuest READ isGuest NOTIFY stateChanged)
    Q_PROPERTY(bool canAccessApp READ canAccessApp NOTIFY stateChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY stateChanged)

public:
    explicit AuthNotifier(
        LoginUseCase* loginUseCase,
        RegisterUseCase* registerUseCase,
        LogoutUseCase* logoutUseCase,
        core::SecureStorageService* secureStorage,
        core::AuthInterceptor* authInterceptor,
        AuthRepository* authRepository,
        QObject* parent = nullptr);

    ~AuthNotifier() override = default;

    // Property accessors
    int statusInt() const;
    QString userName() const;
    QString userEmail() const;
    QString userAvatarUrl() const;
    QString errorMessage() const;
    bool isAuthenticated() const;
    bool isGuest() const;
    bool canAccessApp() const;
    bool isLoading() const;

    const AuthState& state() const { return m_state; }

    Q_INVOKABLE void login(const QString& email, const QString& password);
    Q_INVOKABLE void register_(const QString& name, const QString& email, const QString& password);
    Q_INVOKABLE void logout();
    Q_INVOKABLE void skipAuth();
    Q_INVOKABLE void tryAutoLogin();

signals:
    void stateChanged();
    void authError(const QString& message);
    void authenticated();
    void loggedOut();

private:
    void setState(const AuthState& newState);
    void handleSessionExpired();
    void clearSession();

    AuthState m_state;
    LoginUseCase* m_loginUseCase;
    RegisterUseCase* m_registerUseCase;
    LogoutUseCase* m_logoutUseCase;
    core::SecureStorageService* m_secureStorage;
    core::AuthInterceptor* m_authInterceptor;
    AuthRepository* m_authRepository;
};

} // namespace gopost::auth
