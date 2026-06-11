/**
 * @file   MissingState.cpp
 * @brief  Implementation of MissingState.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 */

#include "domain/states/MissingState.h"
#include "domain/entities/Package.h"

namespace wms::domain
{

    void MissingState::handle(Package& /*pkg*/)
    {
        // No automatic action — recovery requires manual intervention by staff.
        // When the package is found, WarehouseManager transitions it back to
        // InStorageState explicitly.
    }

    std::string_view MissingState::getStateLabel() const
    {
        return "Missing";
    }

    PackageStateId MissingState::stateId() const
    {
        return PackageStateId::Missing;
    }

}