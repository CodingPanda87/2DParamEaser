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
