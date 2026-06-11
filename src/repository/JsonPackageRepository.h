/**
 * @file   JsonPackageRepository.h
 * @brief  JSON-file–backed implementation of IPackageRepository.
 *
 * @author Do Minh Khang
 * @date   2026-06-10
 *
 * This is the ONLY file outside gui/ that is allowed to use Qt (QString,
 * QFile, QJsonDocument, etc.). All other layers must remain Qt-free.
 *
 * JSON schema per package object:
 * {
 *   "id"          : "uuid-string",
 *   "state"       : "OnRoute" | "InStorage" | "Dispatched" | "Missing" | "Overdue",
 *   "metadata"    : { "category", "weight", "cost", "description",
 *                     "dimensions": { "length", "width", "height" } },
 *   "source"      : { "street", "city", "country", "postalCode" },
 *   "destination" : { "street", "city", "country", "postalCode" },
 *   "logistics"   : { "importDate", "expectedExportDate",    // "YYYY-MM-DD"
 *                     "importVehicle", "exportVehicle", "containerId" },
 *   "location"    : { "zone", "aisle", "shelf", "slot" }
 * }
 */

#pragma once

#include "repository/IPackageRepository.h"

#include <QString>
#include <QJsonObject>
#include <QJsonArray>

#include <unordered_map>

namespace wms::repository
{
    /**
     * @class JsonPackageRepository
     * @brief Loads and saves packages to a UTF-8 JSON file using Qt JSON APIs.
     *
     * The in-memory store is an ordered map keyed by package id for O(1) lookup.
     * Call load() once at startup and save() after any mutating operation (or
     * let WarehouseManager call save() at the end of each unit of work).
     */
    class JsonPackageRepository : public IPackageRepository
    {
    public:
        /**
         * @brief  Construct with the path to the JSON data file.
         * @param  filePath  Absolute or relative path, e.g. "data/packages.json".
         *                   The file is created on the first save() if absent.
         */
        explicit JsonPackageRepository(QString filePath);

        // --IPackageRepository--
        std::vector<domain::Package>        getAll()   const override;
        std::optional<domain::Package>      getById(const std::string& id) const override;
        void add   (domain::Package package) override;
        void update(domain::Package package) override;
        void remove(const std::string& id)   override;
        void save() override;
        void load() override;

    private:
        QString m_filePath;

        /// In-memory store: id → Package.
        /// std::unordered_map gives O(1) average lookup.
        std::unordered_map<std::string, domain::Package> m_store;

        // --Serialisation helpers--

        /// Serialise one Package to a QJsonObject.
        static QJsonObject  packageToJson(const domain::Package& pkg);

        /// Deserialise one QJsonObject to a Package.
        /// @throws std::runtime_error on malformed data.
        static domain::Package packageFromJson(const QJsonObject& obj);

        // Field-level helpers
        static QJsonObject  addressToJson   (const domain::Address& a);
        static domain::Address addressFromJson(const QJsonObject& o);

        static QJsonObject  logisticsToJson   (const domain::LogisticsInfo& l);
        static domain::LogisticsInfo logisticsFromJson(const QJsonObject& o);

        static QJsonObject  locationToJson   (const domain::StorageLocation& l);
        static domain::StorageLocation locationFromJson(const QJsonObject& o);

        static QJsonObject  metadataToJson   (const domain::PackageMetadata& m);
        static domain::PackageMetadata metadataFromJson(const QJsonObject& o);

        /// Convert PackageStateId ↔ string for JSON storage.
        static QString            stateIdToString(domain::PackageStateId id);
        static domain::PackageStateId stateIdFromString(const QString& s);

        /// "YYYY-MM-DD" ↔ std::chrono::year_month_day
        static QString            dateToString(const domain::Date& d);
        static domain::Date       dateFromString(const QString& s);
    };
}
