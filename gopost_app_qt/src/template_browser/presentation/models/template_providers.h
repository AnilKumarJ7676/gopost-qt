#pragma once

#include <QObject>
#include <QQmlEngine>
#include <memory>

#include "template_browser/data/datasources/template_remote_datasource.h"
#include "template_browser/data/repositories/template_repository_impl.h"
#include "template_browser/domain/usecases/get_categories_usecase.h"
#include "template_browser/domain/usecases/get_featured_templates_usecase.h"
#include "template_browser/domain/usecases/get_template_detail_usecase.h"
#include "template_browser/domain/usecases/get_templates_usecase.h"
#include "template_browser/domain/usecases/request_template_access_usecase.h"
#include "template_browser/domain/usecases/search_templates_usecase.h"
#include "template_browser/presentation/models/download_notifier.h"
#include "template_browser/presentation/models/home_notifier.h"
#include "template_browser/presentation/models/template_detail_notifier.h"
#include "template_browser/presentation/models/template_list_notifier.h"
#include "template_browser/presentation/models/template_search_notifier.h"

namespace gopost::core { class HttpClient; }

namespace gopost::template_browser {

/// DIP: Wires the template browser dependency graph and exposes
/// notifiers to QML. Upper layers depend on abstract TemplateRepository,
/// not the concrete TemplateRepositoryImpl.
class TemplateProviders : public QObject {
    Q_OBJECT
    Q_PROPERTY(HomeNotifier* home READ home CONSTANT)
    Q_PROPERTY(TemplateListNotifier* templateList READ templateList CONSTANT)
    Q_PROPERTY(TemplateSearchNotifier* templateSearch READ templateSearch CONSTANT)
    Q_PROPERTY(DownloadNotifier* download READ download CONSTANT)

public:
    explicit TemplateProviders(core::HttpClient* httpClient, QObject* parent = nullptr);
    ~TemplateProviders() override = default;

    HomeNotifier* home() const { return m_home.get(); }
    TemplateListNotifier* templateList() const { return m_templateList.get(); }
    TemplateSearchNotifier* templateSearch() const { return m_templateSearch.get(); }
    DownloadNotifier* download() const { return m_download.get(); }

    /// Create a TemplateDetailNotifier for a specific template ID.
    Q_INVOKABLE TemplateDetailNotifier* createDetailNotifier(const QString& id);

    /// Register QML types for the template browser module.
    static void registerTypes();

private:
    // Data layer
    std::unique_ptr<TemplateRemoteDataSourceImpl> m_dataSource;
    std::unique_ptr<TemplateRepositoryImpl> m_repository;

    // Use cases
    std::unique_ptr<GetTemplatesUseCase> m_getTemplatesUC;
    std::unique_ptr<GetTemplateDetailUseCase> m_getDetailUC;
    std::unique_ptr<SearchTemplatesUseCase> m_searchUC;
    std::unique_ptr<GetCategoriesUseCase> m_getCategoriesUC;
    std::unique_ptr<RequestTemplateAccessUseCase> m_requestAccessUC;
    std::unique_ptr<GetFeaturedTemplatesUseCase> m_getFeaturedUC;
    std::unique_ptr<GetTrendingTemplatesUseCase> m_getTrendingUC;
    std::unique_ptr<GetRecentTemplatesUseCase> m_getRecentUC;

    // Notifiers
    std::unique_ptr<HomeNotifier> m_home;
    std::unique_ptr<TemplateListNotifier> m_templateList;
    std::unique_ptr<TemplateSearchNotifier> m_templateSearch;
    std::unique_ptr<DownloadNotifier> m_download;
};

} // namespace gopost::template_browser
