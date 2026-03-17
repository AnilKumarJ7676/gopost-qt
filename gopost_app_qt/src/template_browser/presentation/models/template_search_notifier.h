#pragma once

#include <QObject>
#include <QTimer>
#include <QList>
#include <QString>

#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/entities/template_filter.h"
#include "template_browser/domain/usecases/search_templates_usecase.h"

namespace gopost::template_browser {

/// SRP: Handles debounced search with pagination support.
class TemplateSearchNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(QList<TemplateEntity> results READ results NOTIFY resultsChanged)
    Q_PROPERTY(bool isSearching READ isSearching NOTIFY isSearchingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(QString query READ query NOTIFY queryChanged)
    Q_PROPERTY(bool hasMore READ hasMore NOTIFY hasMoreChanged)

public:
    explicit TemplateSearchNotifier(SearchTemplatesUseCase* searchTemplates, QObject* parent = nullptr);
    ~TemplateSearchNotifier() override = default;

    const QList<TemplateEntity>& results() const { return m_results; }
    bool isSearching() const { return m_isSearching; }
    QString error() const { return m_error; }
    QString query() const { return m_query; }
    bool hasMore() const { return m_hasMore; }

    Q_INVOKABLE void search(const QString& query);
    Q_INVOKABLE void loadMore();
    Q_INVOKABLE void updateFilter(const TemplateFilter& filter);
    Q_INVOKABLE void clear();

signals:
    void resultsChanged();
    void isSearchingChanged();
    void errorChanged();
    void queryChanged();
    void hasMoreChanged();

private:
    void executeSearch(const QString& query);

    SearchTemplatesUseCase* m_searchTemplates;
    QTimer m_debounceTimer;
    QList<TemplateEntity> m_results;
    bool m_isSearching = false;
    QString m_error;
    QString m_query;
    bool m_hasMore = false;
    std::optional<QString> m_nextCursor;
    TemplateFilter m_filter;
};

} // namespace gopost::template_browser
