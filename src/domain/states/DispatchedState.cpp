/**
 * @file   DispatchedState.cpp
 * @brief  Implementation of DispatchedState.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 */

#include "domain/states/DispatchedState.h"
#include "domain/entities/Package.h"

namespace wms::domain
{

    void DispatchedState::handle(Package& /*pkg*/)
    {
        // Terminal state — no automatic action or transition.
        // The package has left the warehouse and is no longer managed here.
    }

    std::string_view DispatchedState::getStateLabel() const
    {
        return "Dispatched";
    }

    PackageStateId DispatchedState::stateId() const
    {
        return PackageStateId::Dispatched;
    }

}