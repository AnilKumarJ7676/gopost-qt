#include "template_browser/presentation/models/template_detail_notifier.h"

namespace gopost::template_browser {

TemplateDetailNotifier::TemplateDetailNotifier(GetTemplateDetailUseCase* getDetail, QObject* parent)
    : QObject(parent)
    , m_getDetail(getDetail)
{
}

void TemplateDetailNotifier::load(const QString& id)
{
    m_isLoading = true;
    m_error.clear();
    emit isLoadingChanged();
    emit errorChanged();

    (*m_getDetail)(id, [this](auto result) {
        m_isLoading = false;
        if (auto* entity = std::get_if<TemplateEntity>(&result)) {
            m_template = *entity;
            emit templateChanged();
        } else {
            const auto& failure = std::get<core::Failure>(result);
            m_error = failure.message();
            emit errorChanged();
        }
        emit isLoadingChanged();
    });
}

} // namespace gopost::template_browser
