// src/core/DxfToSvgRenderer.cpp
#include "core/DxfToSvgRenderer.h"
#include <sstream>
#include <cmath>
#include <algorithm>
#include <numbers>

std::string DxfToSvgRenderer::render(const std::string& dxfContent) {
    auto entities = extractEntities(dxfContent);

    if (entities.empty()) {
        return "<svg xmlns=\"http://www.w3.org/2000/svg\" viewBox=\"0 0 100 100\">\n"
               "<g id=\"drawing\" fill=\"none\" stroke=\"rgba(255,255,255,0.4)\" stroke-width=\"1.5\">\n"
               "</g>\n</svg>";
    }

    double minX = 1e30, minY = 1e30, maxX = -1e30, maxY = -1e30;
    for (const auto& ent : entities)
        updateBounds(ent, minX, minY, maxX, maxY);

    if (minX > maxX) { minX = 0; minY = 0; maxX = 100; maxY = 100; }

    const double pad = 20.0;
    double w = maxX - minX + pad * 2;
    double h = maxY - minY + pad * 2;
    if (w < 1) w = 100;
    if (h < 1) h = 100;

    std::ostringstream svg;
    svg << "<svg xmlns=\"http://www.w3.org/2000/svg\""
        << " viewBox=\"" << (minX - pad) << " " << (minY - pad)
        << " " << w << " " << h << "\">\n"
        << "<g id=\"drawing\" fill=\"none\" stroke=\"rgba(255,255,255,0.4)\" stroke-width=\"1.5\">\n";

    for (const auto& ent : entities) {
        svg << "  " << entityTag(ent) << "\n";
    }

    svg << "</g>\n</svg>";
    return svg.str();
}

std::vector<DxfToSvgRenderer::ParsedEntity>
DxfToSvgRenderer::extractEntities(const std::string& content) {
    std::vector<ParsedEntity> entities;
    std::istringstream stream(content);
    std::string line;
    bool hasQueuedLine = false;

    auto trimLine = [](std::string& s) {
        s.erase(0, s.find_first_not_of(" \r\n\t"));
        auto end = s.find_last_not_of(" \r\n\t");
        if (end != std::string::npos) s.erase(end + 1);
    };

    while (true) {
        // Read next group code
        if (!hasQueuedLine) {
            if (!std::getline(stream, line)) break;
            trimLine(line);
        }
        hasQueuedLine = false;

        if (line.empty()) continue;

        int code;
        try { code = std::stoi(line); }
        catch (...) { continue; }

        // Only interested in entity type markers (code 0)
        if (code != 0) continue;

        // Read entity type
        if (!std::getline(stream, line)) break;
        trimLine(line);

        // Skip non-geometry entities
        if (line != "LINE" && line != "CIRCLE" && line != "ARC" &&
            line != "LWPOLYLINE" && line != "POLYLINE") {
            continue;
        }

        ParsedEntity ent;
        ent.type = line;

        // Read properties until next code 0
        while (std::getline(stream, line)) {
            trimLine(line);
            if (line.empty()) continue;

            try { code = std::stoi(line); }
            catch (...) { break; }

            if (code == 0) {
                hasQueuedLine = true; // queue this line for outer loop
                break;
            }

            if (!std::getline(stream, line)) break;
            trimLine(line);

            if (code == 10 || code == 20 || code == 11 ||
                code == 21 || code == 40 || code == 50 ||
                code == 51) {
                try { ent.nums[code] = std::stod(line); }
                catch (...) {}
            }
        }
        entities.push_back(ent);
    }
    return entities;
}

std::string DxfToSvgRenderer::entityTag(const ParsedEntity& ent) {
    std::ostringstream tag;
    auto g = [&](int code, double def) {
        auto it = ent.nums.find(code);
        return it != ent.nums.end() ? it->second : def;
    };

    if (ent.type == "LINE") {
        tag << "<line x1=\"" << g(10, 0) << "\" y1=\"" << g(20, 0)
            << "\" x2=\"" << g(11, 0) << "\" y2=\"" << g(21, 0) << "\"/>";
    }
    else if (ent.type == "CIRCLE") {
        tag << "<circle cx=\"" << g(10, 0) << "\" cy=\"" << g(20, 0)
            << "\" r=\"" << g(40, 0) << "\"/>";
    }
    else if (ent.type == "ARC") {
        double cx = g(10, 0), cy = g(20, 0), r = g(40, 0);
        double sa = g(50, 0), ea = g(51, 0);
        auto toRad = [](double deg) { return deg * std::numbers::pi / 180.0; };
        double sx = cx + r * std::cos(toRad(sa));
        double sy = cy + r * std::sin(toRad(sa));
        double ex = cx + r * std::cos(toRad(ea));
        double ey = cy + r * std::sin(toRad(ea));

        // Normalize sweep to handle arcs that cross the 0-degree boundary
        double sweep = std::fmod(ea - sa, 360.0);
        if (sweep < 0) sweep += 360.0;
        int largeArc = (sweep > 180.0) ? 1 : 0;

        tag << "<path d=\"M " << sx << " " << sy
            << " A " << r << " " << r << " 0 " << largeArc << " 0 "
            << ex << " " << ey << "\"/>";
    }

    return tag.str();
}

void DxfToSvgRenderer::updateBounds(const ParsedEntity& ent,
                                     double& minX, double& minY,
                                     double& maxX, double& maxY) {
    auto g = [&](int code, double def) {
        auto it = ent.nums.find(code);
        return it != ent.nums.end() ? it->second : def;
    };
    auto upd = [&](double x, double y) {
        minX = std::min(minX, x); minY = std::min(minY, y);
        maxX = std::max(maxX, x); maxY = std::max(maxY, y);
    };

    if (ent.type == "LINE") {
        upd(g(10, 0), g(20, 0));
        upd(g(11, 0), g(21, 0));
    } else if (ent.type == "CIRCLE" || ent.type == "ARC") {
        double cx = g(10, 0), cy = g(20, 0), r = g(40, 0);
        upd(cx - r, cy - r);
        upd(cx + r, cy + r);
    }
}
