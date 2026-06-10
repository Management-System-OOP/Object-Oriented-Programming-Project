/**
 * @file   PackageMetadata.h
 * @brief  Metadata describing package characteristics used across the domain.
 *
 * @author Do Minh Khang
 * @date   2026-06-09
 */

#pragma once

#include "domain/entities/Category.h"
#include "domain/entities/Dimension.h"

#include "pch.h"

namespace wms::domain
{
    /**
     * @struct PackageMetadata
     * @brief  Aggregates descriptive attributes of a package.
     *
     * The struct is a plain value object used by the domain layer for pricing,
     * handling, and storage decisions. It intentionally uses std::string and
     * other domain types to remain Qt-free.
     */
    struct PackageMetadata
    {
        Category category;
        double weight;
        Dimension dimensions;
        double cost;
        std::string description;
    };
}