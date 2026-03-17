#pragma once

#include <QObject>
#include <QString>
#include <optional>

#include "template_browser/domain/entities/template_entity.h"
#include "template_browser/domain/usecases/get_template_detail_usecase.h"

namespace gopost::template_browser {

/// SRP: Only manages loading a single template detail.
class TemplateDetailNotifier : public QObject {
    Q_OBJECT
    Q_PROPERTY(TemplateEntity templateData READ templateData NOTIFY templateChanged)
    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged)
    Q_PROPERTY(QString error READ error NOTIFY errorChanged)
    Q_PROPERTY(bool hasTemplate READ hasTemplate NOTIFY templateChanged)

public:
    explicit TemplateDetailNotifier(GetTemplateDetailUseCase* getDetail, QObject* parent = nullptr);
    ~TemplateDetailNotifier() override = default;

    TemplateEntity templateData() const { return m_template.value_or(TemplateEntity()); }
    bool isLoading() const { return m_isLoading; }
    QString error() const { return m_error; }
    bool hasTemplate() const { return m_template.has_value(); }

    Q_INVOKABLE void load(const QString& id);

signals:
    void templateChanged();
    void isLoadingChanged();
    void errorChanged();

private:
    GetTemplateDetailUseCase* m_getDetail;
    std::optional<TemplateEntity> m_template;
    bool m_isLoading = false;
    QString m_error;
};

} // namespace gopost::template_browser
