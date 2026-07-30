/**
 * @file    FilterableHeaderView.h
 * @brief   QHeaderView subclass that paints a small filter-dropdown button
 *          (▼) on the right side of every section.
 * @author  Nguyen Viet Bach
 * @date    2026-07-30
 * @changelog
 *   - Initial implementation.
 *   - Left-click on the body of a section  → sort (existing Qt behaviour).
 *   - Left-click on the ▼ button           → emits filterButtonClicked(col).
 *   - Active filter columns show a filled orange ▼ as indicator.
 */

#pragma once

#include <QHeaderView>
#include <QSet>

namespace wms::gui {

class FilterableHeaderView : public QHeaderView
{
    Q_OBJECT

public:
    explicit FilterableHeaderView(Qt::Orientation orientation,
                                  QWidget* parent = nullptr);

    /** Call this to update the visual indicator when a filter is set/cleared. */
    void setFilterActive(int logicalIndex, bool active);

signals:
    /** Emitted when the user clicks the ▼ button on a header section. */
    void filterButtonClicked(int logicalIndex);

protected:
    void paintSection(QPainter* painter,
                      const QRect& rect,
                      int logicalIndex) const override;

    void mousePressEvent(QMouseEvent* event) override;

private:
    /** Returns the rect of the ▼ button inside a section rect. */
    static QRect buttonRect(const QRect& sectionRect);

    QSet<int> m_activeFilters; ///< columns that currently have an active filter
};

} // namespace wms::gui
