#pragma once

#include <QTest>

class TestPermissions : public QObject {
    Q_OBJECT
private slots:
    void desktopFileAccessGranted();
    void desktopStorageWriteGranted();
    void desktopCameraGranted();
    void permissionRequestedEventPublished();
    void denialHandlerPermanentlyDenied();
    void denialHandlerTemporaryDenied();
    void cachePreventsDuplicateCheck();
    void rationaleProviderReturnsText();
    void policyTableDesktop();
};
