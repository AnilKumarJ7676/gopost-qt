#pragma once

#include <QObject>
#include <QList>
#include <QString>

#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/entities/template_filter.h"
#include "template_browser/domain/usecases/get_templates_usecase.h"

namespace gopost::template_browser {

/// SRP: Only manages the paginated template list state.
class TemplateListNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<TemplateEntity> templates READ templates NOTIFY templatesChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(bool isLoadingMore READ isLoadingMore NOTIFY isLoadingMoreChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)

public:
    explicit TemplateListNotifier(GetTemplatesUseCase* getTemplates, QObject* parent = nullptr);
    ~TemplateListNotifier() override = default;

    const QList<TemplateEntity>& templates() const { return m_templates; }
    bool isLoading() const { return m_isLoading; }
    bool isLoadingMore() const { return m_isLoadingMore; }
    QString error() const { return m_error; }
    bool hasMore() const { return m_hasMore; }
    const TemplateFilter& filter() const { return m_filter; }

    Q_INVOKABLE void loadTemplates();
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void updateFilter(const TemplateFilter& filter);
    Q_INVOKABLE void refresh();

signals:
    void templatesChanged();
    void isLoadingChanged();
    void isLoadingMoreChanged();
    void errorChanged();
    void hasMoreChanged();

private:
    void doLoad(const TemplateFilter& filter);

    GetTemplatesUseCase* m_getTemplates;
    QList<TemplateEntity> m_templates;
    bool m_isLoading = false;
    bool m_isLoadingMore = false;
    QString m_error;
    TemplateFilter m_filter;
    std::optional<QString> m_nextCursor;
    bool m_hasMore = true;
};

} // namespace gopost::template_browser
