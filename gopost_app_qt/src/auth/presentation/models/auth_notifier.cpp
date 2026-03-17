#include "auth/presentation/models/auth_notifier.h"

#include "core/constants/api_constants.h"
#include "core/logging/app_logger.h"
#include "core/security/secure_storage_service.h"
#include "core/network/interceptors/auth_interceptor.h"

namespace gopost::auth {

AuthNotifier::AuthNotifier(
    LoginUseCase* loginUseCase,
    RegisterUseCase* registerUseCase,
    LogoutUseCase* logoutUseCase,
    core::SecureStorageService* secureStorage,
    core::AuthInterceptor* authInterceptor,
    AuthRepository* authRepository,
    QObject* parent)
    : QObject(parent)
    , m_loginUseCase(loginUseCase)
    , m_registerUseCase(registerUseCase)
    , m_logoutUseCase(logoutUseCase)
    , m_secureStorage(secureStorage)
    , m_authInterceptor(authInterceptor)
    , m_authRepository(authRepository)
{
    if (m_authInterceptor) {
        m_authInterceptor->onSessionExpired = [this]() { handleSessionExpired(); };
    }
    if (m_secureStorage && m_authRepository) {
        tryAutoLogin();
    } else {
        // Dependencies not available yet – start in unauthenticated state
        setState(AuthState(AuthStatus::Unauthenticated));
    }
}

// ---------- Property accessors ----------

int AuthNotifier::statusInt() const
{
    return static_cast<int>(m_state.status());
}

QString AuthNotifier::userName() const
{
    return m_state.user().name();
}

QString AuthNotifier::userEmail() const
{
    return m_state.user().email();
}

QString AuthNotifier::userAvatarUrl() const
{
    return m_state.user().avatarUrl();
}

QString AuthNotifier::errorMessage() const
{
    return m_state.errorMessage();
}

bool AuthNotifier::isAuthenticated() const
{
    return m_state.isAuthenticated();
}

bool AuthNotifier::isGuest() const
{
    return m_state.isGuest();
}

bool AuthNotifier::canAccessApp() const
{
    return m_state.canAccessApp();
}

bool AuthNotifier::isLoading() const
{
    return m_state.status() == AuthStatus::Loading;
}

// ---------- Actions ----------

void AuthNotifier::login(const QString& email, const QString& password)
{
    if (!m_loginUseCase) return;

    setState(m_state.copyWith(AuthStatus::Loading));

    (*m_loginUseCase)(email, password, [this](AuthResult<AuthPair> result) {
        if (auto* pair = std::get_if<AuthPair>(&result)) {
            const auto& [tokens, user] = *pair;
            if (m_authInterceptor) m_authInterceptor->setToken(tokens.accessToken());
            if (m_secureStorage) m_secureStorage->write(core::ApiConstants::refreshTokenKey, tokens.refreshToken());

            setState(AuthState(
                AuthStatus::Authenticated,
                user,
                tokens.accessToken(),
                tokens.refreshToken()
            ));
            emit authenticated();
        } else if (auto* failure = std::get_if<core::Failure>(&result)) {
            setState(m_state.copyWith(AuthStatus::Error,
                                      std::nullopt, std::nullopt, std::nullopt,
                                      failure->message()));
            emit authError(failure->message());
        }
    });
}

void AuthNotifier::register_(const QString& name, const QString& email, const QString& password)
{
    if (!m_registerUseCase) return;

    setState(m_state.copyWith(AuthStatus::Loading));

    (*m_registerUseCase)(name, email, password, [this](AuthResult<AuthPair> result) {
        if (auto* pair = std::get_if<AuthPair>(&result)) {
            const auto& [tokens, user] = *pair;
            if (m_authInterceptor) m_authInterceptor->setToken(tokens.accessToken());
            if (m_secureStorage) m_secureStorage->write(core::ApiConstants::refreshTokenKey, tokens.refreshToken());

            setState(AuthState(
                AuthStatus::Authenticated,
                user,
                tokens.accessToken(),
                tokens.refreshToken()
            ));
            emit authenticated();
        } else if (auto* failure = std::get_if<core::Failure>(&result)) {
            setState(m_state.copyWith(AuthStatus::Error,
                                      std::nullopt, std::nullopt, std::nullopt,
                                      failure->message()));
            emit authError(failure->message());
        }
    });
}

void AuthNotifier::logout()
{
    const QString refreshToken = m_state.refreshToken();
    if (!refreshToken.isEmpty() && m_logoutUseCase) {
        (*m_logoutUseCase)(refreshToken, [](std::optional<core::Failure> /*ignored*/) {
            // Best-effort server logout; ignore errors.
        });
    }

    clearSession();
    emit loggedOut();
}

void AuthNotifier::skipAuth()
{
    setState(AuthState(AuthStatus::Guest));
}

void AuthNotifier::tryAutoLogin()
{
    const auto refreshToken = m_secureStorage->read(core::ApiConstants::refreshTokenKey);
    if (!refreshToken.has_value() || refreshToken->isEmpty()) {
        setState(AuthState(AuthStatus::Unauthenticated));
        return;
    }

    setState(m_state.copyWith(AuthStatus::Loading));

    m_authRepository->refreshToken(*refreshToken, [this](AuthResult<AuthTokens> result) {
        if (auto* tokens = std::get_if<AuthTokens>(&result)) {
            m_authInterceptor->setToken(tokens->accessToken());
            m_secureStorage->write(core::ApiConstants::refreshTokenKey, tokens->refreshToken());

            setState(AuthState(
                AuthStatus::Authenticated,
                {},  // user will be null until profile fetch
                tokens->accessToken(),
                tokens->refreshToken()
            ));
            core::AppLogger::info(QStringLiteral("Auto-login successful"));
        } else {
            core::AppLogger::warning(QStringLiteral("Auto-login failed"));
            clearSession();
        }
    });
}

// ---------- Private ----------

void AuthNotifier::setState(const AuthState& newState)
{
    if (m_state != newState) {
        m_state = newState;
        emit stateChanged();
    }
}

void AuthNotifier::handleSessionExpired()
{
    core::AppLogger::info(QStringLiteral("Session expired - forcing logout"));
    clearSession();
    emit loggedOut();
}

void AuthNotifier::clearSession()
{
    if (m_authInterceptor) m_authInterceptor->clearToken();
    if (m_secureStorage) m_secureStorage->delete_(core::ApiConstants::refreshTokenKey);
    setState(AuthState(AuthStatus::Unauthenticated));
}

} // namespace gopost::auth
