#pragma once

#include <QObject>
#include <QPixmap>
#include <QCache>
#include <QUrl>
#include <QNetworkAccessManager>
#include <QNetworkReply>

namespace gopost::core {

class ImageCacheManager : public QObject {
    Q_OBJECT
public:
    explicit ImageCacheManager(QObject* parent = nullptr);

    QPixmap* getCached(const QUrl& url) const;
    void loadImage(const QUrl& url);

    void setMaxCacheSize(int megabytes);

signals:
    void imageLoaded(const QUrl& url, const QPixmap& pixmap);
    void imageError(const QUrl& url, const QString& error);

private slots:
    void onNetworkReply(QNetworkReply* reply);

private:
    QCache<QUrl, QPixmap> m_cache;
    QNetworkAccessManager m_network;
};

} // namespace gopost::core
