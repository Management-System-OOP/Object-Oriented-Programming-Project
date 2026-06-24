/**
 * @file   MainWindow.cpp
 * @brief  Implementation of MainWindow driving WMS layouts and manual state transitions.
 * @author Nguyen Viet Bach
 * @date   2026-06-24
 */

#include "gui/MainWindow.h"
#include "gui/dialogs/AddPackageDialog.h"
#include "domain/states/PackageStateId.h"
#include <QMessageBox>

MainWindow::MainWindow(wms::service::WarehouseManager* manager, QWidget* parent)
    : QMainWindow(parent)
    , m_manager(manager)
    , m_tableModel(new PackageTableModel(this))
    , m_tableView(new QTableView(this))
    , m_stateFilterComboBox(new QComboBox(this))
    , m_searchIdLineEdit(new QLineEdit(this))
    , m_addButton(new QPushButton("Add Package", this))
    , m_removeButton(new QPushButton("Remove", this))
    , m_dispatchButton(new QPushButton("Dispatch Package", this))
    , m_missingButton(new QPushButton("Report Missing", this))
{
    // No Q_OBJECT macro here inside the .cpp file to prevent AutoMoc compilation failures
    setupLayout();

    // Inject presentation model cache into the framework TableView component
    m_tableView->setModel(m_tableModel);
    m_tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    // Section 6.2: Establish modern pointer-based signal/slot connections for FilterPanel
    connect(m_stateFilterComboBox, &QComboBox::currentIndexChanged, this, &MainWindow::onFilterChanged);
    connect(m_searchIdLineEdit, &QLineEdit::textChanged, this, &MainWindow::onSearchIdChanged);

    // Section 6.2: Establish modern pointer-based signal/slot connections for ActionBar
    connect(m_addButton, &QPushButton::clicked, this, &MainWindow::on_addButton_clicked);
    connect(m_removeButton, &QPushButton::clicked, this, &MainWindow::on_removeButton_clicked);
    connect(m_dispatchButton, &QPushButton::clicked, this, &MainWindow::on_dispatchButton_clicked);
    connect(m_missingButton, &QPushButton::clicked, this, &MainWindow::on_missingButton_clicked);

    // Populate initial dataset from repository on start
    updateTable();
}

void MainWindow::setupLayout()
{
    auto* centralWidget = new QWidget(this);
    auto* mainLayout = new QHBoxLayout(centralWidget);

    // 1. Construct Filter Panel UI elements (Left Sidebar Layout)
    auto* leftLayout = new QVBoxLayout();
    leftLayout->setSpacing(15);

    auto* filterTitle = new QLabel("<b>WMS FILTER PANEL</b>", this);
    m_searchIdLineEdit->setPlaceholderText("Search Package ID...");

    m_stateFilterComboBox->addItem("All Statuses", -1);
    m_stateFilterComboBox->addItem("On Route", static_cast<int>(wms::domain::PackageStateId::OnRoute));
    m_stateFilterComboBox->addItem("In Storage", static_cast<int>(wms::domain::PackageStateId::InStorage));
    m_stateFilterComboBox->addItem("Dispatched", static_cast<int>(wms::domain::PackageStateId::Dispatched));
    m_stateFilterComboBox->addItem("Missing", static_cast<int>(wms::domain::PackageStateId::Missing));
    m_stateFilterComboBox->addItem("Overdue", static_cast<int>(wms::domain::PackageStateId::Overdue));

    leftLayout->addWidget(filterTitle);
    leftLayout->addWidget(new QLabel("Quick Status Filter:", this));
    leftLayout->addWidget(m_stateFilterComboBox);
    leftLayout->addWidget(new QLabel("Search Box:", this));
    leftLayout->addWidget(m_searchIdLineEdit);
    leftLayout->addStretch();

    // 2. Construct Action Bar & Main Table View (Right Workplace Layout)
    auto* rightLayout = new QVBoxLayout();
    auto* actionBarLayout = new QHBoxLayout();

    // Styling key operational control layout buttons
    m_addButton->setStyleSheet("background-color: #2ecc71; color: white; font-weight: bold;");
    m_dispatchButton->setStyleSheet("background-color: #3498db; color: white;");
    m_missingButton->setStyleSheet("background-color: #e74c3c; color: white;");

    actionBarLayout->addWidget(m_addButton);
    actionBarLayout->addWidget(m_dispatchButton);
    actionBarLayout->addWidget(m_missingButton);
    actionBarLayout->addStretch();
    actionBarLayout->addWidget(m_removeButton);

    rightLayout->addLayout(actionBarLayout);
    rightLayout->addWidget(m_tableView);

    // Consolidate geometric layout ratios (1 part Left Panel : 4 parts Right Dashboard)
    mainLayout->addLayout(leftLayout, 1);
    mainLayout->addLayout(rightLayout, 4);

    setCentralWidget(centralWidget);
    setWindowTitle("Warehouse Management System (WMS) - Premium Interface Layout");
    resize(1024, 640);
}

void MainWindow::updateTable()
{
    // Section 7.2: One-way data flow - fetch backend collection records and pipe to UI model
    auto packages = m_manager->getAllPackages();
    m_tableModel->refresh(std::move(packages));
}

void MainWindow::onFilterChanged()
{
    int selectedData = m_stateFilterComboBox->currentData().toInt();
    if (selectedData == -1) {
        updateTable();
        return;
    }

    // Direct binding invocation to native state filters inside the manager service module
    wms::domain::PackageStateId state = static_cast<wms::domain::PackageStateId>(selectedData);
    auto filteredPackages = m_manager->getByState(state);
    m_tableModel->refresh(std::move(filteredPackages));
}

void MainWindow::onSearchIdChanged(const QString& text)
{
    if (text.isEmpty()) {
        updateTable();
        return;
    }

    try {
        // Section 6.1: Convert parameters at boundaries (QString to std::string mapping)
        auto pkg = m_manager->getPackage(text.toStdString());
        m_tableModel->refresh({ pkg });
    }
    catch (const std::runtime_error&) {
        // Clear dataset list representation mapping on mismatch lookup errors
        m_tableModel->refresh({});
    }
}

void MainWindow::on_addButton_clicked()
{
    AddPackageDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted)
    {
        m_manager->addPackage(dialog.packageData());
        updateTable();
    }
}

void MainWindow::on_removeButton_clicked()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) return;

    QString id = m_tableModel->packageIdAt(idx.row());
    auto reply = QMessageBox::question(this, "Delete", "Remove this package from the system?", QMessageBox::Yes | QMessageBox::No);
    if (reply == QMessageBox::Yes) {
        m_manager->removePackage(id.toStdString());
        updateTable();
    }
}

void MainWindow::on_dispatchButton_clicked()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::warning(this, "Warning", "Please select a package to dispatch.");
        return;
    }

    std::string id = m_tableModel->packageIdAt(idx.row()).toStdString();
    try {
        // Drive domain runtime changes via explicit state transitions rules inside service layer
        m_manager->dispatchPackage(id);
        updateTable();
        QMessageBox::information(this, "Success", "Package dispatched successfully!");
    }
    catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Operation Error", e.what());
    }
}

void MainWindow::on_missingButton_clicked()
{
    QModelIndex idx = m_tableView->currentIndex();
    if (!idx.isValid()) {
        QMessageBox::warning(this, "Warning", "Please select a package.");
        return;
    }

    std::string id = m_tableModel->packageIdAt(idx.row()).toStdString();
    try {
        // Drive domain runtime changes via explicit state transitions rules inside service layer
        m_manager->markMissing(id);
        updateTable();
        QMessageBox::warning(this, "Status Alert", "Package marked as MISSING!");
    }
    catch (const std::runtime_error& e) {
        QMessageBox::critical(this, "Operation Error", e.what());
    }
}