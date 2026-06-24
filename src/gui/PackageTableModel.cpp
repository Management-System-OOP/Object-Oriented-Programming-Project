/**
 * @file   PackageTableModel.cpp
 * @brief  Table model mirroring package business data into Qt View.
 * @author Nguyen Viet Bach
 * @date   2026-06-24
 */

#include "gui/PackageTableModel.h"

#include <QString>

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
    return 6;
}

QVariant PackageTableModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || role != Qt::DisplayRole)
        return {};

    const int row = index.row();
    if (row < 0 || row >= static_cast<int>(m_packages.size()))
        return {};

    const wms::domain::Package& pkg = m_packages[static_cast<std::size_t>(row)];

    switch (index.column())
    {
    case 0:
        return QString::fromStdString(pkg.id());
    case 1:
        return QString::fromStdString(pkg.metadata().description);
    case 2:
        return pkg.metadata().weight;
    case 3:
        return categoryLabel(pkg.metadata().category);
    case 4:
        return QString::fromStdString(pkg.location().zone);
    case 5:
        return QString::fromUtf8(
            pkg.currentState().getStateLabel().data(),
            static_cast<int>(pkg.currentState().getStateLabel().size()));
    default:
        return {};
    }
}

QVariant PackageTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole || orientation != Qt::Horizontal)
        return {};

    switch (section)
    {
    case 0: return QStringLiteral("ID");
    case 1: return QStringLiteral("Description");
    case 2: return QStringLiteral("Weight (kg)");
    case 3: return QStringLiteral("Category");
    case 4: return QStringLiteral("Zone");
    case 5: return QStringLiteral("Status");
    default:
        return {};
    }
}

void PackageTableModel::refresh(std::vector<wms::domain::Package> newData)
{
    beginResetModel();
    m_packages = std::move(newData);
    endResetModel();
}

QString PackageTableModel::packageIdAt(int row) const
{
    if (row < 0 || row >= static_cast<int>(m_packages.size()))
        return {};
    return QString::fromStdString(m_packages[static_cast<std::size_t>(row)].id());
}
