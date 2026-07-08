/**
 * @file   WarehouseManager.h
 * @brief  Central service coordinating package lifecycle operations.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-11
 *
 * Responsibilities:
 *  - CRUD operations delegated to IPackageRepository.
 *  - Periodic overdue check: iterates all InStorage packages and calls
 *    handleCurrentState(), which lets InStorageState auto-transition to
 *    OverdueState when today > expectedExportDate.
 *  - Manual state transitions: receive, dispatch, markMissing, markFound.
 *  - Exposes query helpers via PackageFilter.
 */

#pragma once

#include "repository/IPackageRepository.h"
#include "service/PackageFilter.h"
#include "domain/entities/Package.h"
#include "domain/states/PackageStateId.h"

#include <vector>
#include <string>
#include <memory>

namespace wms::service
{
    /**
     * @class WarehouseManager
     * @brief Orchestrates all business operations on packages.
     *
     * Depends only on IPackageRepository (injected via constructor) so the
     * concrete persistence backend is swappable without touching this class.
     */
    class WarehouseManager
    {
    public:
        /**
         * @brief  Construct with an injected repository.
         * @param  repo  Owning pointer to any IPackageRepository implementation.
         */
        explicit WarehouseManager(std::unique_ptr<repository::IPackageRepository> repo);

        // --CRUD--

        /**
         * @brief  Register a new package in the system (starts in OnRouteState).
         * @param  package  Fully constructed Package to persist.
         */
        void addPackage(domain::Package package);

        /**
         * @brief  Retrieve all packages.
         */
        std::vector<domain::Package> getAllPackages() const;

        /**
         * @brief  Retrieve a single package by UUID.
         * @throws std::runtime_error if not found.
         */
        domain::Package getPackage(const std::string& id) const;

        /**
         * @brief  Persist changes to an existing package.
         */
        void updatePackage(domain::Package package);

        /**
         * @brief  Remove a package from the system entirely.
         */
        void removePackage(const std::string& id);

        // --State transitions (manual)--

        /**
         * @brief  Mark package as physically received → InStorageState.
         * @throws std::runtime_error if package is not currently OnRoute.
         */
        void receivePackage(const std::string& id);

        /**
         * @brief  Dispatch package for outbound delivery → DispatchedState.
         * @throws std::runtime_error if package is not InStorage or Overdue.
         */
        void dispatchPackage(const std::string& id);

        /**
         * @brief  Flag package as missing → MissingState.
         * @throws std::runtime_error if package is already Dispatched.
         */
        void markMissing(const std::string& id);

        /**
         * @brief  Recover a missing package → InStorageState.
         * @throws std::runtime_error if package is not currently Missing.
         */
        void markFound(const std::string& id);

        // --Overdue check--

        /**
         * @brief  Scan all InStorage packages and transition overdue ones.
         *
         *  Call this periodically (e.g. daily via QTimer, or on app startup).
         *  Calls Package::handleCurrentState() on each InStorage package so that
         *  InStorageState::handle() can perform the date check and call
         *  transitionTo(OverdueState) when necessary.
         *
         * @return Number of packages that were transitioned to OverdueState.
         */
        int checkOverduePackages();

        // --Queries (delegated to PackageFilter)--

        std::vector<domain::Package> getByState   (domain::PackageStateId state) const;
        std::vector<domain::Package> getByCategory(domain::Category category)    const;
        std::vector<domain::Package> getOverdue   ()                             const;
        std::vector<domain::Package> getMissing   ()                             const;

        // --Persistence--

        /** Flush all in-memory changes to the backing store. */
        void save();

        /** Reload from the backing store (discards in-memory state). */
        void load();

    private:
        std::unique_ptr<repository::IPackageRepository> m_repo;

        /**
         * @brief  Helper: fetch package, apply mutation, then update repo.
         * @param  id     UUID of the package to mutate.
         * @param  mutate Lambda that receives a Package& and modifies it.
         */
        void mutatePackage(const std::string& id,
                           const std::function<void(domain::Package&)>& mutate);
    };
}
