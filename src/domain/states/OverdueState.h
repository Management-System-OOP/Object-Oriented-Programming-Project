/**
 * @file   OverdueState.h
 * @brief  State representing a package that has not left the warehouse by its
 *         expected export date.
 *
 *  Entered automatically from InStorageState when handle() detects that
 *  today's date is past expectedExportDate. Staff must investigate and either
 *  dispatch the package (→ DispatchedState) or mark it missing (→ MissingState).
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 */

#pragma once

#include "domain/states/IPackageState.h"

namespace wms::domain
{

    /**
     * @brief  The package is still in the warehouse past its expected export date.
     *
     *  handle() is a no-op — the package is already in the worst normal state
     *  the system can detect automatically. Further action requires human decisions.
     */
    class OverdueState final : public IPackageState
    {
    public:
        /**
         * @brief  No further automatic transitions from the overdue state.
         * @param  pkg  The owning package (unused in this state).
         */
        void handle(Package& pkg) override;

        /**
         * @brief  Returns "Overdue".
         */
        std::string_view getStateLabel() const override;

        /**
         * @brief  Returns PackageStateId::Overdue.
         */
        PackageStateId stateId() const override;
    };

}