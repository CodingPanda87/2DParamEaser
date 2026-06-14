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
};
