/**
 * @file    MainWindow.cpp
 * @brief   Implementation of the main application window for the Warehouse Management System.
 * @author  Nguyen Viet Bach
 * @date    2026-06-24
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-04
 * @changelog
 *   - Refactored to multi-page layout with sidebar navigation
 *   - Added Dashboard, Inventory, State Operations and Reports pages
 *   - Integrated periodic overdue-check timer
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-12
 * @changelog
 *   - Updated text color for various strings for contrast
 *   - Replaced Dashboard statistics with pie chart
 *   - Fixed unsaved changes prompt appearing when no changes were made
 *
 * @update
 * @author  Lam Hong Hai Hoang Le
 * @date    2026-07-26
 * @changelog
 *   - Commented out Save and Load buttons, and dirty workspace check due to redundancy with SQLite database
 *
 * @update
 * @author  Do Minh Khang
 * @date    2026-07-23
 * @changelog
 *   - Replaced WarehouseManager* with WarehouseGateway* throughout (see
 *     MainWindow.h for the full rationale). persistAndRefresh() is gone -
 *     renamed to onPackagesChanged() and now triggered by
 *     WarehouseGateway::packagesChanged() via a single connect() in the
 *     constructor, instead of being called explicitly after every mutation.
 *   - refreshTable() removed: it had no callers anywhere in this file (a
 *     leftover from before the multi-page redesign) and its entire body
 *     was just the now-removed persistAndRefresh(true).
 * 
 * @note onSave()/onLoad() are still present and still compile correctly
 *       against the gateway, but are effectively unreachable following the
 *       2026-07-26 change above (closeEvent()'s dirty check is hardcoded
 *       `if (false)`, and no button is connected to onLoad() anymore).
 *
 * @update
 * @author  Nguyen Viet Bach
 * @date    2026-07-25
 * @changelog
 *   - Added Export/Import buttons (CSV & JSON) to the Inventory page toolbar.
 *   - Implemented onExportCsv / onImportCsv / onExportJson / onImportJson
 *     slots: each opens a QFileDialog, calls the appropriate WarehouseGateway
 *     method, and shows a QMessageBox for success or failure.
 *   - Import slots rely on the existing packagesChanged() → onPackagesChanged()
 *     Observer chain for view refresh - no extra wiring needed.
 */

#define WAREHOUSE_MAX 50

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "dialogs/AddPackageDialog.h"
#include "dialogs/EditPackageDialog.h"

#include "service/PackageFilter.h"
#include "domain/states/PackageStateId.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QHeaderView>
#include <QMessageBox>
#include <QFileDialog>
#include <QDir>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QItemSelectionModel>
#include <QPieSeries>
#include <QPieSlice>
#include <QToolTip>
#include <QCursor>
#include <QChartView>
#include <QPieLegendMarker>

namespace wms::gui {

    namespace
    {
        QString buttonStyle(const char* bg, const char* fg = "white")
        {
            return QStringLiteral(
                "QPushButton { background-color: %1; color: %2; padding: 8px 14px; "
                "border: none; border-radius: 4px; font-weight: bold; }"
                "QPushButton:disabled { background-color: #CBD5E0; color: #718096; }")
                .arg(QString::fromUtf8(bg), QString::fromUtf8(fg));
        }
    }

    MainWindow::MainWindow(WarehouseGateway* gateway, QWidget* parent)
        : QMainWindow(parent)
        , ui(new Ui::MainWindow)
        , m_gateway(gateway)
    {
        ui->setupUi(this);
        resize(1280, 800);
        setWindowTitle("Warehouse Management System");

        auto* centralWidget = new QWidget(this);
        setCentralWidget(centralWidget);
        auto* mainLayout = new QHBoxLayout(centralWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(0);

        m_sidebarMenu = new QListWidget(this);
        m_sidebarMenu->setFixedWidth(240);
        m_sidebarMenu->addItems({
            "Dashboard",
            "Package Inventory",
            "State Operations",
            "Reports"
            });
        m_sidebarMenu->setStyleSheet(
            "QListWidget { background-color: #1E2640; color: #AEB7C2; font-size: 14px; border: none; }"
            "QListWidget::item { padding: 15px 20px; }"
            "QListWidget::item:selected { background-color: #00B96B; color: white; font-weight: bold; }"
        );
        mainLayout->addWidget(m_sidebarMenu);

        m_stackedWidget = new QStackedWidget(this);
        mainLayout->addWidget(m_stackedWidget);

        auto* dashboardPage = new QWidget(this);
        auto* inventoryPage = new QWidget(this);
        auto* operationsPage = new QWidget(this);
        auto* reportsPage = new QWidget(this);

        setupDashboardPage(dashboardPage);
        setupInventoryPage(inventoryPage);
        setupOperationsPage(operationsPage);
        setupReportsPage(reportsPage);

        m_stackedWidget->addWidget(dashboardPage);
        m_stackedWidget->addWidget(inventoryPage);
        m_stackedWidget->addWidget(operationsPage);
        m_stackedWidget->addWidget(reportsPage);

        connect(m_sidebarMenu, &QListWidget::currentRowChanged, this, &MainWindow::onSidebarCurrentRowChanged);

        // The Observer connection: every WarehouseGateway mutation ends by
        // emitting packagesChanged(), and this is the one place that gets
        // wired to react to it - no other call site needs to know a signal
        // exists at all.
        connect(m_gateway, &WarehouseGateway::packagesChanged, this, &MainWindow::onPackagesChanged);

        m_overdueTimer = new QTimer(this);
        m_overdueTimer->setInterval(60 * 60 * 1000);
        connect(m_overdueTimer, &QTimer::timeout, this, &MainWindow::onOverdueTimer);
        m_overdueTimer->start();

        // Startup overdue scan, through the gateway like any other mutation.
        // Note this does NOT replace the explicit onPackagesChanged() call
        // below: WarehouseGateway::checkOverduePackages() only emits
        // packagesChanged() when it actually finds something (count > 0),
        // so relying on that signal alone would leave every page empty on
        // a normal startup where nothing happens to be overdue yet. The
        // views' initial population is a separate concern from "did this
        // particular action produce a change worth notifying about", so it
        // gets its own direct call here.
        m_gateway->checkOverduePackages();
        onPackagesChanged();
        m_dirty = false;   // an automatic startup scan is never an unsaved
        // user edit, even though onPackagesChanged()
        // above unconditionally sets m_dirty = true -
        // this matches the original behaviour, where
        // persistAndRefresh(true) always reset m_dirty
        // to false here regardless of overdueCount.
        m_sidebarMenu->setCurrentRow(0);
    }

    MainWindow::~MainWindow()
    {
        delete ui;
    }

    void MainWindow::closeEvent(QCloseEvent* event)
    {
        if (false) // if (m_dirty)
        {
            const auto reply = QMessageBox::question(
                this,
                "Unsaved Changes",
                "Save changes before closing?",
                QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

            if (reply == QMessageBox::Cancel)
            {
                event->ignore();
                return;
            }
            if (reply == QMessageBox::Save)
                onSave();
        }
        event->accept();
    }

    // Page 0: Dashboard Page Setup
    void MainWindow::setupDashboardPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* title = new QLabel("Warehouse Performance Dashboard", page);
        title->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
        layout->addWidget(title);

        auto* dashboardTopLayout = new QHBoxLayout();
        dashboardTopLayout->setSpacing(12);

        auto* capacityLayout = new QVBoxLayout();
        auto* capacityFrame = new QFrame(page);
        capacityFrame->setStyleSheet("background-color: #FFFFFF; padding: 12px;");
        auto* capLayout = new QVBoxLayout(capacityFrame);
        capLayout->setAlignment(Qt::AlignCenter);

        m_dbCapacityLabel = new QLabel(QString("Occupancy<br>0 / %1").arg(WAREHOUSE_MAX), capacityFrame);
        m_dbCapacityLabel->setStyleSheet("font-size: 14px; font-weight: bold; color: #2D3748; text-align: center;");
        m_dbCapacityLabel->setAlignment(Qt::AlignHCenter);
        capLayout->addWidget(m_dbCapacityLabel);

        m_dbCapacityProgress = new QProgressBar(capacityFrame);
        m_dbCapacityProgress->setOrientation(Qt::Vertical);
        m_dbCapacityProgress->setRange(0, WAREHOUSE_MAX);
        m_dbCapacityProgress->setValue(0);
        m_dbCapacityProgress->setTextVisible(true);
        m_dbCapacityProgress->setStyleSheet(
            "QProgressBar { background-color: #EDF2F7; color: #2D3748; border-radius: 6px; text-align: center; width: 22px; font-weight: bold; border: none; }"
            "QProgressBar::chunk { background-color: #48BB78; color: #2D3748; border-radius: 6px; }");

        m_dbCapacityProgress->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
        capLayout->addWidget(m_dbCapacityProgress, 1, Qt::AlignHCenter);
        capacityLayout->addWidget(capacityFrame);
        dashboardTopLayout->addLayout(capacityLayout);

        auto* pieLayout = new QVBoxLayout();

        auto* series = new QPieSeries();

        m_dbPlaceholderSlice = new QPieSlice("Placeholder", 0);
        m_dbPlaceholderSlice->setProperty("isPlaceholder", true);
        m_dbPlaceholderSlice->setBrush(QBrush(QColor("#E0E0E0")));
        m_dbPlaceholderSlice->setPen(QPen(QColor("#9E9E9E"), 2, Qt::DashLine));
        m_dbPlaceholderSlice->setLabelVisible(false);
        series->append(m_dbPlaceholderSlice);

        m_dbStorageSlice = new QPieSlice(QString("<b>In Storage</b>"), 0);
        series->append(m_dbStorageSlice);

        m_dbOnRouteSlice = new QPieSlice(QString("<b>On Route</b>"), 0);
        series->append(m_dbOnRouteSlice);

        m_dbDispatchedSlice = new QPieSlice(QString("<b>Dispatched</b>"), 0);
        series->append(m_dbDispatchedSlice);

        m_dbOverdueSlice = new QPieSlice(QString("<b>Overdue</b>"), 0);
        series->append(m_dbOverdueSlice);

        m_dbMissingSlice = new QPieSlice(QString("<b>Missing</b>"), 0);
        series->append(m_dbMissingSlice);

        auto* chart = new QChart();
        chart->addSeries(series);
        chart->setTitle(QString("<b>Statistics</b>"));
        chart->setTheme(QChart::ChartThemeLight);
        chart->setAnimationOptions(QChart::SeriesAnimations);
        chart->titleFont().setBold(true);
        chart->legend()->setAlignment(Qt::AlignBottom);

        const QList<QLegendMarker*> markers = chart->legend()->markers(series);
        for (QLegendMarker* marker : markers) {
            QPieLegendMarker* pieMarker = qobject_cast<QPieLegendMarker*>(marker);
            if (pieMarker && pieMarker->slice()) {
                if (pieMarker->slice()->property("isPlaceholder").toBool()) pieMarker->setVisible(false);
            }
        }

        QChartView* chartView = new QChartView(chart);
        chartView->setRenderHint(QPainter::Antialiasing);
        chartView->setStyleSheet("background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; padding: 15px;");

        QLabel* chartTooltip = new QLabel(chartView);
        chartTooltip->setStyleSheet(
            "QLabel { background-color: #2D3748; color: #FFFFFF; padding: 6px 12px; font-size: 12px; }"
        );
        chartTooltip->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        chartTooltip->hide();

        for (QPieSlice* slice : series->slices()) {
            QObject::connect(slice, &QPieSlice::hovered, [slice, chartView, chartTooltip](bool isHovered) {
                if (isHovered && !slice->property("isPlaceholder").toBool() && slice->percentage() != 0) {
                    double currentPct = slice->percentage() * 100;

                    QString info = QString("<b>%1</b><br/>"
                                        "Count: %2<br/>"
                                        "Percentage: %3%")
                                    .arg(slice->label())
                                    .arg(slice->value())
                                    .arg(QString::number(currentPct, 'f', 1));

                    chartTooltip->setText(info);
                    chartTooltip->adjustSize();

                    QPoint globalPos = QCursor::pos();
                    QPoint localPos = chartView->mapFromGlobal(globalPos);
                    chartTooltip->move(localPos + QPoint(10, 10));
                    chartTooltip->show();
                    chartTooltip->raise();

                    slice->setExploded(true);
                } 
                else {
                    chartTooltip->hide();
                    slice->setExploded(false);
                }
            });
        }

        pieLayout->addWidget(chartView);
        dashboardTopLayout->addLayout(pieLayout);

        auto* todoLayout = new QVBoxLayout();
        auto* todoTitle = new QLabel("Today's To-Do List", page);
        todoTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");

        auto* m_dbTodoTableView = new QTableView(page);
        m_dbTodoModel = new PackageSmallTableModel(page);
        m_dbTodoTableView->setModel(m_dbTodoModel);
        m_dbTodoTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_dbTodoTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_dbTodoTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_dbTodoTableView->verticalHeader()->setVisible(false);
        m_dbTodoTableView->setStyleSheet(
            "QTableView { outline: none; }"
            "QTableView::item:focus { outline: none; border: none; }"
            "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none }"
            "QTableView { background-color: white; color: #2D3748; gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
            "QHeaderView::section { background-color: #F7FAFC; padding: 10px; color: #4A5568; "
            "font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; }"
        );

        todoLayout->addWidget(todoTitle);
        todoLayout->addWidget(m_dbTodoTableView);
        dashboardTopLayout->addLayout(todoLayout);

        layout->addLayout(dashboardTopLayout);

        auto* recentHeader = new QHBoxLayout();
        auto* recentTitle = new QLabel("All Packages Activity Tracker", page);
        recentTitle->setStyleSheet("font-size: 16px; font-weight: bold; color: #FFFFFF;");
        recentHeader->addWidget(recentTitle);
        recentHeader->addStretch();

        m_overdueBtn = new QPushButton("Scan Overdue Packages", page);
        m_overdueBtn->setStyleSheet(buttonStyle("#E53E3E"));
        recentHeader->addWidget(m_overdueBtn);
        layout->addLayout(recentHeader);

        auto* recentLayout = new QHBoxLayout();

        m_dbRecentTableView = new QTableView(page);
        m_dbRecentModel = new PackageTableModel(page);
        m_dbRecentTableView->setModel(m_dbRecentModel);
        m_dbRecentTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_dbRecentTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_dbRecentTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_dbRecentTableView->verticalHeader()->setVisible(false);
        m_dbRecentTableView->setStyleSheet(
            "QTableView { outline: none; }"
            "QTableView::item:focus { outline: none; border: none; }"
            "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none }"
            "QTableView { background-color: white; color: #2D3748; gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
            "QHeaderView::section { background-color: #F7FAFC; padding: 10px; color: #4A5568; "
            "font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; }"
        );
        layout->addWidget(m_dbRecentTableView);

        connect(m_overdueBtn, &QPushButton::clicked, this, &MainWindow::onCheckOverdue);
    }

    // Page 1: Inventory Explorer Setup
    void MainWindow::setupInventoryPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* title = new QLabel("Package Inventory Explorer", page);
        title->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
        layout->addWidget(title);

        m_filterPanel = new FilterPanel(page);
        layout->addWidget(m_filterPanel);

        m_packageTableView = new QTableView(page);
        m_tableModel = new PackageTableModel(page);
        m_packageTableView->setModel(m_tableModel);
        m_packageTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_packageTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_packageTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_packageTableView->verticalHeader()->setVisible(false);
        m_packageTableView->setStyleSheet(
            "QTableView { outline: none; }"
            "QTableView::item:focus { outline: none; border: none; }"
            "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none }"
            "QTableView { background-color: white; color: #2D3748; gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
            "QHeaderView::section { background-color: #F7FAFC; padding: 10px; color: #4A5568; "
            "font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; }"
        );
        layout->addWidget(m_packageTableView);

        m_summaryLabel = new QLabel(page);
        m_summaryLabel->setStyleSheet(
            "background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; "
            "padding: 12px; color: #2D3748; font-weight: bold;");
        layout->addWidget(m_summaryLabel);

        auto* toolbar = new QHBoxLayout();
        toolbar->setSpacing(8);

        m_addBtn = new QPushButton("Add Package", page);
        m_editBtn = new QPushButton("Edit Details", page);
        m_removeBtn = new QPushButton("Remove Package", page);
        //m_saveBtn = new QPushButton("Save Changes", page);
        //m_loadBtn = new QPushButton("Reload Data", page);

        m_exportCsvBtn  = new QPushButton("Export CSV",  page);
        m_importCsvBtn  = new QPushButton("Import CSV",  page);
        m_exportJsonBtn = new QPushButton("Export JSON", page);
        m_importJsonBtn = new QPushButton("Import JSON", page);

        m_addBtn->setStyleSheet(buttonStyle("#00B96B"));
        m_editBtn->setStyleSheet(buttonStyle("#4299E1"));
        m_removeBtn->setStyleSheet(buttonStyle("#E53E3E"));
        //m_saveBtn->setStyleSheet(buttonStyle("#805AD5"));
        //m_loadBtn->setStyleSheet(buttonStyle("#718096"));

        m_exportCsvBtn ->setStyleSheet(buttonStyle("#805AD5"));
        m_importCsvBtn ->setStyleSheet(buttonStyle("#6B46C1"));
        m_exportJsonBtn->setStyleSheet(buttonStyle("#319795"));
        m_importJsonBtn->setStyleSheet(buttonStyle("#2C7A7B"));

        toolbar->addWidget(m_addBtn);
        toolbar->addWidget(m_editBtn);
        toolbar->addWidget(m_removeBtn);
        //toolbar->addWidget(m_saveBtn);
        //toolbar->addWidget(m_loadBtn);
        toolbar->addStretch();
        toolbar->addWidget(m_exportCsvBtn);
        toolbar->addWidget(m_importCsvBtn);
        toolbar->addWidget(m_exportJsonBtn);
        toolbar->addWidget(m_importJsonBtn);

        layout->addLayout(toolbar);

        connect(m_filterPanel, &FilterPanel::filtersChanged, this, &MainWindow::applyFilters);
        connect(m_filterPanel, &FilterPanel::clearFiltersRequested, this, &MainWindow::onClearFilters);

        connect(m_addBtn, &QPushButton::clicked, this, &MainWindow::onAddPackage);
        connect(m_editBtn, &QPushButton::clicked, this, &MainWindow::onEditPackage);
        connect(m_removeBtn, &QPushButton::clicked, this, &MainWindow::onRemovePackage);
        //connect(m_saveBtn, &QPushButton::clicked, this, &MainWindow::onSave);
        //connect(m_loadBtn, &QPushButton::clicked, this, &MainWindow::onLoad);

        connect(m_exportCsvBtn,  &QPushButton::clicked, this, &MainWindow::onExportCsv);
        connect(m_importCsvBtn,  &QPushButton::clicked, this, &MainWindow::onImportCsv);
        connect(m_exportJsonBtn, &QPushButton::clicked, this, &MainWindow::onExportJson);
        connect(m_importJsonBtn, &QPushButton::clicked, this, &MainWindow::onImportJson);

        connect(m_packageTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::onSelectionChanged);
    }

    // Page 2: State Operations Setup
    void MainWindow::setupOperationsPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* title = new QLabel("State Transition Operations", page);
        title->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
        layout->addWidget(title);

        auto* bodyLayout = new QHBoxLayout();
        bodyLayout->setSpacing(15);
        layout->addLayout(bodyLayout);

        auto* leftPanel = new QVBoxLayout();
        m_opsTableView = new QTableView(page);
        m_opsModel = new PackageTableModel(page);
        m_opsTableView->setModel(m_opsModel);
        m_opsTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_opsTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_opsTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_opsTableView->verticalHeader()->setVisible(false);
        m_opsTableView->setStyleSheet(
            "QTableView { outline: none; }"
            "QTableView::item:focus { outline: none; border: none; }"
            "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none }"
            "QTableView { background-color: white; color: #2D3748; gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
            "QHeaderView::section { background-color: #F7FAFC; padding: 10px; color: #4A5568; "
            "font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; }"
        );
        leftPanel->addWidget(new QLabel("Select a Package to execute warehouse operations:", page));
        leftPanel->addWidget(m_opsTableView);
        bodyLayout->addLayout(leftPanel, 3);

        auto* rightCard = new QFrame(page);
        rightCard->setFixedWidth(380);
        rightCard->setStyleSheet(
            "QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; }");
        auto* rightLayout = new QVBoxLayout(rightCard);
        rightLayout->setContentsMargins(15, 15, 15, 15);
        rightLayout->setSpacing(15);

        auto* detailHeader = new QLabel("Package Operations Center", rightCard);
        detailHeader->setStyleSheet("font-size: 16px; font-weight: bold; color: #1A202C; border-bottom: 2px solid #EDF2F7; padding-bottom: 8px;");
        rightLayout->addWidget(detailHeader);

        m_opsDetailsLabel = new QLabel(rightCard);
        m_opsDetailsLabel->setWordWrap(true);
        m_opsDetailsLabel->setStyleSheet("font-size: 13px; color: #4A5568; line-height: 1.5;");
        m_opsDetailsLabel->setText("Select a package from the table to view details and execute state actions.");
        rightLayout->addWidget(m_opsDetailsLabel);

        rightLayout->addStretch();

        auto* btnGroup = new QFrame(rightCard);
        btnGroup->setStyleSheet("QFrame { border: none; }");
        auto* btnLayout = new QVBoxLayout(btnGroup);
        btnLayout->setContentsMargins(0, 0, 0, 0);
        btnLayout->setSpacing(8);

        m_opsReceiveBtn = new QPushButton("Receive Package (Inbound)", btnGroup);
        m_opsDispatchBtn = new QPushButton("Dispatch Package (Outbound)", btnGroup);
        m_opsMissingBtn = new QPushButton("Mark as Missing", btnGroup);
        m_opsFoundBtn = new QPushButton("Mark as Found (Recovered)", btnGroup);

        m_opsReceiveBtn->setStyleSheet(buttonStyle("#48BB78"));
        m_opsDispatchBtn->setStyleSheet(buttonStyle("#3182CE"));
        m_opsMissingBtn->setStyleSheet(buttonStyle("#ED8936"));
        m_opsFoundBtn->setStyleSheet(buttonStyle("#ECC94B"));

        btnLayout->addWidget(m_opsReceiveBtn);
        btnLayout->addWidget(m_opsDispatchBtn);
        btnLayout->addWidget(m_opsMissingBtn);
        btnLayout->addWidget(m_opsFoundBtn);

        rightLayout->addWidget(btnGroup);
        bodyLayout->addWidget(rightCard, 2);

        connect(m_opsReceiveBtn, &QPushButton::clicked, this, &MainWindow::onOpsReceivePackage);
        connect(m_opsDispatchBtn, &QPushButton::clicked, this, &MainWindow::onOpsDispatchPackage);
        connect(m_opsMissingBtn, &QPushButton::clicked, this, &MainWindow::onOpsMarkMissing);
        connect(m_opsFoundBtn, &QPushButton::clicked, this, &MainWindow::onOpsMarkFound);

        connect(m_opsTableView->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &MainWindow::onOpsSelectionChanged);
    }

    // Page 3: Reports & Alerts Setup
    void MainWindow::setupReportsPage(QWidget* page)
    {
        auto* layout = new QVBoxLayout(page);
        layout->setContentsMargins(20, 20, 20, 20);
        layout->setSpacing(15);

        auto* title = new QLabel("Warehouse Reports & Action Items", page);
        title->setStyleSheet("font-size: 20px; font-weight: bold; color: #FFFFFF;");
        layout->addWidget(title);

        auto* bodyLayout = new QHBoxLayout();
        bodyLayout->setSpacing(15);
        layout->addLayout(bodyLayout);

        auto* leftPanel = new QFrame(page);
        leftPanel->setFixedWidth(300);
        leftPanel->setStyleSheet("QFrame { background-color: #FFFFFF; border: 1px solid #E2E8F0; border-radius: 6px; padding: 15px; }");
        auto* leftLayout = new QVBoxLayout(leftPanel);

        auto* summaryHeader = new QLabel("Storage Breakdown", leftPanel);
        summaryHeader->setStyleSheet("font-size: 15px; font-weight: bold; color: #2D3748; border-bottom: 1px solid #EDF2F7; padding-bottom: 5px;");
        leftLayout->addWidget(summaryHeader);

        m_repStatsLabel = new QLabel(leftPanel);
        m_repStatsLabel->setStyleSheet("font-size: 13px; color: #4A5568; line-height: 1.6;");
        leftLayout->addWidget(m_repStatsLabel);
        leftLayout->addStretch();
        bodyLayout->addWidget(leftPanel);

        auto* rightPanel = new QVBoxLayout();
        rightPanel->setSpacing(10);

        rightPanel->addWidget(new QLabel("Overdue Packages (Action Required):", page));
        m_repOverdueTableView = new QTableView(page);
        m_repOverdueModel = new PackageTableModel(page);
        m_repOverdueTableView->setModel(m_repOverdueModel);
        m_repOverdueTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_repOverdueTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_repOverdueTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_repOverdueTableView->verticalHeader()->setVisible(false);
        m_repOverdueTableView->setStyleSheet(
            "QTableView { outline: none; }"
            "QTableView::item:focus { outline: none; border: none; }"
            "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none }"
            "QTableView { background-color: white; color: #2D3748; gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
            "QHeaderView::section { background-color: #F7FAFC; padding: 6px; color: #4A5568; font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; }"
        );
        rightPanel->addWidget(m_repOverdueTableView);

        rightPanel->addWidget(new QLabel("Missing Packages (Under Investigation):", page));
        m_repMissingTableView = new QTableView(page);
        m_repMissingModel = new PackageTableModel(page);
        m_repMissingTableView->setModel(m_repMissingModel);
        m_repMissingTableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        m_repMissingTableView->setSelectionMode(QAbstractItemView::SingleSelection);
        m_repMissingTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_repMissingTableView->verticalHeader()->setVisible(false);
        m_repMissingTableView->setStyleSheet(
            "QTableView { outline: none; }"
            "QTableView::item:focus { outline: none; border: none; }"
            "QTableView::item:selected { background-color: #4299E1; color: #FFFFFF; font-weight: bold; border: none }"
            "QTableView { background-color: white; color: #2D3748; gridline-color: #EDF2F7; border: 1px solid #E2E8F0; }"
            "QHeaderView::section { background-color: #F7FAFC; padding: 6px; color: #4A5568; font-weight: bold; border: none; border-bottom: 2px solid #E2E8F0; }"
        );
        rightPanel->addWidget(m_repMissingTableView);

        bodyLayout->addLayout(rightPanel);
    }

    void MainWindow::refreshDashboard()
    {
        const auto packages = m_gateway->getAllPackages();

        int total = static_cast<int>(packages.size());
        int storage = 0;
        int onRoute = 0;
        int dispatched = 0;
        int overdue = 0;
        int missing = 0;

        for (const auto& pkg : packages)
        {
            switch (pkg.currentStateId())
            {
            case wms::domain::PackageStateId::InStorage:
                storage++;
                break;
            case wms::domain::PackageStateId::OnRoute:
                onRoute++;
                break;
            case wms::domain::PackageStateId::Dispatched:
                dispatched++;
                break;
            case wms::domain::PackageStateId::Overdue:
                overdue++;
                break;
            case wms::domain::PackageStateId::Missing:
                missing++;
                break;
            }
        }

        if (m_dbPlaceholderSlice) m_dbPlaceholderSlice->setValue((total == 0));
        if (m_dbStorageSlice) m_dbStorageSlice->setValue(storage);
        if (m_dbOnRouteSlice) m_dbOnRouteSlice->setValue(onRoute);
        if (m_dbDispatchedSlice) m_dbDispatchedSlice->setValue(dispatched);
        if (m_dbOverdueSlice) m_dbOverdueSlice->setValue(overdue);
        if (m_dbMissingSlice) m_dbMissingSlice->setValue(missing);

        if (m_dbCapacityProgress)
        {
            m_dbCapacityProgress->setValue(storage);
            double percent = (static_cast<double>(storage) / WAREHOUSE_MAX) * 100;
            if (m_dbCapacityLabel)
            {
                m_dbCapacityLabel->setText(QString("Occupancy<br>%1 / %3 (%2%)")
                    .arg(storage)
                    .arg(percent, 0, 'f', 1)
                    .arg(WAREHOUSE_MAX));
            }

            if (percent >= 90.0)
            {
                m_dbCapacityProgress->setStyleSheet(
                    "QProgressBar { background-color: #EDF2F7; border-radius: 6px; text-align: center; height: 22px; font-weight: bold; border: none; }"
                    "QProgressBar::chunk { background-color: #E53E3E; border-radius: 6px; }");
            }
            else if (percent >= 75.0)
            {
                m_dbCapacityProgress->setStyleSheet(
                    "QProgressBar { background-color: #EDF2F7; border-radius: 6px; text-align: center; height: 22px; font-weight: bold; border: none; }"
                    "QProgressBar::chunk { background-color: #DD6B20; border-radius: 6px; }");
            }
            else
            {
                m_dbCapacityProgress->setStyleSheet(
                    "QProgressBar { background-color: #EDF2F7; border-radius: 6px; text-align: center; height: 22px; font-weight: bold; border: none; }"
                    "QProgressBar::chunk { background-color: #48BB78; border-radius: 6px; }");
            }
        }

        if (m_dbRecentModel)
        {
            m_dbRecentModel->refresh(packages);
        }

        auto toDoList = m_gateway->getDailyTodoList();
        if (m_dbTodoModel)
        {
            m_dbTodoModel->refresh(toDoList.importedToday);
        }
    }

    void MainWindow::refreshOperations()
    {
        const auto packages = m_gateway->getAllPackages();
        if (m_opsModel)
        {
            m_opsModel->refresh(packages);
        }
        updateOpsButtonStates();
    }

    void MainWindow::refreshReports()
    {
        const auto packages = m_gateway->getAllPackages();

        if (m_repOverdueModel)
        {
            m_repOverdueModel->refresh(m_gateway->getOverdue());
        }
        if (m_repMissingModel)
        {
            m_repMissingModel->refresh(m_gateway->getMissing());
        }

        int standard = 0, fragile = 0, perishable = 0, hazmat = 0, oversized = 0, liquid = 0;
        int total = 0;
        for (const auto& pkg : packages)
        {
            total++;
            switch (pkg.metadata().category)
            {
            case wms::domain::Category::Standard:   standard++;   break;
            case wms::domain::Category::Fragile:    fragile++;    break;
            case wms::domain::Category::Perishable: perishable++; break;
            case wms::domain::Category::Hazmat:     hazmat++;     break;
            case wms::domain::Category::Oversized:  oversized++;  break;
            case wms::domain::Category::Liquid:     liquid++;     break;
            }
        }

        if (m_repStatsLabel)
        {
            QString statsHtml = QString(
                "<h3>Warehouse Summary</h3>"
                "<p><b>Total Packages:</b> %1</p>"
                "<br>"
                "<h3>Category Breakdown</h3>"
                "<p>📦 <b>Standard:</b> %2</p>"
                "<p>🍷 <b>Fragile:</b> %3</p>"
                "<p>🍎 <b>Perishable:</b> %4</p>"
                "<p>☣️ <b>Hazmat:</b> %5</p>"
                "<p>🏋️ <b>Oversized:</b> %6</p>"
                "<p>🧪 <b>Liquid:</b> %7</p>"
            )
                .arg(total)
                .arg(standard)
                .arg(fragile)
                .arg(perishable)
                .arg(hazmat)
                .arg(oversized)
                .arg(liquid);

            m_repStatsLabel->setText(statsHtml);
        }
    }

    void MainWindow::setupToolbar(QVBoxLayout* /*contentLayout*/)
    {
        // No longer used, empty stub for backward compatibility
    }

    void MainWindow::applyFilters()
    {
        if (!m_filterPanel || !m_tableModel)
            return;

        auto packages = m_gateway->getAllPackages();
        wms::service::PackageFilter::Predicate predicate = [](const wms::domain::Package&) {
            return true;
            };

        const int stateData = m_filterPanel->stateFilterData();
        if (stateData >= 0)
        {
            const auto state = static_cast<wms::domain::PackageStateId>(stateData);
            predicate = wms::service::PackageFilter::combine(
                predicate,
                wms::service::PackageFilter::byState(state));
        }

        const int categoryData = m_filterPanel->categoryFilterData();
        if (categoryData >= 0)
        {
            const auto category = static_cast<wms::domain::Category>(categoryData);
            predicate = wms::service::PackageFilter::combine(
                predicate,
                wms::service::PackageFilter::byCategory(category));
        }

        const QString zone = m_filterPanel->zoneFilterText();
        if (!zone.isEmpty())
        {
            predicate = wms::service::PackageFilter::combine(
                predicate,
                wms::service::PackageFilter::byZone(zone.toStdString()));
        }

        const QString search = m_filterPanel->searchText();
        if (!search.isEmpty())
        {
            const std::string keyword = search.toStdString();
            predicate = wms::service::PackageFilter::combine(
                predicate,
                [keyword](const wms::domain::Package& pkg) {
                    if (pkg.id().find(keyword) != std::string::npos)
                        return true;
                    return wms::service::PackageFilter::byDescriptionKeyword(keyword)(pkg);
                });
        }

        m_tableModel->refresh(wms::service::PackageFilter::apply(packages, predicate));
        if (m_summaryLabel)
        {
            m_summaryLabel->setText(
                QStringLiteral("Total filtered packages: %1").arg(m_tableModel->rowCount()));
        }
        updateActionStates();
    }

    void MainWindow::onClearFilters()
    {
        if (m_filterPanel)
        {
            m_filterPanel->resetControls();
        }
        applyFilters();
    }

    void MainWindow::onAddPackage()
    {
        dialogs::AddPackageDialog dialog(this);
        if (dialog.exec() != QDialog::Accepted)
            return;

        try
        {
            m_gateway->addPackage(dialog.packageData());
        }
        catch (const std::exception& error)
        {
            showOperationError("Add Package", error);
        }
    }

    void MainWindow::onEditPackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            wms::domain::Package package = m_gateway->getPackage(id.toStdString());
            dialogs::EditPackageDialog dialog(package, this);
            if (dialog.exec() != QDialog::Accepted)
                return;

            m_gateway->updatePackage(dialog.updatedPackage());
        }
        catch (const std::exception& error)
        {
            showOperationError("Edit Package", error);
        }
    }

    void MainWindow::onRemovePackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        const auto reply = QMessageBox::question(
            this,
            "Remove Package",
            "Remove this package from the system?",
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        try
        {
            m_gateway->removePackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Remove Package", error);
        }
    }

    void MainWindow::onSave()
    {
        try
        {
            m_gateway->save();
            m_dirty = false;
            QMessageBox::information(this, "Saved", "Package data saved successfully.");
        }
        catch (const std::exception& error)
        {
            showOperationError("Save", error);
        }
    }

    void MainWindow::onLoad()
    {
        const auto reply = QMessageBox::question(
            this,
            "Reload Data",
            "Reload from disk? Unsaved changes will be lost.",
            QMessageBox::Yes | QMessageBox::No);

        if (reply != QMessageBox::Yes)
            return;

        try
        {
            // load() and checkOverduePackages() both emit packagesChanged()
            // internally, each triggering a full refresh mid-sequence - but
            // resetControls() below still needs its own explicit final
            // refresh afterward, since applyFilters() must run again with
            // the filter panel actually cleared, not with whatever state it
            // was in when the automatic mid-sequence refreshes fired.
            m_gateway->load();
            m_gateway->checkOverduePackages();
            if (m_filterPanel)
            {
                m_filterPanel->resetControls();
            }
            onPackagesChanged();
            m_dirty = false;   // reload/refresh here is not an unsaved user
            // edit - matches the original behaviour.
        }
        catch (const std::exception& error)
        {
            showOperationError("Reload", error);
        }
    }

    void MainWindow::onReceivePackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->receivePackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Receive Package", error);
        }
    }

    void MainWindow::onDispatchPackage()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->dispatchPackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Dispatch Package", error);
        }
    }

    void MainWindow::onMarkMissing()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->markMissing(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Missing", error);
        }
    }

    void MainWindow::onMarkFound()
    {
        const QString id = selectedPackageId();
        if (id.isEmpty())
            return;

        try
        {
            m_gateway->markFound(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Found", error);
        }
    }

    void MainWindow::onCheckOverdue()
    {
        const int count = m_gateway->checkOverduePackages();
        if (count > 0)
        {
            QMessageBox::information(
                this,
                "Overdue Check",
                QString("%1 package(s) moved to Overdue status.").arg(count));
        }
        else
        {
            QMessageBox::information(this, "Overdue Check", "No packages are overdue.");
        }
    }

    // --Export / Import helpers--

    /**
     * @brief  Returns the fixed export/import directory (<app_dir>/exports/).
     *
     *  The directory is created automatically if it does not exist yet, so
     *  the first export call will always have a valid destination folder.
     */
    static QString exportDir()
    {
        QDir dir{ QCoreApplication::applicationDirPath() + "/exports" };
        if (!dir.exists())
            dir.mkpath(".");
        return dir.absolutePath();
    }

    // --Export / Import slots--

    void MainWindow::onExportCsv()
    {
        const QString dir  = exportDir();
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Export Packages to CSV",
            dir + "/packages_export.csv",
            "CSV Files (*.csv);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->exportDataCsv(path.toStdString());
            QMessageBox::information(
                this,
                "Export Successful",
                QString("All packages exported successfully to:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Export CSV", error);
        }
    }

    void MainWindow::onImportCsv()
    {
        const QString dir  = exportDir();
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Import Packages from CSV",
            dir,
            "CSV Files (*.csv);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->importDataCsv(path.toStdString());
            // packagesChanged() is emitted by the gateway; onPackagesChanged()
            // already refreshes every view - no extra call needed here.
            QMessageBox::information(
                this,
                "Import Successful",
                QString("Packages imported successfully from:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Import CSV", error);
        }
    }

    void MainWindow::onExportJson()
    {
        const QString dir  = exportDir();
        const QString path = QFileDialog::getSaveFileName(
            this,
            "Export Packages to JSON",
            dir + "/packages_export.json",
            "JSON Files (*.json);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->exportDataJson(path.toStdString());
            QMessageBox::information(
                this,
                "Export Successful",
                QString("All packages exported successfully to:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Export JSON", error);
        }
    }

    void MainWindow::onImportJson()
    {
        const QString dir  = exportDir();
        const QString path = QFileDialog::getOpenFileName(
            this,
            "Import Packages from JSON",
            dir,
            "JSON Files (*.json);;All Files (*)");
        if (path.isEmpty())
            return;

        try
        {
            m_gateway->importDataJson(path.toStdString());
            // packagesChanged() is emitted by the gateway; onPackagesChanged()
            // already refreshes every view - no extra call needed here.
            QMessageBox::information(
                this,
                "Import Successful",
                QString("Packages imported successfully from:\n%1").arg(path));
        }
        catch (const std::exception& error)
        {
            showOperationError("Import JSON", error);
        }
    }

    void MainWindow::onSelectionChanged()
    {
        updateActionStates();
    }

    void MainWindow::onOverdueTimer()
    {
        // The count check that used to gate a persistAndRefresh() call is
        // gone - WarehouseGateway::checkOverduePackages() only emits
        // packagesChanged() when count > 0 on its own, so there is nothing
        // left for this method to conditionally do.
        m_gateway->checkOverduePackages();
    }

    void MainWindow::updateActionStates()
    {
        if (!m_packageTableView)
            return;
        const bool hasSelection = m_packageTableView->selectionModel()->hasSelection();
        if (m_editBtn) m_editBtn->setEnabled(hasSelection);
        if (m_removeBtn) m_removeBtn->setEnabled(hasSelection);
    }

    QString MainWindow::selectedPackageId() const
    {
        if (!m_packageTableView || !m_tableModel)
            return {};
        const QModelIndex index = m_packageTableView->currentIndex();
        if (!index.isValid())
        {
            QMessageBox::warning(
                const_cast<MainWindow*>(this),
                "Selection Required",
                "Please select a package first.");
            return {};
        }
        return m_tableModel->packageIdAt(index.row());
    }

    void MainWindow::showOperationError(const char* title, const std::exception& error)
    {
        QMessageBox::critical(this, title, error.what());
    }

    void MainWindow::onPackagesChanged()
    {
        m_dirty = true;
        applyFilters();
        refreshDashboard();
        refreshOperations();
        refreshReports();
    }

    // New slots implementation
    void MainWindow::onSidebarCurrentRowChanged(int row)
    {
        if (m_stackedWidget)
        {
            m_stackedWidget->setCurrentIndex(row);
        }
        switch (row)
        {
        case 0:
            refreshDashboard();
            break;
        case 1:
            applyFilters();
            break;
        case 2:
            refreshOperations();
            break;
        case 3:
            refreshReports();
            break;
        }
    }

    QString MainWindow::selectedOpsPackageId() const
    {
        if (!m_opsTableView || !m_opsModel)
            return {};
        const QModelIndex index = m_opsTableView->currentIndex();
        if (!index.isValid())
            return {};
        return m_opsModel->packageIdAt(index.row());
    }

    void MainWindow::onOpsSelectionChanged()
    {
        updateOpsButtonStates();
    }

    void MainWindow::onOpsReceivePackage()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->receivePackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Receive Package", error);
        }
    }

    void MainWindow::onOpsDispatchPackage()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->dispatchPackage(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Dispatch Package", error);
        }
    }

    void MainWindow::onOpsMarkMissing()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->markMissing(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Missing", error);
        }
    }

    void MainWindow::onOpsMarkFound()
    {
        const QString id = selectedOpsPackageId();
        if (id.isEmpty())
            return;
        try
        {
            m_gateway->markFound(id.toStdString());
        }
        catch (const std::exception& error)
        {
            showOperationError("Mark Found", error);
        }
    }

    void MainWindow::updateOpsButtonStates()
    {
        if (!m_opsTableView || !m_opsModel || !m_opsDetailsLabel)
            return;

        const QModelIndex index = m_opsTableView->currentIndex();
        if (!index.isValid())
        {
            m_opsDetailsLabel->setText("Select a package from the table to view details and execute state actions.");
            if (m_opsReceiveBtn) m_opsReceiveBtn->setEnabled(false);
            if (m_opsDispatchBtn) m_opsDispatchBtn->setEnabled(false);
            if (m_opsMissingBtn) m_opsMissingBtn->setEnabled(false);
            if (m_opsFoundBtn) m_opsFoundBtn->setEnabled(false);
            return;
        }

        const wms::domain::Package* pkg = m_opsModel->packageAt(index.row());
        if (!pkg)
            return;

        auto formatDate = [](const wms::domain::Date& date) {
            return QString("%1-%2-%3")
                .arg(static_cast<int>(date.year()), 4, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.month()), 2, 10, QChar('0'))
                .arg(static_cast<unsigned>(date.day()), 2, 10, QChar('0'));
            };

        QString categoryStr;
        switch (pkg->metadata().category)
        {
        case wms::domain::Category::Standard:   categoryStr = "Standard"; break;
        case wms::domain::Category::Fragile:    categoryStr = "Fragile"; break;
        case wms::domain::Category::Perishable: categoryStr = "Perishable"; break;
        case wms::domain::Category::Hazmat:     categoryStr = "Hazmat"; break;
        case wms::domain::Category::Oversized:  categoryStr = "Oversized"; break;
        case wms::domain::Category::Liquid:     categoryStr = "Liquid"; break;
        }

        QString details = QString(
            "<b>ID:</b> %1<br>"
            "<b>Name:</b> %2<br>"
            "<b>Description:</b> %3<br>"
            "<b>Category:</b> %4<br>"
            "<b>Weight:</b> %5 kg<br>"
            "<b>Status:</b> %6<br>"
            "<b>Location:</b> Zone %7, Aisle %8, Shelf %9, Slot %10<br>"
            "<b>Source:</b> %11<br>"
            "<b>Destination:</b> %12<br>"
            "<b>Import Date:</b> %13<br>"
            "<b>Export Date:</b> %14"
        )
            .arg(QString::fromStdString(pkg->id()))
            .arg(QString::fromStdString(pkg->metadata().name))
            .arg(QString::fromStdString(pkg->metadata().description))
            .arg(categoryStr)
            .arg(pkg->metadata().weight)
            .arg(QString::fromUtf8(pkg->currentState().getStateLabel().data(), static_cast<int>(pkg->currentState().getStateLabel().size())))
            .arg(QString::fromStdString(pkg->location().zone))
            .arg(QString::fromStdString(pkg->location().aisle))
            .arg(pkg->location().shelf)
            .arg(pkg->location().slot)
            .arg(QString::fromStdString(pkg->source().city))
            .arg(QString::fromStdString(pkg->destination().city))
            .arg(formatDate(pkg->logistics().importDate))
            .arg(formatDate(pkg->logistics().expectedExportDate));

        m_opsDetailsLabel->setText(details);

        const auto stateId = pkg->currentStateId();

        if (m_opsReceiveBtn) m_opsReceiveBtn->setEnabled(stateId == wms::domain::PackageStateId::OnRoute);
        if (m_opsDispatchBtn) m_opsDispatchBtn->setEnabled(stateId == wms::domain::PackageStateId::InStorage ||
            stateId == wms::domain::PackageStateId::Overdue);
        if (m_opsMissingBtn) m_opsMissingBtn->setEnabled(stateId != wms::domain::PackageStateId::Dispatched &&
            stateId != wms::domain::PackageStateId::Missing);
        if (m_opsFoundBtn) m_opsFoundBtn->setEnabled(stateId == wms::domain::PackageStateId::Missing);
    }

} // namespace wms::gui