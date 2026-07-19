/**
 * @file   WarehouseManager.cpp
 * @brief  Implementation of WarehouseManager service.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-10
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-07-18
 * @changelog
 *   - Change query methods to use findByCriteria().
 *   - Add getDailyTodoList() implementation.
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - checkOverduePackages() now fetches only InStorage packages via
 *     findByCriteria() instead of getAll(), avoiding a full table scan.
 *     The stateBefore guard is retained: even though we only fetched
 *     InStorage packages, handle() could theoretically transition to a
 *     state other than Overdue in a future implementation, so we still
 *     check that the state changed to Overdue before calling update().
 */

#include "service/WarehouseManager.h"

#include "domain/queries/PackageQueryCriteria.h"
#include "domain/states/OnRouteState.h"
#include "domain/states/InStorageState.h"
#include "domain/states/DispatchedState.h"
#include "domain/states/MissingState.h"
#include "domain/states/OverdueState.h"

#include <stdexcept>

namespace wms::service
{
    // -------------------------------------------------------------------------
    // Construction
    // -------------------------------------------------------------------------

    WarehouseManager::WarehouseManager(
        std::unique_ptr<repository::IPackageRepository> repo)
        : m_repo{ std::move(repo) }
    {
        if (!m_repo)
            throw std::invalid_argument("WarehouseManager - repo must not be nullptr");
    }

    // -------------------------------------------------------------------------
    // CRUD
    // -------------------------------------------------------------------------

    void WarehouseManager::addPackage(domain::Package package)
    {
        m_repo->add(std::move(package));
    }

    std::vector<domain::Package> WarehouseManager::getAllPackages() const
    {
        return m_repo->getAll();
    }

    domain::Package WarehouseManager::getPackage(const std::string& id) const
    {
        auto opt = m_repo->getById(id);
        if (!opt)
            throw std::runtime_error("WarehouseManager::getPackage - not found: " + id);
        return std::move(*opt);
    }

    void WarehouseManager::updatePackage(domain::Package package)
    {
        m_repo->update(std::move(package));
    }

    void WarehouseManager::removePackage(const std::string& id)
    {
        m_repo->remove(id);
    }

    // -------------------------------------------------------------------------
    // State transitions - manual
    // -------------------------------------------------------------------------

    void WarehouseManager::receivePackage(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            if (pkg.currentStateId() != domain::PackageStateId::OnRoute)
                throw std::runtime_error(
                    "receivePackage - package is not OnRoute: " + pkg.id());

            pkg.transitionTo(std::make_unique<domain::InStorageState>());
        });
    }

    void WarehouseManager::dispatchPackage(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            const auto state = pkg.currentStateId();
            if (state != domain::PackageStateId::InStorage &&
                state != domain::PackageStateId::Overdue)
            {
                throw std::runtime_error(
                    "dispatchPackage - package must be InStorage or Overdue: " + pkg.id());
            }

            pkg.transitionTo(std::make_unique<domain::DispatchedState>());
        });
    }

    void WarehouseManager::markMissing(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            if (pkg.currentStateId() == domain::PackageStateId::Dispatched)
                throw std::runtime_error(
                    "markMissing - cannot mark a dispatched package as missing: " + pkg.id());

            pkg.transitionTo(std::make_unique<domain::MissingState>());
        });
    }

    void WarehouseManager::markFound(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            if (pkg.currentStateId() != domain::PackageStateId::Missing)
                throw std::runtime_error(
                    "markFound - package is not Missing: " + pkg.id());

            pkg.transitionTo(std::make_unique<domain::InStorageState>());
        });
    }

    // -------------------------------------------------------------------------
    // Overdue check
    // -------------------------------------------------------------------------

    int WarehouseManager::checkOverduePackages()
    {
        int count = 0;

        // Only fetch InStorage packages - no need to load the entire repository.
        // We iterate a local snapshot so transitions inside handle() do not
        // invalidate the repository's internal container mid-loop.
        domain::PackageQueryCriteria criteria;
        criteria.state = domain::PackageStateId::InStorage;
        auto packages = m_repo->findByCriteria(criteria);

        for (auto& pkg : packages)
        {
            const auto stateBefore = pkg.currentStateId();
            pkg.handleCurrentState(); // InStorageState may call transitionTo(Overdue)

            if (pkg.currentStateId() == domain::PackageStateId::Overdue &&
                stateBefore != domain::PackageStateId::Overdue)
            {
                m_repo->update(pkg); // persist the transition
                ++count;
            }
        }

        return count;
    }

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    std::vector<domain::Package> WarehouseManager::getByState(
        domain::PackageStateId state) const
    {
        domain::PackageQueryCriteria criteria;
        criteria.state = state;
        return m_repo->findByCriteria(criteria);
    }

    std::vector<domain::Package> WarehouseManager::getByCategory(
        domain::Category category) const
    {
        domain::PackageQueryCriteria criteria;
        criteria.category = category;
        return m_repo->findByCriteria(criteria);
    }

    std::vector<domain::Package> WarehouseManager::getOverdue() const
    {
        domain::PackageQueryCriteria criteria;
        criteria.state = domain::PackageStateId::Overdue;
        return m_repo->findByCriteria(criteria);
    }

    std::vector<domain::Package> WarehouseManager::getMissing() const
    {
        domain::PackageQueryCriteria criteria;
        criteria.state = domain::PackageStateId::Missing;
        return m_repo->findByCriteria(criteria);
    }

    WarehouseManager::DailyTodoList WarehouseManager::getDailyTodoList() const
    {
        domain::PackageQueryCriteria importedCriteria;
        importedCriteria.importedToday = true;

        domain::PackageQueryCriteria dueCriteria;
        dueCriteria.exportDueToday = true;

        return DailyTodoList{
            m_repo->findByCriteria(importedCriteria),
            m_repo->findByCriteria(dueCriteria)
        };
    }

    // -------------------------------------------------------------------------
    // Persistence
    // -------------------------------------------------------------------------

    void WarehouseManager::save()
    {
        m_repo->save();
    }

    void WarehouseManager::load()
    {
        m_repo->load();
    }

    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    void WarehouseManager::mutatePackage(
        const std::string& id,
        const std::function<void(domain::Package&)>& mutate)
    {
        auto opt = m_repo->getById(id);
        if (!opt)
            throw std::runtime_error("WarehouseManager - package not found: " + id);

        mutate(*opt);           // apply the mutation
        m_repo->update(*opt);   // persist the changed package
    }

}
