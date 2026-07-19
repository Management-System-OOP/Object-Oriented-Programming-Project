/**
 * @file   JsonPackageRepository.cpp
 * @brief  JSON persistence implementation for IPackageRepository.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-10
 *
 * @update
 * @author Duong Anh Hao
 * @date   2026-06-16
 * @changelog
 *   - Fixed missing Qt headers (<QStringList>, <QByteArray>, <QJsonParseError>).
 *   - Included concrete state headers to resolve 'undeclared identifier' and
 *     std::make_unique errors.
 *   - Refactored parameters to use std::string to strictly match the
 *     IPackageRepository interface, ensuring the domain layer remains Qt-free.
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-06-24
 * @changelog
 *   - Change packageFromJson() to use load() instead of constructor.
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-07-11
 * @changelog
 *   - Add package name field to metadataToJson() / metadataFromJson().
 *
 * @update
 * @author Do Minh Khang
 * @date   2026-07-16
 * @changelog
 *   - Implement findByCriteria().
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Replace all local categoryToString / categoryFromString / stateIdToString /
 *     stateIdFromString / dateToString / dateFromString definitions with calls to
 *     the shared helpers in RepositoryHelpers.h. No behaviour change; this removes
 *     ~60 lines of code that were duplicated verbatim in SqlitePackageRepository.cpp.
 */

#include "repository/JsonPackageRepository.h"
#include "repository/RepositoryHelpers.h"

#include "domain/states/PackageStateId.h"
#include "domain/states/InStorageState.h"
#include "domain/states/DispatchedState.h"
#include "domain/states/MissingState.h"
#include "domain/states/OverdueState.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QString>
#include <QStringList>
#include <QByteArray>
#include <QJsonParseError>

#include <stdexcept>
#include <chrono>
#include <memory>
#include <algorithm>
#include <cctype>

namespace wms::repository
{
    // --Construction--

    JsonPackageRepository::JsonPackageRepository(QString filePath)
        : m_filePath{ std::move(filePath) }
    {
        // Attempt to load existing data; silently ignore if file doesn't exist yet.
        try { load(); }
        catch (...) {}
    }

    // --IPackageRepository - Read--

    std::vector<domain::Package> JsonPackageRepository::getAll() const
    {
        std::vector<domain::Package> result;
        result.reserve(m_store.size());
        for (const auto& [id, pkg] : m_store)
            result.push_back(pkg);
        return result;
    }

    std::optional<domain::Package> JsonPackageRepository::getById(const std::string& id) const
    {
        auto it = m_store.find(id);
        if (it == m_store.end())
            return std::nullopt;
        return it->second;
    }

    // --IPackageRepository - Write--

    void JsonPackageRepository::add(domain::Package package)
    {
        const std::string id = package.id();
        if (m_store.count(id))
            throw std::runtime_error(
                "JsonPackageRepository::add - package id already exists: " + id);
        m_store.emplace(id, std::move(package));
    }

    void JsonPackageRepository::update(domain::Package package)
    {
        const std::string id = package.id();
        if (!m_store.count(id))
            throw std::runtime_error(
                "JsonPackageRepository::update - package not found: " + id);
        m_store.at(id) = std::move(package);
    }

    void JsonPackageRepository::remove(const std::string& id)
    {
        if (!m_store.erase(id))
            throw std::runtime_error(
                "JsonPackageRepository::remove - package not found: " + id);
    }

    // --IPackageRepository - Persistence--

    void JsonPackageRepository::save()
    {
        QJsonArray array;
        for (const auto& [id, pkg] : m_store)
            array.append(packageToJson(pkg));

        QJsonDocument doc{ array };

        QFile file{ m_filePath };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::save - cannot open file: " +
                m_filePath.toStdString());

        file.write(doc.toJson(QJsonDocument::Indented));
    }

    void JsonPackageRepository::load()
    {
        QFile file{ m_filePath };
        if (!file.exists())
            return; // Fresh start - no error

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::load - cannot open file: " +
                m_filePath.toStdString());

        const QByteArray raw = file.readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(
                "JsonPackageRepository::load - JSON parse error: " +
                parseError.errorString().toStdString());

        if (!doc.isArray())
            throw std::runtime_error(
                "JsonPackageRepository::load - root element must be a JSON array");

        m_store.clear();
        const QJsonArray array = doc.array();
        for (const QJsonValue& val : array)
        {
            domain::Package pkg = packageFromJson(val.toObject());
            const std::string id = pkg.id();
            m_store.emplace(id, std::move(pkg));
        }
    }

    // --Serialisation helpers - Package--

    QJsonObject JsonPackageRepository::packageToJson(const domain::Package& pkg)
    {
        QJsonObject obj;
        obj["id"]          = QString::fromStdString(pkg.id());
        obj["state"]       = helpers::stateIdToString(pkg.currentStateId());
        obj["metadata"]    = metadataToJson(pkg.metadata());
        obj["source"]      = addressToJson(pkg.source());
        obj["destination"] = addressToJson(pkg.destination());
        obj["logistics"]   = logisticsToJson(pkg.logistics());
        obj["location"]    = locationToJson(pkg.location());
        return obj;
    }

    domain::Package JsonPackageRepository::packageFromJson(const QJsonObject& obj)
    {
        auto metadata    = metadataFromJson(obj["metadata"].toObject());
        auto source      = addressFromJson(obj["source"].toObject());
        auto destination = addressFromJson(obj["destination"].toObject());
        auto logistics   = logisticsFromJson(obj["logistics"].toObject());
        auto location    = locationFromJson(obj["location"].toObject());

        const std::string id = obj["id"].toString().toStdString();
        const domain::PackageStateId stateId =
            helpers::stateIdFromString(obj["state"].toString());

        return domain::Package::load(
            id,
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            stateId
        );
    }

    // --Serialisation helpers - Address--

    QJsonObject JsonPackageRepository::addressToJson(const domain::Address& a)
    {
        QJsonObject obj;
        obj["street"]     = QString::fromStdString(a.street);
        obj["city"]       = QString::fromStdString(a.city);
        obj["country"]    = QString::fromStdString(a.country);
        obj["postalCode"] = QString::fromStdString(a.postalCode);
        return obj;
    }

    domain::Address JsonPackageRepository::addressFromJson(const QJsonObject& o)
    {
        return domain::Address{
            o["street"].toString().toStdString(),
            o["city"].toString().toStdString(),
            o["country"].toString().toStdString(),
            o["postalCode"].toString().toStdString()
        };
    }

    // --Serialisation helpers - LogisticsInfo--

    QJsonObject JsonPackageRepository::logisticsToJson(const domain::LogisticsInfo& l)
    {
        QJsonObject obj;
        obj["importDate"]         = helpers::dateToString(l.importDate);
        obj["expectedExportDate"] = helpers::dateToString(l.expectedExportDate);
        obj["importVehicle"]      = QString::fromStdString(l.importVehicle);
        obj["exportVehicle"]      = QString::fromStdString(l.exportVehicle);
        obj["containerId"]        = QString::fromStdString(l.containerId);
        return obj;
    }

    domain::LogisticsInfo JsonPackageRepository::logisticsFromJson(const QJsonObject& o)
    {
        return domain::LogisticsInfo{
            helpers::dateFromString(o["importDate"].toString()),
            helpers::dateFromString(o["expectedExportDate"].toString()),
            o["importVehicle"].toString().toStdString(),
            o["exportVehicle"].toString().toStdString(),
            o["containerId"].toString().toStdString()
        };
    }

    // --Serialisation helpers - StorageLocation--

    QJsonObject JsonPackageRepository::locationToJson(const domain::StorageLocation& l)
    {
        QJsonObject obj;
        obj["zone"]  = QString::fromStdString(l.zone);
        obj["aisle"] = QString::fromStdString(l.aisle);
        obj["shelf"] = l.shelf;
        obj["slot"]  = l.slot;
        return obj;
    }

    domain::StorageLocation JsonPackageRepository::locationFromJson(const QJsonObject& o)
    {
        return domain::StorageLocation{
            o["zone"].toString().toStdString(),
            o["aisle"].toString().toStdString(),
            o["shelf"].toInt(),
            o["slot"].toInt()
        };
    }

    // --Serialisation helpers - PackageMetadata--

    QJsonObject JsonPackageRepository::metadataToJson(const domain::PackageMetadata& m)
    {
        QJsonObject dim;
        dim["length"] = m.dimensions.length;
        dim["width"]  = m.dimensions.width;
        dim["height"] = m.dimensions.height;

        QJsonObject obj;
        obj["name"]        = QString::fromStdString(m.name);
        obj["category"]    = helpers::categoryToString(m.category);
        obj["weight"]      = m.weight;
        obj["cost"]        = m.cost;
        obj["description"] = QString::fromStdString(m.description);
        obj["dimensions"]  = dim;
        return obj;
    }

    domain::PackageMetadata JsonPackageRepository::metadataFromJson(const QJsonObject& o)
    {
        const QJsonObject dim = o["dimensions"].toObject();
        return domain::PackageMetadata{
            o["name"].toString().toStdString(),
            helpers::categoryFromString(o["category"].toString()),
            o["weight"].toDouble(),
            domain::Dimension{
                dim["length"].toDouble(),
                dim["width"].toDouble(),
                dim["height"].toDouble()
            },
            o["cost"].toDouble(),
            o["description"].toString().toStdString()
        };
    }

    // --Query--

    std::vector<domain::Package> JsonPackageRepository::findByCriteria(
        const domain::PackageQueryCriteria& criteria) const
    {
        // Filtering logic is evaluated in-memory here rather than delegating
        // to service::PackageFilter, because repository/ must not depend on
        // service/ per the layer dependency rules.
        std::vector<domain::Package> result;
        result.reserve(m_store.size());

        const auto toLower = [](std::string s)
        {
            std::transform(s.begin(), s.end(), s.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return s;
        };

        std::optional<std::string> lowerName;
        if (criteria.name.has_value())
            lowerName = toLower(*criteria.name);

        std::optional<std::string> lowerKeyword;
        if (criteria.descriptionKeyword.has_value())
            lowerKeyword = toLower(*criteria.descriptionKeyword);

        const auto today = std::chrono::year_month_day{
            std::chrono::floor<std::chrono::days>(std::chrono::system_clock::now())
        };

        for (const auto& [id, pkg] : m_store)
        {
            if (lowerName.has_value() &&
                toLower(pkg.metadata().name).find(*lowerName) == std::string::npos)
                continue;
            if (criteria.state.has_value() && pkg.currentStateId() != *criteria.state)
                continue;
            if (criteria.category.has_value() && pkg.metadata().category != *criteria.category)
                continue;
            if (criteria.minWeight.has_value() && pkg.metadata().weight < *criteria.minWeight)
                continue;
            if (criteria.maxWeight.has_value() && pkg.metadata().weight > *criteria.maxWeight)
                continue;
            if (criteria.zone.has_value() && pkg.location().zone != *criteria.zone)
                continue;
            if (criteria.containerId.has_value() &&
                pkg.logistics().containerId != *criteria.containerId)
                continue;
            if (lowerKeyword.has_value() &&
                toLower(pkg.metadata().description).find(*lowerKeyword) == std::string::npos)
                continue;
            if (criteria.overdueOnly && today <= pkg.logistics().expectedExportDate)
                continue;
            if (criteria.importedToday && pkg.logistics().importDate != today)
                continue;
            if (criteria.exportDueToday && pkg.logistics().expectedExportDate != today)
                continue;

            result.push_back(pkg);
        }

        return result;
    }

}
