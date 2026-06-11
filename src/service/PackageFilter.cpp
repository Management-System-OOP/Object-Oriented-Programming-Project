/**
 * @file   PackageFilter.cpp
 * @brief  Implementation of PackageFilter predicate factories.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-11
 */

#include "service/PackageFilter.h"

#include <algorithm>
#include <chrono>
#include <cctype>
#include <string>

namespace wms::service
{
    // Apply

    std::vector<domain::Package> PackageFilter::apply(
        const std::vector<domain::Package>& packages,
        const Predicate& predicate)
    {
        std::vector<domain::Package> result;
        result.reserve(packages.size());
        for (const auto& pkg : packages)
            if (predicate(pkg))
                result.push_back(pkg);
        return result;
    }

    // Predicate factories

    PackageFilter::Predicate PackageFilter::byState(domain::PackageStateId stateId)
    {
        return [stateId](const domain::Package& pkg) {
            return pkg.currentStateId() == stateId;
        };
    }

    PackageFilter::Predicate PackageFilter::byCategory(domain::Category category)
    {
        return [category](const domain::Package& pkg) {
            return pkg.metadata().category == category;
        };
    }

    PackageFilter::Predicate PackageFilter::byMinWeight(double minWeight)
    {
        return [minWeight](const domain::Package& pkg) {
            return pkg.metadata().weight >= minWeight;
        };
    }

    PackageFilter::Predicate PackageFilter::byMaxWeight(double maxWeight)
    {
        return [maxWeight](const domain::Package& pkg) {
            return pkg.metadata().weight <= maxWeight;
        };
    }

    PackageFilter::Predicate PackageFilter::byDescriptionKeyword(const std::string& keyword)
    {
        // Lowercase copy for case-insensitive comparison.
        std::string lowerKeyword = keyword;
        std::transform(lowerKeyword.begin(), lowerKeyword.end(),
                       lowerKeyword.begin(),
                       [](unsigned char c){ return static_cast<char>(std::tolower(c)); });

        return [lowerKeyword](const domain::Package& pkg) {
            std::string desc = pkg.metadata().description;
            std::transform(desc.begin(), desc.end(), desc.begin(),
                           [](unsigned char c){ return static_cast<char>(std::tolower(c)); });
            return desc.find(lowerKeyword) != std::string::npos;
        };
    }

    PackageFilter::Predicate PackageFilter::byZone(const std::string& zone)
    {
        return [zone](const domain::Package& pkg) {
            return pkg.location().zone == zone;
        };
    }

    PackageFilter::Predicate PackageFilter::byOverdueDate()
    {
        return [](const domain::Package& pkg) {
            const auto today = std::chrono::floor<std::chrono::days>(
                std::chrono::system_clock::now());
            const std::chrono::year_month_day todayYmd{ today };
            return todayYmd > pkg.logistics().expectedExportDate;
        };
    }

    // Combinators

    PackageFilter::Predicate PackageFilter::combine(
        const Predicate& a, const Predicate& b)
    {
        return [a, b](const domain::Package& pkg) {
            return a(pkg) && b(pkg);
        };
    }

    PackageFilter::Predicate PackageFilter::combineOr(
        const Predicate& a, const Predicate& b)
    {
        return [a, b](const domain::Package& pkg) {
            return a(pkg) || b(pkg);
        };
    }

    PackageFilter::Predicate PackageFilter::negate(const Predicate& p)
    {
        return [p](const domain::Package& pkg) {
            return !p(pkg);
        };
    }

} // namespace wms::service
