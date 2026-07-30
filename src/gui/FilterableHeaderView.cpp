/**
 * @file    FilterableHeaderView.cpp
 * @brief   Implementation of FilterableHeaderView.
 * @author  Nguyen Viet Bach
 * @date    2026-07-30
 */

#include "FilterableHeaderView.h"

#include <QPainter>
#include <QMouseEvent>
#include <QStyleOption>

namespace wms::gui {

// Width (px) of the clickable filter button area on the right of each section.
static constexpr int kBtnW = 20;
static constexpr int kBtnH = 20;

FilterableHeaderView::FilterableHeaderView(Qt::Orientation orientation,
                                           QWidget* parent)
    : QHeaderView(orientation, parent)
{
    setSectionsClickable(true);
}

// ─── Painting ───────────────────────────────────────────────────────────────

void FilterableHeaderView::paintSection(QPainter* painter,
                                        const QRect& rect,
                                        int logicalIndex) const
{
    painter->save();

    // 1. Let Qt draw the normal section (background, text, sort indicator).
    //    We clip to the full rect minus the button area so text doesn't overlap.
    const QRect textRect = rect.adjusted(0, 0, -kBtnW, 0);
    painter->setClipRect(textRect);
    QHeaderView::paintSection(painter, rect, logicalIndex);
    painter->restore();

    // 2. Draw the filter ▼ button on the right edge.
    const QRect btn = buttonRect(rect);

    // Button background
    const bool active = m_activeFilters.contains(logicalIndex);
    const QColor bgColor = active ? QColor(0xD6, 0x97, 0x08)   // orange when active
                                  : QColor(0xED, 0xF2, 0xF7);  // light grey

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(bgColor);
    painter->drawRoundedRect(btn.adjusted(2, 3, -2, -3), 3, 3);

    // ▼ arrow glyph
    const QColor arrowColor = active ? Qt::white : QColor(0x4A, 0x55, 0x68);
    painter->setPen(arrowColor);
    painter->setFont([&] {
        QFont f = painter->font();
        f.setPixelSize(9);
        f.setBold(true);
        return f;
    }());
    painter->drawText(btn, Qt::AlignCenter, QString(QChar(0x25BC))); // ▼
    painter->restore();
}

// ─── Mouse handling ──────────────────────────────────────────────────────────

void FilterableHeaderView::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        const int logical = logicalIndexAt(event->pos());
        if (logical >= 0)
        {
            const int visual  = visualIndex(logical);
            const int sectionPos = sectionViewportPosition(logical);
            const int sectionSz  = sectionSize(logical);
            const QRect sectionR(sectionPos, 0, sectionSz, height());
            const QRect btn = buttonRect(sectionR);

            if (btn.contains(event->pos()))
            {
                emit filterButtonClicked(logical);
                return; // don't propagate → no sort triggered
            }
        }
    }
    QHeaderView::mousePressEvent(event); // normal sort
}

// ─── Helpers ─────────────────────────────────────────────────────────────────

void FilterableHeaderView::setFilterActive(int logicalIndex, bool active)
{
    if (active)
        m_activeFilters.insert(logicalIndex);
    else
        m_activeFilters.remove(logicalIndex);

    // Repaint just the affected section
    const int pos = sectionViewportPosition(logicalIndex);
    update(pos, 0, sectionSize(logicalIndex), height());
}

/*static*/ QRect FilterableHeaderView::buttonRect(const QRect& sectionRect)
{
    // Right-aligned, vertically centred inside the section.
    return QRect(sectionRect.right() - kBtnW + 1,
                 sectionRect.top() + (sectionRect.height() - kBtnH) / 2,
                 kBtnW,
                 kBtnH);
}

} // namespace wms::gui
