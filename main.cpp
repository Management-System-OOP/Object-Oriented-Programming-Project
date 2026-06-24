/**
 * @file   main.cpp
 * @brief  Launching the Qt GUI interface.
 *
 * @author  Nguyen Viet Bach
 * @date   2026-06-23
 */
#include <iostream>
#include <exception>
#include <string>
#include <chrono>
#include <memory>

 // Qt Framework Infrastructure
#include <QApplication>

// Include Domain Entities
#include "domain/entities/Package.h"
#include "domain/entities/PackageMetadata.h"
#include "domain/entities/Address.h"
#include "domain/entities/LogisticsInfo.h"
#include "domain/entities/StorageLocation.h"

// Include Repository & Service Layer
#include "repository/JsonPackageRepository.h"
#include "service/WarehouseManager.h"

// Include GUI Layer
#include "gui/MainWindow.h"

int main(int argc, char* argv[]) {
    // Khởi tạo Qt Application framework ngay từ đầu để sẵn sàng cho GUI
    QApplication app(argc, argv);

    // SỬA ĐỔI 1: Thay shared_ptr bằng unique_ptr chuyên biệt cho GUI/Manager sử dụng dữ liệu thật
    auto guiRepo = std::make_unique<wms::repository::JsonPackageRepository>("resources/data/packages.json");

    try {
        std::cout << "      WMS REPOSITORY MODULE TEST        \n";

        // 1. Initialize Repository (Dùng file tạm test_data.json như cũ để test)
        const QString testFile = "test_data.json";
        wms::repository::JsonPackageRepository repo(testFile);
        std::cout << "[INFO] Repository initialized successfully.\n";

        // 2. Prepare mock data for a new Package
        wms::domain::PackageMetadata metadata{
            wms::domain::Category::Standard,
            15.5,
            {10.0, 5.0, 5.0},
            250.0,
            "Test Electronics Package"
        };

        // Source and Destination Addresses
        wms::domain::Address source{ "123 Tech Street", "Hanoi", "Vietnam", "100000" };
        wms::domain::Address destination{ "456 Startup Blvd", "Ho Chi Minh City", "Vietnam", "700000" };

        // Logistics Information 
        std::chrono::year_month_day importDate{ std::chrono::year{2026}, std::chrono::month{6}, std::chrono::day{15} };
        std::chrono::year_month_day exportDate{ std::chrono::year{2026}, std::chrono::month{6}, std::chrono::day{20} };
        wms::domain::LogisticsInfo logistics{ importDate, exportDate, "Truck-HN-01", "Truck-HCM-02", "CONT-1234" };

        // Storage Location
        wms::domain::StorageLocation location{ "ZoneA", "Aisle1", 1, 1 };

        // 3. Create a new Package instance
        wms::domain::Package newPackage(metadata, source, destination, logistics, location);
        std::string pkgId = newPackage.id();
        std::cout << "[INFO] Created new Package. UUID: " << pkgId << "\n";

        // 4. Test ADD and SAVE operations
        repo.add(newPackage);
        repo.save();
        std::cout << "[SUCCESS] Package added and saved to JSON file.\n";

        // 5. Test READ operations (Get All)
        auto allPackages = repo.getAll();
        std::cout << "[INFO] Total packages in repository: " << allPackages.size() << "\n";

        // 6. Test READ operation (Get by ID)
        auto foundPkg = repo.getById(pkgId);
        if (foundPkg.has_value()) {
            std::cout << "[SUCCESS] Retrieved Package successfully by ID: " << foundPkg->id() << "\n";
        }
        else {
            std::cout << "[ERROR] Could not find the package by ID.\n";
        }

        // 7. Test LOAD operation (Simulate restarting the application)
        std::cout << "\n--- Simulating App Restart ---\n";
        wms::repository::JsonPackageRepository repoRestart(testFile);
        auto loadedPackages = repoRestart.getAll();
        std::cout << "[SUCCESS] Reloaded repository from file. Total packages: " << loadedPackages.size() << "\n";

        // 8. Test REMOVE operation
        std::cout << "\n--- Cleaning up test data ---\n";
        repoRestart.remove(pkgId);
        repoRestart.save();
        std::cout << "[SUCCESS] Package removed and JSON file updated.\n";
        std::cout << "        ALL CONSOLE TESTS PASSED!              \n\n";

    }
    catch (const std::exception& e) {
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
    }

    // =========================================================================
    // PHẦN SỬA ĐỔI BỔ SUNG: KÍCH HOẠT VÀ LIÊN KẾT GUI VÀO MANAGER
    // =========================================================================
    std::cout << "[INFO] Launching Graphical User Interface...\n";

    // SỬA ĐỔI 2: Dùng std::move để tiêm chuyển giao quyền sở hữu unique_ptr vào WarehouseManager
    wms::service::WarehouseManager manager(std::move(guiRepo));

    // Tạo màn hình chính và tiêm (inject) địa chỉ Manager vào UI điều khiển dữ liệu
    MainWindow mainWindow(&manager);
    mainWindow.show();

    // Chạy vòng lặp ứng dụng Qt GUI
    return app.exec();
}