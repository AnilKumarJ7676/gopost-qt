#pragma once

#include <QObject>
#include <QList>
#include <QString>

#include "template_browser/domain/entities/category_entity.h"
#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/usecases/get_categories_usecase.h"
#include "template_browser/domain/usecases/get_featured_templates_usecase.h"

namespace gopost::template_browser {

/// SRP: Orchestrates loading home screen data from multiple use cases.
class HomeNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<TemplateEntity> featured READ featured NOTIFY dataChanged)
    Q_PROPERTY(QList<TemplateEntity> trending READ trending NOTIFY dataChanged)
    Q_PROPERTY(QList<TemplateEntity> recent READ recent NOTIFY dataChanged)
    Q_PROPERTY(QList<CategoryEntity> categories READ categories NOTIFY dataChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)

public:
    HomeNotifier(GetFeaturedTemplatesUseCase* getFeatured,
                 GetTrendingTemplatesUseCase* getTrending,
                 GetRecentTemplatesUseCase* getRecent,
                 GetCategoriesUseCase* getCategories,
                 QObject* parent = nullptr);
    ~HomeNotifier() override = default;

    const QList<TemplateEntity>& featured() const { return m_featured; }
    const QList<TemplateEntity>& trending() const { return m_trending; }
    const QList<TemplateEntity>& recent() const { return m_recent; }
    const QList<CategoryEntity>& categories() const { return m_categories; }
    bool isLoading() const { return m_isLoading; }
    QString error() const { return m_error; }

    Q_INVOKABLE void load();
    Q_INVOKABLE void refresh();

signals:
    void dataChanged();
    void isLoadingChanged();
    void errorChanged();

private:
    void checkAllLoaded();

    GetFeaturedTemplatesUseCase* m_getFeatured;
    GetTrendingTemplatesUseCase* m_getTrending;
    GetRecentTemplatesUseCase* m_getRecent;
    GetCategoriesUseCase* m_getCategories;

    QList<TemplateEntity> m_featured;
    QList<TemplateEntity> m_trending;
    QList<TemplateEntity> m_recent;
    QList<CategoryEntity> m_categories;
    bool m_isLoading = false;
    QString m_error;
    int m_pendingRequests = 0;
};

} // namespace gopost::template_browser
