/**
 * @file    EditPackageDialog.cpp
 * @brief   Implementation of the dialog used to edit mutable fields of an
 *          existing Package (metadata, logistics, storage location).
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Added pre-populated form fields that load existing Package data and
 *     return updated value objects (metadata, logistics, location) on accept
 * 
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Added support for name field in metadata
 */

#include "EditPackageDialog.h"

#include "domain/entities/Category.h"

#include <chrono>

#include <QFormLayout>
#include <QVBoxLayout>

namespace wms::gui::dialogs {

    namespace
    {
        QDate qDateFromDomain(const wms::domain::Date& date)
        {
            return QDate(
                static_cast<int>(date.year()),
                static_cast<int>(static_cast<unsigned>(date.month())),
                static_cast<int>(static_cast<unsigned>(date.day())));
        }

        wms::domain::Date dateFromQDate(const QDate& date)
        {
            return wms::domain::Date{
                std::chrono::year{ date.year() },
                std::chrono::month{ static_cast<unsigned>(date.month()) },
                std::chrono::day{ static_cast<unsigned>(date.day()) }
            };
        }

        int categoryIndex(QComboBox* combo, wms::domain::Category category)
        {
            const int data = static_cast<int>(category);
            for (int i = 0; i < combo->count(); ++i)
            {
                if (combo->itemData(i).toInt() == data)
                    return i;
            }
            return 0;
        }
    }

    EditPackageDialog::EditPackageDialog(const wms::domain::Package& package, QWidget* parent)
        : QDialog(parent)
        , m_original(package)
    {
        setWindowTitle("Edit Package");
        setModal(true);
        setMinimumSize(480, 360);

        auto* mainLayout = new QVBoxLayout(this);
        auto* formLayout = new QFormLayout();
        
        m_nameEdit = new QLineEdit(QString::fromStdString(package.metadata().name), this);
        m_nameEdit->setPlaceholderText("Package Name");

        m_descriptionEdit = new QLineEdit(QString::fromStdString(package.metadata().description), this);
        m_descriptionEdit->setPlaceholderText("Package Description");

        m_categoryCombo = new QComboBox(this);
        m_categoryCombo->addItem("Standard", static_cast<int>(wms::domain::Category::Standard));
        m_categoryCombo->addItem("Fragile",  static_cast<int>(wms::domain::Category::Fragile));
        m_categoryCombo->addItem("Perishable", static_cast<int>(wms::domain::Category::Perishable));
        m_categoryCombo->addItem("Hazmat",   static_cast<int>(wms::domain::Category::Hazmat));
        m_categoryCombo->addItem("Oversized", static_cast<int>(wms::domain::Category::Oversized));
        m_categoryCombo->addItem("Liquid",   static_cast<int>(wms::domain::Category::Liquid));
        m_categoryCombo->setCurrentIndex(categoryIndex(m_categoryCombo, package.metadata().category));

        m_weightSpin = new QDoubleSpinBox(this);
        m_weightSpin->setRange(0.1, 10000.0);
        m_weightSpin->setValue(package.metadata().weight);
        m_weightSpin->setSuffix(" kg");

        m_zoneEdit  = new QLineEdit(QString::fromStdString(package.location().zone),  this);
        m_aisleEdit = new QLineEdit(QString::fromStdString(package.location().aisle), this);
        m_shelfSpin = new QSpinBox(this);
        m_shelfSpin->setRange(1, 99);
        m_shelfSpin->setValue(package.location().shelf);
        m_slotSpin  = new QSpinBox(this);
        m_slotSpin->setRange(1, 99);
        m_slotSpin->setValue(package.location().slot);

        m_importDateEdit = new QDateEdit(qDateFromDomain(package.logistics().importDate), this);
        m_importDateEdit->setCalendarPopup(true);
        m_exportDateEdit = new QDateEdit(qDateFromDomain(package.logistics().expectedExportDate), this);
        m_exportDateEdit->setCalendarPopup(true);

        formLayout->addRow("Name:",         m_nameEdit);
        formLayout->addRow("Description:",  m_descriptionEdit);
        formLayout->addRow("Category:",     m_categoryCombo);
        formLayout->addRow("Weight:",       m_weightSpin);
        formLayout->addRow("Zone:",         m_zoneEdit);
        formLayout->addRow("Aisle:",        m_aisleEdit);
        formLayout->addRow("Shelf:",        m_shelfSpin);
        formLayout->addRow("Slot:",         m_slotSpin);
        formLayout->addRow("Import Date:",  m_importDateEdit);
        formLayout->addRow("Export Date:",  m_exportDateEdit);
        mainLayout->addLayout(formLayout);

        m_buttonBox = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
        mainLayout->addWidget(m_buttonBox);

        connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    }

    wms::domain::Package EditPackageDialog::updatedPackage() const
    {
        wms::domain::Package updated = m_original;

        wms::domain::PackageMetadata metadata = updated.metadata();
        metadata.name        = m_nameEdit->text().toStdString();
        metadata.category    = static_cast<wms::domain::Category>(m_categoryCombo->currentData().toInt());
        metadata.weight      = m_weightSpin->value();
        metadata.description = m_descriptionEdit->text().toStdString();
        updated.setMetadata(std::move(metadata));

        updated.setLocation(wms::domain::StorageLocation{
            m_zoneEdit->text().toStdString(),
            m_aisleEdit->text().toStdString(),
            m_shelfSpin->value(),
            m_slotSpin->value()
        });

        wms::domain::LogisticsInfo logistics = updated.logistics();
        logistics.importDate           = dateFromQDate(m_importDateEdit->date());
        logistics.expectedExportDate   = dateFromQDate(m_exportDateEdit->date());
        updated.setLogistics(std::move(logistics));

        return updated;
    }

} // namespace wms::gui::dialogs
