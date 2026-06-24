/**
 * @file   FilterPanel.h
 * @brief  UI widget panel facilitating query filtering.
 * @author Nguyen Viet Bach
 * @date   2026-06-23
 */

#pragma once

#include <QWidget>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <vector>
#include "service/PackageFilter.h"

class FilterPanel : public QWidget
{
    Q_OBJECT

signals:
    void filterChanged();

public:
    explicit FilterPanel(QWidget* parent = nullptr);
    std::vector<wms::service::PackageFilter::Predicate> currentPredicates() const;

private slots:
    void on_applyButton_clicked();

private:
    QComboBox* m_stateComboBox;
    QPushButton* m_applyButton;
};