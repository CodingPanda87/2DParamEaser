// src/core/DxfParser.cpp
#include "core/DxfParser.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <cctype>

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

    auto trim = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \r\n\t"));
        auto end = s.find_last_not_of(" \r\n\t");
        if (end != std::string::npos) s.erase(end + 1);
    };

    while (std::getline(stream, line)) {
        trim(line);
        if (line.empty()) continue;

        int groupCode;
        try {
            groupCode = std::stoi(line);
        } catch (...) {
            continue;
        }

        if (!std::getline(stream, line)) break;
        trim(line);

        if (groupCode == 0) {
            if (line == "SECTION") {
                finalizeEntity();
                if (!std::getline(stream, line)) break;
                trim(line);
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

        // Numeric group codes used for dimensions
        if (groupCode == 70 || groupCode == 40 || groupCode == 50 ||
            groupCode == 10 || groupCode == 20 ||
            groupCode == 11 || groupCode == 21 ||
            groupCode == 13 || groupCode == 23 ||
            groupCode == 14 || groupCode == 24 ||
            groupCode == 41 || groupCode == 42) {
            try {
                current.values[groupCode] = std::stod(line);
            } catch (...) {}
        } else if (groupCode == 1) {
            current.strings[groupCode] = line;
        } else if (groupCode == 8) {
            current.strings[groupCode] = line;
        }
    }
    return entities;
}

Dimension DxfParser::dimensionFromEntity(const ParsedEntity& ent, int index) {
    Dimension dim;
    dim.id = "D" + std::to_string(index + 1);

    auto getVal = [&](int code, double def) -> double {
        auto it = ent.values.find(code);
        return it != ent.values.end() ? it->second : def;
    };

    int typeCode = static_cast<int>(getVal(70, 0));
    switch (typeCode) {
        case 0: dim.type = "aligned"; break;
        case 1: dim.type = "linear"; break;
        case 2: dim.type = "angular"; break;
        case 3: dim.type = "diameter"; break;
        case 4: dim.type = "radial"; break;
        default: dim.type = "aligned"; break;
    }

    dim.centerX = getVal(11, 0.0);
    dim.centerY = getVal(21, 0.0);
    dim.value = getVal(40, 0.0);
    dim.rotation = getVal(50, 0.0);

    // Determine orientation for linear dimensions
    if (dim.type == "linear") {
        double rot = std::fmod(dim.rotation, 180.0);
        if (rot < 0) rot += 180.0;
        dim.orientation = (rot >= 45.0 && rot <= 135.0) ? "vertical" : "horizontal";
    }

    auto it = ent.strings.find(1);
    if (it != ent.strings.end()) {
        dim.unit = it->second; // raw dim text (might be value string)
    }

    dim.param = std::nullopt;
    return dim;
}
