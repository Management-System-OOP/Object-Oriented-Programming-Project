/**
 * @file    PackageTableModel.cpp
 * @brief   Implementation of the QAbstractTableModel adapter for domain Package objects.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Added full Doxygen file header
 */

#include "PackageTableModel.h"

#include <QColor>
#include <QString>

namespace wms::gui {

    namespace
    {
        QString categoryLabel(wms::domain::Category category)
        {
            switch (category)
            {
            case wms::domain::Category::Standard:   return QStringLiteral("Standard");
            case wms::domain::Category::Fragile:    return QStringLiteral("Fragile");
            case wms::domain::Category::Perishable: return QStringLiteral("Perishable");
            case wms::domain::Category::Hazmat:     return QStringLiteral("Hazmat");
            case wms::domain::Category::Oversized:  return QStringLiteral("Oversized");
            case wms::domain::Category::Liquid:     return QStringLiteral("Liquid");
            }
            return QStringLiteral("Unknown");
        }

        QString formatDate(const wms::domain::Date& date)
        {
            return QStringLiteral("%1-%2-%3")
                .arg(static_cast<int>(date.year()), 4, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.month()), 2, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.day()), 2, 10, QChar('0'));
        }
    }

    PackageTableModel::PackageTableModel(QObject* parent)
        : QAbstractTableModel(parent)
    {
    }

    int PackageTableModel::rowCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;
        return static_cast<int>(m_packages.size());
    }

    int PackageTableModel::columnCount(const QModelIndex& parent) const
    {
        if (parent.isValid())
            return 0;
        return 8;
    }

    QVariant PackageTableModel::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return {};

        const int row = index.row();
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return {};

        const wms::domain::Package& pkg = m_packages[static_cast<std::size_t>(row)];

        if (role == Qt::DisplayRole)
        {
            switch (index.column())
            {
            case 0: return QString::fromStdString(pkg.id());
            case 1: return QString::fromStdString(pkg.metadata().description);
            case 2: return categoryLabel(pkg.metadata().category);
            case 3: return pkg.metadata().weight;
            case 4: return QString::fromStdString(pkg.location().zone);
            case 5: return QString::fromUtf8(
                pkg.currentState().getStateLabel().data(),
                static_cast<int>(pkg.currentState().getStateLabel().size()));
            case 6: return formatDate(pkg.logistics().importDate);
            case 7: return formatDate(pkg.logistics().expectedExportDate);
            default: return {};
            }
        }

        if (role == Qt::ForegroundRole && index.column() == 5)
        {
            switch (pkg.currentStateId())
            {
            case wms::domain::PackageStateId::Overdue:
                return QColor(235, 87, 87);
            case wms::domain::PackageStateId::Missing:
                return QColor(229, 62, 62);
            case wms::domain::PackageStateId::Dispatched:
                return QColor(47, 128, 237);
            case wms::domain::PackageStateId::InStorage:
                return QColor(39, 174, 96);
            default:
                break;
            }
        }

        return {};
    }

    QVariant PackageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
    {
        if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
            return {};

        switch (section)
        {
        case 0: return QStringLiteral("ID");
        case 1: return QStringLiteral("Description");
        case 2: return QStringLiteral("Category");
        case 3: return QStringLiteral("Weight (kg)");
        case 4: return QStringLiteral("Zone");
        case 5: return QStringLiteral("Status");
        case 6: return QStringLiteral("Import Date");
        case 7: return QStringLiteral("Export Date");
        default: return {};
        }
    }

    void PackageTableModel::refresh(std::vector<wms::domain::Package> newPackages)
    {
        beginResetModel();
        m_packages = std::move(newPackages);
        endResetModel();
    }

    QString PackageTableModel::packageIdAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return {};
        return QString::fromStdString(m_packages[static_cast<std::size_t>(row)].id());
    }

    const wms::domain::Package* PackageTableModel::packageAt(int row) const
    {
        if (row < 0 || row >= static_cast<int>(m_packages.size()))
            return nullptr;
        return &m_packages[static_cast<std::size_t>(row)];
    }

} // namespace wms::gui
