/**
 * @file   FilterPanel.cpp
 * @brief  Implementation of FilterPanel triggering custom Qt signals.
 * @author Nguyen Viet Bach
 * @date   2026-06-23
 */

#include "gui/FilterPanel.h"

FilterPanel::FilterPanel(QWidget* parent)
    : QWidget(parent)
    , m_stateComboBox(new QComboBox(this))
    , m_applyButton(new QPushButton("Apply Filter", this))
{
    auto* layout = new QVBoxLayout(this);
    m_stateComboBox->addItem("All States", -1);
    m_stateComboBox->addItem("On Route", 0);
    m_stateComboBox->addItem("In Storage", 1);

    layout->addWidget(m_stateComboBox);
    layout->addWidget(m_applyButton);

    connect(m_applyButton, &QPushButton::clicked, this, &FilterPanel::on_applyButton_clicked);
}

void FilterPanel::on_applyButton_clicked()
{
    emit filterChanged();
}

std::vector<wms::service::PackageFilter::Predicate> FilterPanel::currentPredicates() const
{
    std::vector<wms::service::PackageFilter::Predicate> predicates;
    int index = m_stateComboBox->currentData().toInt();

    if (index == 0) {
        predicates.push_back(wms::service::PackageFilter::byState(wms::domain::PackageStateId::OnRoute));
    }
    else if (index == 1) {
        predicates.push_back(wms::service::PackageFilter::byState(wms::domain::PackageStateId::InStorage));
    }
    return predicates;
}