#include "core/cache/image_cache_manager.h"

#include <QNetworkRequest>

namespace gopost::core {

ImageCacheManager::ImageCacheManager(QObject* parent)
    : QObject(parent) {
    m_cache.setMaxCost(100 * 1024 * 1024); // 100 MB default
    connect(&m_network, &QNetworkAccessManager::finished,
            this, &ImageCacheManager::onNetworkReply);
}

QPixmap* ImageCacheManager::getCached(const QUrl& url) const {
    return m_cache.object(url);
}

void ImageCacheManager::loadImage(const QUrl& url) {
    if (m_cache.contains(url)) {
        emit imageLoaded(url, *m_cache.object(url));
        return;
    }

    QNetworkRequest request(url);
    request.setAttribute(QNetworkRequest::CacheLoadControlAttribute,
                         QNetworkRequest::PreferCache);
    m_network.get(request);
}

void ImageCacheManager::setMaxCacheSize(int megabytes) {
    m_cache.setMaxCost(megabytes * 1024 * 1024);
}

void ImageCacheManager::onNetworkReply(QNetworkReply* reply) {
    reply->deleteLater();

    const QUrl url = reply->url();

    if (reply->error() != QNetworkReply::NoError) {
        emit imageError(url, reply->errorString());
        return;
    }

    const QByteArray data = reply->readAll();
    auto pixmap = new QPixmap;
    if (pixmap->loadFromData(data)) {
        const int cost = pixmap->width() * pixmap->height() * 4; // RGBA
        m_cache.insert(url, pixmap, cost);
        emit imageLoaded(url, *pixmap);
    } else {
        delete pixmap;
        emit imageError(url, QStringLiteral("Failed to decode image"));
    }
}

} // namespace gopost::core
