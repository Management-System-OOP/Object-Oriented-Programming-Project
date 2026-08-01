void PackageFilterDialog::setupQuickTogglesGroup(QVBoxLayout* mainLayout)
    {
        auto* group = new QGroupBox("Date & Status Filters", this);
        auto* layout = new QGridLayout(group);
        layout->setHorizontalSpacing(10);
        layout->setVerticalSpacing(10);

        // 1. Import Filter
        m_filterImportCheck = new QCheckBox("Filter by Import Date:", this);
        m_importDateEdit = new QDateEdit(QDate::currentDate(), this);
        m_importDateEdit->setCalendarPopup(true);
        m_importDateEdit->setEnabled(false);

        connect(m_filterImportCheck, &QCheckBox::toggled, m_importDateEdit, &QWidget::setEnabled);

        layout->addWidget(m_filterImportCheck, 0, 0);
        layout->addWidget(m_importDateEdit, 0, 1);

        // 2. Export Filter
        m_filterExportCheck = new QCheckBox("Filter by Export Date:", this);
        m_exportDateEdit = new QDateEdit(QDate::currentDate(), this);
        m_exportDateEdit->setCalendarPopup(true);
        m_exportDateEdit->setEnabled(false);

        connect(m_filterExportCheck, &QCheckBox::toggled, m_exportDateEdit, &QWidget::setEnabled);

        layout->addWidget(m_filterExportCheck, 1, 0);
        layout->addWidget(m_exportDateEdit, 1, 1);

        mainLayout->addWidget(group);
    }
