# AI Usage Log
## Packages Warehouse Management System

**Group ID:** 1
**Member name:** Do Minh Khang
**Student ID:** 25127363

---

## Log Entries

---

### Entry 1

| Field | Details |
|---|---|
| **Date** | June 5, 2026 |
| **Tool** | Claude (Anthropic) |
| **Task** | Architecture design — reviewing our initial OOP design and getting a class diagram |
| **Prompt summary** | I described our project (a warehouse package management system in C++20 + Qt) and asked Claude to evaluate our planned design, suggest improvements, and produce a UML class diagram. I gave it our four requirements: package entity modeling, state lifecycle, system operations, and the State + Observer pattern constraints. |
| **AI output summary** | Claude gave a solid architecture review. It suggested splitting `Package` fields into smaller value objects (`PackageMetadata`, `Address`, `LogisticsInfo`, `StorageLocation`) instead of one flat class, introduced an `IPackageRepository` interface for dependency injection, and proposed a `PackageFilter` class built on `std::function` predicates. It also produced a full UML SVG diagram showing all four layers — value objects, state pattern, core entity, and Qt GUI. For C++20 it recommended `std::unique_ptr` for state ownership, `std::optional` for nullable lookups, and Concepts for constraining filter predicates. |
| **My modification** | I liked the value-object split — it aligns with what we learned in class about cohesion. I adopted it but merged `LogisticsInfo` and `StorageLocation` into one struct initially since our data is simpler than what Claude assumed. I also cut the `PackageFilter` Concepts template for now; we'll add that later once everyone is comfortable with templates. I rejected Claude's suggestion to use `std::variant` for states — the virtual State pattern is cleaner for our graded demo. The UML was used as a reference, but I redrew it myself in draw.io to match our simplified class list. |

---

### Entry 2

| Field | Details |
|---|---|
| **Date** | June 5, 2026 |
| **Tool** | Claude (Anthropic) |
| **Task** | Generating a team development guideline document |
| **Prompt summary** | After settling on the architecture, I asked Claude to write a comprehensive `GUIDELINE.md` for our team covering naming conventions, code formatting, documentation practices, and collaboration rules. I gave it the context of our stack (C++20, Qt 6) and the four-layer project structure we agreed on. |
| **AI output summary** | Claude produced a full 11-section markdown document. It covered file and class naming (PascalCase, `I`-prefix for interfaces, `m_` for private members), a ready-to-paste `.clang-format` config, Doxygen comment templates, a smart-pointer ownership table, Qt-specific rules (new connect syntax, parent ownership, `beginResetModel`), Conventional Commits format with our project scopes, a PR checklist, test naming conventions, and an AI usage policy section matching the course requirements. It ended with a quick-reference cheatsheet. |
| **My modification** | The document was well-structured but too long for some teammates to read in one sitting, so I reorganized the table of contents to put Naming and Git rules first — those are what people need day one. I trimmed the Doxygen example down to just the method-comment format since we're not generating docs externally. I also replaced Claude's generic commit scope list with our actual layer names (`domain`, `state`, `filter`, `gui`) to make it immediately usable. The `.clang-format` config was tested locally and kept as-is since it compiled and formatted correctly. |

---

### Entry 3

| Field | Details |
|---|---|
| **Date** | June 5, 2026 |
| **Tool** | Claude (Anthropic) |
| **Task** | Writing a Qt beginner's guide for teammates with no Qt experience |
| **Prompt summary** | Two teammates had never used Qt before. I asked Claude to write a concise `QTINSTRUCTION.md` covering the essentials — `QObject`, signals/slots, widgets, Model/View, Qt types, JSON, and memory management — keeping it practical and tied to our project rather than giving a generic Qt tutorial. I also asked it to advise me to add the link to `GUIDELINE.md` myself rather than doing it automatically. |
| **AI output summary** | Claude wrote a focused 11-section guide. It explained the `Q_OBJECT` macro, the new connect syntax vs old macro syntax, the parent–child memory model, and why `unique_ptr` and Qt parents must not be mixed. It included a widget reference table, a layout summary, a JSON read/write code example using `QJsonDocument`, and a mistake table listing the most common beginner errors with their symptoms and fixes. It correctly deferred the `GUIDELINE.md` link edit to me and explained where exactly to add it. |
| **My modification** | I removed the "Where to Learn More" section from the file shared with the team — I linked directly to the two most relevant Qt docs pages instead (Model/View and Signals/Slots) to avoid overwhelming people with options. I added one example of a real slot from our `MainWindow` to Section 3 so teammates could see something they'd actually type, not just a generic example. The mistake table was kept verbatim since every row describes a real issue we hit during initial setup. |

---

### Entry 4

| Field | Details |
|---|---|
| **Date** | June 5, 2026 |
| **Tool** | Claude (Anthropic) |
| **Task** | Creating this AI usage log |
| **Prompt summary** | I asked Claude to draft an AI usage log covering all three previous interactions, filling every field in the format defined in `GUIDELINE.md`. I asked for a natural, human-like tone rather than something that reads like it was written by a machine, and asked it not to include any extra explanation outside the log itself. |
| **AI output summary** | Claude produced this document — four log entries, one per interaction, with all fields filled in a consistent first-person voice. Each entry summarised both what the AI produced and what I actually changed, making the individual contribution clear. |
| **My modification** | Adjusted a few word choices in the prompt summaries to more accurately reflect how I actually phrased my questions. Updated the group ID and student ID placeholder fields. Will review with the team lead before submitting to confirm the modification descriptions are specific enough to satisfy the oral questioning requirement. |

---

*This log is an individual record and reflects my personal use of AI tools throughout the project. All AI-generated content referenced here was reviewed, tested where applicable, and modified before being committed to the repository or submitted as part of the project.*
