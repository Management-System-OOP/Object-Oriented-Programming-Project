/**
 * @file    FilterPanel.cpp
 * @brief   Implementation of the filter control panel for the Package Inventory page.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 */

#include "FilterPanel.h"

#include "domain/entities/Category.h"
#include "domain/states/PackageStateId.h"

#include <QHBoxLayout>
#include <QLabel>

namespace wms::gui {

    FilterPanel::FilterPanel(QWidget* parent)
        : QWidget(parent)
    {
        auto* layout = new QHBoxLayout(this);
        layout->setContentsMargins(15, 10, 15, 10);
        layout->setSpacing(12);

        layout->addWidget(new QLabel("Status:", this));
        m_stateCombo = new QComboBox(this);
        m_stateCombo->addItem("All Statuses", -1);
        m_stateCombo->addItem("On Route", static_cast<int>(wms::domain::PackageStateId::OnRoute));
        m_stateCombo->addItem("In Storage", static_cast<int>(wms::domain::PackageStateId::InStorage));
        m_stateCombo->addItem("Dispatched", static_cast<int>(wms::domain::PackageStateId::Dispatched));
        m_stateCombo->addItem("Missing", static_cast<int>(wms::domain::PackageStateId::Missing));
        m_stateCombo->addItem("Overdue", static_cast<int>(wms::domain::PackageStateId::Overdue));
        layout->addWidget(m_stateCombo);

        layout->addWidget(new QLabel("Category:", this));
        m_categoryCombo = new QComboBox(this);
        m_categoryCombo->addItem("All Categories", -1);
        m_categoryCombo->addItem("Standard", static_cast<int>(wms::domain::Category::Standard));
        m_categoryCombo->addItem("Fragile", static_cast<int>(wms::domain::Category::Fragile));
        m_categoryCombo->addItem("Perishable", static_cast<int>(wms::domain::Category::Perishable));
        m_categoryCombo->addItem("Hazmat", static_cast<int>(wms::domain::Category::Hazmat));
        m_categoryCombo->addItem("Oversized", static_cast<int>(wms::domain::Category::Oversized));
        m_categoryCombo->addItem("Liquid", static_cast<int>(wms::domain::Category::Liquid));
        layout->addWidget(m_categoryCombo);

        layout->addWidget(new QLabel("Zone:", this));
        m_zoneEdit = new QLineEdit(this);
        m_zoneEdit->setPlaceholderText("e.g. ZoneA");
        m_zoneEdit->setFixedWidth(120);
        layout->addWidget(m_zoneEdit);

        layout->addStretch();

        m_searchEdit = new QLineEdit(this);
        m_searchEdit->setPlaceholderText("Search ID or description...");
        m_searchEdit->setFixedWidth(240);
        layout->addWidget(m_searchEdit);

        m_clearBtn = new QPushButton("Clear Filters", this);
        layout->addWidget(m_clearBtn);

        connect(m_stateCombo, &QComboBox::currentIndexChanged, this, &FilterPanel::onFilterChanged);
        connect(m_categoryCombo, &QComboBox::currentIndexChanged, this, &FilterPanel::onFilterChanged);
        connect(m_zoneEdit, &QLineEdit::textChanged, this, &FilterPanel::onFilterChanged);
        connect(m_searchEdit, &QLineEdit::textChanged, this, &FilterPanel::onSearchChanged);
        connect(m_clearBtn, &QPushButton::clicked, this, &FilterPanel::onClearClicked);
    }

    int FilterPanel::stateFilterData() const
    {
        return m_stateCombo->currentData().toInt();
    }

    int FilterPanel::categoryFilterData() const
    {
        return m_categoryCombo->currentData().toInt();
    }

    QString FilterPanel::zoneFilterText() const
    {
        return m_zoneEdit->text().trimmed();
    }

    QString FilterPanel::searchText() const
    {
        return m_searchEdit->text().trimmed();
    }

    void FilterPanel::resetControls()
    {
        m_stateCombo->setCurrentIndex(0);
        m_categoryCombo->setCurrentIndex(0);
        m_zoneEdit->clear();
        m_searchEdit->clear();
    }

    void FilterPanel::onFilterChanged()
    {
        emit filtersChanged();
    }

    void FilterPanel::onSearchChanged(const QString& /*text*/)
    {
        emit filtersChanged();
    }

    void FilterPanel::onClearClicked()
    {
        resetControls();
        emit clearFiltersRequested();
    }

} // namespace wms::gui