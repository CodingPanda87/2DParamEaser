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
    std::string drawingSvg;         // DXF to SVG (geometry only)
    std::vector<Dimension> dimensions;
};

// Marker data for the preview widget overlay
struct DimensionMarker {
    std::string id;
    double centerX;
    double centerY;
    double rotation;
    bool isBound;
    std::string label;  // bound to param key, unbound to id
    std::string paramType;
};
