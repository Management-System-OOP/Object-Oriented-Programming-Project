/**
 * @file    PackageTableModel.h
 * @brief   Table model mirroring domain Package data into QTableView.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Added full Doxygen file header
 */

#pragma once

#include <QAbstractTableModel>
#include <vector>

#include "domain/entities/Package.h"

namespace wms::gui {

    class PackageTableModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        explicit PackageTableModel(QObject* parent = nullptr);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;
        QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
        QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

        void refresh(std::vector<wms::domain::Package> newPackages);
        QString packageIdAt(int row) const;
        const wms::domain::Package* packageAt(int row) const;

    private:
        std::vector<wms::domain::Package> m_packages;
    };

} // namespace wms::gui