/**
 * @file    AddPackageDialog.cpp
 * @brief   Implementation of AddPackageDialog mapping UI fields to domain Package attributes.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 * 
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Added support for name field in metadata
 */

#include "AddPackageDialog.h"
#include "domain/entities/Category.h"
#include <chrono>
#include <QFormLayout>
#include <QVBoxLayout>

namespace wms::gui::dialogs {

    namespace
    {
        wms::domain::Category categoryFromIndex(int data)
        {
            return static_cast<wms::domain::Category>(data);
        }

        wms::domain::Date dateFromQDate(const QDate& date)
        {
            return wms::domain::Date{
                std::chrono::year{ date.year() },
                std::chrono::month{ static_cast<unsigned>(date.month()) },
                std::chrono::day{ static_cast<unsigned>(date.day()) }
            };
        }
    }

    AddPackageDialog::AddPackageDialog(QWidget* parent)
        : QDialog(parent)
    {
        setWindowTitle("Add New Package");
        setModal(true);
        setMinimumSize(480, 420);

        auto* mainLayout = new QVBoxLayout(this);
        auto* formLayout = new QFormLayout();
        formLayout->setSpacing(10);

        m_nameEdit = new QLineEdit(this);
        m_nameEdit->setPlaceholderText("Package Name");

        m_descriptionEdit = new QLineEdit(this);
        m_descriptionEdit->setPlaceholderText("Package Description");

        m_categoryCombo = new QComboBox(this);
        m_categoryCombo->addItem("Standard", static_cast<int>(wms::domain::Category::Standard));
        m_categoryCombo->addItem("Fragile", static_cast<int>(wms::domain::Category::Fragile));
        m_categoryCombo->addItem("Perishable", static_cast<int>(wms::domain::Category::Perishable));
        m_categoryCombo->addItem("Hazmat", static_cast<int>(wms::domain::Category::Hazmat));
        m_categoryCombo->addItem("Oversized", static_cast<int>(wms::domain::Category::Oversized));
        m_categoryCombo->addItem("Liquid", static_cast<int>(wms::domain::Category::Liquid));

        m_weightSpin = new QDoubleSpinBox(this);
        m_weightSpin->setRange(0.1, 10000.0);
        m_weightSpin->setValue(10.0);
        m_weightSpin->setSuffix(" kg");

        m_zoneEdit = new QLineEdit("ZoneA", this);
        m_aisleEdit = new QLineEdit("Aisle1", this);
        m_shelfSpin = new QSpinBox(this);
        m_shelfSpin->setRange(1, 99);
        m_slotSpin = new QSpinBox(this);
        m_slotSpin->setRange(1, 99);

        m_sourceCityEdit = new QLineEdit("Hanoi", this);
        m_destinationCityEdit = new QLineEdit("Ho Chi Minh City", this);

        m_importDateEdit = new QDateEdit(QDate::currentDate(), this);
        m_importDateEdit->setCalendarPopup(true);
        m_exportDateEdit = new QDateEdit(QDate::currentDate().addDays(5), this);
        m_exportDateEdit->setCalendarPopup(true);

        formLayout->addRow("Name:", m_nameEdit);
        formLayout->addRow("Description:", m_descriptionEdit);
        formLayout->addRow("Category:", m_categoryCombo);
        formLayout->addRow("Weight:", m_weightSpin);
        formLayout->addRow("Zone:", m_zoneEdit);
        formLayout->addRow("Aisle:", m_aisleEdit);
        formLayout->addRow("Shelf:", m_shelfSpin);
        formLayout->addRow("Slot:", m_slotSpin);
        formLayout->addRow("Source City:", m_sourceCityEdit);
        formLayout->addRow("Destination City:", m_destinationCityEdit);
        formLayout->addRow("Import Date:", m_importDateEdit);
        formLayout->addRow("Export Date:", m_exportDateEdit);
        mainLayout->addLayout(formLayout);

        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(m_buttonBox);

        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    wms::domain::Package AddPackageDialog::packageData() const
    {
        const QString description = m_descriptionEdit->text().trimmed().isEmpty()
            ? QStringLiteral("New Package")
            : m_descriptionEdit->text().trimmed();

        wms::domain::PackageMetadata metadata{
            m_nameEdit->text().toStdString(),
            categoryFromIndex(m_categoryCombo->currentData().toInt()),
            m_weightSpin->value(),
            wms::domain::Dimension(10.0, 10.0, 10.0),
            150.0,
            description.toStdString()
        };

        wms::domain::Address source{
            "Origin Address",
            m_sourceCityEdit->text().toStdString(),
            "Vietnam",
            "100000"
        };

        wms::domain::Address destination{
            "Destination Address",
            m_destinationCityEdit->text().toStdString(),
            "Vietnam",
            "700000"
        };

        wms::domain::LogisticsInfo logistics{
            dateFromQDate(m_importDateEdit->date()),
            dateFromQDate(m_exportDateEdit->date()),
            "Truck-01",
            "Truck-02",
            "CONT-100"
        };

        wms::domain::StorageLocation location{
            m_zoneEdit->text().toStdString(),
            m_aisleEdit->text().toStdString(),
            m_shelfSpin->value(),
            m_slotSpin->value()
        };

        return wms::domain::Package::create(metadata, source, destination, logistics, location);
    }

} // namespace wms::gui::dialogs