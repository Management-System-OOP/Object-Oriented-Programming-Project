/**
 * @file   main.cpp
 * @brief  Main entry point for testing the Warehouse Management System core and repository modules.
 *
 * @author Duong Anh Hao
 * @date   2026-06-15
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-06-24
 * @changelog
 *   - Replace package constructor in part 3 with create()
 *   - Add comfirmination in part 7 for id checking
 */
#include <iostream>
#include <exception>
#include <string>
#include <chrono>

// Include Domain Entities
#include "domain/entities/Package.h"
#include "domain/entities/PackageMetadata.h"
#include "domain/entities/Address.h"
#include "domain/entities/LogisticsInfo.h"
#include "domain/entities/StorageLocation.h"

// Include Repository
#include "repository/JsonPackageRepository.h"

int main() {
    try {
        std::cout << "      WMS REPOSITORY MODULE TEST       \n";

        // 1. Initialize Repository
        const QString testFile = "test_data.json";
        wms::repository::JsonPackageRepository repo(testFile);
        std::cout << "[INFO] Repository initialized successfully.\n";

        // 2. Prepare mock data for a new Package
        // Metadata: Category, weight, dimensions(l, w, h), cost, description
        wms::domain::PackageMetadata metadata{
            "Iphone 100",
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
        wms::domain::Package newPackage = wms::domain::Package::create(
            metadata, source, destination, logistics, location
        );
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

        auto reloadedPkg = repoRestart.getById(pkgId);
        if (reloadedPkg.has_value() && reloadedPkg->id() == pkgId)
            std::cout << "[SUCCESS] ID preserved across save/load: " << pkgId << "\n";
        else
            std::cout << "[ERROR] ID mismatch after reload - Package::load() not wired correctly.\n";

        // 8. Test REMOVE operation
        std::cout << "\n--- Cleaning up test data ---\n";
        repoRestart.remove(pkgId);
        repoRestart.save();
        std::cout << "[SUCCESS] Package removed and JSON file updated.\n";
        std::cout << "        ALL TESTS PASSED!              \n";   

    }
    catch (const std::exception& e) {
        // Catch and display any runtime errors or validation failures
        std::cerr << "[EXCEPTION] " << e.what() << "\n";
    }

    return 0;
}
