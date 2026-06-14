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
target_link_libraries(parameaser-core PUBLIC)
target_compile_features(parameaser-core PUBLIC cxx_std_20)

# Preview library
add_library(parameaser-preview STATIC
    preview/ParamEaserPreview.cpp
    preview/ParamEaserPreview.h
)
target_include_directories(parameaser-preview PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(parameaser-preview PUBLIC parameaser-core Qt6::Widgets Qt6::Svg)

add_subdirectory(app)
