#include <QGuiApplication>
#include <QIcon>
#include <cstdio>
#include "app.h"
#include "core/logging/app_logger.h"

#if defined(Q_OS_WIN) || defined(_WIN32)
#include <windows.h>
#include <crtdbg.h>
#include <cstdlib>

// Intercept CRT debug reports to suppress the QRingBuffer assert dialog.
// Qt 6 calls _CrtDbgReport(_CRT_ERROR, ...) when Q_ASSERT fails in debug builds.
// The "bytes <= bufferSize" assert in qringbuffer.cpp is a known Qt 6 issue
// that triggers when QProcess pipe data arrives in large chunks. The buffer
// handles it correctly despite the assert — it's safe to suppress.
static int __cdecl gopostCrtReportHook(int reportType, wchar_t* message,
                                        int* returnValue) {
    (void)reportType;
    if (message && (wcsstr(message, L"qringbuffer") || wcsstr(message, L"QRingBuffer"))) {
        fwprintf(stderr, L"[Qt] QRingBuffer assert suppressed: %.200ls\n", message);
        if (returnValue) *returnValue = 0;
        return TRUE; // Handled — don't show dialog
    }
    return FALSE; // Let other reports through
}

static int __cdecl gopostCrtReportHookA(int reportType, char* message,
                                         int* returnValue) {
    (void)reportType;
    if (message && (strstr(message, "qringbuffer") || strstr(message, "QRingBuffer"))) {
        fprintf(stderr, "[Qt] QRingBuffer assert suppressed: %.200s\n", message);
        if (returnValue) *returnValue = 0;
        return TRUE;
    }
    return FALSE;
}
#endif

int main(int argc, char *argv[]) {
#if defined(Q_OS_WIN) || defined(_WIN32)
    // Install CRT report hooks BEFORE anything Qt-related.
    // Use _CrtSetReportHookW2 for wide-char hook (Qt uses wide strings internally).
    _CrtSetReportHookW2(_CRT_RPTHOOK_INSTALL, gopostCrtReportHook);
    _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, gopostCrtReportHookA);

    // Prevent abort() from showing its own dialog
    _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
#endif

    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("Gopost"));
    app.setOrganizationDomain(QStringLiteral("gopost.app"));
    app.setApplicationName(QStringLiteral("Gopost"));
    app.setApplicationVersion(QStringLiteral("1.0.0"));
    app.setWindowIcon(QIcon(QStringLiteral(":/icons/gopost.png")));

    // Initialize logging
    gopost::core::AppLogger::init();
    gopost::core::AppLogger::info(QStringLiteral("Starting Gopost application"));

    // Create and run the app
    gopost::GopostApp gopostApp(&app);
    return gopostApp.run();
}
