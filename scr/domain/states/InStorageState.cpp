/**
 * @file   InStorageState.cpp
 * @brief  Implementation of InStorageState.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 */

#include "domain/states/InStorageState.h"
#include "domain/states/OverdueState.h"
#include "domain/entities/Package.h"

#include "pch.h"

namespace wms::domain
{

    void InStorageState::handle(Package& pkg)
    {
        const auto today = std::chrono::year_month_day{
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())
        };
        const auto dueDate = pkg.logistics().expectedExportDate;

        // Transition to OverdueState if today is strictly past the expected export date.
        // The check uses std::chrono::year_month_day's built-in operator> so no
        // manual day/month/year comparison is needed.
        if (today > dueDate)
        {
            pkg.transitionTo(std::make_unique<OverdueState>());
        }
    }

    std::string_view InStorageState::getStateLabel() const
    {
        return "In Storage";
    }

    PackageStateId InStorageState::stateId() const
    {
        return PackageStateId::InStorage;
    }

}