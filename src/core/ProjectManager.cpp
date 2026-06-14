// src/core/ProjectManager.cpp
#include "core/ProjectManager.h"
#include <sstream>
#include <algorithm>
#include <iterator>
#include <cstdlib>

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
    keys.reserve(m_drawings.size());
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
    return static_cast<int>(std::count_if(dd->dimensions.begin(), dd->dimensions.end(),
        [](auto& d) { return d.isBound(); }));
}

int ProjectManager::unboundCount(const std::string& dk) const {
    auto* dd = getDrawing(dk);
    if (!dd) return 0;
    return static_cast<int>(std::count_if(dd->dimensions.begin(), dd->dimensions.end(),
        [](auto& d) { return !d.isBound(); }));
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
    markers.reserve(dd->dimensions.size());
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
    std::ostringstream os;
    os << "{\"version\":\"1.0\",\"drawings\":[";
    bool firstDrawing = true;
    for (const auto& [key, dd] : m_drawings) {
        if (!firstDrawing) os << ",";
        firstDrawing = false;
        os << "{\"key\":\"" << key << "\",\"source\":\"" << dd.sourceFile
           << "\",\"dimensions\":[";
        bool firstDim = true;
        for (const auto& dim : dd.dimensions) {
            if (!firstDim) os << ",";
            firstDim = false;
            os << "{\"id\":\"" << dim.id << "\",\"type\":\"" << dim.type
               << "\",\"orientation\":\"" << dim.orientation
               << "\",\"value\":" << dim.value
               << ",\"unit\":\"" << dim.unit
               << "\",\"centerX\":" << dim.centerX
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
        // Include drawing SVG and raw DXF content for round-trip fidelity
        auto escapeJson = [](const std::string& s) -> std::string {
            std::string out;
            out.reserve(s.size());
            for (char c : s) {
                switch (c) {
                    case '"': out += "\\\""; break;
                    case '\\': out += "\\\\"; break;
                    case '\n': out += "\\n"; break;
                    case '\r': out += "\\r"; break;
                    case '\t': out += "\\t"; break;
                    default: out += c;
                }
            }
            return out;
        };
        os << "],\"drawingSvg\":\"" << escapeJson(dd.drawingSvg) << "\""
           << ",\"rawDxfContent\":\"" << escapeJson(dd.rawDxfContent) << "\"}";
    }
    os << "]}";
    return os.str();
}

ProjectManager ProjectManager::fromJson(const std::string& json) {
    ProjectManager pm;
    // Simple field-by-field extraction without full JSON parser dependency
    // Handles the JSON structure produced by toJson()
    auto extractStr = [](const std::string& s, size_t& pos, const std::string& key) -> std::string {
        auto kpos = s.find("\"" + key + "\":\"", pos);
        if (kpos == std::string::npos) return "";
        kpos += key.size() + 4; // skip "key":"
        std::string val;
        while (kpos < s.size()) {
            if (s[kpos] == '\\' && kpos + 1 < s.size()) {
                if (s[kpos + 1] == '"') val += '"';
                else if (s[kpos + 1] == 'n') val += '\n';
                else if (s[kpos + 1] == 'r') val += '\r';
                else if (s[kpos + 1] == 't') val += '\t';
                else if (s[kpos + 1] == '\\') val += '\\';
                else { val += s[kpos]; val += s[kpos + 1]; }
                kpos += 2;
            } else if (s[kpos] == '"') {
                break;
            } else {
                val += s[kpos];
                kpos++;
            }
        }
        pos = kpos + 1;
        return val;
    };
    auto extractNum = [](const std::string& s, size_t& pos, const std::string& key) -> double {
        auto kpos = s.find("\"" + key + "\":", pos);
        if (kpos == std::string::npos) return 0.0;
        kpos += key.size() + 3; // skip "key":
        char* end = nullptr;
        double val = std::strtod(s.c_str() + kpos, &end);
        pos = end ? (end - s.c_str()) : (kpos + 1);
        return val;
    };
    auto extractBool = [](const std::string& s, size_t& pos, const std::string& key, bool& found) -> bool {
        auto kpos = s.find("\"" + key + "\":", pos);
        if (kpos == std::string::npos) { found = false; return false; }
        kpos += key.size() + 3;
        found = true;
        if (s.compare(kpos, 4, "true") == 0) return true;
        if (s.compare(kpos, 5, "false") == 0) return false;
        found = false;
        return false;
    };

    size_t drawingStart = 0;
    while (true) {
        auto dpos = json.find("\"key\":\"", drawingStart);
        if (dpos == std::string::npos) break;

        DrawingData dd;
        size_t pos = dpos;
        dd.key = extractStr(json, pos, "key");
        dd.sourceFile = extractStr(json, pos, "source");
        dd.drawingSvg = extractStr(json, pos, "drawingSvg");
        dd.rawDxfContent = extractStr(json, pos, "rawDxfContent");

        // Extract dimensions array
        auto dimStart = json.find("\"dimensions\":[", pos);
        if (dimStart != std::string::npos) {
            size_t dimPos = dimStart + 14;
            while (dimPos < json.size() && json[dimPos] != ']') {
                // Skip whitespace
                while (dimPos < json.size() && (json[dimPos] == ' ' || json[dimPos] == '\n')) dimPos++;
                if (dimPos >= json.size() || json[dimPos] != '{') break;

                Dimension dim;
                dim.id = extractStr(json, dimPos, "id");
                dim.type = extractStr(json, dimPos, "type");
                // orientation is optional
                size_t orientPos = dimPos;
                dim.orientation = extractStr(json, orientPos, "orientation");
                if (!dim.orientation.empty()) dimPos = orientPos;
                dim.value = extractNum(json, dimPos, "value");
                dim.centerX = extractNum(json, dimPos, "centerX");
                dim.centerY = extractNum(json, dimPos, "centerY");
                dim.rotation = extractNum(json, dimPos, "rotation");

                // Check for param binding
                auto paramPos = json.find("\"param\":", dimPos);
                if (paramPos != std::string::npos && paramPos < json.find("}", dimPos)) {
                    if (json.find("null", paramPos) == std::string::npos ||
                        json.find("\"param\":null", paramPos) == std::string::npos) {
                        // Has a param binding
                        size_t pp = paramPos + 8;
                        ParamBinding pb;
                        pb.key = extractStr(json, pp, "key");
                        pb.type = extractStr(json, pp, "type");
                        pb.defaultValue = extractNum(json, pp, "default");
                        dim.param = pb;
                    }
                }

                dd.dimensions.push_back(dim);

                // Advance past this dimension object
                auto closeBrace = json.find("}", dimPos);
                dimPos = (closeBrace != std::string::npos) ? closeBrace + 1 : dimPos + 1;
            }
        }

        pm.addDrawing(dd.key, std::move(dd));

        // Advance past this drawing object
        auto closeBrace = json.find("}", drawingStart + 1);
        if (closeBrace == std::string::npos) break;
        // Find closing brace that matches the drawing object
        int braceCount = 0;
        for (size_t i = drawingStart; i < json.size(); i++) {
            if (json[i] == '{') braceCount++;
            else if (json[i] == '}') { braceCount--; if (braceCount == 0) { drawingStart = i + 1; break; } }
        }
        if (braceCount != 0) break;
    }

    return pm;
}
