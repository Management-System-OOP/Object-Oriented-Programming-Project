/**
 * @file   MainWindow.h
 * @brief  Complete WMS graphical interface implementation connecting to WarehouseManager.
 * @author Nguyen Viet Bach
 * @date   2026-06-23
 */

#pragma once

#include <QMainWindow>
#include <QTableView>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGridLayout>

#include "service/WarehouseManager.h"
#include "gui/PackageTableModel.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(wms::service::WarehouseManager* manager, QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    // Action Bar slots
    void on_addButton_clicked();
    void on_removeButton_clicked();
    void on_dispatchButton_clicked();
    void on_missingButton_clicked();

    // FilterPanel slots
    void onFilterChanged();
    void onSearchIdChanged(const QString& text);

private:
    void setupLayout();
    void updateTable();

    // Core components
    wms::service::WarehouseManager* m_manager;
    PackageTableModel* m_tableModel;

    // UI Widgets - Main View
    QTableView* m_tableView;

    // UI Widgets - FilterPanel (Left)
    QComboBox* m_stateFilterComboBox;
    QLineEdit* m_searchIdLineEdit;

    // UI Widgets - Action Bar
    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_dispatchButton;
    QPushButton* m_missingButton;
};