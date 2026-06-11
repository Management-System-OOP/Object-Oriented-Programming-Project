/**
 * @file   JsonPackageRepository.cpp
 * @brief  JSON persistence implementation for IPackageRepository.
 *
 * @author Huynh Phuc Nguyen
 * @date   2026-06-10
 */

#include "repository/JsonPackageRepository.h"

#include "domain/states/PackageStateId.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>

#include <stdexcept>
#include <chrono>

namespace wms::repository
{
    // Construction

    JsonPackageRepository::JsonPackageRepository(QString filePath)
        : m_filePath{ std::move(filePath) }
    {
        // Attempt to load existing data; silently ignore if file doesn't exist yet.
        try { load(); } catch (...) {}
    }

    // IPackageRepository — Read

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

    // IPackageRepository — Write

    void JsonPackageRepository::add(domain::Package package)
    {
        const std::string id = package.id();
        if (m_store.count(id))
            throw std::runtime_error("JsonPackageRepository::add — package id already exists: " + id);
        m_store.emplace(id, std::move(package));
    }

    void JsonPackageRepository::update(domain::Package package)
    {
        const std::string id = package.id();
        if (!m_store.count(id))
            throw std::runtime_error("JsonPackageRepository::update — package not found: " + id);
        m_store.at(id) = std::move(package);
    }

    void JsonPackageRepository::remove(const std::string& id)
    {
        if (!m_store.erase(id))
            throw std::runtime_error("JsonPackageRepository::remove — package not found: " + id);
    }

    // IPackageRepository — Persistence

    void JsonPackageRepository::save()
    {
        QJsonArray array;
        for (const auto& [id, pkg] : m_store)
            array.append(packageToJson(pkg));

        QJsonDocument doc{ array };

        QFile file{ m_filePath };
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::save — cannot open file: "
                + m_filePath.toStdString());

        file.write(doc.toJson(QJsonDocument::Indented));
    }

    void JsonPackageRepository::load()
    {
        QFile file{ m_filePath };
        if (!file.exists())
            return; // Fresh start — no error

        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            throw std::runtime_error(
                "JsonPackageRepository::load — cannot open file: "
                + m_filePath.toStdString());

        const QByteArray raw = file.readAll();
        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(raw, &parseError);

        if (parseError.error != QJsonParseError::NoError)
            throw std::runtime_error(
                "JsonPackageRepository::load — JSON parse error: "
                + parseError.errorString().toStdString());

        if (!doc.isArray())
            throw std::runtime_error(
                "JsonPackageRepository::load — root element must be a JSON array");

        m_store.clear();
        const QJsonArray array = doc.array();
        for (const QJsonValue& val : array)
        {
            domain::Package pkg = packageFromJson(val.toObject());
            const std::string id = pkg.id();
            m_store.emplace(id, std::move(pkg));
        }
    }

    // Serialisation helpers — Package

    QJsonObject JsonPackageRepository::packageToJson(const domain::Package& pkg)
    {
        QJsonObject obj;
        obj["id"]          = QString::fromStdString(pkg.id());
        obj["state"]       = stateIdToString(pkg.currentStateId());
        obj["metadata"]    = metadataToJson(pkg.metadata());
        obj["source"]      = addressToJson(pkg.source());
        obj["destination"] = addressToJson(pkg.destination());
        obj["logistics"]   = logisticsToJson(pkg.logistics());
        obj["location"]    = locationToJson(pkg.location());
        return obj;
    }

    domain::Package JsonPackageRepository::packageFromJson(const QJsonObject& obj)
    {
        // Deserialise all value objects first.
        auto metadata    = metadataFromJson (obj["metadata"]   .toObject());
        auto source      = addressFromJson  (obj["source"]     .toObject());
        auto destination = addressFromJson  (obj["destination"].toObject());
        auto logistics   = logisticsFromJson(obj["logistics"]  .toObject());
        auto location    = locationFromJson (obj["location"]   .toObject());

        // Construct the package (constructor assigns a new UUID — we must
        // overwrite it with the stored id afterwards using a move-assign trick).
        // Because Package::m_id is private with no setter, we restore it via
        // the copy-assign from a temporary that carries the right id.
        // *** If you add a Package(id, ...) ctor later you can simplify this. ***
        domain::Package pkg{
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location)
        };

        // Restore the saved state (constructor always starts in OnRoute).
        const auto savedStateId = stateIdFromString(obj["state"].toString());
        // Only transition if different from default OnRoute.
        if (savedStateId != domain::PackageStateId::OnRoute)
        {
            // We need makeStateFromId — it's private in Package, so we call
            // transitionTo with the right concrete type via the public API.
            // Use a switch here mirroring Package::makeStateFromId.
            using domain::PackageStateId;
            switch (savedStateId)
            {
            case PackageStateId::InStorage:
                pkg.transitionTo(std::make_unique<domain::InStorageState>());  break;
            case PackageStateId::Dispatched:
                pkg.transitionTo(std::make_unique<domain::DispatchedState>()); break;
            case PackageStateId::Missing:
                pkg.transitionTo(std::make_unique<domain::MissingState>());    break;
            case PackageStateId::Overdue:
                pkg.transitionTo(std::make_unique<domain::OverdueState>());    break;
            default: break;
            }
        }

        return pkg;
    }

    // Serialisation helpers — Address

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
            o["street"]    .toString().toStdString(),
            o["city"]      .toString().toStdString(),
            o["country"]   .toString().toStdString(),
            o["postalCode"].toString().toStdString()
        };
    }

    // Serialisation helpers — LogisticsInfo

    QJsonObject JsonPackageRepository::logisticsToJson(const domain::LogisticsInfo& l)
    {
        QJsonObject obj;
        obj["importDate"]         = dateToString(l.importDate);
        obj["expectedExportDate"] = dateToString(l.expectedExportDate);
        obj["importVehicle"]      = QString::fromStdString(l.importVehicle);
        obj["exportVehicle"]      = QString::fromStdString(l.exportVehicle);
        obj["containerId"]        = QString::fromStdString(l.containerId);
        return obj;
    }

    domain::LogisticsInfo JsonPackageRepository::logisticsFromJson(const QJsonObject& o)
    {
        return domain::LogisticsInfo{
            dateFromString(o["importDate"]        .toString()),
            dateFromString(o["expectedExportDate"].toString()),
            o["importVehicle"].toString().toStdString(),
            o["exportVehicle"].toString().toStdString(),
            o["containerId"]  .toString().toStdString()
        };
    }

    // Serialisation helpers — StorageLocation

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
            o["zone"] .toString().toStdString(),
            o["aisle"].toString().toStdString(),
            o["shelf"].toInt(),
            o["slot"] .toInt()
        };
    }

    // Serialisation helpers — PackageMetadata

    QJsonObject JsonPackageRepository::metadataToJson(const domain::PackageMetadata& m)
    {
        // Category → string
        static const auto categoryStr = [](domain::Category c) -> QString {
            switch (c) {
            case domain::Category::Standard:   return "Standard";
            case domain::Category::Fragile:    return "Fragile";
            case domain::Category::Perishable: return "Perishable";
            case domain::Category::Hazmat:     return "Hazmat";
            case domain::Category::Oversized:  return "Oversized";
            case domain::Category::Liquid:     return "Liquid";
            }
            return "Standard";
        };

        QJsonObject dim;
        dim["length"] = m.dimensions.length;
        dim["width"]  = m.dimensions.width;
        dim["height"] = m.dimensions.height;

        QJsonObject obj;
        obj["category"]    = categoryStr(m.category);
        obj["weight"]      = m.weight;
        obj["cost"]        = m.cost;
        obj["description"] = QString::fromStdString(m.description);
        obj["dimensions"]  = dim;
        return obj;
    }

    domain::PackageMetadata JsonPackageRepository::metadataFromJson(const QJsonObject& o)
    {
        static const auto categoryFromStr = [](const QString& s) -> domain::Category {
            if (s == "Fragile")    return domain::Category::Fragile;
            if (s == "Perishable") return domain::Category::Perishable;
            if (s == "Hazmat")     return domain::Category::Hazmat;
            if (s == "Oversized")  return domain::Category::Oversized;
            if (s == "Liquid")     return domain::Category::Liquid;
            return domain::Category::Standard;
        };

        const QJsonObject dim = o["dimensions"].toObject();
        return domain::PackageMetadata{
            categoryFromStr(o["category"].toString()),
            o["weight"].toDouble(),
            domain::Dimension{
                dim["length"].toDouble(),
                dim["width"] .toDouble(),
                dim["height"].toDouble()
            },
            o["cost"].toDouble(),
            o["description"].toString().toStdString()
        };
    }

    // Serialisation helpers — State / Date

    QString JsonPackageRepository::stateIdToString(domain::PackageStateId id)
    {
        switch (id) {
        case domain::PackageStateId::OnRoute:    return "OnRoute";
        case domain::PackageStateId::InStorage:  return "InStorage";
        case domain::PackageStateId::Dispatched: return "Dispatched";
        case domain::PackageStateId::Missing:    return "Missing";
        case domain::PackageStateId::Overdue:    return "Overdue";
        }
        return "OnRoute";
    }

    domain::PackageStateId JsonPackageRepository::stateIdFromString(const QString& s)
    {
        if (s == "InStorage")  return domain::PackageStateId::InStorage;
        if (s == "Dispatched") return domain::PackageStateId::Dispatched;
        if (s == "Missing")    return domain::PackageStateId::Missing;
        if (s == "Overdue")    return domain::PackageStateId::Overdue;
        return domain::PackageStateId::OnRoute;
    }

    QString JsonPackageRepository::dateToString(const domain::Date& d)
    {
        // Format: "YYYY-MM-DD"
        return QString("%1-%2-%3")
            .arg(static_cast<int>(d.year()),  4, 10, QChar('0'))
            .arg(static_cast<unsigned>(d.month()), 2, 10, QChar('0'))
            .arg(static_cast<unsigned>(d.day()),   2, 10, QChar('0'));
    }

    domain::Date JsonPackageRepository::dateFromString(const QString& s)
    {
        // Expected: "YYYY-MM-DD"
        const QStringList parts = s.split('-');
        if (parts.size() != 3)
            throw std::runtime_error(
                "JsonPackageRepository::dateFromString — invalid date: "
                + s.toStdString());

        return std::chrono::year_month_day{
            std::chrono::year  { parts[0].toInt() },
            std::chrono::month { static_cast<unsigned>(parts[1].toUInt()) },
            std::chrono::day   { static_cast<unsigned>(parts[2].toUInt()) }
        };
    }

} // namespace wms::repository
