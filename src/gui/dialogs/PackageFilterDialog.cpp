/**
 * @file    PackageFilterDialog.cpp
 * @brief   Implementation of the filter dialog mapping UI to PackageQueryCriteria.
 * @author  Duong Anh Hao
 * @date    2026-07-27
 */

#include "PackageFilterDialog.h"
#include <QFormLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>

 // Enums from domain
#include "domain/entities/Category.h"
#include "domain/states/PackageStateId.h"

namespace wms::gui::dialogs {

    PackageFilterDialog::PackageFilterDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Filter Packages");
        setModal(true);
        setMinimumWidth(400);

        auto* mainLayout = new QVBoxLayout(this);
        mainLayout->setSpacing(15);

        // Build the UI groups
        setupClassificationGroup(mainLayout);
        setupQuickTogglesGroup(mainLayout);

        // Buttons: Apply, Reset, Cancel
        m_buttonBox = new QDialogButtonBox(this);
        auto* btnApply = m_buttonBox->addButton("Apply Filter", QDialogButtonBox::AcceptRole);
        auto* btnReset = m_buttonBox->addButton("Reset", QDialogButtonBox::ResetRole);
        auto* btnCancel = m_buttonBox->addButton(QDialogButtonBox::Cancel);

        mainLayout->addWidget(m_buttonBox);

        // Connect signals
        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
        connect(btnReset, &QPushButton::clicked, this, &PackageFilterDialog::resetFilters);
    }

    

    void PackageFilterDialog::setupClassificationGroup(QVBoxLayout* mainLayout)
    {
        auto* group = new QGroupBox("Classification & Constraints", this);
        auto* layout = new QFormLayout(group);

        // State Combo
        m_stateCombo = new QComboBox(this);
        m_stateCombo->addItem("Any State", -1); // Sentinel value for std::nullopt
        m_stateCombo->addItem("On Route", static_cast<int>(wms::domain::PackageStateId::OnRoute));
        m_stateCombo->addItem("In Storage", static_cast<int>(wms::domain::PackageStateId::InStorage));
        m_stateCombo->addItem("Dispatched", static_cast<int>(wms::domain::PackageStateId::Dispatched));
        m_stateCombo->addItem("Missing", static_cast<int>(wms::domain::PackageStateId::Missing));
        m_stateCombo->addItem("Overdue", static_cast<int>(wms::domain::PackageStateId::Overdue));

        // Category Combo
        m_categoryCombo = new QComboBox(this);
        m_categoryCombo->addItem("Any Category", -1); // Sentinel value for std::nullopt
        m_categoryCombo->addItem("Standard", static_cast<int>(wms::domain::Category::Standard));
        m_categoryCombo->addItem("Fragile", static_cast<int>(wms::domain::Category::Fragile));
        m_categoryCombo->addItem("Perishable", static_cast<int>(wms::domain::Category::Perishable));
        m_categoryCombo->addItem("Hazmat", static_cast<int>(wms::domain::Category::Hazmat));
        m_categoryCombo->addItem("Oversized", static_cast<int>(wms::domain::Category::Oversized));
        m_categoryCombo->addItem("Liquid", static_cast<int>(wms::domain::Category::Liquid));

        // Weight SpinBoxes (-1.0 means no limit)
        m_minWeightSpin = new QDoubleSpinBox(this);
        m_minWeightSpin->setRange(-1.0, 10000.0);
        m_minWeightSpin->setValue(-1.0);
        m_minWeightSpin->setSpecialValueText("No Minimum"); // Displays this when value is at min (-1.0)

        m_maxWeightSpin = new QDoubleSpinBox(this);
        m_maxWeightSpin->setRange(-1.0, 10000.0);
        m_maxWeightSpin->setValue(-1.0);
        m_maxWeightSpin->setSpecialValueText("No Maximum");

        layout->addRow("State:", m_stateCombo);
        layout->addRow("Category:", m_categoryCombo);
        layout->addRow("Min Weight (kg):", m_minWeightSpin);
        layout->addRow("Max Weight (kg):", m_maxWeightSpin);

        mainLayout->addWidget(group);
    }

    void PackageFilterDialog::setupQuickTogglesGroup(QVBoxLayout* mainLayout)
    {
        auto* group = new QGroupBox("Quick Toggles", this);
        auto* layout = new QVBoxLayout(group);

        m_overdueCheck = new QCheckBox("Show Overdue Packages Only", this);
        m_importedTodayCheck = new QCheckBox("Imported Today", this);
        m_exportDueTodayCheck = new QCheckBox("Export Due Today", this);

        layout->addWidget(m_overdueCheck);
        layout->addWidget(m_importedTodayCheck);
        layout->addWidget(m_exportDueTodayCheck);

        mainLayout->addWidget(group);
    }

    void PackageFilterDialog::resetFilters()
    {
        

        m_stateCombo->setCurrentIndex(0);
        m_categoryCombo->setCurrentIndex(0);

        m_minWeightSpin->setValue(-1.0);
        m_maxWeightSpin->setValue(-1.0);

        m_overdueCheck->setChecked(false);
        m_importedTodayCheck->setChecked(false);
        m_exportDueTodayCheck->setChecked(false);
    }

    wms::domain::PackageQueryCriteria PackageFilterDialog::getCriteria() const
    {
        wms::domain::PackageQueryCriteria criteria;

        
        // 1. Combo Boxes (check for sentinel value -1)
        if (m_stateCombo->currentData().toInt() != -1)
            criteria.state = static_cast<wms::domain::PackageStateId>(m_stateCombo->currentData().toInt());

        if (m_categoryCombo->currentData().toInt() != -1)
            criteria.category = static_cast<wms::domain::Category>(m_categoryCombo->currentData().toInt());

        // 2. Weight Limits (check for sentinel value -1.0)
        if (m_minWeightSpin->value() >= 0.0)
            criteria.minWeight = m_minWeightSpin->value();

        if (m_maxWeightSpin->value() >= 0.0)
            criteria.maxWeight = m_maxWeightSpin->value();

        // 3. Booleans
        criteria.overdueOnly = m_overdueCheck->isChecked();
        criteria.importedToday = m_importedTodayCheck->isChecked();
        criteria.exportDueToday = m_exportDueTodayCheck->isChecked();

        return criteria;
    }

} // namespace wms::gui::dialogs
