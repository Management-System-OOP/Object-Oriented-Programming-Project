/**
 * @file   WarehouseManager.cpp
 * @brief  Implementation of WarehouseManager service.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-10
 */

#include "service/WarehouseManager.h"

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
            throw std::invalid_argument("WarehouseManager — repo must not be nullptr");
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
            throw std::runtime_error("WarehouseManager::getPackage — not found: " + id);
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
    // State transitions — manual
    // -------------------------------------------------------------------------

    void WarehouseManager::receivePackage(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            if (pkg.currentStateId() != domain::PackageStateId::OnRoute)
                throw std::runtime_error(
                    "receivePackage — package is not OnRoute: " + pkg.id());

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
                    "dispatchPackage — package must be InStorage or Overdue: " + pkg.id());
            }

            pkg.transitionTo(std::make_unique<domain::DispatchedState>());
        });
    }

    void WarehouseManager::markMissing(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            if (pkg.currentStateId() == domain::PackageStateId::Dispatched)
                throw std::runtime_error(
                    "markMissing — cannot mark a dispatched package as missing: " + pkg.id());

            pkg.transitionTo(std::make_unique<domain::MissingState>());
        });
    }

    void WarehouseManager::markFound(const std::string& id)
    {
        mutatePackage(id, [](domain::Package& pkg) {
            if (pkg.currentStateId() != domain::PackageStateId::Missing)
                throw std::runtime_error(
                    "markFound — package is not Missing: " + pkg.id());

            pkg.transitionTo(std::make_unique<domain::InStorageState>());
        });
    }

    // -------------------------------------------------------------------------
    // Overdue check
    // -------------------------------------------------------------------------

    int WarehouseManager::checkOverduePackages()
    {
        int count = 0;

        // We iterate a snapshot so that transitions inside handle() do not
        // invalidate the repository's internal container mid-loop.
        auto packages = m_repo->getAll();

        for (auto& pkg : packages)
        {
            if (pkg.currentStateId() == domain::PackageStateId::InStorage)
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
        }

        return count;
    }

    // -------------------------------------------------------------------------
    // Queries
    // -------------------------------------------------------------------------

    std::vector<domain::Package> WarehouseManager::getByState(
        domain::PackageStateId state) const
    {
        return PackageFilter::apply(m_repo->getAll(),
                                   PackageFilter::byState(state));
    }

    std::vector<domain::Package> WarehouseManager::getByCategory(
        domain::Category category) const
    {
        return PackageFilter::apply(m_repo->getAll(),
                                   PackageFilter::byCategory(category));
    }

    std::vector<domain::Package> WarehouseManager::getOverdue() const
    {
        return PackageFilter::apply(m_repo->getAll(),
                                   PackageFilter::byState(
                                       domain::PackageStateId::Overdue));
    }

    std::vector<domain::Package> WarehouseManager::getMissing() const
    {
        return PackageFilter::apply(m_repo->getAll(),
                                   PackageFilter::byState(
                                       domain::PackageStateId::Missing));
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
            throw std::runtime_error("WarehouseManager — package not found: " + id);

        mutate(*opt);           // apply the mutation
        m_repo->update(*opt);   // persist the changed package
    }

} // namespace wms::service
