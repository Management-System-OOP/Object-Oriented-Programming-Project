/**
 * @file   PackageFilter.h
 * @brief  Composable predicates for filtering packages by various criteria.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-10
 *
 * Design:
 *  - Each filter function accepts a const Package& and returns bool.
 *  - PackageFilter::apply() takes a list of packages and a predicate and
 *    returns the matching subset.
 *  - Predicates are composable with standard lambda combinators.
 *
 * Example usage:
 * @code
 *     auto overdue = PackageFilter::apply(repo.getAll(),
 *                        PackageFilter::byState(PackageStateId::Overdue));
 *
 *     auto heavy = PackageFilter::apply(repo.getAll(),
 *                      PackageFilter::byMinWeight(50.0));
 *
 *     // Combine: overdue AND fragile
 *     auto combined = PackageFilter::apply(repo.getAll(),
 *         PackageFilter::combine(
 *             PackageFilter::byState(PackageStateId::Overdue),
 *             PackageFilter::byCategory(Category::Fragile)));
 * @endcode
 */

#pragma once

#include "domain/entities/Package.h"
#include "domain/states/PackageStateId.h"
#include "domain/entities/Category.h"

#include <vector>
#include <functional>
#include <string>

namespace wms::service
{
    /**
     * @class PackageFilter
     * @brief  Static utility providing predicate factories and an apply() method.
     *
     * All methods are static; the class is not meant to be instantiated.
     */
    class PackageFilter
    {
    public:
        PackageFilter() = delete;

        using Predicate = std::function<bool(const domain::Package&)>;

        // --Apply--

        /**
         * @brief  Returns all packages from @p packages for which @p predicate
         *         returns true.
         */
        static std::vector<domain::Package> apply(
            const std::vector<domain::Package>& packages,
            const Predicate& predicate);

        // --Predicate factories--

        /** Match packages currently in the given state. */
        static Predicate byState(domain::PackageStateId stateId);

        /** Match packages whose category equals @p category. */
        static Predicate byCategory(domain::Category category);

        /** Match packages whose weight >= @p minWeight (kg). */
        static Predicate byMinWeight(double minWeight);

        /** Match packages whose weight <= @p maxWeight (kg). */
        static Predicate byMaxWeight(double maxWeight);

        /**
         * @brief  Match packages whose description contains @p keyword
         *         (case-insensitive substring search).
         */
        static Predicate byDescriptionKeyword(const std::string& keyword);

        /**
         * @brief  Match packages stored in the given warehouse zone.
         * @param  zone  e.g. "A", "B", "Cold"
         */
        static Predicate byZone(const std::string& zone);

        /**
         * @brief  Match packages whose expectedExportDate is before today
         *         (i.e. they are overdue by date regardless of current state).
         */
        static Predicate byOverdueDate();

        // --Combinators--

        /**
         * @brief  Returns a predicate that is true when BOTH @p a and @p b
         *         are true (logical AND).
         */
        static Predicate combine(const Predicate& a, const Predicate& b);

        /**
         * @brief  Returns a predicate that is true when EITHER @p a or @p b
         *         is true (logical OR).
         */
        static Predicate combineOr(const Predicate& a, const Predicate& b);

        /**
         * @brief  Returns a predicate that negates @p p.
         */
        static Predicate negate(const Predicate& p);
    };
}
