function(add_parameaser_test name source)
    add_executable(${name} ${source})
    target_link_libraries(${name} PRIVATE parameaser-core Qt6::Test)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_parameaser_test(test_DxfParser test_DxfParser.cpp)
add_parameaser_test(test_ProjectManager test_ProjectManager.cpp)
add_parameaser_test(test_SvgGenerator test_SvgGenerator.cpp)
