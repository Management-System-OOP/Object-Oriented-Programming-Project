/**
 * @file    FilterPanel.h
 * @brief   Filter controls mapped to PackageFilter predicates.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 */

#pragma once

#include <QWidget>
#include <QComboBox>
#include <QLineEdit>
#include <QPushButton>

namespace wms::gui {

    class FilterPanel : public QWidget
    {
        Q_OBJECT

    public:
        explicit FilterPanel(QWidget* parent = nullptr);

        int stateFilterData() const;
        int categoryFilterData() const;
        QString zoneFilterText() const;
        QString searchText() const;

        void resetControls();

    signals:
        void filtersChanged();
        void clearFiltersRequested();

    private slots:
        void onFilterChanged();
        void onSearchChanged(const QString& text);
        void onClearClicked();

    private:
        QComboBox* m_stateCombo{ nullptr };
        QComboBox* m_categoryCombo{ nullptr };
        QLineEdit* m_zoneEdit{ nullptr };
        QLineEdit* m_searchEdit{ nullptr };
        QPushButton* m_clearBtn{ nullptr };
    };

} // namespace wms::gui