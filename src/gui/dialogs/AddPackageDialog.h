/**
 * @file   AddPackageDialog.h
 * @brief  Dialog for collecting minimal package fields and building a domain Package.
 * @author Nguyen Viet Bach
 * @date   2026-06-24
 */

#pragma once

#include <QDialog>
#include <QLineEdit>
#include <QDoubleSpinBox>
#include <QPushButton>
#include <QFormLayout>

#include "domain/entities/Package.h"

class AddPackageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPackageDialog(QWidget* parent = nullptr);
    wms::domain::Package packageData() const;

private slots:
    void on_saveButton_clicked();

private:
    QLineEdit* m_descEdit;
    QDoubleSpinBox* m_weightSpin;
    QPushButton* m_saveButton;
};
