# Development Guidelines
## Packages Warehouse Management System

> **Version:** 1.1.2 | **Stack:** C++20 · Qt 6 | **Last updated:** June 2026

---

## Table of Contents

1. [Project Structure](#1-project-structure)
2. [Naming Conventions](#2-naming-conventions)
3. [Code Clarity](#3-code-clarity)
4. [Documentation Practices](#4-documentation-practices)
5. [C++20 Standards](#5-c20-standards)
6. [Qt-Specific Standards](#6-qt-specific-standards)
7. [Design Pattern Implementation](#7-design-pattern-implementation)
8. [Git & Collaboration Rules](#8-git--collaboration-rules)
9. [Testing Standards](#9-testing-standards)
10. [AI Usage Policy](#10-ai-usage-policy)
11. [Quick Reference Cheatsheet](#11-quick-reference-cheatsheet)

---

## 1. Project Structure

The project is divided into four strict layers. Each layer has a single responsibility and may only depend on layers below it.

```
WarehouseMS/
│
├── src/
│   ├── domain/                 # Pure C++ — zero Qt dependency
│   │   ├── entities/           # Package and its value objects
│   │   │   ├── Package.h / .cpp
│   │   │   ├── PackageMetadata.h
│   │   │   ├── Address.h
│   │   │   ├── LogisticsInfo.h
│   │   │   └── StorageLocation.h
│   │   └── states/             # State pattern implementations
│   │       ├── IPackageState.h
│   │       ├── OnRouteState.h / .cpp
│   │       ├── InStorageState.h / .cpp
│   │       ├── DispatchedState.h / .cpp
│   │       ├── MissingState.h / .cpp
│   │       └── OverdueState.h / .cpp
│   │
│   ├── repository/             # Data persistence — depends on domain only
│   │   ├── IPackageRepository.h
│   │   └── JsonPackageRepository.h / .cpp    # The only file outside gui that allowed to use Qt
│   │
│   ├── service/                # Business logic — depends on domain + repository
│   │   ├── WarehouseManager.h / .cpp
│   │   └── PackageFilter.h / .cpp
│   │
│   └── gui/                    # Qt UI — depends on service + Qt
│       ├── MainWindow.h / .cpp
│       ├── MainWindow.ui
│       ├── PackageTableModel.h / .cpp
│       ├── FilterPanel.h / .cpp
│       └── dialogs/
│           ├── AddPackageDialog.h / .cpp
│           └── EditPackageDialog.h / .cpp
│
├── tests/
│   ├── domain/
│   ├── service/
│   └── repository/
│
├── resources/
│   ├── icons/
│   └── data/                   # Default JSON data files
│
├── docs/
│   └── GUIDELINE.md            # This file
│
├── CMakeLists.txt
└── README.md
```

### Layer Dependency Rules

| Layer | May depend on | Must NOT depend on |
|---|---|---|
| `domain/` | Standard library, C++20 only | Qt, repository, service, gui |
| `repository/` | `domain/`, Qt (QJsonDocument, QString) | `service/`, `gui/` |
| `service/` | `domain/`, `repository/` | `gui/` |
| `gui/` | All layers, Qt | — |

> **Rule:** If you find yourself `#include`-ing a `gui/` header inside `service/` or `domain/`, stop. Something is architecturally wrong.

---

## 2. Naming Conventions

Consistent naming is the first thing a teammate reads. Follow these rules with no exceptions.

### 2.1 Files

| What | Convention | Example |
|---|---|---|
| Header files | `PascalCase.h` | `PackageFilter.h` |
| Source files | `PascalCase.cpp` | `PackageFilter.cpp` |
| UI form files | `PascalCase.ui` | `MainWindow.ui` |
| Test files | `Test_PascalCase.cpp` | `Test_PackageFilter.cpp` |
| One class per file | Always | No exceptions |

### 2.2 Classes and Types

| What | Convention | Example |
|---|---|---|
| Classes | `PascalCase` | `WarehouseManager` |
| Abstract interfaces | Prefix `I` + `PascalCase` | `IPackageRepository` |
| Structs (value objects) | `PascalCase` | `StorageLocation` |
| Enumerations | `PascalCase` | `PackageStateId` |
| Enum values | `PascalCase` | `PackageStateId::InStorage` |
| Type aliases / using | `PascalCase` | `using Predicate = std::function<bool(const Package&)>;` |
| Template parameters | Single uppercase or `PascalCase` | `T`, `StateType` |

### 2.3 Functions and Methods

| What | Convention | Example |
|---|---|---|
| Regular methods | `camelCase` | `findById()` |
| Qt slots | `on_` + object + `_` + signal OR plain `camelCase` | `on_addButton_clicked()` |
| Qt signals | `camelCase` describing the event | `filterChanged()`, `packageAdded()` |
| Getters | Plain noun, no `get` prefix | `id()`, `state()`, `metadata()` |
| Setters | `set` + `PascalCase` | `setMetadata()`, `setState()` |
| Boolean getters | `is` / `has` / `can` prefix | `isOverdue()`, `hasLocation()` |
| Static factory methods | `create` or `make` prefix | `Package::create()` |

```cpp
// CORRECT
QString id() const;
void setMetadata(PackageMetadata meta);
bool isOverdue() const;
static Package create(PackageMetadata meta, Address src, Address dst);

// WRONG
QString getId() const;       // no 'get' prefix for getters
void UpdateMetadata(...);    // wrong case
```

### 2.4 Variables and Parameters

| What | Convention | Example |
|---|---|---|
| Local variables | `camelCase` | `packageCount`, `filterResult` |
| Function parameters | `camelCase` | `containerId`, `fromDate` |
| Private member variables | `m_camelCase` | `m_state`, `m_packages` |
| Static member variables | `s_camelCase` | `s_instance` |
| Constants (`constexpr`) | `SCREAMING_SNAKE_CASE` | `MAX_PACKAGES_PER_ZONE` |
| Qt UI member pointers | `ui->` via generated code only | `ui->tableView` |

```cpp
class WarehouseManager {
private:
    std::shared_ptr<IPackageRepository> m_repo;
    PackageFilter m_filter;
    static constexpr int MAX_QUERY_RESULTS = 1000;
};
```

### 2.5 Namespaces

All project code lives under the `wms` namespace. Sub-namespaces follow the layer name.

```cpp
namespace wms::domain { ... }
namespace wms::service { ... }
namespace wms::repository { ... }
// gui code uses Qt conventions and may omit namespace
```

---

## 3. Code Clarity

The goal here is not rigid formatting rules — it's making sure any teammate can read your code easily. Focus on clarity first.

### 3.1 Include Order

Includes must appear in this exact order, separated by blank lines:

```cpp
// 1. Matching header (for .cpp files only)
#include "PackageFilter.h"

// 2. Other project headers (alphabetical within group)
#include "domain/entities/Package.h"
#include "domain/states/IPackageState.h"

// 3. Qt headers
#include <QDate>
#include <QString>

// 4. Standard library headers
#include <functional>
#include <optional>
#include <vector>
```

### 3.2 Line Length and Wrapping

- Length limit: Should **not** exceed **100 characters** per line.
- Wrap long function signatures at each parameter:

```cpp
// CORRECT
std::vector<Package> queryPackages(
    const std::vector<PackageFilter::Predicate>& predicates,
    int maxResults = MAX_QUERY_RESULTS) const;

// WRONG (exceeds 100 chars or unreadable)
std::vector<Package> queryPackages(const std::vector<PackageFilter::Predicate>& predicates, int maxResults = MAX_QUERY_RESULTS) const;
```

### 3.3 Whitespace Rules

- **2 blank lines** between top-level class definitions in a file.
- **1 blank line** between method implementations.
- **No trailing whitespace.**
- **No blank lines** at the start or end of a function body.
- Spaces around binary operators: `a + b`, `x == y`.
- No space before `(` in function calls: `doSomething()`, not `doSomething ()`.

### 3.4 Header Guard vs `#pragma once`

Use `#pragma once` in all headers. It is supported by all modern compilers we target.

```cpp
#pragma once

namespace wms::domain {
// ...
}
```

---

## 4. Documentation Practices

### 4.1 File Header

Every `.h` and `.cpp` file starts with a standard comment block:

```cpp
/**
 * @file   PackageFilter.h
 * @brief  Composable predicate-based filter for Package queries.
 *
 * @author [Your Name]
 * @date   YYYY-MM-DD
 */
```

### 4.2 Class Documentation (Doxygen)

Document every class, especially abstractions and public API classes.

```cpp
/**
 * @class  IPackageRepository
 * @brief  Abstract repository interface for Package persistence.
 *
 * Implementations must guarantee that all operations are atomic
 * and that the underlying storage is updated before returning.
 * Callers must not assume any specific ordering of results from findAll().
 *
 * @see JsonPackageRepository
 */
class IPackageRepository
{
public:
    virtual ~IPackageRepository() = default;

    /**
     * @brief  Persists a new package to the repository.
     * @param  pkg  The package to add. Must have a unique, non-empty ID.
     * @throws std::invalid_argument if pkg.id() is empty or already exists.
     */
    virtual void add(Package pkg) = 0;

    /**
     * @brief  Retrieves a package by its unique identifier.
     * @param  id  The UUID string of the target package.
     * @return An engaged optional if found, std::nullopt otherwise.
     */
    virtual std::optional<Package> findById(const QString& id) const = 0;
};
```

### 4.3 Inline Comments

- Write comments that explain **why**, not **what**. The code shows what; the comment explains the reasoning.
- Do not comment obvious code.
- Use `// TODO(name):` and `// FIXME(name):` tags for deferred work.

```cpp
// CORRECT — explains non-obvious reasoning
// We copy the package before moving it into the repo because the GUI
// may still hold a reference to the original via QModelIndex.
Package copy = pkg;
m_repo->add(std::move(pkg));
notifyViews(copy);

// WRONG — restates the code
// Add the package to the repo
m_repo->add(std::move(pkg));

// TODO(an): Replace linear scan with QHash lookup when count > 500
for (const auto& p : m_packages) { ... }
```

### 4.4 README.md

The project root `README.md` must always contain:

1. Project name and one-sentence description.
2. Build prerequisites (CMake version, Qt version, compiler).
3. Build and run instructions (step by step).
4. Group member list and roles.
5. A link to this guideline file.

---

## 5. C++20 Standards

### 5.1 Smart Pointers — Never Raw Owning Pointers

| Ownership Scenario | Use |
|---|---|
| Single exclusive owner | `std::unique_ptr<T>` |
| Shared ownership | `std::shared_ptr<T>` |
| Non-owning observer | Raw `T*` or `const T*` |
| Optional non-owning ref | `std::optional<std::reference_wrapper<T>>` |

```cpp
// Package owns its state exclusively
std::unique_ptr<IPackageState> m_state;

// WarehouseManager shares the repo (multiple objects may reference it)
std::shared_ptr<IPackageRepository> m_repo;

// Non-owning reference passed to a function — raw pointer is fine
void inspect(const Package* pkg);
```

**Never use `new` or `delete` directly.** Use `std::make_unique` and `std::make_shared`.

```cpp
// CORRECT
auto state = std::make_unique<InStorageState>();
package.transitionTo(std::move(state));

// WRONG
IPackageState* state = new InStorageState();
```

### 5.2 Use `std::optional` for Nullable Results

```cpp
// CORRECT
std::optional<Package> findById(const QString& id) const;

// Usage
if (auto pkg = repo.findById(id); pkg.has_value())
{
    processPackage(*pkg);
}

// WRONG — forces callers to check for nullptr or sentinel values
Package* findById(const QString& id) const;
```

### 5.3 Use Concepts for Template Constraints

```cpp
// Define the concept once in a shared header
template<typename F>
concept PackagePredicate =
    std::invocable<F, const Package&> &&
    std::same_as<std::invoke_result_t<F, const Package&>, bool>;

// Use it to constrain templates clearly
template<PackagePredicate... Filters>
std::vector<Package> queryPackages(Filters&&... filters) const;
```

### 5.4 Prefer `auto` Wisely

Use `auto` when the type is obvious from context. Do not use it when it hides an important type.

```cpp
// GOOD — type is obvious
auto manager = std::make_unique<WarehouseManager>(repo);
for (const auto& pkg : m_packages) { ... }

// BAD — type is not obvious; hides that this is a Qt model index
auto x = model.index(row, col);   // prefer: QModelIndex idx = ...
```

### 5.5 Structured Bindings and Range-Based Loops

```cpp
// Prefer structured bindings over .first / .second
for (const auto& [id, package] : m_packageMap)
{
    // ...
}

// Prefer range-based for over index loops when index isn't needed
for (const auto& pkg : packages)
{
    filter(pkg);
}
```

### 5.6 `enum class` for All Enumerations

Never use plain `enum`. Always use `enum class` to prevent accidental implicit conversions.

```cpp
// CORRECT
enum class PackageStateId
{
    OnRoute,
    InStorage,
    Dispatched,
    Missing,
    Overdue
};

// WRONG
enum PackageState { ON_ROUTE, IN_STORAGE };
```

---

## 6. Qt-Specific Standards
> **New to Qt?** Read the [Qt Framework Essentials guide](QTINSTRUCTION.md) first before working on anything in `src/gui/`.

### 6.1 QString vs std::string

- Use `QString` everywhere Qt APIs are involved (display, serialization, file I/O).
- Use `std::string` inside pure domain objects that must not depend on Qt.
- Convert at the boundary: `QString::fromStdString()` / `.toStdString()`.

### 6.2 Signals and Slots

- Always use the **new connect syntax** (function pointer, not SIGNAL/SLOT macros).
- Emit signals only at the end of a state change — after all internal state is consistent.

```cpp
// CORRECT — new syntax, type-checked at compile time
connect(m_filterPanel, &FilterPanel::filterChanged,
        this,          &MainWindow::onFilterChanged);

// WRONG — old macro syntax, no compile-time checking
connect(m_filterPanel, SIGNAL(filterChanged(FilterCriteria)),
        this,          SLOT(onFilterChanged(FilterCriteria)));
```

- Signals are declared in the `signals:` section and have **no implementation**.
- Slots are regular methods declared under `public slots:` or `private slots:`.

```cpp
class FilterPanel : public QWidget
{
    Q_OBJECT

signals:
    void filterChanged(const FilterCriteria& criteria);

private slots:
    void on_applyButton_clicked();
};
```

### 6.3 QObject Ownership and Parenting

- Always pass a `QObject* parent` to Qt widget constructors to enable automatic memory management.
- Do **not** use `std::unique_ptr` for `QObject` subclasses that have a Qt parent — the parent's destructor will call `delete` on its children, causing a double-free.

```cpp
// CORRECT — Qt owns FilterPanel via parent chain
auto* filterPanel = new FilterPanel(this);  // 'this' is parent

// WRONG — double-free if parent also deletes it
auto filterPanel = std::make_unique<FilterPanel>(this);
```

### 6.4 Model/View Architecture

- **Never** put business data directly in a `QWidget`. All data lives in the `WarehouseManager`, mirrored into a `QAbstractTableModel` subclass.
- The model's `refresh()` method is the only place that updates the model's internal cache from the manager.
- Call `beginResetModel()` / `endResetModel()` for full data reloads; use `beginInsertRows()` / `endInsertRows()` for surgical updates.

```cpp
void PackageTableModel::refresh(std::vector<Package> newData)
{
    beginResetModel();
    m_packages = std::move(newData);
    endResetModel();
}
```

### 6.5 JSON Persistence

Use `QJsonDocument` for all file I/O. Each `Package` must implement `toJson()` and a static `fromJson()`:

```cpp
QJsonObject Package::toJson() const;
static Package Package::fromJson(const QJsonObject& obj);
```

---

## 7. Design Pattern Implementation

### 7.1 State Pattern

**Rule:** Package state transitions must only happen through `Package::transitionTo()`. No code outside the `Package` class may replace its state directly.

```cpp
// Inside Package — the only valid way to change state
void Package::transitionTo(std::unique_ptr<IPackageState> newState)
{
    m_state = std::move(newState);
}

// Calling code
pkg.transitionTo(std::make_unique<InStorageState>());
```

- Each concrete state must override **all three** pure virtual methods: `handle()`, `getStateLabel()`, `stateId()`.
- States must be **stateless** (carry no mutable data). If context is needed, pass `Package&` through `handle()`.

### 7.2 Observer Pattern (Qt Signals/Slots)

The data flow is strictly one-way: **Service → Model → View**.

```
User Action (GUI)
    │
    ▼
MainWindow (slot)
    │ calls
    ▼
WarehouseManager
    │ returns updated data
    ▼
PackageTableModel::refresh()
    │ emits dataChanged
    ▼
QTableView (auto-updates)
```

Never let the view call `refresh()` on itself in response to its own user actions without going through the manager first.

### 7.3 Repository Pattern

- `IPackageRepository` must be injected into `WarehouseManager` via the constructor (dependency injection).
- No code in `service/` or `gui/` may instantiate a concrete repository directly.

```cpp
// CORRECT — dependency injected, testable
WarehouseManager manager(std::make_shared<JsonPackageRepository>("data.json"));

// WRONG — hard-coded dependency, untestable
WarehouseManager::WarehouseManager() {
    m_repo = std::make_shared<JsonPackageRepository>("data.json");
}
```

---

## 8. Git & Collaboration Rules

### 8.1 Branch Strategy

```
main          ← production-ready, protected. Direct push forbidden.
develop       ← integration branch. All features merge here first.
feature/xxx   ← individual feature branches, branched from develop.
fix/xxx       ← bug fix branches, branched from develop.
```

**Branch naming:** use kebab-case, be descriptive.

```
feature/state-pattern-implementation
feature/package-filter-ui
fix/overdue-detection-timer
```

### 8.2 Commit Message Format

Follow the **Conventional Commits** specification. Every commit message must follow:

```
<type>: <short description in imperative mood>

[optional body — explain WHY, not WHAT]

[optional footer: Closes #issue]
```

**Types:** `feat`, `fix`, `refactor`, `docs`, `test`, `style`, `chore`

```
# CORRECT
feat: add OverdueState with auto-detection timer
fix: prevent duplicate package ID on add
refactor: extract PackageFilter into its own class
docs: add Qt-specific naming conventions
test: add unit tests for Package state transitions

# WRONG
fixed stuff
update code
WIP
```

**Rules:**
- Subject line: max **72 characters**.
- Subject line: present tense, imperative mood ("add", not "added" or "adding").
- Never commit directly to `main` or `develop`.
- Never commit commented-out code.
- Never commit debug print statements (`qDebug()`, `std::cout`).

### 8.3 Code Review Checklist

Reviewers must verify:

- [ ] Follows naming conventions from Section 2.
- [ ] No raw `new` / `delete`.
- [ ] No Qt headers inside `domain/`.
- [ ] New public methods are Doxygen-documented.
- [ ] New features have corresponding tests.
- [ ] No `qDebug()` or `std::cout` left in code.
- [ ] Signals/slots use new connect syntax.
- [ ] Commit history is clean and meaningful.

### 8.5 Conflict Resolution

- If a merge conflict occurs, the person who **created the PR resolves it** (not the reviewer).
- Resolve conflicts locally: pull latest, rebase, resolve, force-push the feature branch.
- When in doubt about a conflict, call a team meeting — do not guess.

### 8.6 Task Assignment

- Tasks are assigned and tracked in our shared Google Sheet
- Before starting any work, create or assign a task card to yourself.
- A task card is **In Progress** when you create the feature branch.
- A task card moves to **Done** when the task is finished.
- If something is blocked or taking longer than expected, note it in the sheet.


---

## 9. Testing Standards

### 9.1 What Must Be Tested

| Component | Test requirement |
|---|---|
| `Package` state transitions | Unit test every valid transition |
| `PackageFilter` predicates | Unit test each filter type and combinations |
| `WarehouseManager` operations | Unit test add, remove, update, query |
| `IPackageRepository` | Integration test with a temp JSON file |
| GUI models (`PackageTableModel`) | Test `rowCount`, `data`, and `refresh` |

### 9.2 Test File Conventions

- Test files live in `tests/` mirroring the `src/` structure.
- Each test file tests exactly one class: `Test_PackageFilter.cpp` tests `PackageFilter`.
- Use Qt Test (`QTest`) for all test cases for consistency with the Qt build system.

```cpp
// Test_PackageFilter.cpp
#include <QTest>
#include "service/PackageFilter.h"

class Test_PackageFilter : public QObject
{
    Q_OBJECT

private slots:
    void byVehicle_returnsOnlyMatchingPackages();
    void byDateRange_excludesPackagesOutsideRange();
    void composedFilters_applyAllPredicates();
};
```

### 9.3 Test Naming

Test method names follow the pattern: `methodUnderTest_scenarioDescription`.

```cpp
void transitionTo_fromOnRoute_toInStorage_succeeds();
void findById_withNonExistentId_returnsNullopt();
void queryPackages_withOverdueFilter_returnsOnlyOverduePackages();
```

---

## 10. AI Usage Policy

This project is developed in an academic context. AI tools are permitted and encouraged, but with strict accountability requirements.

### 10.1 Permitted AI Uses

- Generating boilerplate or scaffolding code (then reviewed and modified).
- Suggesting class designs or method signatures (then evaluated critically).
- Explaining compiler errors or debugging.
- Writing or improving Doxygen comments.
- Drafting the technical report or presentation slides.

### 10.2 Mandatory Requirements

- Every student must maintain a personal **AI Usage Log** in the format required by the course.
- AI-generated code must be **reviewed, understood, and modified** before committing. Submitting verbatim AI output without modification is a violation.
- When using AI for class design or architecture, **document the original AI suggestion and your modifications** in your log.
- Every team member must be able to **explain any code they commit**, regardless of how it was generated.

### 10.3 AI Log Format

Each entry in your personal log should capture:

| Field | Description |
|---|---|
| Date | When AI was used |
| Tool | ChatGPT, Claude, Copilot, etc. |
| Task | What you asked the AI to help with |
| Prompt summary | What you asked |
| AI output summary | What the AI produced |
| Your modification | What you changed, verified, or rejected |

---

## 11. Quick Reference Cheatsheet

```
NAMING QUICK REFERENCE
──────────────────────────────────────────────────────
Files:            PascalCase.h / PascalCase.cpp
Classes:          PascalCase
Interfaces:       IPrefix + PascalCase      → IPackageRepository
Enums:            PascalCase + enum class   → PackageStateId
Enum values:      PascalCase               → PackageStateId::InStorage
Private members:  m_camelCase             → m_packages
Constants:        SCREAMING_SNAKE_CASE    → MAX_QUERY_RESULTS
Methods:          camelCase               → findById()
Getters:          noun, no 'get'          → state(), id()
Setters:          set + PascalCase        → setState()
Bool getters:     is/has/can prefix       → isOverdue()

COMMIT TYPE QUICK REFERENCE
──────────────────────────────────────────────────────
feat      New feature
fix       Bug fix
refactor  Code restructure, no behavior change
docs      Documentation only
test      Adding or fixing tests
style     Formatting, whitespace (no logic change)
chore     Build scripts, dependencies, project setup

POINTER OWNERSHIP QUICK REFERENCE
──────────────────────────────────────────────────────
Exclusive owner:   std::unique_ptr<T>    → Package's state
Shared owner:      std::shared_ptr<T>    → WarehouseManager's repo
Non-owner ref:     const T*              → function parameters
Nullable result:   std::optional<T>      → findById()
Qt widgets:        raw T* with Qt parent → new FilterPanel(this)
```

---

*This document is owned by the team and should be updated collaboratively.*
