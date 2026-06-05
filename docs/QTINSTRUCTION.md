# Qt Framework Essentials
## Packages Warehouse Management System

> A practical primer for teammates new to Qt. Read this before touching any code in `src/gui/`.

---

## Table of Contents

1. [What Qt Is (and Isn't)](#1-what-qt-is-and-isnt)
2. [The QObject System](#2-the-qobject-system)
3. [Signals & Slots — Qt's Observer Pattern](#3-signals--slots--qts-observer-pattern)
4. [Widgets — Building the UI](#4-widgets--building-the-ui)
5. [Model/View — Displaying Data](#5-modelview--displaying-data)
6. [Qt Data Types We Use](#6-qt-data-types-we-use)
7. [JSON with Qt](#7-json-with-qt)
8. [Memory Management in Qt](#8-memory-management-in-qt)
9. [How Our Project Uses Qt](#9-how-our-project-uses-qt)
10. [Common Mistakes to Avoid](#10-common-mistakes-to-avoid)
11. [Where to Learn More](#11-where-to-learn-more)

---

## 1. What Qt Is (and Isn't)

**Qt is a C++ application framework**, not just a UI library. For our project, we use it for three things only:

| We use Qt for | We do NOT use Qt for |
|---|---|
| Building the GUI (windows, buttons, tables) | Domain logic (`Package`, states, filters) |
| Signals & Slots between UI components | Data structures — we use `std::vector`, not `QList` |
| Reading/writing JSON files | Business rules — those live in `WarehouseManager` |

> **Golden rule:** Qt stays in `src/gui/` and `src/repository/`. The rest of the project is plain C++. If you're writing `#include <QWidget>` inside `src/domain/`, stop.

---

## 2. The QObject System

Almost every Qt class you'll touch inherits from `QObject`. This base class is what gives Qt its superpowers: signals, slots, and automatic memory management.

### The three things every QObject subclass needs

```cpp
#include <QWidget>

class FilterPanel : public QWidget   // ← inherits a Qt class
{
    Q_OBJECT                         // ← REQUIRED macro — never omit this

public:
    explicit FilterPanel(QWidget* parent = nullptr);  // ← always take a parent
};
```

- **Inherits a Qt class** — your class gets Qt features by inheriting `QObject` (or `QWidget`, `QMainWindow`, etc., which all inherit `QObject` themselves).
- **`Q_OBJECT` macro** — paste it at the very top of the `private:` section (or before any access modifier). Qt's build tool (`moc`) reads it to generate the signals/slots glue code. Without it, nothing will compile.
- **`parent` parameter** — explained in [Section 8](#8-memory-management-in-qt).

### You don't need to understand `moc` deeply

Just know: when you add `Q_OBJECT` and declare signals/slots, Qt's build system automatically generates the wiring code for you. CMake with `find_package(Qt6)` handles this automatically.

---

## 3. Signals & Slots — Qt's Observer Pattern

This is the most important concept in Qt. It lets two objects communicate without knowing anything about each other — the same idea as the Observer pattern, but built into the language toolchain.

### The mental model

Think of it like a radio broadcast:
- A **signal** is the broadcaster — it announces that something happened.
- A **slot** is a receiver tuned to that broadcast — it reacts when the signal fires.
- A **connection** is the act of tuning in.

### Declaring signals and slots

```cpp
class FilterPanel : public QWidget
{
    Q_OBJECT

signals:
    // A signal is just a declaration — no implementation, ever.
    void filterChanged(const QString& vehicleId);

private slots:
    // A slot is a normal method — it has an implementation.
    void on_applyButton_clicked();
};
```

### Making the connection

```cpp
// In MainWindow.cpp — connect FilterPanel's signal to our slot
connect(m_filterPanel, &FilterPanel::filterChanged,
        this,          &MainWindow::onFilterChanged);
//      ^sender        ^signal        ^receiver      ^slot
```

Always use this **function-pointer syntax** (with `&`). Never use the old string-based `SIGNAL()` / `SLOT()` macros — they're error-prone and skip compile-time checks.

### Emitting a signal

```cpp
void FilterPanel::on_applyButton_clicked()
{
    QString vehicle = m_vehicleInput->text();
    emit filterChanged(vehicle);   // ← broadcast to all connected slots
}
```

That's it. `emit` is just a Qt keyword that calls the signal. Qt automatically calls every slot connected to it.

### Signal → Slot flow in our project

```
User clicks "Apply Filter" in FilterPanel
    │  emit filterChanged(criteria)
    ▼
MainWindow::onFilterChanged(criteria)      ← connected slot
    │  calls WarehouseManager::queryPackages(...)
    ▼
PackageTableModel::refresh(results)
    │  emit dataChanged(...)               ← Qt built-in signal
    ▼
QTableView redraws automatically           ← Qt handles this for free
```

---

## 4. Widgets — Building the UI

A **widget** is any visual element: a window, a button, a text field, a table. They all inherit `QWidget`.

### Widgets we use in this project

| Qt Class | What it is | Where we use it |
|---|---|---|
| `QMainWindow` | The top-level application window | `MainWindow` |
| `QWidget` | A generic panel or container | `FilterPanel` |
| `QDialog` | A pop-up window | `AddPackageDialog`, `EditPackageDialog` |
| `QTableView` | A table that displays model data | Showing package list |
| `QLineEdit` | A single-line text input | Filter inputs, form fields |
| `QDateEdit` | A date picker | Import/export date fields |
| `QComboBox` | A dropdown selector | Category, zone selectors |
| `QPushButton` | A clickable button | Add, Remove, Apply buttons |
| `QLabel` | Static display text | Field labels |

### Layouts — how widgets are arranged

Never set pixel positions manually. Use **layout managers** instead; they handle resizing automatically.

```cpp
// Stack widgets vertically
auto* layout = new QVBoxLayout(this);
layout->addWidget(m_filterLabel);
layout->addWidget(m_vehicleInput);
layout->addWidget(m_applyButton);
```

| Layout class | Arranges children |
|---|---|
| `QVBoxLayout` | Top to bottom |
| `QHBoxLayout` | Left to right |
| `QFormLayout` | Label + field pairs (ideal for forms) |
| `QGridLayout` | In a grid |

### Qt Designer (.ui files)

Most of the visual layout is built with **Qt Designer** — a drag-and-drop editor that saves layouts as `.ui` files. These are compiled into C++ by Qt's build system.

When a `.ui` file is set up, you access its widgets through the `ui` pointer:

```cpp
// auto-generated — just use it
ui->tableView->setModel(m_model);
ui->addButton->setEnabled(true);
```

You do not manually create widgets that already exist in the `.ui` file.

---

## 5. Model/View — Displaying Data

Qt separates **data** from **display** using the Model/View pattern. You never put raw package data into a table widget directly — you put it into a **model**, and the view reads from the model automatically.

### How it works

```
PackageTableModel (your data)   ←→   QTableView (the visual table)
        ↑
  WarehouseManager provides the data
```

### Our `PackageTableModel`

It inherits `QAbstractTableModel` and overrides three methods Qt requires:

```cpp
class PackageTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    // How many rows?
    int rowCount(const QModelIndex& parent = {}) const override;

    // How many columns?
    int columnCount(const QModelIndex& parent = {}) const override;

    // What value goes in cell (row, column)?
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    // Our custom method: replace all data and refresh the view
    void refresh(std::vector<Package> newData);

private:
    std::vector<Package> m_packages;
};
```

### Refreshing the view

```cpp
void PackageTableModel::refresh(std::vector<Package> newData)
{
    beginResetModel();           // ← tell Qt: data is about to change
    m_packages = std::move(newData);
    endResetModel();             // ← tell Qt: data changed, redraw everything
}
```

These two calls are mandatory wrappers — Qt will not update the view without them.

---

## 6. Qt Data Types We Use

Qt has its own versions of common types. Use them at the Qt boundary (UI, file I/O), and standard C++ types everywhere else.

| Qt type | Equivalent | When to use it |
|---|---|---|
| `QString` | `std::string` | All text in Qt — widget labels, JSON keys, file paths |
| `QDate` | `std::chrono::year_month_day` | Dates in `LogisticsInfo`, date pickers |
| `QVariant` | `std::any` | Required return type in `QAbstractTableModel::data()` |
| `QJsonObject` | `nlohmann::json` object | One package serialized as JSON |
| `QJsonArray` | JSON array | The full list of packages in the file |
| `QJsonDocument` | Whole JSON file | Reading/writing the data file |

### Converting between `QString` and `std::string`

```cpp
std::string stdStr = "hello";
QString qtStr = QString::fromStdString(stdStr);   // std → Qt
std::string back = qtStr.toStdString();            // Qt → std
```

Do this conversion at the boundary — in repository code or in the GUI layer — never inside `domain/`.

---

## 7. JSON with Qt

Our `JsonPackageRepository` uses Qt's built-in JSON support to save and load packages. No third-party library needed.

### Writing packages to a file

```cpp
QJsonArray array;
for (const auto& pkg : m_packages)
{
    array.append(pkg.toJson());           // Package::toJson() returns QJsonObject
}

QJsonDocument doc(array);
QFile file(m_filePath);
file.open(QIODevice::WriteOnly);
file.write(doc.toJson());
```

### Reading packages from a file

```cpp
QFile file(m_filePath);
file.open(QIODevice::ReadOnly);

QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
QJsonArray array = doc.array();

for (const auto& val : array)
{
    m_packages.push_back(Package::fromJson(val.toObject()));
}
```

### Each `Package` serializes itself

```cpp
// In Package.cpp
QJsonObject Package::toJson() const
{
    QJsonObject obj;
    obj["id"]       = m_id;
    obj["category"] = QString::fromStdString(m_metadata.category);
    obj["zone"]     = QString::fromStdString(m_location.zone);
    // ... and so on
    return obj;
}
```

---

## 8. Memory Management in Qt

Qt has its own memory system that works alongside C++ smart pointers. Understanding the difference is critical.

### The parent–child tree

When you create a widget with a `parent`, Qt takes ownership. When the parent is destroyed, it automatically destroys all its children.

```cpp
// Qt owns this button — no delete needed, no unique_ptr
auto* button = new QPushButton("Add Package", this);  // 'this' = parent widget
```

### The rule: smart pointers and Qt parents don't mix

| Object type | How to manage memory |
|---|---|
| `QWidget` or any `QObject` with a parent | Raw `new` — Qt handles deletion |
| `QObject` without a parent (rare) | `std::unique_ptr<T>` is safe |
| Non-Qt domain objects (`Package`, etc.) | Always `std::unique_ptr` or `std::shared_ptr` |

**Never** wrap a parented Qt widget in `std::unique_ptr`. The parent will call `delete` when it is destroyed, and the `unique_ptr` destructor will call `delete` again — this is a crash.

```cpp
// WRONG — double deletion crash
auto panel = std::make_unique<FilterPanel>(this);

// CORRECT — Qt manages lifetime via parent
auto* panel = new FilterPanel(this);
```

---

## 9. How Our Project Uses Qt

Here is a concrete picture of where Qt appears in each file:

| File | What Qt does there |
|---|---|
| `MainWindow.h/.cpp` | Inherits `QMainWindow`, connects signals/slots, owns UI widgets |
| `FilterPanel.h/.cpp` | Inherits `QWidget`, declares `filterChanged` signal, emits it on button click |
| `PackageTableModel.h/.cpp` | Inherits `QAbstractTableModel`, feeds data to `QTableView` |
| `AddPackageDialog.h/.cpp` | Inherits `QDialog`, collects form input, emits `packageSubmitted` |
| `JsonPackageRepository.h/.cpp` | Uses `QFile`, `QJsonDocument` — the only Qt usage in `repository/` |
| `domain/`, `service/` | **Zero Qt** — pure C++20 |

### Startup sequence

```cpp
// main.cpp
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);   // ← must be created first, before any widget

    auto repo    = std::make_shared<JsonPackageRepository>("data.json");
    auto manager = WarehouseManager(repo);

    MainWindow window(manager);
    window.show();

    return app.exec();   // ← starts the Qt event loop; blocks until window closes
}
```

`app.exec()` hands control to Qt's event loop, which then dispatches user input and signals until the app exits.

---

## 10. Common Mistakes to Avoid

These are the errors that trip up almost everyone new to Qt.

| Mistake | What goes wrong | Fix |
|---|---|---|
| Forgetting `Q_OBJECT` | Signals/slots silently don't work; confusing linker errors | Always add it as the first line inside the class body |
| Using old `SIGNAL()`/`SLOT()` macros | Typos compile fine but break at runtime | Use `&ClassName::methodName` syntax |
| Wrapping Qt widgets in `unique_ptr` with a parent | Double-free crash on close | Use raw `new` with a parent pointer |
| Calling `beginResetModel()` without `endResetModel()` | View is permanently locked and shows nothing | Always call them as a pair |
| Putting Qt types inside `domain/` | Breaks the layer separation; untestable domain | Use `std::string`, `std::chrono` in domain; convert at the boundary |
| Not setting a layout on a widget | Widgets overlap or don't resize | Always set a `QVBoxLayout` / `QHBoxLayout` / `QFormLayout` on container widgets |
| Emitting a signal before state is fully updated | Slots see a half-updated object | Update all internal state first, then `emit` |

---

## 11. Where to Learn More

All official, free, and reliable:

| Resource | Best for |
|---|---|
| [Qt 6 official docs](https://doc.qt.io/qt-6/) | Complete API reference for any class |
| [Qt Examples & Tutorials](https://doc.qt.io/qt-6/qtexamplesandtutorials.html) | Runnable code for Model/View, Signals/Slots, dialogs |
| [Qt Model/View Programming](https://doc.qt.io/qt-6/model-view-programming.html) | Deep dive into `QAbstractTableModel` |
| [Signals & Slots docs](https://doc.qt.io/qt-6/signalsandslots.html) | Full reference for connect syntax and rules |

When in doubt, search `qt6 <ClassName>` on the official docs site rather than random Stack Overflow answers — Qt 5 and Qt 6 differ in important ways and old answers are often outdated.

---

*Questions? Ask in the team chat before guessing. A 2-minute question saves a 2-hour debug session.*
