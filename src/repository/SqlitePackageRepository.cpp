/**
 * @file   SqlitePackageRepository.cpp
 * @brief  Implementation of SqlitePackageRepository.
 *
 * @author Do Minh Khang
 * @date   2026-07-18
 *
 * @update
 * @author Huynh Phuc Nguyen
 * @date   2026-07-19
 * @changelog
 *   - Replace all local categoryToString / categoryFromString / stateIdToString /
 *     stateIdFromString / dateToString / dateFromString / todayAsString
 *     definitions with calls to the shared helpers in RepositoryHelpers.h.
 *     No behaviour change; this removes ~80 lines of code that were duplicated
 *     verbatim from JsonPackageRepository.cpp.
 */

#include "repository/SqlitePackageRepository.h"
#include "repository/RepositoryHelpers.h"

#include "domain/entities/PackageMetadata.h"
#include "domain/entities/Address.h"
#include "domain/entities/LogisticsInfo.h"
#include "domain/entities/StorageLocation.h"

#include <QMetaType>
#include <QSqlError>
#include <QStringList>
#include <QVariant>

#include <chrono>
#include <stdexcept>

namespace wms::repository
{
    // --Construction--

    SqlitePackageRepository::SqlitePackageRepository(const DatabaseConnection& connection)
        : m_connection{ connection }
    {
    }

    // --IPackageRepository - Read--

    std::vector<domain::Package> SqlitePackageRepository::getAll() const
    {
        return findByCriteria(domain::PackageQueryCriteria{});
    }

    std::optional<domain::Package> SqlitePackageRepository::getById(const std::string& id) const
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare("SELECT * FROM packages WHERE id = :id");
        query.bindValue(":id", QString::fromStdString(id));

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::getById - query failed: " +
                query.lastError().text().toStdString());

        if (!query.next())
            return std::nullopt;

        return packageFromRecord(query);
    }

    // --IPackageRepository - Write--

    void SqlitePackageRepository::add(domain::Package package)
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare(
            "INSERT INTO packages ("
            "  id, state, package_name, category, weight, dim_length, dim_width, dim_height,"
            "  cost, description,"
            "  src_street, src_city, src_country, src_postal,"
            "  dst_street, dst_city, dst_country, dst_postal,"
            "  import_date, expected_export_date, import_vehicle, export_vehicle,"
            "  container_id, zone, aisle, shelf, slot"
            ") VALUES ("
            "  :id, :state, :packageName, :category, :weight, :dimLength, :dimWidth, :dimHeight,"
            "  :cost, :description,"
            "  :srcStreet, :srcCity, :srcCountry, :srcPostal,"
            "  :dstStreet, :dstCity, :dstCountry, :dstPostal,"
            "  :importDate, :expectedExportDate, :importVehicle, :exportVehicle,"
            "  :containerId, :zone, :aisle, :shelf, :slot"
            ")");

        bindPackageFields(query, package);

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::add - insert failed (id may already "
                "exist): " + query.lastError().text().toStdString());
    }

    void SqlitePackageRepository::update(domain::Package package)
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare(
            "UPDATE packages SET"
            "  state = :state, package_name = :packageName, category = :category,"
            "  weight = :weight,"
            "  dim_length = :dimLength, dim_width = :dimWidth, dim_height = :dimHeight,"
            "  cost = :cost, description = :description,"
            "  src_street = :srcStreet, src_city = :srcCity,"
            "  src_country = :srcCountry, src_postal = :srcPostal,"
            "  dst_street = :dstStreet, dst_city = :dstCity,"
            "  dst_country = :dstCountry, dst_postal = :dstPostal,"
            "  import_date = :importDate, expected_export_date = :expectedExportDate,"
            "  import_vehicle = :importVehicle, export_vehicle = :exportVehicle,"
            "  container_id = :containerId, zone = :zone, aisle = :aisle,"
            "  shelf = :shelf, slot = :slot"
            " WHERE id = :id");

        bindPackageFields(query, package);

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::update - update failed: " +
                query.lastError().text().toStdString());

        if (query.numRowsAffected() == 0)
            throw std::runtime_error(
                "SqlitePackageRepository::update - package not found: " + package.id());
    }

    void SqlitePackageRepository::remove(const std::string& id)
    {
        QSqlQuery query{ m_connection.handle() };
        query.prepare("DELETE FROM packages WHERE id = :id");
        query.bindValue(":id", QString::fromStdString(id));

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::remove - delete failed: " +
                query.lastError().text().toStdString());

        if (query.numRowsAffected() == 0)
            throw std::runtime_error(
                "SqlitePackageRepository::remove - package not found: " + id);
    }

    // --IPackageRepository - Persistence--

    void SqlitePackageRepository::save()
    {
        // No-op: add()/update()/remove() already commit directly to SQLite.
        // Kept as an explicit override so WarehouseManager's save() call sites
        // work unmodified when swapped from JsonPackageRepository.
    }

    void SqlitePackageRepository::load()
    {
        // No-op for the same reason as save() - there is no separate
        // in-memory cache to refresh from the database.
    }

    // --Query--

    std::vector<domain::Package> SqlitePackageRepository::findByCriteria(
        const domain::PackageQueryCriteria& criteria) const
    {
        const QString sql = "SELECT * FROM packages" + buildWhereClause(criteria);

        QSqlQuery query{ m_connection.handle() };
        query.prepare(sql);
        bindWhereClause(query, criteria);

        if (!query.exec())
            throw std::runtime_error(
                "SqlitePackageRepository::findByCriteria - query failed: " +
                query.lastError().text().toStdString());

        std::vector<domain::Package> result;
        while (query.next())
            result.push_back(packageFromRecord(query));

        return result;
    }

    // --Query building--

    QString SqlitePackageRepository::buildWhereClause(const domain::PackageQueryCriteria& c)
    {
        QStringList clauses;

        if (c.name.has_value())               clauses << "LOWER(package_name) LIKE :nameKeyword";
        if (c.state.has_value())              clauses << "state = :state";
        if (c.category.has_value())           clauses << "category = :category";
        if (c.minWeight.has_value())          clauses << "weight >= :minWeight";
        if (c.maxWeight.has_value())          clauses << "weight <= :maxWeight";
        if (c.zone.has_value())               clauses << "zone = :zone";
        if (c.containerId.has_value())        clauses << "container_id = :containerId";
        if (c.descriptionKeyword.has_value()) clauses << "LOWER(description) LIKE :descriptionKeyword";
        if (c.overdueOnly)                    clauses << "expected_export_date < :today";
        if (c.importedToday)                  clauses << "import_date = :today";
        if (c.exportDueToday)                 clauses << "expected_export_date = :today";

        if (clauses.isEmpty())
            return QString{};

        return " WHERE " + clauses.join(" AND ");
    }

    // --Query binding--

    void SqlitePackageRepository::bindWhereClause(QSqlQuery& query,
                                                   const domain::PackageQueryCriteria& c)
    {
        if (c.name.has_value())
        {
            const QString keyword = QString::fromStdString(*c.name).toLower();
            query.bindValue(":nameKeyword", "%" + keyword + "%");
        }
        if (c.state.has_value())
            query.bindValue(":state", helpers::stateIdToString(*c.state));
        if (c.category.has_value())
            query.bindValue(":category", helpers::categoryToString(*c.category));
        if (c.minWeight.has_value())
            query.bindValue(":minWeight", *c.minWeight);
        if (c.maxWeight.has_value())
            query.bindValue(":maxWeight", *c.maxWeight);
        if (c.zone.has_value())
            query.bindValue(":zone", QString::fromStdString(*c.zone));
        if (c.containerId.has_value())
            query.bindValue(":containerId", QString::fromStdString(*c.containerId));
        if (c.descriptionKeyword.has_value())
        {
            const QString keyword = QString::fromStdString(*c.descriptionKeyword).toLower();
            query.bindValue(":descriptionKeyword", "%" + keyword + "%");
        }
        if (c.overdueOnly || c.importedToday || c.exportDueToday)
            query.bindValue(":today", helpers::todayAsString());
    }

    // --Row mapping--

    domain::Package SqlitePackageRepository::packageFromRecord(const QSqlQuery& query)
    {
        domain::PackageMetadata metadata{
            query.value("package_name").toString().toStdString(),
            helpers::categoryFromString(query.value("category").toString()),
            query.value("weight").toDouble(),
            domain::Dimension{
                query.value("dim_length").toDouble(),
                query.value("dim_width").toDouble(),
                query.value("dim_height").toDouble()
            },
            query.value("cost").toDouble(),
            query.value("description").toString().toStdString()
        };

        domain::Address source{
            query.value("src_street").toString().toStdString(),
            query.value("src_city").toString().toStdString(),
            query.value("src_country").toString().toStdString(),
            query.value("src_postal").toString().toStdString()
        };

        domain::Address destination{
            query.value("dst_street").toString().toStdString(),
            query.value("dst_city").toString().toStdString(),
            query.value("dst_country").toString().toStdString(),
            query.value("dst_postal").toString().toStdString()
        };

        const QVariant containerId = query.value("container_id");

        domain::LogisticsInfo logistics{
            helpers::dateFromString(query.value("import_date").toString()),
            helpers::dateFromString(query.value("expected_export_date").toString()),
            query.value("import_vehicle").toString().toStdString(),
            query.value("export_vehicle").toString().toStdString(),
            containerId.isNull() ? std::string{} : containerId.toString().toStdString()
        };

        domain::StorageLocation location{
            query.value("zone").toString().toStdString(),
            query.value("aisle").toString().toStdString(),
            query.value("shelf").toInt(),
            query.value("slot").toInt()
        };

        return domain::Package::load(
            query.value("id").toString().toStdString(),
            std::move(metadata),
            std::move(source),
            std::move(destination),
            std::move(logistics),
            std::move(location),
            helpers::stateIdFromString(query.value("state").toString())
        );
    }

    void SqlitePackageRepository::bindPackageFields(QSqlQuery& query, const domain::Package& pkg)
    {
        const auto& meta     = pkg.metadata();
        const auto& src      = pkg.source();
        const auto& dst      = pkg.destination();
        const auto& logistics = pkg.logistics();
        const auto& location  = pkg.location();

        query.bindValue(":id",          QString::fromStdString(pkg.id()));
        query.bindValue(":state",       helpers::stateIdToString(pkg.currentStateId()));
        query.bindValue(":packageName", QString::fromStdString(meta.name));
        query.bindValue(":category",    helpers::categoryToString(meta.category));
        query.bindValue(":weight",      meta.weight);
        query.bindValue(":dimLength",   meta.dimensions.length);
        query.bindValue(":dimWidth",    meta.dimensions.width);
        query.bindValue(":dimHeight",   meta.dimensions.height);
        query.bindValue(":cost",        meta.cost);
        query.bindValue(":description", QString::fromStdString(meta.description));

        query.bindValue(":srcStreet",  QString::fromStdString(src.street));
        query.bindValue(":srcCity",    QString::fromStdString(src.city));
        query.bindValue(":srcCountry", QString::fromStdString(src.country));
        query.bindValue(":srcPostal",  QString::fromStdString(src.postalCode));

        query.bindValue(":dstStreet",  QString::fromStdString(dst.street));
        query.bindValue(":dstCity",    QString::fromStdString(dst.city));
        query.bindValue(":dstCountry", QString::fromStdString(dst.country));
        query.bindValue(":dstPostal",  QString::fromStdString(dst.postalCode));

        query.bindValue(":importDate",         helpers::dateToString(logistics.importDate));
        query.bindValue(":expectedExportDate",  helpers::dateToString(logistics.expectedExportDate));
        query.bindValue(":importVehicle",       QString::fromStdString(logistics.importVehicle));
        query.bindValue(":exportVehicle",       QString::fromStdString(logistics.exportVehicle));

        // An empty containerId means "not assigned to a container" and is
        // stored as SQL NULL rather than an empty string. There is no FOREIGN
        // KEY on this column yet (see schema.sql header note), but storing NULL
        // now keeps the column's meaning correct and requires no migration once
        // the FK constraint is added later.
        if (logistics.containerId.empty())
            query.bindValue(":containerId", QVariant(QMetaType::fromType<QString>()));
        else
            query.bindValue(":containerId", QString::fromStdString(logistics.containerId));

        query.bindValue(":zone",  QString::fromStdString(location.zone));
        query.bindValue(":aisle", QString::fromStdString(location.aisle));
        query.bindValue(":shelf", location.shelf);
        query.bindValue(":slot",  location.slot);
    }
}
