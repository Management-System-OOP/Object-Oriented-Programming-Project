/**
 * @file   AddPackageDialog.cpp
 * @brief  Implementation of AddPackageDialog mapping UI fields to pure C++ value objects.
 * @author Nguyen Viet Bach
 * @date   2026-06-24
 */

#include "gui/dialogs/AddPackageDialog.h"
#include <chrono>

AddPackageDialog::AddPackageDialog(QWidget* parent)
    : QDialog(parent)
    , m_descEdit(new QLineEdit(this))
    , m_weightSpin(new QDoubleSpinBox(this))
    , m_saveButton(new QPushButton("Save Package", this))
{
    auto* layout = new QFormLayout(this);
    m_weightSpin->setRange(0.1, 1000.0);

    layout->addRow("Description:", m_descEdit);
    layout->addRow("Weight (kg):", m_weightSpin);
    layout->addWidget(m_saveButton);

    // Section 6.2: Modern pointer-based signal/slot connections
    connect(m_saveButton, &QPushButton::clicked, this, &AddPackageDialog::on_saveButton_clicked);
    setWindowTitle("Add New Package");
}

void AddPackageDialog::on_saveButton_clicked()
{
    if (m_descEdit->text().isEmpty()) {
        m_descEdit->setText("Standard Package Data");
    }
    accept();
}

wms::domain::Package AddPackageDialog::packageData() const
{
    wms::domain::PackageMetadata metadata{
        wms::domain::Category::Standard,
        m_weightSpin->value(),
        {10.0, 10.0, 10.0},
        150.0,
        m_descEdit->text().toStdString()
    };

    wms::domain::Address mockAddr{ "123 Tech Street", "Hanoi", "Vietnam", "100000" };

    // FIX: Initialize chronological dates explicitly to satisfy Domain invariants validation
    std::chrono::year_month_day defaultImportDate{ std::chrono::year{2026}, std::chrono::month{6}, std::chrono::day{24} };
    std::chrono::year_month_day defaultExportDate{ std::chrono::year{2026}, std::chrono::month{6}, std::chrono::day{25} };

    wms::domain::LogisticsInfo validLogistics{
        defaultImportDate,
        defaultExportDate,
        "Default-Truck-01",
        "Default-Truck-02",
        "CONT-MOCK"
    };

    wms::domain::StorageLocation mockLoc{ "ZoneA", "Aisle1", 1, 1 };

    // Returns the fully constructed domain object (starts as OnRouteState internally)
    return wms::domain::Package(metadata, mockAddr, mockAddr, validLogistics, mockLoc);
}