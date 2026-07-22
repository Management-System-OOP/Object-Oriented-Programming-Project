/**
 * @file   WarehouseManager.h
 * @brief  Central service coordinating package lifecycle operations.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-11
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-07-18
 * @changelog
 *   - Add DailyTodoList struct for dashboard visualisation.
 *   - Add getDailyTodoList().
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Remove #include "service/PackageFilter.h". PackageFilter is no longer
 *     used inside WarehouseManager; all query methods delegate to
 *     IPackageRepository::findByCriteria() via PackageQueryCriteria, which
 *     is more efficient (SQL-backed path) and keeps the service layer thin.
 *   - Update Responsibilities comment to reflect the removal of PackageFilter.
 *   - Update checkOverduePackages() doc comment: the method now passes a
 *     state filter to the repository instead of calling getAll().
 * 
 * @update
 * @author Do Minh Khang
 * @date   2026-07-22
 * @changelog
 *   - Add mulpti-query for filter
 *
 * Responsibilities:
 *  - CRUD operations delegated to IPackageRepository.
 *  - Periodic overdue check: fetches only InStorage packages via
 *    findByCriteria(), calls handleCurrentState() on each, and persists
 *    any that transitioned to OverdueState.
 *  - Manual state transitions: receive, dispatch, markMissing, markFound.
 *  - Query helpers delegated to IPackageRepository::findByCriteria().
 */

#pragma once

#include "repository/IPackageRepository.h"
#include "domain/entities/Package.h"
#include "domain/states/PackageStateId.h"

#include <vector>
#include <string>
#include <memory>
#include <functional>

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
         *  Fetches only InStorage packages via findByCriteria() to avoid
         *  loading the entire repository. Calls Package::handleCurrentState()
         *  on each, allowing InStorageState::handle() to transition overdue
         *  packages to OverdueState. Each transitioned package is immediately
         *  persisted back via update().
         *
         * @return Number of packages that were transitioned to OverdueState.
         */
        int checkOverduePackages();

        // --Queries--

        // WarehouseManager.h, in the --Queries-- section, above the four existing methods

        /**
         * @brief  General-purpose query entry point: forwards a caller-composed
         *         PackageQueryCriteria straight to the repository.
         *
         * @param  criteria  Any combination of fields; unset fields are not
         *                   constrained.
         */
        std::vector<domain::Package> queryPackages(const domain::PackageQueryCriteria& criteria) const;

        std::vector<domain::Package> getByState(domain::PackageStateId state) const;
        std::vector<domain::Package> getByCategory(domain::Category category) const;
        std::vector<domain::Package> getOverdue() const;
        std::vector<domain::Package> getMissing() const;

        /**
         * @brief  Groups packages by today's import and export activity.
         *
         *  Returns two separate lists - packages that arrived today and packages
         *  due to leave today - since each group calls for different actions.
         *  A package satisfying both criteria appears in both lists.
         */
        struct DailyTodoList
        {
            std::vector<domain::Package> importedToday;
            std::vector<domain::Package> exportDueToday;
        };

        /**
         * @brief  Dashboard query: packages imported today and packages due for
         *         export today, issued as two separate findByCriteria() calls.
         */
        DailyTodoList getDailyTodoList() const;

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
