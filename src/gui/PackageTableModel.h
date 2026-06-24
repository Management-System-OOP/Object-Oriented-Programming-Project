/**
 * @file   PackageTableModel.h
 * @brief  Table model mirroring package business data into Qt View.
 * @author Nguyen Viet Bach
 * @date   2026-06-24
 */

#pragma once

#include <QAbstractTableModel>
#include <vector>
#include "domain/entities/Package.h"

class PackageTableModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit PackageTableModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    void refresh(std::vector<wms::domain::Package> newData);
    QString packageIdAt(int row) const;

private:
    std::vector<wms::domain::Package> m_packages;
};