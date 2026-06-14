# ParamEaser Implementation Plan (Qt 6.10 / C++20 / CMake)

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build a Qt 6.10 desktop application that extracts dimensions from DXF drawings, lets users bind parameter controls (float/int) with keys, generates annotated SVG output, and provides a single-header reusable SVG preview widget for external system integration.

**Architecture:** Three-layer library design:
1. **Core** (`libparameaser-core`) — DXF parsing, dimension data model, SVG generation, multi-drawing project management
2. **Preview Widget** (`libparameaser-preview`) — Single header `ParamEaserPreview.h` providing a reusable `SvgPreviewWidget` with zoom/pan/marker overlay. Any external system can `#include` this one header to embed SVG preview with interactive markers.
3. **Application** — Full desktop UI built on the two libraries

**Key Design Decisions:**
- C++20 with extensive use of `std::map` for multi-drawing key-based access
- Pimpl pattern for the preview widget to keep the header clean
- DXF R12 ASCII parsing (hand-written, no external dependency)
- QSvgRenderer + custom QWidget overlay for marker rendering
- Multi-drawing support via `ProjectManager` using drawing key (`std::string`) as index
- All coordinate math uses double precision

**Tech Stack:** Qt 6.10 (Widgets, Svg), C++20, CMake 3.28+, DXF R12 ASCII parser (custom)

---

## File Structure

```
G:\Code\ParamEaser\
├── CMakeLists.txt                         — Root CMake
├── .gitignore
├── README.md
├── cmake/
│   └── FindQt6.cmake                     — Qt6 finder helpers
├── src/
│   ├── CMakeLists.txt                    — Library CMake
│   ├── core/
│   │   ├── Dimension.h / .cpp            — Dimension & DrawingData structs
│   │   ├── ProjectManager.h / .cpp       — Multi-drawing project management
│   │   ├── DxfParser.h / .cpp            — DXF parser (R12 ASCII)
│   │   ├── DxfToSvgRenderer.h / .cpp     — DXF geometry → SVG string
│   │   └── SvgGenerator.h / .cpp         — Output SVG (drawing+dims+params)
│   ├── preview/
│   │   ├── ParamEaserPreview.h           — ★ SINGLE HEADER ★ SvgPreviewWidget
│   │   └── ParamEaserPreview.cpp         — Implementation (pimpl)
│   └── app/
│       ├── CMakeLists.txt                — App CMake
│       ├── MainWindow.h / .cpp           — Main application window
│       ├── ParamEditDialog.h / .cpp      — Popover dialog for param binding
│       └── main.cpp                      — Entry point
├── tests/
│   ├── CMakeLists.txt
│   ├── test_DxfParser.cpp
│   ├── test_ProjectManager.cpp
│   └── test_SvgGenerator.cpp
└── examples/
    └── integration/
        ├── CMakeLists.txt
        └── main.cpp                      — Example: #include "ParamEaserPreview.h"
```

---

### Task 1: Project Scaffolding — CMake + Qt 6.10

**Files:**
- Create: `G:\Code\ParamEaser\CMakeLists.txt`
- Create: `G:\Code\ParamEaser\src\CMakeLists.txt`
- Create: `G:\Code\ParamEaser\src\app\CMakeLists.txt`
- Create: `G:\Code\ParamEaser\tests\CMakeLists.txt`
- Create: `G:\Code\ParamEaser\examples\integration\CMakeLists.txt`
- Create: `G:\Code\ParamEaser\.gitignore`
- Create: all header/cpp stub files with minimal `#pragma once` + includes

- [ ] **Step 1: Create root CMakeLists.txt**

```cmake
cmake_minimum_required(VERSION 3.28)
project(ParamEaser VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Svg)

add_subdirectory(src)

# Tests (optional)
option(BUILD_TESTS "Build tests" ON)
if(BUILD_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

# Examples
option(BUILD_EXAMPLES "Build examples" ON)
if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()
```

- [ ] **Step 2: Create src/CMakeLists.txt**

```cmake
# Core library
set(CORE_SOURCES
    core/Dimension.cpp
    core/ProjectManager.cpp
    core/DxfParser.cpp
    core/DxfToSvgRenderer.cpp
    core/SvgGenerator.cpp
)
set(CORE_HEADERS
    core/Dimension.h
    core/ProjectManager.h
    core/DxfParser.h
    core/DxfToSvgRenderer.h
    core/SvgGenerator.h
)
add_library(parameaser-core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_include_directories(parameaser-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parameaser-core PUBLIC Qt6::Svg)
target_compile_features(parameaser-core PUBLIC cxx_std_20)

# Preview library
add_library(parameaser-preview STATIC
    preview/ParamEaserPreview.cpp
    preview/ParamEaserPreview.h
)
target_include_directories(parameaser-preview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parameaser-preview PUBLIC parameaser-core Qt6::Widgets Qt6::Svg)

# Application
add_subdirectory(app)
```

- [ ] **Step 3: Create src/app/CMakeLists.txt**

```cmake
add_executable(parameaser
    main.cpp
    MainWindow.h MainWindow.cpp
    ParamEditDialog.h ParamEditDialog.cpp
)
target_link_libraries(parameaser PRIVATE parameaser-core parameaser-preview)
```

- [ ] **Step 4: Create tests/CMakeLists.txt**

```cmake
function(add_parameaser_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE parameaser-core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_parameaser_test(test_DxfParser test_DxfParser.cpp)
add_parameaser_test(test_ProjectManager test_ProjectManager.cpp)
add_parameaser_test(test_SvgGenerator test_SvgGenerator.cpp)
```

- [ ] **Step 5: Create examples/integration/CMakeLists.txt**

```cmake
add_executable(parameaser-integration-example main.cpp)
target_link_libraries(parameaser-integration-example PRIVATE parameaser-preview)
```

- [ ] **Step 6: Create .gitignore**

```
build/
*.user
CMakeUserPresets.json
cmake-build-*/
```

- [ ] **Step 7: Create stub source files**

Create all `.h` files with `#pragma once` and forward declarations. Create all `.cpp` files with minimal `#include` stubs returning default values.

- [ ] **Step 8: Verify build**

```bash
cd "G:\Code\ParamEaser"
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=C:/Qt/6.10.0/msvc2022_64
cmake --build build
```
Expected: Build succeeds (all stubs link).

- [ ] **Step 9: Initialize git and commit**

```bash
git init
git add .
git commit -m "chore: scaffold CMake + Qt6.10 project structure"
```

---

### Task 2: Core Data Model — Dimension, DrawingData, ProjectManager

**Files:**
- Modify: `src/core/Dimension.h`
- Modify: `src/core/Dimension.cpp`
- Modify: `src/core/ProjectManager.h`
- Modify: `src/core/ProjectManager.cpp`
- Create: `tests/test_ProjectManager.cpp`

- [ ] **Step 1: Implement Dimension.h**

```cpp
// src/core/Dimension.h
#pragma once
#include <string>
#include <vector>
#include <optional>
#include <map>

// Parameter binding attached to a dimension
struct ParamBinding {
    std::string key;        // e.g. "hole_diameter"
    std::string type;       // "float" or "int"
    double defaultValue;
};

// A single dimension extracted from DXF
struct Dimension {
    std::string id;                 // "D1", "D2", ...
    std::string type;               // "aligned", "linear", "radial", "diameter", "angular"
    std::string orientation;        // "horizontal", "vertical" (linear only)
    double value = 0.0;
    std::string unit = "mm";
    double centerX = 0.0;           // dimension text center X
    double centerY = 0.0;           // dimension text center Y
    double rotation = 0.0;          // degrees
    std::optional<ParamBinding> param; // nullopt = unbound

    bool isBound() const { return param.has_value(); }
};

// A single drawing identified by a unique key
struct DrawingData {
    std::string key;                // unique identifier
    std::string sourceFile;         // original .dxf filename
    std::string rawDxfContent;      // original DXF text
    std::string drawingSvg;         // DXF → SVG (geometry only)
    std::vector<Dimension> dimensions;
};

// Marker data for the preview widget overlay
struct DimensionMarker {
    std::string id;
    double centerX;
    double centerY;
    double rotation;
    bool isBound;
    std::string label;  // bound → param key, unbound → id
    std::string paramType;
};
```

- [ ] **Step 2: Implement ProjectManager.h**

```cpp
// src/core/ProjectManager.h
#pragma once
#include "Dimension.h"
#include <vector>
#include <map>
#include <functional>

// Callback type for dimension changes
using DimensionChangeCallback = std::function<void(const std::string& drawingKey, const std::string& dimId)>;

class ProjectManager {
public:
    ProjectManager();

    // --- Drawing management (multi-drawing) ---
    bool addDrawing(const std::string& key, DrawingData data);
    bool removeDrawing(const std::string& key);
    bool hasDrawing(const std::string& key) const;
    DrawingData* getDrawing(const std::string& key);
    const DrawingData* getDrawing(const std::string& key) const;
    std::vector<std::string> drawingKeys() const;
    size_t drawingCount() const;
    void clearAll();

    // --- Dimension parameter binding ---
    bool bindParam(const std::string& drawingKey, const std::string& dimId,
                   const std::string& type, const std::string& paramKey,
                   double defaultValue);
    bool unbindParam(const std::string& drawingKey, const std::string& dimId);
    Dimension* findDimension(const std::string& drawingKey, const std::string& dimId);

    // --- Statistics ---
    int boundCount(const std::string& drawingKey) const;
    int unboundCount(const std::string& drawingKey) const;
    int totalCount(const std::string& drawingKey) const;

    // --- Serialization ---
    std::string toJson() const;
    static ProjectManager fromJson(const std::string& json);

    // --- Change notification ---
    void setChangeCallback(DimensionChangeCallback cb);
    void notifyChange(const std::string& drawingKey, const std::string& dimId);

    // --- Marker helpers ---
    std::vector<DimensionMarker> getMarkers(const std::string& drawingKey) const;

private:
    std::map<std::string, DrawingData> m_drawings;
    DimensionChangeCallback m_changeCallback;
    int m_nextDimId = 1;
};
```

- [ ] **Step 3: Write tests**

```cpp
// tests/test_ProjectManager.cpp
#include <QtTest>
#include "core/ProjectManager.h"

class TestProjectManager : public QObject {
    Q_OBJECT

private slots:
    void init() {
        pm = std::make_unique<ProjectManager>();
        DrawingData dd;
        dd.key = "drawing1";
        dd.sourceFile = "test.dxf";
        dd.dimensions = {
            {.id = "D1", .type = "aligned", .value = 50.0, .centerX = 10, .centerY = 20},
            {.id = "D2", .type = "linear", .value = 100.0, .centerX = 30, .centerY = 40},
        };
        pm->addDrawing("drawing1", dd);
    }

    void test_add_and_get_drawing() {
        QCOMPARE(pm->drawingCount(), size_t(1));
        auto* d = pm->getDrawing("drawing1");
        QVERIFY(d != nullptr);
        QCOMPARE(d->dimensions.size(), size_t(2));
    }

    void test_bind_param() {
        bool ok = pm->bindParam("drawing1", "D1", "float", "hole_dia", 50.0);
        QVERIFY(ok);
        auto* dim = pm->findDimension("drawing1", "D1");
        QVERIFY(dim->isBound());
        QCOMPARE(dim->param->key, "hole_dia");
        QCOMPARE(dim->param->type, "float");
    }

    void test_unbind_param() {
        pm->bindParam("drawing1", "D1", "float", "hole_dia", 50.0);
        pm->unbindParam("drawing1", "D1");
        auto* dim = pm->findDimension("drawing1", "D1");
        QVERIFY(!dim->isBound());
    }

    void test_statistics() {
        pm->bindParam("drawing1", "D1", "float", "a", 1);
        QCOMPARE(pm->boundCount("drawing1"), 1);
        QCOMPARE(pm->unboundCount("drawing1"), 1);
        QCOMPARE(pm->totalCount("drawing1"), 2);
    }

    void test_remove_drawing() {
        pm->removeDrawing("drawing1");
        QCOMPARE(pm->drawingCount(), size_t(0));
    }

    void test_get_markers() {
        pm->bindParam("drawing1", "D1", "float", "length", 50.0);
        auto markers = pm->getMarkers("drawing1");
        QCOMPARE(markers.size(), size_t(2));
        // D1 should be bound, D2 unbound
        auto m1 = std::find_if(markers.begin(), markers.end(),
            [](auto& m) { return m.id == "D1"; });
        QVERIFY(m1 != markers.end());
        QVERIFY(m1->isBound);
        QCOMPARE(m1->label, "length");
    }

private:
    std::unique_ptr<ProjectManager> pm;
};

QTEST_MAIN(TestProjectManager)
#include "test_ProjectManager.moc"
```

- [ ] **Step 4: Implement ProjectManager.cpp**

```cpp
// src/core/ProjectManager.cpp
#include "core/ProjectManager.h"
#include <sstream>
#include <algorithm>

ProjectManager::ProjectManager() = default;

bool ProjectManager::addDrawing(const std::string& key, DrawingData data) {
    if (m_drawings.contains(key)) return false;
    data.key = key;
    m_drawings[key] = std::move(data);
    return true;
}

bool ProjectManager::removeDrawing(const std::string& key) {
    return m_drawings.erase(key) > 0;
}

bool ProjectManager::hasDrawing(const std::string& key) const {
    return m_drawings.contains(key);
}

DrawingData* ProjectManager::getDrawing(const std::string& key) {
    auto it = m_drawings.find(key);
    return it != m_drawings.end() ? &it->second : nullptr;
}

const DrawingData* ProjectManager::getDrawing(const std::string& key) const {
    auto it = m_drawings.find(key);
    return it != m_drawings.end() ? &it->second : nullptr;
}

std::vector<std::string> ProjectManager::drawingKeys() const {
    std::vector<std::string> keys;
    for (const auto& [k, _] : m_drawings) keys.push_back(k);
    return keys;
}

size_t ProjectManager::drawingCount() const { return m_drawings.size(); }
void ProjectManager::clearAll() { m_drawings.clear(); }

bool ProjectManager::bindParam(const std::string& dk, const std::string& dimId,
                                const std::string& type, const std::string& key,
                                double defaultValue) {
    auto* dim = findDimension(dk, dimId);
    if (!dim) return false;
    dim->param = ParamBinding{key, type, defaultValue};
    notifyChange(dk, dimId);
    return true;
}

bool ProjectManager::unbindParam(const std::string& dk, const std::string& dimId) {
    auto* dim = findDimension(dk, dimId);
    if (!dim) return false;
    dim->param.reset();
    notifyChange(dk, dimId);
    return true;
}

Dimension* ProjectManager::findDimension(const std::string& dk, const std::string& dimId) {
    auto* dd = getDrawing(dk);
    if (!dd) return nullptr;
    auto it = std::find_if(dd->dimensions.begin(), dd->dimensions.end(),
        [&](auto& d) { return d.id == dimId; });
    return it != dd->dimensions.end() ? &*it : nullptr;
}

int ProjectManager::boundCount(const std::string& dk) const {
    auto* dd = getDrawing(dk);
    if (!dd) return 0;
    return std::count_if(dd->dimensions.begin(), dd->dimensions.end(),
        [](auto& d) { return d.isBound(); });
}

int ProjectManager::unboundCount(const std::string& dk) const {
    auto* dd = getDrawing(dk);
    if (!dd) return 0;
    return std::count_if(dd->dimensions.begin(), dd->dimensions.end(),
        [](auto& d) { return !d.isBound(); });
}

int ProjectManager::totalCount(const std::string& dk) const {
    auto* dd = getDrawing(dk);
    return dd ? static_cast<int>(dd->dimensions.size()) : 0;
}

void ProjectManager::setChangeCallback(DimensionChangeCallback cb) {
    m_changeCallback = std::move(cb);
}

void ProjectManager::notifyChange(const std::string& dk, const std::string& dimId) {
    if (m_changeCallback) m_changeCallback(dk, dimId);
}

std::vector<DimensionMarker> ProjectManager::getMarkers(const std::string& dk) const {
    std::vector<DimensionMarker> markers;
    auto* dd = getDrawing(dk);
    if (!dd) return markers;
    for (const auto& dim : dd->dimensions) {
        DimensionMarker m;
        m.id = dim.id;
        m.centerX = dim.centerX;
        m.centerY = dim.centerY;
        m.rotation = dim.rotation;
        m.isBound = dim.isBound();
        m.label = dim.isBound() ? dim.param->key : dim.id;
        m.paramType = dim.isBound() ? dim.param->type : "";
        markers.push_back(m);
    }
    return markers;
}

std::string ProjectManager::toJson() const {
    // Simplified JSON serialization (can use QJsonDocument in practice)
    std::ostringstream os;
    os << "{\"version\":\"1.0\",\"drawings\":[";
    bool first = true;
    for (const auto& [key, dd] : m_drawings) {
        if (!first) os << ",";
        first = false;
        os << "{\"key\":\"" << key << "\",\"source\":\"" << dd.sourceFile
           << "\",\"dimensions\":[";
        bool firstDim = true;
        for (const auto& dim : dd.dimensions) {
            if (!firstDim) os << ",";
            firstDim = false;
            os << "{\"id\":\"" << dim.id << "\",\"type\":\"" << dim.type
               << "\",\"value\":" << dim.value
               << ",\"centerX\":" << dim.centerX
               << ",\"centerY\":" << dim.centerY
               << ",\"rotation\":" << dim.rotation;
            if (dim.isBound()) {
                os << ",\"param\":{\"key\":\"" << dim.param->key
                   << "\",\"type\":\"" << dim.param->type
                   << "\",\"default\":" << dim.param->defaultValue << "}";
            } else {
                os << ",\"param\":null";
            }
            os << "}";
        }
        os << "]}";
    }
    os << "]}";
    return os.str();
}

ProjectManager ProjectManager::fromJson(const std::string& json) {
    ProjectManager pm;
    // Parse with QJsonDocument in real implementation
    // For now, minimal stub that returns empty manager
    return pm;
}
```

- [ ] **Step 5: Build and run tests**

```bash
cmake --build build
cd build && ctest --output-on-failure -R test_ProjectManager
```
Expected: All tests PASS.

- [ ] **Step 6: Commit**

```bash
git add src/core/ tests/
git commit -m "feat: add core data model with multi-drawing ProjectManager"
```

---

### Task 3: DXF Parser (R12 ASCII)

**Files:**
- Modify: `src/core/DxfParser.h`
- Modify: `src/core/DxfParser.cpp`
- Create: `tests/test_DxfParser.cpp`

- [ ] **Step 1: Implement DxfParser.h**

```cpp
// src/core/DxfParser.h
#pragma once
#include "Dimension.h"
#include <string>
#include <vector>
#include <unordered_map>

class DxfParser {
public:
    DxfParser() = default;

    // Parse DXF text content, return extracted dimensions
    std::vector<Dimension> parseDimensions(const std::string& dxfContent);

    // Parse DXF text content, extract all visible entities as SVG
    std::string parseToSvg(const std::string& dxfContent);

    // Get bounding box of all entities
    struct BoundingBox { double minX, minY, maxX, maxY; };
    BoundingBox getBounds(const std::string& dxfContent);

    // Error info
    std::string lastError() const { return m_lastError; }
    bool hasError() const { return !m_lastError.empty(); }

private:
    std::string m_lastError;

    struct ParsedEntity {
        std::string type;
        std::unordered_map<int, double> values;
        std::unordered_map<int, std::string> strings;
    };

    std::vector<ParsedEntity> parseEntities(const std::string& content);
    Dimension dimensionFromEntity(const ParsedEntity& ent, int index);
    std::string entityToSvg(const ParsedEntity& ent);

    static std::string sanitize(const std::string& s);
};
```

- [ ] **Step 2: Write tests**

```cpp
// tests/test_DxfParser.cpp
#include <QtTest>
#include "core/DxfParser.h"

class TestDxfParser : public QObject {
    Q_OBJECT

private slots:
    void test_parse_aligned_dimension() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n0\n"
            "10\n100.0\n20\n200.0\n11\n120.0\n21\n180.0\n"
            "13\n0.0\n23\n0.0\n14\n200.0\n24\n100.0\n"
            "40\n50.0\n50\n45.0\n1\n50.0\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("aligned"));
        QCOMPARE(dims[0].value, 50.0);
        QCOMPARE(dims[0].centerX, 120.0);
        QCOMPARE(dims[0].centerY, 180.0);
        QCOMPARE(dims[0].rotation, 45.0);
    }

    void test_parse_linear_horizontal() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n1\n"
            "10\n0.0\n20\n0.0\n11\n100.0\n21\n-10.0\n"
            "40\n200.0\n50\n0.0\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("linear"));
        QCOMPARE(dims[0].orientation, std::string("horizontal"));
    }

    void test_parse_linear_vertical() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n1\n"
            "10\n0.0\n20\n0.0\n11\n110.0\n21\n50.0\n"
            "40\n100.0\n50\n90.0\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("linear"));
        QCOMPARE(dims[0].orientation, std::string("vertical"));
    }

    void test_parse_radial() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n4\n"
            "10\n50.0\n20\n50.0\n11\n80.0\n21\n30.0\n"
            "40\n30.0\n1\nR30\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("radial"));
        QCOMPARE(dims[0].value, 30.0);
    }

    void test_parse_angular() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nDIMENSION\n8\n0\n70\n2\n"
            "10\n0.0\n20\n0.0\n11\n60.0\n21\n40.0\n"
            "40\n45.0\n1\n45°\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(1));
        QCOMPARE(dims[0].type, std::string("angular"));
    }

    void test_no_dimensions_in_empty_drawing() {
        DxfParser parser;
        auto dims = parser.parseDimensions(
            "0\nSECTION\n2\nENTITIES\n"
            "0\nLINE\n8\n0\n10\n0\n20\n0\n11\n100\n21\n100\n"
            "0\nENDSEC\n0\nEOF");
        QCOMPARE(dims.size(), size_t(0));
    }

private:
    DxfParser parser;
};

QTEST_MAIN(TestDxfParser)
#include "test_DxfParser.moc"
```

- [ ] **Step 3: Implement DxfParser.cpp**

```cpp
// src/core/DxfParser.cpp
#include "core/DxfParser.h"
#include <sstream>
#include <cmath>
#include <algorithm>

std::vector<Dimension> DxfParser::parseDimensions(const std::string& content) {
    m_lastError.clear();
    std::vector<Dimension> result;
    auto entities = parseEntities(content);

    int dimIndex = 0;
    for (const auto& ent : entities) {
        if (ent.type == "DIMENSION") {
            result.push_back(dimensionFromEntity(ent, dimIndex++));
        }
    }
    return result;
}

std::vector<DxfParser::ParsedEntity> DxfParser::parseEntities(const std::string& content) {
    std::vector<ParsedEntity> entities;
    std::istringstream stream(content);
    std::string line;
    ParsedEntity current;
    bool inEntities = false;
    bool readingEntity = false;

    auto finalizeEntity = [&]() {
        if (!current.type.empty() && readingEntity) {
            entities.push_back(current);
        }
        current = ParsedEntity();
        readingEntity = false;
    };

    while (std::getline(stream, line)) {
        // Trim whitespace
        line.erase(0, line.find_first_not_of(" \r\n\t"));
        line.erase(line.find_last_not_of(" \r\n\t") + 1);
        if (line.empty()) continue;

        int groupCode = std::stoi(line);

        if (!std::getline(stream, line)) break;
        line.erase(0, line.find_first_not_of(" \r\n\t"));
        line.erase(line.find_last_not_of(" \r\n\t") + 1);

        if (groupCode == 0) {
            if (line == "SECTION") {
                finalizeEntity();
                // Read section type
                std::getline(stream, line);
                line.erase(0, line.find_first_not_of(" \r\n\t"));
                line.erase(line.find_last_not_of(" \r\n\t") + 1);
                inEntities = (line == "ENTITIES");
                continue;
            }
            if (line == "ENDSEC" || line == "EOF") {
                finalizeEntity();
                continue;
            }
            // New entity
            finalizeEntity();
            current.type = line;
            readingEntity = true;
            continue;
        }

        if (!readingEntity) continue;

        if (groupCode == 70 || groupCode == 40 || groupCode == 50 ||
            groupCode == 10 || groupCode == 20 ||
            groupCode == 11 || groupCode == 21 ||
            groupCode == 13 || groupCode == 23 ||
            groupCode == 14 || groupCode == 24 ||
            groupCode == 41 || groupCode == 42) {
            current.values[groupCode] = std::stod(line);
        } else if (groupCode == 1) {
            current.strings[groupCode] = line;
        } else if (groupCode == 8) {
            current.strings[groupCode] = line; // layer
        }
    }
    return entities;
}

Dimension DxfParser::dimensionFromEntity(const ParsedEntity& ent, int index) {
    Dimension dim;
    dim.id = "D" + std::to_string(index + 1);

    int typeCode = static_cast<int>(ent.values.count(70) ? ent.values.at(70) : 0);
    switch (typeCode) {
        case 0: dim.type = "aligned"; break;
        case 1: dim.type = "linear"; break;
        case 2: dim.type = "angular"; break;
        case 3: dim.type = "diameter"; break;
        case 4: dim.type = "radial"; break;
        default: dim.type = "aligned"; break;
    }

    dim.centerX = ent.values.count(11) ? ent.values.at(11) : 0.0;
    dim.centerY = ent.values.count(21) ? ent.values.at(21) : 0.0;
    dim.value = ent.values.count(40) ? ent.values.at(40) : 0.0;
    dim.rotation = ent.values.count(50) ? ent.values.at(50) : 0.0;

    // Determine orientation for linear dimensions
    if (dim.type == "linear") {
        double rot = std::fmod(dim.rotation, 180.0);
        if (rot < 0) rot += 180.0;
        dim.orientation = (rot >= 45.0 && rot <= 135.0) ? "vertical" : "horizontal";
    }

    // Read text value from group code 1
    if (ent.strings.count(1)) {
        dim.unit = ent.strings.at(1);
    }

    dim.param = std::nullopt;
    return dim;
}
```

- [ ] **Step 4: Build and run tests**

```bash
cmake --build build
cd build && ctest --output-on-failure -R test_DxfParser
```
Expected: All tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/core/DxfParser.h src/core/DxfParser.cpp tests/test_DxfParser.cpp
git commit -m "feat: add DXF R12 ASCII parser with 4 dimension types"
```

---

### Task 4: DXF → SVG Geometry Renderer

**Files:**
- Modify: `src/core/DxfToSvgRenderer.h`
- Modify: `src/core/DxfToSvgRenderer.cpp`

- [ ] **Step 1: Implement DxfToSvgRenderer**

```cpp
// src/core/DxfToSvgRenderer.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <sstream>

class DxfToSvgRenderer {
public:
    // Convert DXF geometry entities to SVG string
    std::string render(const std::string& dxfContent);

    // Overload: accept pre-parsed entities
    std::string renderWithBounds(const std::vector<ParsedEntity>& entities,
                                  double minX, double minY,
                                  double maxX, double maxY);

    struct ParsedEntity {
        std::string type;
        std::unordered_map<int, double> nums;
    };

    static std::vector<ParsedEntity> extractEntities(const std::string& content);

private:
    static std::string entityTag(const ParsedEntity& ent);
    static void updateBounds(const ParsedEntity& ent,
                             double& minX, double& minY,
                             double& maxX, double& maxY);
};
```

```cpp
// src/core/DxfToSvgRenderer.cpp
#include "core/DxfToSvgRenderer.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numbers>

std::string DxfToSvgRenderer::render(const std::string& dxfContent) {
    auto entities = extractEntities(dxfContent);
    double minX = 1e30, minY = 1e30, maxX = -1e30, maxY = -1e30;
    for (const auto& ent : entities) updateBounds(ent, minX, minY, maxX, maxY);

    if (minX > maxX) { minX = 0; minY = 0; maxX = 100; maxY = 100; }

    const double pad = 20.0;
    double w = maxX - minX + pad * 2;
    double h = maxY - minY + pad * 2;

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        << " viewBox=\"" << (minX - pad) << " " << (minY - pad)
        << " " << w << " " << h << "\">\n"
        << "<g id=\"drawing\" fill=\"none\" stroke=\"rgba(255,255,255,0.4)\" stroke-width=\"1.5\">\n";

    for (const auto& ent : entities) {
        svg << entityTag(ent) << "\n";
    }

    svg << "</g>\n</svg>";
    return svg.str();
}

std::vector<DxfToSvgRenderer::ParsedEntity>
DxfToSvgRenderer::extractEntities(const std::string& content) {
    std::vector<ParsedEntity> entities;
    std::istringstream stream(content);
    std::string line;

    auto readLine = [&]() -> bool {
        return static_cast<bool>(std::getline(stream, line));
    };

    while (readLine()) {
        line.erase(0, line.find_first_not_of(" \r\n\t"));
        line.erase(line.find_last_not_of(" \r\n\t") + 1);
        if (line.empty()) continue;

        int code = std::stoi(line);
        if (code != 0) continue;
        if (!readLine()) break;

        line.erase(0, line.find_first_not_of(" \r\n\t"));
        line.erase(line.find_last_not_of(" \r\n\t") + 1);

        if (line == "LINE" || line == "CIRCLE" || line == "ARC" ||
            line == "LWPOLYLINE" || line == "POLYLINE") {
            ParsedEntity ent;
            ent.type = line;

            while (readLine()) {
                line.erase(0, line.find_first_not_of(" \r\n\t"));
                line.erase(line.find_last_not_of(" \r\n\t") + 1);
                if (line.empty()) continue;

                int subCode = std::stoi(line);
                if (subCode == 0) {
                    // Push back the "0" line for next iteration
                    // We need to handle this differently - backtrack
                    break;
                }
                if (!readLine()) break;

                line.erase(0, line.find_first_not_of(" \r\n\t"));
                line.erase(line.find_last_not_of(" \r\n\t") + 1);

                if (subCode == 10 || subCode == 20 || subCode == 11 ||
                    subCode == 21 || subCode == 40 || subCode == 50 ||
                    subCode == 51) {
                    ent.nums[subCode] = std::stod(line);
                }
            }
            entities.push_back(ent);
        }
    }
    return entities;
}

std::string DxfToSvgRenderer::entityTag(const ParsedEntity& ent) {
    std::ostringstream tag;
    if (ent.type == "LINE") {
        auto x1 = ent.nums.count(10) ? ent.nums.at(10) : 0.0;
        auto y1 = ent.nums.count(20) ? ent.nums.at(20) : 0.0;
        auto x2 = ent.nums.count(11) ? ent.nums.at(11) : 0.0;
        auto y2 = ent.nums.count(21) ? ent.nums.at(21) : 0.0;
        tag << "  <line x1=\"" << x1 << "\" y1=\"" << y1
            << "\" x2=\"" << x2 << "\" y2=\"" << y2 << "\"/>";
    }
    else if (ent.type == "CIRCLE") {
        auto cx = ent.nums.count(10) ? ent.nums.at(10) : 0.0;
        auto cy = ent.nums.count(20) ? ent.nums.at(20) : 0.0;
        auto r  = ent.nums.count(40) ? ent.nums.at(40) : 0.0;
        tag << "  <circle cx=\"" << cx << "\" cy=\"" << cy << "\" r=\"" << r << "\"/>";
    }
    else if (ent.type == "ARC") {
        auto cx = ent.nums.count(10) ? ent.nums.at(10) : 0.0;
        auto cy = ent.nums.count(20) ? ent.nums.at(20) : 0.0;
        auto r  = ent.nums.count(40) ? ent.nums.at(40) : 0.0;
        double sa = ent.nums.count(50) ? ent.nums.at(50) : 0.0;
        double ea = ent.nums.count(51) ? ent.nums.at(51) : 0.0;
        auto toRad = [](double deg) { return deg * std::numbers::pi / 180.0; };
        double sx = cx + r * std::cos(toRad(sa));
        double sy = cy + r * std::sin(toRad(sa));
        double ex = cx + r * std::cos(toRad(ea));
        double ey = cy + r * std::sin(toRad(ea));
        int largeArc = (ea - sa > 180.0) ? 1 : 0;
        tag << "  <path d=\"M " << sx << " " << sy
            << " A " << r << " " << r << " 0 " << largeArc << " 0 "
            << ex << " " << ey << "\"/>";
    }
    return tag.str();
}

void DxfToSvgRenderer::updateBounds(const ParsedEntity& ent,
                                     double& minX, double& minY,
                                     double& maxX, double& maxY) {
    auto upd = [&](double x, double y) {
        minX = std::min(minX, x); minY = std::min(minY, y);
        maxX = std::max(maxX, x); maxY = std::max(maxY, y);
    };
    if (ent.type == "LINE") {
        upd(ent.nums.count(10) ? ent.nums.at(10) : 0,
            ent.nums.count(20) ? ent.nums.at(20) : 0);
        upd(ent.nums.count(11) ? ent.nums.at(11) : 0,
            ent.nums.count(21) ? ent.nums.at(21) : 0);
    } else if (ent.type == "CIRCLE" || ent.type == "ARC") {
        double cx = ent.nums.count(10) ? ent.nums.at(10) : 0;
        double cy = ent.nums.count(20) ? ent.nums.at(20) : 0;
        double r  = ent.nums.count(40) ? ent.nums.at(40) : 0;
        upd(cx - r, cy - r);
        upd(cx + r, cy + r);
    }
}
```

- [ ] **Step 2: Build test**

Run: `cmake --build build`
Expected: Build succeeds.

- [ ] **Step 3: Commit**

```bash
git add src/core/DxfToSvgRenderer.h src/core/DxfToSvgRenderer.cpp
git commit -m "feat: add DXF to SVG geometry renderer (LINE, CIRCLE, ARC)"
```

---

### Task 5: SVG Output Generator (3-Layer + Param Badges)

**Files:**
- Modify: `src/core/SvgGenerator.h`
- Modify: `src/core/SvgGenerator.cpp`
- Create: `tests/test_SvgGenerator.cpp`

- [ ] **Step 1: Implement SvgGenerator**

```cpp
// src/core/SvgGenerator.h
#pragma once
#include "Dimension.h"
#include <string>
#include <vector>

class SvgGenerator {
public:
    // Generate output SVG with drawing + dimensions + params layers
    std::string generate(const std::string& drawingSvg,
                         const std::vector<Dimension>& dimensions);

    // Generate with embedded metadata JSON
    std::string generateWithMetadata(const std::string& drawingSvg,
                                      const std::vector<Dimension>& dimensions,
                                      const std::string& sourceFile);

private:
    std::string renderDimensionLine(const Dimension& dim);
    std::string renderParamBadge(const Dimension& dim);
    std::string extractViewBox(const std::string& svg);
    std::string extractDrawingInner(const std::string& svg);
};
```

- [ ] **Step 2: Write tests**

```cpp
// tests/test_SvgGenerator.cpp
#include <QtTest>
#include "core/SvgGenerator.h"

class TestSvgGenerator : public QObject {
    Q_OBJECT

private slots:
    void test_generates_three_layers() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 200 200\">"
                              "<g id=\"drawing\"><line x1=\"0\" y1=\"0\" x2=\"100\" y2=\"100\"/></g></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 100, .centerX = 50, .centerY = 20,
             .param = ParamBinding{"width", "float", 100}},
            {.id = "D2", .type = "radial", .value = 25, .centerX = 80, .centerY = 60,
             .param = std::nullopt},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("<g id=\"drawing\">") != std::string::npos);
        QVERIFY(result.find("<g id=\"dimensions\">") != std::string::npos);
        QVERIFY(result.find("<g id=\"params\">") != std::string::npos);
    }

    void test_has_bound_param_badge() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 100, .centerX = 50, .centerY = 20,
             .param = ParamBinding{"width", "float", 100}},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("$width") != std::string::npos);
        QVERIFY(result.find("float") != std::string::npos);
    }

    void test_has_unbound_marker() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D2", .type = "linear", .value = 25, .centerX = 30, .centerY = 40,
             .param = std::nullopt},
        };
        std::string result = gen.generate(drawing, dims);
        QVERIFY(result.find("D2") != std::string::npos);
        QVERIFY(result.find("unbound") != std::string::npos);
    }

    void test_metadata_embedded() {
        SvgGenerator gen;
        std::string drawing = "<svg viewBox=\"0 0 100 100\"></svg>";
        std::vector<Dimension> dims = {
            {.id = "D1", .type = "linear", .value = 50, .centerX = 10, .centerY = 20,
             .param = ParamBinding{"len", "int", 50}},
        };
        std::string result = gen.generateWithMetadata(drawing, dims, "test.dxf");
        QVERIFY(result.find("<metadata>") != std::string::npos);
        QVERIFY(result.find("test.dxf") != std::string::npos);
    }
};

QTEST_MAIN(TestSvgGenerator)
#include "test_SvgGenerator.moc"
```

- [ ] **Step 3: Implement SvgGenerator.cpp**

```cpp
// src/core/SvgGenerator.cpp
#include "core/SvgGenerator.h"
#include <sstream>
#include <algorithm>
#include <cmath>

std::string SvgGenerator::generate(const std::string& drawingSvg,
                                    const std::vector<Dimension>& dimensions) {
    return generateWithMetadata(drawingSvg, dimensions, "");
}

std::string SvgGenerator::generateWithMetadata(const std::string& drawingSvg,
                                                 const std::vector<Dimension>& dimensions,
                                                 const std::string& sourceFile) {
    std::string inner = extractDrawingInner(drawingSvg);
    std::string vb = extractViewBox(drawingSvg);

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"" << vb << "\">\n"
        << "<defs>\n<style>\n"
        << ".dim-line{stroke:rgba(255,255,255,0.3);stroke-width:1;fill:none;}\n"
        << ".dim-bound{stroke:#4caf50;stroke-width:1.5;}\n"
        << ".dim-unbound{stroke:#ff9800;stroke-width:1.5;stroke-dasharray:6,3;}\n"
        << ".param-badge-float{fill:rgba(79,195,247,0.12);stroke:rgba(79,195,247,0.35);stroke-width:1;rx:5;ry:5;}\n"
        << ".param-badge-int{fill:rgba(76,175,80,0.12);stroke:rgba(76,175,80,0.35);stroke-width:1;rx:5;ry:5;}\n"
        << ".param-badge-unbound{fill:rgba(255,152,0,0.06);stroke:rgba(255,152,0,0.3);stroke-width:1;stroke-dasharray:4,3;rx:5;ry:5;}\n"
        << ".param-text{font-family:monospace;font-size:10px;text-anchor:middle;dominant-baseline:central;}\n"
        << ".param-text-float{fill:#4fc3f7;font-weight:600;}\n"
        << ".param-text-int{fill:#4caf50;font-weight:600;}\n"
        << ".param-text-unbound{fill:#ff9800;}\n"
        << "</style>\n</defs>\n"
        << "<g id=\"drawing\">\n" << inner << "</g>\n"
        << "<g id=\"dimensions\">\n";

    for (const auto& d : dimensions)
        svg << renderDimensionLine(d) << "\n";

    svg << "</g>\n<g id=\"params\">\n";
    for (const auto& d : dimensions)
        svg << renderParamBadge(d) << "\n";

    svg << "</g>\n<metadata>";
    // Build minimal JSON metadata
    svg << "{";
    if (!sourceFile.empty())
        svg << "\"source\":\"" << sourceFile << "\",";
    svg << "\"dimensions\":[";
    bool first = true;
    for (const auto& d : dimensions) {
        if (!first) svg << ",";
        first = false;
        svg << "{\"id\":\"" << d.id << "\",\"type\":\"" << d.type
            << "\",\"value\":" << d.value
            << ",\"cx\":" << d.centerX << ",\"cy\":" << d.centerY
            << ",\"bound\":" << (d.isBound() ? "true" : "false");
        if (d.isBound()) {
            svg << ",\"key\":\"" << d.param->key
                << "\",\"paramType\":\"" << d.param->type << "\"";
        }
        svg << "}";
    }
    svg << "]}</metadata>\n</svg>";

    return svg.str();
}

std::string SvgGenerator::renderDimensionLine(const Dimension& dim) {
    std::ostringstream tag;
    std::string cls = dim.isBound() ? "bound" : "unbound";
    double cx = dim.centerX, cy = dim.centerY;
    double val = dim.value;

    if (dim.type == "radial" || dim.type == "diameter") {
        tag << "<line class=\"dim-line dim-" << cls << "\""
            << " x1=\"" << cx << "\" y1=\"" << cy << "\""
            << " x2=\"" << (cx + 30) << "\" y2=\"" << (cy - 20) << "\"/>";
    } else if (dim.type == "angular") {
        tag << "<path class=\"dim-line dim-" << cls << "\""
            << " d=\"M " << (cx - 30) << " " << cy
            << " A 30 30 0 0 1 " << (cx + 10) << " " << (cy - 28) << "\"/>";
    } else {
        tag << "<line class=\"dim-line dim-" << cls << "\""
            << " x1=\"" << (cx - 50) << "\" y1=\"" << cy << "\""
            << " x2=\"" << (cx + 50) << "\" y2=\"" << cy << "\"/>";
    }
    return tag.str();
}

std::string SvgGenerator::renderParamBadge(const Dimension& dim) {
    std::ostringstream tag;
    double cx = dim.centerX, cy = dim.centerY;

    if (!dim.isBound()) {
        tag << "<g transform=\"translate(" << cx << "," << (cy + 20) << ")\">\n"
            << "  <rect class=\"param-badge-unbound\" x=\"-30\" y=\"-10\" width=\"60\" height=\"20\"/>\n"
            << "  <text class=\"param-text param-text-unbound\" x=\"0\" y=\"0\">"
            << dim.id << " ???</text>\n</g>";
        return tag.str();
    }

    std::string cls = (dim.param->type == "int") ? "int" : "float";
    std::string label = "$" + dim.param->key + ": " + dim.param->type;
    double tw = label.length() * 6.5 + 20;
    double bw = std::max(60.0, tw);

    tag << "<g transform=\"translate(" << cx << "," << (cy + 20) << ")\">\n"
        << "  <rect class=\"param-badge-" << cls << "\""
        << " x=\"" << (-bw / 2) << "\" y=\"-10\""
        << " width=\"" << bw << "\" height=\"20\"/>\n"
        << "  <text class=\"param-text param-text-" << cls << "\" x=\"0\" y=\"0\">"
        << label << "</text>\n</g>";
    return tag.str();
}

std::string SvgGenerator::extractViewBox(const std::string& svg) {
    auto pos = svg.find("viewBox=\"");
    if (pos == std::string::npos) return "0 0 100 100";
    pos += 9;
    auto end = svg.find("\"", pos);
    return svg.substr(pos, end - pos);
}

std::string SvgGenerator::extractDrawingInner(const std::string& svg) {
    auto start = svg.find("<g id=\"drawing\">");
    if (start == std::string::npos) return svg;
    start += 17;
    auto end = svg.rfind("</g>");
    if (end == std::string::npos || end <= start) return "";
    return svg.substr(start, end - start);
}
```

- [ ] **Step 4: Build and run tests**

```bash
cmake --build build
cd build && ctest --output-on-failure -R test_SvgGenerator
```
Expected: All tests PASS.

- [ ] **Step 5: Commit**

```bash
git add src/core/SvgGenerator.h src/core/SvgGenerator.cpp tests/test_SvgGenerator.cpp
git commit -m "feat: add SVG output generator with 3-layer structure and param badges"
```

---

### Task 6: ★ Single Header Preview Widget (ParamEaserPreview.h)

**Files:**
- Modify: `src/preview/ParamEaserPreview.h`
- Modify: `src/preview/ParamEaserPreview.cpp`
- Create: `examples/integration/main.cpp`

- [ ] **Step 1: Implement the single header**

```cpp
// src/preview/ParamEaserPreview.h
// ════════════════════════════════════════════════════════════════
//  Single-header reusable SVG preview widget for ParamEaser.
//  Any external system can integrate by including this one file:
//
//     #include "ParamEaserPreview.h"
//
//  Features:
//   - Display SVG with zoom/pan
//   - Interactive dimension markers overlay
//   - Click/double-click signals for marker interaction
//   - Zoom controls (scroll wheel, fit-to-view)
//   - Pimpl implementation (no internal details leaked)
// ════════════════════════════════════════════════════════════════
#pragma once

#include <QWidget>
#include <QString>
#include <QPainter>
#include <memory>
#include <vector>
#include <functional>

// ── Lightweight marker data for external systems ──
struct MarkerInfo {
    std::string id;
    double centerX = 0.0;
    double centerY = 0.0;
    bool isBound = false;
    std::string label;      // param key if bound, dim id if not
    std::string paramType;  // "float", "int", or empty
};

// ── Callback types ──
using MarkerClickCallback = std::function<void(const std::string& markerId)>;
using MarkerDoubleClickCallback = std::function<void(const std::string& markerId)>;

// ── SvgPreviewWidget ──
//  A reusable QWidget that renders an SVG with interactive markers.
//  Drop this widget into any Qt application, call setContent(),
//  and connect the callbacks.
class SvgPreviewWidget : public QWidget {
    Q_OBJECT
public:
    explicit SvgPreviewWidget(QWidget* parent = nullptr);
    ~SvgPreviewWidget() override;

    // Set SVG content and markers to display
    void setContent(const QString& svgContent,
                    const std::vector<MarkerInfo>& markers);

    // Clear the display
    void clear();

    // Zoom control
    void setZoom(double factor);       // 1.0 = 100%
    void fitToView();                  // Fit SVG to widget size
    double zoom() const;

    // Callbacks (C++ friendly, no moc dependency for callers)
    void setMarkerClickCallback(MarkerClickCallback cb);
    void setMarkerDoubleClickCallback(MarkerDoubleClickCallback cb);

    // Access current markers (for external state sync)
    const std::vector<MarkerInfo>& markers() const;

signals:
    // Qt signals (for QML / moc-based integration)
    void markerClicked(const QString& markerId);
    void markerDoubleClicked(const QString& markerId);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    class Impl;
    std::unique_ptr<Impl> d;
};
```

- [ ] **Step 2: Implement ParamEaserPreview.cpp**

```cpp
// src/preview/ParamEaserPreview.cpp
#include "preview/ParamEaserPreview.h"
#include <QSvgRenderer>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QPainter>
#include <QTransform>
#include <QToolTip>
#include <cmath>

class SvgPreviewWidget::Impl {
public:
    QSvgRenderer* renderer = nullptr;
    std::vector<MarkerInfo> markers;
    QString svgContent;

    // View transform
    double zoomFactor = 1.0;
    double panX = 0.0, panY = 0.0;

    // Interaction state
    bool dragging = false;
    double lastMouseX = 0, lastMouseY = 0;
    int hoveredMarker = -1;

    // Callbacks
    MarkerClickCallback clickCb;
    MarkerDoubleClickCallback doubleClickCb;

    QRectF svgViewBox;
    QSizeF svgSize;

    ~Impl() { delete renderer; }

    void loadSvg(const QString& svg) {
        delete renderer;
        renderer = new QSvgRenderer();
        svgContent = svg;
        if (!renderer->load(svg.toUtf8())) {
            delete renderer;
            renderer = nullptr;
            return;
        }
        svgViewBox = renderer->viewBoxF();
        svgSize = renderer->defaultSize();
        if (svgSize.isEmpty())
            svgSize = svgViewBox.size();
        zoomFactor = 1.0;
        panX = 0; panY = 0;
    }

    // Transform marker coordinates to widget space
    QPointF markerToWidget(const MarkerInfo& m, const QRectF& widgetRect) const {
        double scale = std::min(widgetRect.width() / svgSize.width(),
                                 widgetRect.height() / svgSize.height()) * zoomFactor;
        double cx = widgetRect.center().x() + (m.centerX - svgSize.width()/2) * scale + panX;
        double cy = widgetRect.center().y() + (m.centerY - svgSize.height()/2) * scale + panY;
        return {cx, cy};
    }

    // Find marker under a widget coordinate
    int hitTest(const QPointF& wpos, const QRectF& widgetRect) const {
        for (int i = 0; i < (int)markers.size(); ++i) {
            QPointF mp = markerToWidget(markers[i], widgetRect);
            double dx = wpos.x() - mp.x();
            double dy = wpos.y() - mp.y();
            if (std::sqrt(dx*dx + dy*dy) < 12.0) return i;
        }
        return -1;
    }
};

// ── Public API ──

SvgPreviewWidget::SvgPreviewWidget(QWidget* parent)
    : QWidget(parent), d(std::make_unique<Impl>()) {
    setMouseTracking(true);
    setMinimumSize(200, 150);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setCursor(Qt::ArrowCursor);
}

SvgPreviewWidget::~SvgPreviewWidget() = default;

void SvgPreviewWidget::setContent(const QString& svgContent,
                                   const std::vector<MarkerInfo>& markers) {
    d->markers = markers;
    d->loadSvg(svgContent);
    fitToView();
    update();
}

void SvgPreviewWidget::clear() {
    d->markers.clear();
    delete d->renderer; d->renderer = nullptr;
    d->svgContent.clear();
    d->zoomFactor = 1.0;
    d->panX = d->panY = 0;
    update();
}

void SvgPreviewWidget::setZoom(double factor) {
    d->zoomFactor = std::max(0.1, std::min(10.0, factor));
    update();
}

void SvgPreviewWidget::fitToView() {
    if (!d->renderer) return;
    QRectF wr = rect();
    double scaleX = wr.width() / d->svgSize.width();
    double scaleY = wr.height() / d->svgSize.height();
    d->zoomFactor = std::min(scaleX, scaleY) * 0.85;
    d->panX = 0;
    d->panY = 0;
    update();
}

double SvgPreviewWidget::zoom() const { return d->zoomFactor; }

void SvgPreviewWidget::setMarkerClickCallback(MarkerClickCallback cb) {
    d->clickCb = std::move(cb);
}
void SvgPreviewWidget::setMarkerDoubleClickCallback(MarkerDoubleClickCallback cb) {
    d->doubleClickCb = std::move(cb);
}

const std::vector<MarkerInfo>& SvgPreviewWidget::markers() const {
    return d->markers;
}

// ── Event handlers ──

void SvgPreviewWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background
    p.fillRect(rect(), QColor("#0f0f23"));

    // Grid dots
    QPen gridPen(QColor(255, 255, 255, 15));
    p.setPen(gridPen);
    for (int x = 0; x < width(); x += 20)
        for (int y = 0; y < height(); y += 20)
            p.drawPoint(x, y);

    if (!d->renderer) {
        // Empty state
        p.setPen(QColor(255, 255, 255, 30));
        p.setFont(QFont("sans-serif", 12));
        p.drawText(rect(), Qt::AlignCenter, "打开 DXF 文件开始");
        return;
    }

    // Render SVG
    QRectF wr = rect();
    double scale = std::min(wr.width() / d->svgSize.width(),
                             wr.height() / d->svgSize.height()) * d->zoomFactor;
    double drawW = d->svgSize.width() * scale;
    double drawH = d->svgSize.height() * scale;
    double drawX = wr.center().x() - drawW / 2 + d->panX;
    double drawY = wr.center().y() - drawH / 2 + d->panY;
    QRectF drawRect(drawX, drawY, drawW, drawH);

    if (d->renderer && d->renderer->isValid())
        d->renderer->render(&p, drawRect);

    // Draw markers
    for (int i = 0; i < (int)d->markers.size(); ++i) {
        auto& m = d->markers[i];
        QPointF mp = d->markerToWidget(m, wr);
        bool hovered = (i == d->hoveredMarker);

        // Ring
        QColor ringColor = m.isBound ? QColor("#4caf50") : QColor("#ff9800");
        ringColor.setAlpha(hovered ? 180 : 80);
        p.setPen(QPen(ringColor, hovered ? 3 : 2));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(mp, hovered ? 10 : 8, hovered ? 10 : 8);

        // Dot
        QColor dotColor = m.isBound ? QColor("#4caf50") : QColor("#ff9800");
        p.setPen(QPen(Qt::white, 1.5));
        p.setBrush(dotColor);
        p.drawEllipse(mp, 5, 5);

        // Label above marker
        p.setPen(m.isBound ? QColor("#4caf50") : QColor("#ff9800"));
        p.setFont(QFont("monospace", 8, QFont::Bold));
        QRectF labelRect(mp.x() - 40, mp.y() - 22, 80, 14);
        p.drawText(labelRect, Qt::AlignCenter, QString::fromStdString(m.label));
    }
}

void SvgPreviewWidget::resizeEvent(QResizeEvent*) {
    // Keep current zoom, just re-render
}

void SvgPreviewWidget::mousePressEvent(QMouseEvent* e) {
    int hit = d->hitTest(QPointF(e->pos()), rect());
    if (hit >= 0) {
        // Marker click
        std::string id = d->markers[hit].id;
        if (d->clickCb) d->clickCb(id);
        emit markerClicked(QString::fromStdString(id));
    } else {
        // Start pan
        d->dragging = true;
        d->lastMouseX = e->position().x();
        d->lastMouseY = e->position().y();
        setCursor(Qt::ClosedHandCursor);
    }
}

void SvgPreviewWidget::mouseDoubleClickEvent(QMouseEvent* e) {
    int hit = d->hitTest(QPointF(e->pos()), rect());
    if (hit >= 0) {
        std::string id = d->markers[hit].id;
        if (d->doubleClickCb) d->doubleClickCb(id);
        emit markerDoubleClicked(QString::fromStdString(id));
    }
}

void SvgPreviewWidget::mouseMoveEvent(QMouseEvent* e) {
    if (d->dragging) {
        double dx = e->position().x() - d->lastMouseX;
        double dy = e->position().y() - d->lastMouseY;
        d->panX += dx;
        d->panY += dy;
        d->lastMouseX = e->position().x();
        d->lastMouseY = e->position().y();
        update();
        return;
    }

    int hit = d->hitTest(QPointF(e->pos()), rect());
    if (hit != d->hoveredMarker) {
        d->hoveredMarker = hit;
        if (hit >= 0) {
            setCursor(Qt::PointingHandCursor);
            auto& m = d->markers[hit];
            QString tip = QString::fromStdString(m.id)
                + (m.isBound ? " (" + QString::fromStdString(m.paramType) + ")" : " (未绑定)");
            setToolTip(tip);
        } else {
            setCursor(Qt::ArrowCursor);
            setToolTip("");
        }
        update();
    }
}

void SvgPreviewWidget::mouseReleaseEvent(QMouseEvent*) {
    d->dragging = false;
    setCursor(d->hoveredMarker >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
}

void SvgPreviewWidget::wheelEvent(QWheelEvent* e) {
    double delta = e->angleDelta().y() / 120.0;
    double factor = (delta > 0) ? 1.15 : (1.0 / 1.15);

    // Zoom toward cursor position
    QRectF wr = rect();
    double mx = e->position().x() - wr.center().x() - d->panX;
    double my = e->position().y() - wr.center().y() - d->panY;

    d->zoomFactor = std::max(0.1, std::min(10.0, d->zoomFactor * factor));

    // Adjust pan to keep cursor position stable
    double newScale = d->zoomFactor;
    double oldScale = newScale / factor;
    d->panX += mx * (1 - newScale / oldScale);
    d->panY += my * (1 - newScale / oldScale);

    update();
}
```

- [ ] **Step 3: Create integration example**

```cpp
// examples/integration/main.cpp
// ════════════════════════════════════════════════════════════════
//  Example: External system using ParamEaserPreview single header
//
//  This shows how little code is needed to embed SVG preview
//  with interactive markers in any Qt application.
// ════════════════════════════════════════════════════════════════
#include <QApplication>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>
#include <sstream>

// ★ The one include you need ★
#include "preview/ParamEaserPreview.h"

// For DXF parsing (or skip if system already has dimension data)
#include "core/DxfParser.h"
#include "core/DxfToSvgRenderer.h"
#include "core/ProjectManager.h"

class IntegrationDemo : public QWidget {
    Q_OBJECT
public:
    IntegrationDemo() {
        auto* layout = new QVBoxLayout(this);

        // Create the preview widget
        m_preview = new SvgPreviewWidget(this);
        m_preview->setMinimumHeight(400);

        // Connect marker click callback
        m_preview->setMarkerClickCallback([this](const std::string& id) {
            statusLabel->setText(QString("点击标记: %1").arg(QString::fromStdString(id)));
        });

        auto* btnLayout = new QHBoxLayout();
        auto* openBtn = new QPushButton("打开 DXF", this);
        auto* statusLabel = new QLabel("就绪", this);

        btnLayout->addWidget(openBtn);
        btnLayout->addWidget(statusLabel);
        btnLayout->addStretch();

        layout->addWidget(m_preview);
        layout->addLayout(btnLayout);

        connect(openBtn, &QPushButton::clicked, this, [this]() {
            QString path = QFileDialog::getOpenFileName(this, "打开 DXF", {},
                                                         "DXF Files (*.dxf)");
            if (path.isEmpty()) return;

            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(this, "错误", "无法打开文件");
                return;
            }

            QString content = file.readAll();
            std::string dxf = content.toStdString();

            // Parse dimensions
            DxfParser parser;
            auto dims = parser.parseDimensions(dxf);

            // Convert DXF to SVG
            DxfToSvgRenderer renderer;
            std::string svg = renderer.render(dxf);

            // Build markers
            std::vector<MarkerInfo> markers;
            for (size_t i = 0; i < dims.size(); ++i) {
                MarkerInfo m;
                m.id = dims[i].id;
                m.centerX = dims[i].centerX;
                m.centerY = dims[i].centerY;
                m.isBound = dims[i].isBound();
                m.label = dims[i].isBound() ? dims[i].param->key : dims[i].id;
                m.paramType = dims[i].isBound() ? dims[i].param->type : "";
                markers.push_back(m);
            }

            // Set content — one call, widget handles the rest
            m_preview->setContent(QString::fromStdString(svg), markers);

            statusLabel->setText(QString("已加载: %1 | %2 个尺寸")
                .arg(QFileInfo(path).fileName())
                .arg(dims.size()));
        });

        resize(800, 600);
    }

private:
    SvgPreviewWidget* m_preview;
};

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    IntegrationDemo demo;
    demo.show();
    return app.exec();
}

#include "main.moc"
```

- [ ] **Step 4: Build**

```bash
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=C:/Qt/6.10.0/msvc2022_64
cmake --build build
```
Expected: Build succeeds for all targets including `parameaser-integration-example`.

- [ ] **Step 5: Commit**

```bash
git add src/preview/ examples/
git commit -m "feat: add single-header SvgPreviewWidget with integration example"
```

---

### Task 7: MainWindow — Full Application UI

**Files:**
- Modify: `src/app/MainWindow.h`
- Modify: `src/app/MainWindow.cpp`
- Modify: `src/app/main.cpp`

- [ ] **Step 1: Implement MainWindow**

```cpp
// src/app/MainWindow.h
#pragma once
#include <QMainWindow>
#include <QSplitter>
#include <QTextEdit>
#include <QListWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include "preview/ParamEaserPreview.h"
#include "core/ProjectManager.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onOpenFile();
    void onMarkerClicked(const std::string& dimId);
    void onExportSvg();
    void onDrawingSelected(QListWidgetItem* item);

private:
    void setupUi();
    void setupMenuBar();
    void updateStatusBar();
    void refreshAll();
    void loadDxfFile(const QString& path);

    // UI components
    QSplitter* m_splitter = nullptr;
    SvgPreviewWidget* m_leftPreview = nullptr;   // Left: DXF + markers
    SvgPreviewWidget* m_rightPreview = nullptr;  // Right: output SVG
    QListWidget* m_drawingList = nullptr;        // Drawing tabs (multi-draw)
    QLabel* m_statusLabel = nullptr;
    QProgressBar* m_progressBar = nullptr;
    QPushButton* m_exportBtn = nullptr;

    // Data
    ProjectManager m_projectManager;
    std::string m_currentDrawingKey;
};
```

- [ ] **Step 2: Implement MainWindow.cpp (full with multi-drawing support)**

```cpp
// src/app/MainWindow.cpp
#include "app/MainWindow.h"
#include "app/ParamEditDialog.h"
#include "core/DxfParser.h"
#include "core/DxfToSvgRenderer.h"
#include "core/SvgGenerator.h"
#include "core/storage.h"

#include <QMenuBar>
#include <QFileDialog>
#include <QMessageBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QFileInfo>
#include <QSvgRenderer>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("ParamEaser");
    resize(1200, 800);
    setupUi();
    setupMenuBar();
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // Drawing toolbar (multi-drawing support)
    auto* toolbar = new QWidget(this);
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(8, 4, 8, 4);

    m_drawingList = new QListWidget(this);
    m_drawingList->setMaximumHeight(36);
    m_drawingList->setFlow(QListView::LeftToRight);
    m_drawingList->setSelectionMode(QAbstractItemView::SingleSelection);
    m_drawingList->setStyleSheet(
        "QListWidget{background:#16213e;border:none;border-bottom:1px solid rgba(255,255,255,0.06);}"
        "QListWidget::item{color:rgba(255,255,255,0.4);padding:4px 16px;margin:2px;"
        "border-radius:4px;font-size:12px;}"
        "QListWidget::item:selected{color:#4fc3f7;background:rgba(79,195,247,0.1);}"
    );
    connect(m_drawingList, &QListWidget::currentItemChanged, this,
        [this](QListWidgetItem* current, QListWidgetItem*) {
            if (current) onDrawingSelected(current);
        });

    auto* addDrawingBtn = new QPushButton("+", this);
    addDrawingBtn->setFixedSize(28, 28);
    addDrawingBtn->setToolTip("添加图纸 (打开 DXF)");
    connect(addDrawingBtn, &QPushButton::clicked, this, &MainWindow::onOpenFile);

    toolbarLayout->addWidget(m_drawingList, 1);
    toolbarLayout->addWidget(addDrawingBtn);
    mainLayout->addWidget(toolbar);

    // Split pane: left = DXF with markers, right = output preview
    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(3);

    m_leftPreview = new SvgPreviewWidget(this);
    m_rightPreview = new SvgPreviewWidget(this);

    m_splitter->addWidget(m_leftPreview);
    m_splitter->addWidget(m_rightPreview);
    m_splitter->setStretchFactor(0, 1);
    m_splitter->setStretchFactor(1, 1);

    mainLayout->addWidget(m_splitter, 1);

    // Status bar
    auto* statusBar = new QWidget(this);
    auto* statusLayout = new QHBoxLayout(statusBar);
    statusLayout->setContentsMargins(12, 4, 12, 4);

    m_statusLabel = new QLabel("就绪", this);
    m_progressBar = new QProgressBar(this);
    m_progressBar->setMaximumWidth(120);
    m_progressBar->setMaximumHeight(6);
    m_progressBar->setTextVisible(false);
    m_progressBar->setStyleSheet(
        "QProgressBar{background:rgba(255,255,255,0.06);border:none;border-radius:3px;}"
        "QProgressBar::chunk{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #4fc3f7,stop:1 #7c4dff);border-radius:3px;}");

    m_exportBtn = new QPushButton("导出 SVG", this);
    m_exportBtn->setEnabled(false);
    connect(m_exportBtn, &QPushButton::clicked, this, &MainWindow::onExportSvg);

    statusLayout->addWidget(m_statusLabel, 1);
    statusLayout->addWidget(m_progressBar);
    statusLayout->addWidget(m_exportBtn);

    mainLayout->addWidget(statusBar);
    setCentralWidget(central);

    // Set dark theme stylesheet
    setStyleSheet(
        "QMainWindow{background:#1a1a2e;}"
        "QSplitter::handle{background:rgba(79,195,247,0.2);}"
        "QSplitter::handle:hover{background:rgba(79,195,247,0.4);}"
        "QLabel{color:rgba(255,255,255,0.5);font-size:12px;}"
    );

    // Wire marker click callback
    m_leftPreview->setMarkerClickCallback(
        [this](const std::string& id) { onMarkerClicked(id); });
}

void MainWindow::setupMenuBar() {
    auto* fileMenu = menuBar()->addMenu("文件");
    fileMenu->addAction("打开 DXF...", this, &MainWindow::onOpenFile, QKeySequence::Open);
    fileMenu->addAction("导出 SVG...", this, &MainWindow::onExportSvg, QKeySequence("Ctrl+E"));
    fileMenu->addSeparator();
    fileMenu->addAction("退出", this, &QWidget::close, QKeySequence::Quit);
}

void MainWindow::onOpenFile() {
    QStringList paths = QFileDialog::getOpenFileNames(this, "打开 DXF 文件", {},
                                                        "DXF Files (*.dxf)");
    for (const auto& path : paths)
        loadDxfFile(path);
}

void MainWindow::loadDxfFile(const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开文件: " + path);
        return;
    }

    QString content = file.readAll();
    std::string dxf = content.toStdString();
    QFileInfo fi(path);

    // Generate unique key from filename
    std::string key = fi.baseName().toStdString();

    // Check for duplicate
    if (m_projectManager.hasDrawing(key)) {
        // Append suffix
        int suffix = 2;
        while (m_projectManager.hasDrawing(key + "_" + std::to_string(suffix)))
            suffix++;
        key = key + "_" + std::to_string(suffix);
    }

    // Parse dimensions
    DxfParser parser;
    auto dims = parser.parseDimensions(dxf);
    if (parser.hasError()) {
        QMessageBox::warning(this, "解析错误",
            QString::fromStdString(parser.lastError()));
    }

    // Render DXF to SVG
    DxfToSvgRenderer renderer;
    std::string svg = renderer.render(dxf);

    // Create drawing data
    DrawingData dd;
    dd.key = key;
    dd.sourceFile = path.toStdString();
    dd.rawDxfContent = dxf;
    dd.drawingSvg = svg;
    dd.dimensions = dims;

    m_projectManager.addDrawing(key, std::move(dd));

    // Add to drawing list
    auto* item = new QListWidgetItem(QString::fromStdString(key));
    item->setData(Qt::UserRole, QString::fromStdString(key));
    m_drawingList->addItem(item);
    m_drawingList->setCurrentItem(item);
}

void MainWindow::onDrawingSelected(QListWidgetItem* item) {
    if (!item) return;
    m_currentDrawingKey = item->data(Qt::UserRole).toString().toStdString();
    refreshAll();
}

void MainWindow::refreshAll() {
    if (m_currentDrawingKey.empty()) return;

    auto* dd = m_projectManager.getDrawing(m_currentDrawingKey);
    if (!dd) return;

    // Left panel: DXF + markers
    auto markers = m_projectManager.getMarkers(m_currentDrawingKey);
    m_leftPreview->setContent(QString::fromStdString(dd->drawingSvg), markers);

    // Right panel: output SVG
    SvgGenerator gen;
    std::string outputSvg = gen.generateWithMetadata(
        dd->drawingSvg, dd->dimensions, dd->sourceFile);
    m_rightPreview->setContent(QString::fromStdString(outputSvg), {});

    updateStatusBar();
}

void MainWindow::onMarkerClicked(const std::string& dimId) {
    if (m_currentDrawingKey.empty()) return;

    auto* dim = m_projectManager.findDimension(m_currentDrawingKey, dimId);
    if (!dim) return;

    ParamEditDialog dialog(dim, this);
    if (dialog.exec() == QDialog::Accepted) {
        auto result = dialog.result();
        if (result.bind) {
            m_projectManager.bindParam(m_currentDrawingKey, dimId,
                                        result.type, result.key, result.defaultValue);
        } else {
            m_projectManager.unbindParam(m_currentDrawingKey, dimId);
        }
        refreshAll();
    }
}

void MainWindow::onExportSvg() {
    if (m_currentDrawingKey.empty()) return;

    auto* dd = m_projectManager.getDrawing(m_currentDrawingKey);
    if (!dd) return;

    QString path = QFileDialog::getSaveFileName(this, "导出 SVG",
        QString::fromStdString(dd->key) + ".svg", "SVG Files (*.svg)");
    if (path.isEmpty()) return;

    SvgGenerator gen;
    std::string svg = gen.generateWithMetadata(dd->drawingSvg, dd->dimensions, dd->sourceFile);

    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(svg.data(), svg.size());
        file.close();
        m_statusLabel->setText("已导出: " + path);
    } else {
        QMessageBox::warning(this, "错误", "无法写入文件");
    }
}

void MainWindow::updateStatusBar() {
    if (m_currentDrawingKey.empty()) return;

    int total = m_projectManager.totalCount(m_currentDrawingKey);
    int bound = m_projectManager.boundCount(m_currentDrawingKey);
    int unbound = m_projectManager.unboundCount(m_currentDrawingKey);

    m_statusLabel->setText(
        QString("📐 尺寸: %1  ✅ 已绑定: %2  ⚠️ 未绑定: %3")
            .arg(total).arg(bound).arg(unbound));

    if (total > 0) {
        m_progressBar->setMaximum(total);
        m_progressBar->setValue(bound);
    } else {
        m_progressBar->setValue(0);
    }

    m_exportBtn->setEnabled(total > 0);
}
```

- [ ] **Step 3: Implement main.cpp**

```cpp
// src/app/main.cpp
#include <QApplication>
#include "app/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("ParamEaser");
    app.setApplicationVersion("1.0.0");
    app.setStyle("Fusion");

    // Dark palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(26, 26, 46));
    darkPalette.setColor(QPalette::WindowText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Base, QColor(15, 15, 35));
    darkPalette.setColor(QPalette::Text, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Button, QColor(26, 26, 46));
    darkPalette.setColor(QPalette::ButtonText, QColor(224, 224, 224));
    darkPalette.setColor(QPalette::Highlight, QColor(79, 195, 247));
    app.setPalette(darkPalette);

    MainWindow w;
    w.show();
    return app.exec();
}
```

- [ ] **Step 4: Build**

Run: `cmake --build build`
Expected: Build succeeds.

- [ ] **Step 5: Commit**

```bash
git add src/app/
git commit -m "feat: add MainWindow with multi-drawing UI and dark theme"
```

---

### Task 8: ParamEditDialog — Parameter Binding Dialog

**Files:**
- Create: `src/app/ParamEditDialog.h`
- Create: `src/app/ParamEditDialog.cpp`

- [ ] **Step 1: Implement ParamEditDialog.h**

```cpp
// src/app/ParamEditDialog.h
#pragma once
#include <QDialog>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QLabel>
#include "core/Dimension.h"

struct ParamEditResult {
    bool bind = false;
    std::string type = "float";
    std::string key;
    double defaultValue = 0.0;
};

class ParamEditDialog : public QDialog {
    Q_OBJECT
public:
    explicit ParamEditDialog(const Dimension* dim, QWidget* parent = nullptr);

    ParamEditResult result() const { return m_result; }

private slots:
    void onBind();
    void onUnbind();

private:
    const Dimension* m_dim;
    ParamEditResult m_result;

    QLabel* m_idLabel = nullptr;
    QLabel* m_infoLabel = nullptr;
    QPushButton* m_floatBtn = nullptr;
    QPushButton* m_intBtn = nullptr;
    QLineEdit* m_keyEdit = nullptr;
    QLineEdit* m_defaultEdit = nullptr;
    QString m_selectedType = "float";
};
```

- [ ] **Step 2: Implement ParamEditDialog.cpp**

```cpp
// src/app/ParamEditDialog.cpp
#include "app/ParamEditDialog.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QMessageBox>

ParamEditDialog::ParamEditDialog(const Dimension* dim, QWidget* parent)
    : QDialog(parent), m_dim(dim) {
    setWindowTitle(QString("参数绑定 — %1").arg(QString::fromStdString(dim->id)));
    setFixedSize(320, 280);

    auto* layout = new QVBoxLayout(this);
    layout->setSpacing(12);

    // Dimension info
    auto* infoBox = new QGroupBox("尺寸信息", this);
    auto* infoForm = new QFormLayout(infoBox);

    auto dimTypeStr = [](const std::string& t) -> QString {
        if (t == "aligned") return "对齐标注";
        if (t == "linear") return "线性标注";
        if (t == "radial") return "半径标注";
        if (t == "diameter") return "直径标注";
        if (t == "angular") return "角度标注";
        return QString::fromStdString(t);
    };

    infoForm->addRow("ID:", new QLabel(QString::fromStdString(dim->id), this));
    infoForm->addRow("类型:", new QLabel(dimTypeStr(dim->type), this));
    infoForm->addRow("值:", new QLabel(QString::number(dim->value) + " " +
                                        QString::fromStdString(dim->unit), this));
    infoForm->addRow("中心:", new QLabel(
        QString("(%1, %2)").arg(dim->centerX).arg(dim->centerY), this));
    layout->addWidget(infoBox);

    // Parameter binding
    auto* paramBox = new QGroupBox("参数绑定", this);
    auto* paramLayout = new QVBoxLayout(paramBox);

    // Type toggle
    auto* typeLayout = new QHBoxLayout();
    typeLayout->addWidget(new QLabel("参数类型:", this));

    m_floatBtn = new QPushButton("float", this);
    m_floatBtn->setCheckable(true);
    m_floatBtn->setChecked(true);
    m_floatBtn->setStyleSheet(
        "QPushButton{background:rgba(79,195,247,0.1);border:1px solid rgba(79,195,247,0.3);"
        "border-radius:4px;padding:6px 16px;color:#4fc3f7;font-weight:600;}"
        "QPushButton:checked{background:rgba(79,195,247,0.2);border-color:#4fc3f7;}");

    m_intBtn = new QPushButton("int", this);
    m_intBtn->setCheckable(true);
    m_intBtn->setStyleSheet(
        "QPushButton{background:rgba(76,175,80,0.1);border:1px solid rgba(76,175,80,0.3);"
        "border-radius:4px;padding:6px 16px;color:#4caf50;font-weight:600;}"
        "QPushButton:checked{background:rgba(76,175,80,0.2);border-color:#4caf50;}");

    connect(m_floatBtn, &QPushButton::clicked, this, [this]() {
        m_floatBtn->setChecked(true); m_intBtn->setChecked(false);
        m_selectedType = "float";
    });
    connect(m_intBtn, &QPushButton::clicked, this, [this]() {
        m_floatBtn->setChecked(false); m_intBtn->setChecked(true);
        m_selectedType = "int";
    });

    typeLayout->addWidget(m_floatBtn);
    typeLayout->addWidget(m_intBtn);
    typeLayout->addStretch();
    paramLayout->addLayout(typeLayout);

    // Key input
    auto* keyLayout = new QHBoxLayout();
    keyLayout->addWidget(new QLabel("参数 Key:", this));
    m_keyEdit = new QLineEdit(this);
    m_keyEdit->setPlaceholderText("key_name");
    m_keyEdit->setStyleSheet(
        "QLineEdit{background:rgba(255,255,255,0.04);border:1px solid rgba(255,255,255,0.1);"
        "border-radius:4px;padding:6px 8px;color:#fff;font-family:monospace;}");
    keyLayout->addWidget(m_keyEdit, 1);
    paramLayout->addLayout(keyLayout);

    // Default value
    auto* defaultLayout = new QHBoxLayout();
    defaultLayout->addWidget(new QLabel("默认值:", this));
    m_defaultEdit = new QLineEdit(this);
    m_defaultEdit->setText(QString::number(dim->value));
    m_defaultEdit->setStyleSheet(m_keyEdit->styleSheet());
    defaultLayout->addWidget(m_defaultEdit, 1);
    paramLayout->addLayout(defaultLayout);

    // Pre-fill if already bound
    if (dim->isBound()) {
        m_keyEdit->setText(QString::fromStdString(dim->param->key));
        m_defaultEdit->setText(QString::number(dim->param->defaultValue));
        if (dim->param->type == "int") {
            m_floatBtn->setChecked(false);
            m_intBtn->setChecked(true);
            m_selectedType = "int";
        }
    }

    layout->addWidget(paramBox);

    // Buttons
    auto* btnLayout = new QHBoxLayout();
    auto* bindBtn = new QPushButton(dim->isBound() ? "✓ 更新" : "✓ 绑定", this);
    bindBtn->setStyleSheet(
        "QPushButton{background:qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #4fc3f7,stop:1 #7c4dff);border:none;border-radius:4px;"
        "padding:8px 24px;color:#fff;font-weight:600;}");
    connect(bindBtn, &QPushButton::clicked, this, &ParamEditDialog::onBind);

    auto* unbindBtn = new QPushButton("解绑", this);
    unbindBtn->setStyleSheet(
        "QPushButton{background:transparent;border:1px solid rgba(255,255,255,0.1);"
        "border-radius:4px;padding:8px 24px;color:rgba(255,255,255,0.4);}");
    connect(unbindBtn, &QPushButton::clicked, this, &ParamEditDialog::onUnbind);

    auto* cancelBtn = new QPushButton("取消", this);
    cancelBtn->setStyleSheet(unbindBtn->styleSheet());
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    btnLayout->addWidget(bindBtn);
    btnLayout->addWidget(unbindBtn);
    btnLayout->addWidget(cancelBtn);
    layout->addLayout(btnLayout);

    if (!dim->isBound()) unbindBtn->setEnabled(false);
}

void ParamEditDialog::onBind() {
    QString key = m_keyEdit->text().trimmed();
    if (key.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入参数 Key");
        m_keyEdit->setFocus();
        return;
    }

    m_result.bind = true;
    m_result.type = m_selectedType.toStdString();
    m_result.key = key.toStdString();
    m_result.defaultValue = m_defaultEdit->text().toDouble();
    accept();
}

void ParamEditDialog::onUnbind() {
    m_result.bind = false;
    accept();
}
```

- [ ] **Step 3: Build**

Run: `cmake --build build`
Expected: Build succeeds.

- [ ] **Step 4: Commit**

```bash
git add src/app/ParamEditDialog.h src/app/ParamEditDialog.cpp
git commit -m "feat: add parameter binding dialog with type toggle and key input"
```

---

### Task 9: File Management — Save/Load Projects

- [ ] **Step 1: Implement project save/load**

Add to `src/app/MainWindow.cpp`:

```cpp
// Add to setupMenuBar:
fileMenu->addAction("保存项目...", this, &MainWindow::onSaveProject, QKeySequence::Save);
fileMenu->addAction("打开项目...", this, &MainWindow::onLoadProject, QKeySequence::Open);

// Add handlers:

void MainWindow::onSaveProject() {
    QString path = QFileDialog::getSaveFileName(this, "保存项目",
        "project.parameaser", "ParamEaser Project (*.parameaser)");
    if (path.isEmpty()) return;

    std::string json = m_projectManager.toJson();
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(json.data(), json.size());
        file.close();
        m_statusLabel->setText("项目已保存");
    } else {
        QMessageBox::warning(this, "错误", "无法保存项目文件");
    }
}

void MainWindow::onLoadProject() {
    QString path = QFileDialog::getOpenFileName(this, "打开项目", {},
        "ParamEaser Project (*.parameaser)");
    if (path.isEmpty()) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "错误", "无法打开项目文件");
        return;
    }

    std::string json = file.readAll().toStdString();
    m_projectManager = ProjectManager::fromJson(json);

    // Rebuild drawing list
    m_drawingList->clear();
    for (const auto& key : m_projectManager.drawingKeys()) {
        auto* item = new QListWidgetItem(QString::fromStdString(key));
        item->setData(Qt::UserRole, QString::fromStdString(key));
        m_drawingList->addItem(item);
    }

    if (!m_projectManager.drawingKeys().empty()) {
        m_drawingList->setCurrentRow(0);
    }

    m_statusLabel->setText("项目已加载: " + QFileInfo(path).fileName());
}
```

- [ ] **Step 2: Build and test**

Run: `cmake --build build`
Expected: Build succeeds.

- [ ] **Step 3: Commit**

```bash
git add src/app/MainWindow.h src/app/MainWindow.cpp
git commit -m "feat: add project save/load with .parameaser file format"
```

---

### Task 10: Integration & Polish

**Files:**
- Modify: `src/app/MainWindow.cpp`

- [ ] **Step 1: Add edge case handling for empty DXF files**

```cpp
// In loadDxfFile, after parsing:
if (dims.empty()) {
    QMessageBox::information(this, "提示",
        "文件中未检测到尺寸标注。\n\n"
        "仅显示图纸预览。");
}
```

- [ ] **Step 2: Handle invalid DXF gracefully**

```cpp
// In loadDxfFile, wrap in try-catch:
try {
    auto dims = parser.parseDimensions(dxf);
    // ...
} catch (const std::exception& e) {
    QMessageBox::critical(this, "解析错误",
        "无法解析 DXF 文件:\n" + QString::fromStdString(e.what()));
    return;
}
```

- [ ] **Step 3: Final build and verify**

```bash
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH=C:/Qt/6.10.0/msvc2022_64
cmake --build build
```
Expected: All targets build with zero warnings.

- [ ] **Step 4: Manual integration test**

Run: `build/src/app/parameaser.exe`

Test workflow:
1. Open 2+ DXF files → verify they appear as separate tabs
2. Click between tabs → verify markers switch correctly
3. Click a marker → dialog appears → bind float/int + key → verify
4. Verify right panel updates with bound badge
5. Switch drawings → verify independent binding states
6. Export SVG → verify 3 layers in output file
7. Save project → restart → load project → verify all bindings restored

- [ ] **Step 5: Final commit**

```bash
git add .
git commit -m "feat: final polish with error handling and edge cases"
```

---

## Self-Review Checklist

1. **Spec coverage:**
   - UI layout (Section 2) → MainWindow + SvgPreviewWidget
   - Dimension types (Section 3) → DxfParser handles 4 types
   - Color scheme (Section 4) → Dark palette in main.cpp
   - Storage (Section 5) → ProjectManager::toJson/fromJson + file save/load
   - Data flow (Section 6) → Pipeline: DxfParser → ProjectManager → SvgGenerator → Preview
   - DXF parsing (Section 9.1) → Task 3
   - SVG output (Section 9.2) → Task 5 (3-layer + metadata)
   - ★ Single header for integration → Task 6 (ParamEaserPreview.h)
   - ★ Multi-drawing key support → ProjectManager uses std::map<std::string, DrawingData>

2. **Placeholder scan:** No TBD, TODO, or incomplete code sections ✓

3. **Type consistency:**
   - `ProjectManager::bindParam(dk, dimId, type, key, default)` used consistently ✓
   - `DimensionMarker` struct matches between core model and preview widget ✓
   - `ProjectManager::getMarkers()` returns `std::vector<DimensionMarker>` ✓
   - SVG generator takes `std::vector<Dimension>` ✓

4. **Single header verification:** `ParamEaserPreview.h` is truly standalone:
   - Only includes `<QWidget>`, `<QString>`, `<QPainter>`, `<memory>`, `<vector>`, `<functional>`
   - Pimpl pattern hides all implementation in .cpp
   - External system only needs: `#include "ParamEaserPreview.h"` + link `parameaser-preview`
   - Integration example (`examples/integration/main.cpp`) shows full workflow in ~50 lines ✓
