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
        << ".param-badge-float{fill:rgba(79,195,247,0.12);stroke:rgba(79,195,247,0.35);stroke-width:1;}\n"
        << ".param-badge-int{fill:rgba(76,175,80,0.12);stroke:rgba(76,175,80,0.35);stroke-width:1;}\n"
        << ".param-badge-unbound{fill:rgba(255,152,0,0.06);stroke:rgba(255,152,0,0.3);stroke-width:1;stroke-dasharray:4,3;}\n"
        << ".param-text{font-family:monospace;font-size:10px;text-anchor:middle;dominant-baseline:central;}\n"
        << ".param-text-float{fill:#4fc3f7;font-weight:600;}\n"
        << ".param-text-int{fill:#4caf50;font-weight:600;}\n"
        << ".param-text-unbound{fill:#ff9800;}\n"
        << "</style>\n</defs>\n"
        << "<g id=\"drawing\">\n" << inner << "</g>\n"
        << "<g id=\"dimensions\">\n";

    for (const auto& d : dimensions)
        svg << "  " << renderDimensionLine(d) << "\n";

    svg << "</g>\n<g id=\"params\">\n";
    for (const auto& d : dimensions)
        svg << "  " << renderParamBadge(d) << "\n";

    // Build metadata JSON
    svg << "</g>\n<metadata>{\"version\":\"1.0\"";
    if (!sourceFile.empty())
        svg << ",\"source\":\"" << sourceFile << "\"";
    svg << ",\"drawing\":\"" << vb << "\"";
    svg << ",\"dimensions\":[";
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
                << "\",\"paramType\":\"" << d.param->type
                << "\",\"default\":" << d.param->defaultValue;
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

    if (dim.type == "radial" || dim.type == "diameter") {
        tag << "<line class=\"dim-line dim-" << cls << "\""
            << " x1=\"" << cx << "\" y1=\"" << cy << "\""
            << " x2=\"" << (cx + 30) << "\" y2=\"" << (cy - 20) << "\"/>";
    } else if (dim.type == "angular") {
        tag << "<path class=\"dim-line dim-" << cls << "\""
            << " d=\"M " << (cx - 30) << " " << cy
            << " A 30 30 0 0 1 " << (cx + 10) << " " << (cy - 28) << "\"/>";
    } else {
        // linear or aligned: horizontal line
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
            << "    <rect class=\"param-badge-unbound\" x=\"-30\" y=\"-10\" width=\"60\" height=\"20\" rx=\"5\" ry=\"5\"/>\n"
            << "    <text class=\"param-text param-text-unbound\" x=\"0\" y=\"0\">"
            << dim.id << " ???</text>\n  </g>";
        return tag.str();
    }

    std::string cls = (dim.param->type == "int") ? "int" : "float";
    std::string label = "$" + dim.param->key + ": " + dim.param->type;
    double tw = label.length() * 6.5 + 24;
    double bw = std::max(60.0, tw);

    tag << "<g transform=\"translate(" << cx << "," << (cy + 20) << ")\">\n"
        << "    <rect class=\"param-badge-" << cls << "\""
        << " x=\"" << (-bw / 2) << "\" y=\"-10\""
        << " width=\"" << bw << "\" height=\"20\" rx=\"5\" ry=\"5\"/>\n"
        << "    <text class=\"param-text param-text-" << cls << "\" x=\"0\" y=\"0\">"
        << label << "</text>\n  </g>";
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
    // Match <g id="drawing" ...> (may have additional attributes)
    auto start = svg.find("<g id=\"drawing\"");
    if (start == std::string::npos) return svg;
    // Advance past the opening >
    auto gtPos = svg.find(">", start);
    if (gtPos == std::string::npos) return svg;
    start = gtPos + 1;
    auto end = svg.rfind("</g>");
    if (end == std::string::npos || end <= start) return "";
    return svg.substr(start, end - start);
}
