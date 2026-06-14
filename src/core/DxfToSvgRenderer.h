// src/core/DxfToSvgRenderer.h
#pragma once
#include <string>
#include <vector>
#include <unordered_map>

class DxfToSvgRenderer {
public:
    // Convert DXF geometry entities to SVG string with auto-calculated viewBox
    std::string render(const std::string& dxfContent);

private:
    struct ParsedEntity {
        std::string type;
        std::unordered_map<int, double> nums;
    };

    static std::vector<ParsedEntity> extractEntities(const std::string& content);
    static std::string entityTag(const ParsedEntity& ent);
    static void updateBounds(const ParsedEntity& ent,
                             double& minX, double& minY,
                             double& maxX, double& maxY);
};
