add_executable(parameaser
    main.cpp
    MainWindow.h MainWindow.cpp
    ParamEditDialog.h ParamEditDialog.cpp
)
target_link_libraries(parameaser PRIVATE parameaser-core parameaser-preview)
