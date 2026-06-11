/**
 * @file   OnRouteState.cpp
 * @brief  Implementation of OnRouteState.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 */

#include "domain/states/OnRouteState.h"
#include "domain/entities/Package.h"

namespace wms::domain
{

    void OnRouteState::handle(Package& /*pkg*/)
    {
        // No automatic action while in transit.
        // Transition to InStorageState is triggered explicitly when
        // warehouse staff confirm physical receipt of the package.
    }

    std::string_view OnRouteState::getStateLabel() const
    {
        return "On Route";
    }

    PackageStateId OnRouteState::stateId() const
    {
        return PackageStateId::OnRoute;
    }

}