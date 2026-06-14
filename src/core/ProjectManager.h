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
};
