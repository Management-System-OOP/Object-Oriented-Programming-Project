/**
 * @file    MultiColumnFilterProxy.cpp
 * @brief   Implementation of MultiColumnFilterProxy.
 * @author  Nguyen Viet Bach
 * @date    2026-07-30
 */

#include "MultiColumnFilterProxy.h"

namespace wms::gui {

MultiColumnFilterProxy::MultiColumnFilterProxy(QObject* parent)
    : QSortFilterProxyModel(parent)
{}

// ─── Filter management ──────────────────────────────────────────────────────

void MultiColumnFilterProxy::setColumnFilter(int col, const QSet<QString>& allowedValues)
{
    if (allowedValues.isEmpty())
        m_filters.remove(col);
    else
        m_filters[col] = allowedValues;
    invalidateFilter();
}

void MultiColumnFilterProxy::clearColumnFilter(int col)
{
    if (m_filters.remove(col))
        invalidateFilter();
}

void MultiColumnFilterProxy::clearAllFilters()
{
    if (!m_filters.isEmpty())
    {
        m_filters.clear();
        invalidateFilter();
    }
}

bool MultiColumnFilterProxy::hasActiveFilter(int col) const
{
    return m_filters.contains(col);
}

QSet<QString> MultiColumnFilterProxy::columnFilter(int col) const
{
    return m_filters.value(col);
}

// ─── Row acceptance ─────────────────────────────────────────────────────────

bool MultiColumnFilterProxy::filterAcceptsRow(int sourceRow,
                                               const QModelIndex& sourceParent) const
{
    if (m_filters.isEmpty())
        return true;

    const auto* src = sourceModel();
    if (!src)
        return true;

    for (auto it = m_filters.constBegin(); it != m_filters.constEnd(); ++it)
    {
        const QSet<QString>& allowed = it.value();
        if (allowed.isEmpty())
            continue;

        const QModelIndex idx = src->index(sourceRow, it.key(), sourceParent);
        if (!allowed.contains(src->data(idx, Qt::DisplayRole).toString()))
            return false;
    }
    return true;
}

// ─── Unique value enumeration ────────────────────────────────────────────────

QStringList MultiColumnFilterProxy::uniqueValuesForColumn(int col) const
{
    const auto* src = sourceModel();
    if (!src)
        return {};

    QSet<QString> seen;
    const int rows = src->rowCount();
    seen.reserve(rows);

    for (int r = 0; r < rows; ++r)
    {
        const QString val = src->index(r, col).data(Qt::DisplayRole).toString();
        if (!val.isEmpty())
            seen.insert(val);
    }

    QStringList list = seen.values();
    list.sort(Qt::CaseInsensitive);
    return list;
}

} // namespace wms::gui
