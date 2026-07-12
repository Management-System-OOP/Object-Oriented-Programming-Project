/**
 * @file    MainWindow.h
 * @brief   Main WMS interface wired to WarehouseManager.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Extended UI to multi-page layout (Dashboard, Inventory, Operations, Reports)
 *   - Added sidebar navigation and page-specific refresh helpers
 * 
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-12
 * @changelog
 *   - Replaced Dashboard statistics with pie chart
 */

#pragma once

#include <QMainWindow>
#include <QTableView>
#include <QListWidget>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QLabel>
#include <QStackedWidget>
#include <QProgressBar>
#include <QPieSlice>

#include "FilterPanel.h"
#include "PackageTableModel.h"
#include "service/WarehouseManager.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

namespace wms::gui {

    class MainWindow : public QMainWindow
    {
        Q_OBJECT

    public:
        explicit MainWindow(wms::service::WarehouseManager* manager, QWidget* parent = nullptr);
        ~MainWindow() override;

    protected:
        void closeEvent(QCloseEvent* event) override;

    private slots:
        void refreshTable();
        void applyFilters();
        void onClearFilters();
        void onAddPackage();
        void onEditPackage();
        void onRemovePackage();
        void onSave();
        void onLoad();
        void onReceivePackage();
        void onDispatchPackage();
        void onMarkMissing();
        void onMarkFound();
        void onCheckOverdue();
        void onSelectionChanged();
        void onOverdueTimer();

        // New slots for the multi-page interface
        void onSidebarCurrentRowChanged(int row);
        void onOpsSelectionChanged();
        void onOpsReceivePackage();
        void onOpsDispatchPackage();
        void onOpsMarkMissing();
        void onOpsMarkFound();

    private:
        void setupToolbar(QVBoxLayout* contentLayout);
        void updateActionStates();
        QString selectedPackageId() const;
        QString selectedOpsPackageId() const;
        void showOperationError(const char* title, const std::exception& error);
        void persistAndRefresh();

        // Setup helpers for each page
        void setupDashboardPage(QWidget* page);
        void setupInventoryPage(QWidget* page);
        void setupOperationsPage(QWidget* page);
        void setupReportsPage(QWidget* page);

        // Refresh helpers
        void refreshDashboard();
        void refreshOperations();
        void refreshReports();
        void updateOpsButtonStates();

        Ui::MainWindow* ui{ nullptr };
        wms::service::WarehouseManager* m_manager{ nullptr };
        bool m_dirty{ false };

        QListWidget* m_sidebarMenu{ nullptr };
        QStackedWidget* m_stackedWidget{ nullptr };

        // Page 1: Inventory (Uses these existing ones)
        FilterPanel* m_filterPanel{ nullptr };
        QTableView* m_packageTableView{ nullptr };
        PackageTableModel* m_tableModel{ nullptr };

        QTimer* m_overdueTimer{ nullptr };

        QPushButton* m_addBtn{ nullptr };
        QPushButton* m_editBtn{ nullptr };
        QPushButton* m_removeBtn{ nullptr };
        QPushButton* m_saveBtn{ nullptr };
        QPushButton* m_loadBtn{ nullptr };
        QPushButton* m_receiveBtn{ nullptr };
        QPushButton* m_dispatchBtn{ nullptr };
        QPushButton* m_missingBtn{ nullptr };
        QPushButton* m_foundBtn{ nullptr };
        QPushButton* m_overdueBtn{ nullptr };
        QLabel* m_summaryLabel{ nullptr };

        // Page 0: Dashboard metrics
        QPieSlice* m_dbPlaceholderSlice{ nullptr };
        QPieSlice* m_dbStorageSlice{ nullptr };
        QPieSlice* m_dbOnRouteSlice{ nullptr };
        QPieSlice* m_dbDispatchedSlice{ nullptr };
        QPieSlice* m_dbOverdueSlice{ nullptr };
        QPieSlice* m_dbMissingSlice{ nullptr };
        QProgressBar* m_dbCapacityProgress{ nullptr };
        QLabel* m_dbCapacityLabel{ nullptr };
        QTableView* m_dbRecentTableView{ nullptr };
        PackageTableModel* m_dbRecentModel{ nullptr };

        // Page 2: State Operations
        QTableView* m_opsTableView{ nullptr };
        PackageTableModel* m_opsModel{ nullptr };
        QLabel* m_opsDetailsLabel{ nullptr };
        QPushButton* m_opsReceiveBtn{ nullptr };
        QPushButton* m_opsDispatchBtn{ nullptr };
        QPushButton* m_opsMissingBtn{ nullptr };
        QPushButton* m_opsFoundBtn{ nullptr };

        // Page 3: Reports & Analytics
        QTableView* m_repOverdueTableView{ nullptr };
        PackageTableModel* m_repOverdueModel{ nullptr };
        QTableView* m_repMissingTableView{ nullptr };
        PackageTableModel* m_repMissingModel{ nullptr };
        QLabel* m_repStatsLabel{ nullptr };
    };

} // namespace wms::gui