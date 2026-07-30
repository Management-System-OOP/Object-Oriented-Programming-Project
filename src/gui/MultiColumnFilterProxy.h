/**
 * @file    MultiColumnFilterProxy.h
 * @brief   QSortFilterProxyModel subclass that supports independent
 *          per-column value filters (Excel-style dropdown filter).
 * @author  Nguyen Viet Bach
 * @date    2026-07-30
 * @changelog
 *   - Initial implementation: setColumnFilter(), clearColumnFilter(),
 *     clearAllFilters(), hasActiveFilter(), uniqueValuesForColumn().
 *   - filterAcceptsRow() checks every active column filter.
 */

#pragma once

#include <QSortFilterProxyModel>
#include <QSet>
#include <QMap>

namespace wms::gui {

/**
 * @brief Proxy model that filters rows by matching display values
 *        per column, exactly like Excel column filters.
 *
 * Usage:
 *   proxy->setColumnFilter(col, {"Standard", "Fragile"}); // show only these
 *   proxy->clearColumnFilter(col);                        // remove filter
 *   proxy->clearAllFilters();                             // reset everything
 */
class MultiColumnFilterProxy : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit MultiColumnFilterProxy(QObject* parent = nullptr);

    /** Set the set of allowed display values for a column.
     *  An empty set is treated as "no filter" (show all). */
    void setColumnFilter(int col, const QSet<QString>& allowedValues);

    /** Remove filter for a single column. */
    void clearColumnFilter(int col);

    /** Remove all column filters. */
    void clearAllFilters();

    /** Returns true if a filter is currently active for col. */
    bool hasActiveFilter(int col) const;

    /** Returns the active filter set for col (empty if none). */
    QSet<QString> columnFilter(int col) const;

    /** Enumerates every unique display string in source model for col,
     *  sorted alphabetically. Used to populate the filter popup. */
    QStringList uniqueValuesForColumn(int col) const;

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex& sourceParent) const override;

private:
    QMap<int, QSet<QString>> m_filters; ///< col → accepted display values
};

} // namespace wms::gui
