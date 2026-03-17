#include "template_browser/presentation/models/template_providers.h"

#include "core/network/http_client.h"

namespace gopost::template_browser {

TemplateProviders::TemplateProviders(core::HttpClient* httpClient, QObject* parent)
    : QObject(parent)
{
    // Data layer
    m_dataSource = std::make_unique<TemplateRemoteDataSourceImpl>(httpClient, this);
    m_repository = std::make_unique<TemplateRepositoryImpl>(m_dataSource.get());

    // Use cases
    m_getTemplatesUC = std::make_unique<GetTemplatesUseCase>(m_repository.get());
    m_getDetailUC = std::make_unique<GetTemplateDetailUseCase>(m_repository.get());
    m_searchUC = std::make_unique<SearchTemplatesUseCase>(m_repository.get());
    m_getCategoriesUC = std::make_unique<GetCategoriesUseCase>(m_repository.get());
    m_requestAccessUC = std::make_unique<RequestTemplateAccessUseCase>(m_repository.get());
    m_getFeaturedUC = std::make_unique<GetFeaturedTemplatesUseCase>(m_repository.get());
    m_getTrendingUC = std::make_unique<GetTrendingTemplatesUseCase>(m_repository.get());
    m_getRecentUC = std::make_unique<GetRecentTemplatesUseCase>(m_repository.get());

    // Notifiers
    m_home = std::make_unique<HomeNotifier>(
        m_getFeaturedUC.get(), m_getTrendingUC.get(),
        m_getRecentUC.get(), m_getCategoriesUC.get(), this);

    m_templateList = std::make_unique<TemplateListNotifier>(m_getTemplatesUC.get(), this);
    m_templateSearch = std::make_unique<TemplateSearchNotifier>(m_searchUC.get(), this);
    m_download = std::make_unique<DownloadNotifier>(
        m_requestAccessUC.get(), m_repository.get(), this);
}

TemplateDetailNotifier* TemplateProviders::createDetailNotifier(const QString& id)
{
    auto* notifier = new TemplateDetailNotifier(m_getDetailUC.get(), this);
    notifier->load(id);
    return notifier;
}

void TemplateProviders::registerTypes()
{
    qRegisterMetaType<TemplateEntity>("TemplateEntity");
    qRegisterMetaType<CategoryEntity>("CategoryEntity");
    qRegisterMetaType<EditableField>("EditableField");
    qRegisterMetaType<TemplateAccess>("TemplateAccess");
    qRegisterMetaType<TemplateFilter>("TemplateFilter");
    qRegisterMetaType<QList<TemplateEntity>>("QList<TemplateEntity>");
    qRegisterMetaType<QList<CategoryEntity>>("QList<CategoryEntity>");
    qRegisterMetaType<QList<EditableField>>("QList<EditableField>");

    qmlRegisterUncreatableType<HomeNotifier>(
        "GoPost.TemplateBrowser", 1, 0, "HomeNotifier", QStringLiteral("Use TemplateProviders"));
    qmlRegisterUncreatableType<TemplateListNotifier>(
        "GoPost.TemplateBrowser", 1, 0, "TemplateListNotifier", QStringLiteral("Use TemplateProviders"));
    qmlRegisterUncreatableType<TemplateSearchNotifier>(
        "GoPost.TemplateBrowser", 1, 0, "TemplateSearchNotifier", QStringLiteral("Use TemplateProviders"));
    qmlRegisterUncreatableType<TemplateDetailNotifier>(
        "GoPost.TemplateBrowser", 1, 0, "TemplateDetailNotifier", QStringLiteral("Use TemplateProviders"));
    qmlRegisterUncreatableType<DownloadNotifier>(
        "GoPost.TemplateBrowser", 1, 0, "DownloadNotifier", QStringLiteral("Use TemplateProviders"));
}

} // namespace gopost::template_browser
