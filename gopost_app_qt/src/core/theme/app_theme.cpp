#include "core/theme/app_theme.h"
#include "core/theme/app_colors.h"

namespace gopost::core {

QPalette AppTheme::light() {
    QPalette p;

    p.setColor(QPalette::Window, AppColors::neutralBackgroundLight);
    p.setColor(QPalette::WindowText, AppColors::textPrimaryLight);
    p.setColor(QPalette::Base, AppColors::neutralSurfaceLight);
    p.setColor(QPalette::AlternateBase, AppColors::neutralSurfaceVariantLight);
    p.setColor(QPalette::Text, AppColors::textPrimaryLight);
    p.setColor(QPalette::Button, AppColors::brandPrimary);
    p.setColor(QPalette::ButtonText, Qt::white);
    p.setColor(QPalette::Highlight, AppColors::brandPrimary);
    p.setColor(QPalette::HighlightedText, Qt::white);
    p.setColor(QPalette::Link, AppColors::brandPrimary);
    p.setColor(QPalette::PlaceholderText, AppColors::textSecondaryLight);
    p.setColor(QPalette::Mid, AppColors::neutralOutlineLight);

    return p;
}

QPalette AppTheme::dark() {
    QPalette p;

    p.setColor(QPalette::Window, AppColors::neutralBackgroundDark);
    p.setColor(QPalette::WindowText, AppColors::textPrimaryDark);
    p.setColor(QPalette::Base, AppColors::neutralSurfaceDark);
    p.setColor(QPalette::AlternateBase, AppColors::neutralSurfaceVariantDark);
    p.setColor(QPalette::Text, AppColors::textPrimaryDark);
    p.setColor(QPalette::Button, AppColors::brandPrimaryDark);
    p.setColor(QPalette::ButtonText, Qt::black);
    p.setColor(QPalette::Highlight, AppColors::brandPrimaryDark);
    p.setColor(QPalette::HighlightedText, Qt::black);
    p.setColor(QPalette::Link, AppColors::brandPrimaryDark);
    p.setColor(QPalette::PlaceholderText, AppColors::textSecondaryDark);
    p.setColor(QPalette::Mid, AppColors::neutralOutlineDark);

    return p;
}

} // namespace gopost::core
