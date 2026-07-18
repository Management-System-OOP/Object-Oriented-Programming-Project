/**
 * @file   SqlitePackageRepository.h
 * @brief  SQLite-backed implementation of IPackageRepository.
 *
 * @author Do Minh Khang
 * @date   2026-07-18
 *
 * This file (along with JsonPackageRepository.h and DatabaseConnection.h) is
 * one of the files outside gui/ that is allowed to use Qt (QSqlQuery,
 * QString). All other layers must remain Qt-free.
 *
 * Table: packages (see resources/db/schema.sql for full DDL). Value objects
 * with no independent identity (Address, LogisticsInfo, StorageLocation,
 * PackageMetadata) are flattened into columns on this single table since
 * they are always owned 1:1 by exactly one Package row.
 */

#pragma once

#include "repository/IPackageRepository.h"
#include "repository/DatabaseConnection.h"

#include <QSqlQuery>
#include <QString>

namespace wms::repository
{
    /**
     * @class SqlitePackageRepository
     * @brief Loads, saves, and queries packages against a SQLite database.
     *
     * Unlike JsonPackageRepository, this implementation keeps no in-memory
     * cache - every call issues a query directly against the database, so
     * save()/load() are no-ops (SQLite already persists on each write).
     * This also means findByCriteria() benefits from the indexes declared
     * in schema.sql instead of scanning every row in C++.
     */
    class SqlitePackageRepository : public IPackageRepository
    {
    public:
        /**
         * @brief  Construct against an already-open database connection.
         * @param  connection  Non-owning reference to a live
         *                     DatabaseConnection. The connection must
         *                     outlive this repository.
         */
        explicit SqlitePackageRepository(const DatabaseConnection& connection);

        // --IPackageRepository--
        std::vector<domain::Package>   getAll()   const override;
        std::optional<domain::Package> getById(const std::string& id) const override;
        void add(domain::Package package) override;
        void update(domain::Package package) override;
        void remove(const std::string& id)   override;
        void save() override;
        void load() override;

        /**
         * @brief  Query packages matching every set field of @p criteria.
         *
         *  Builds a single parameterised SQL statement with one WHERE
         *  clause fragment per set field (unset optional fields are
         *  omitted entirely, not compared against NULL). All bindings use
         *  named placeholders to prevent SQL injection.
         *
         * @param  criteria  Filter fields; unset fields are not constrained.
         * @return All matching packages, order not guaranteed - callers
         *         that need a stable order should sort the result.
         */
        std::vector<domain::Package> findByCriteria(
            const domain::PackageQueryCriteria& criteria) const override;

    private:
        const DatabaseConnection& m_connection;
        
        // --Enum / date <-> string helpers--
        // Intentionally duplicated from JsonPackageRepository's private
        // helpers (same strings, same "YYYY-MM-DD" date format) rather than
        // shared, so each repository implementation stays self-contained.

        /// Category <-> string.
        static QString categoryToString(domain::Category c);
        static domain::Category categoryFromString(const QString& s);

        /// PackageStateId <-> string.
        static QString stateIdToString(domain::PackageStateId id);
        static domain::PackageStateId stateIdFromString(const QString& s);

        /// "YYYY-MM-DD" <-> std::chrono::year_month_day
        static QString dateToString(const domain::Date& d);
        static domain::Date dateFromString(const QString& s);

        /**
         * @brief Get today's date, formatted the same way as dateToString() 
         */
        static QString todayAsString();

        // --Row mapping--

        /**
         * @brief Maps the current row of an executed QSqlQuery to a domain::Package.
         */
        static domain::Package packageFromRecord(const QSqlQuery& query);

        /**
         * @brief Binds every column of @p pkg onto @p query using named placeholders.
         */
        static void bindPackageFields(QSqlQuery& query, const domain::Package& pkg);

        // --Query building--

        /// Builds the "WHERE ..." clause text (empty string if unfiltered).
        /// Bind values are applied separately in findByCriteria() after
        /// query.prepare(), matching Qt's prepared-statement API.
        static QString buildWhereClause(const domain::PackageQueryCriteria& criteria);

        // --Query binding--
        /// Binds every placeholder buildWhereClause() may have emitted for
        /// @p criteria. Must be kept in sync with buildWhereClause() by
        /// hand - the two are intentionally separate to match Qt's
        /// prepare-then-bind API, but nothing enforces they stay
        /// consistent, so update both together when PackageQueryCriteria
        /// gains a new field.
        static void bindWhereClause(QSqlQuery& query, const domain::PackageQueryCriteria& criteria);
    };
    
}
